/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "real_kms_output_container.h"
#include "kms-utils/drm_mode_resources.h"
#include "real_kms_output.h"
#include <algorithm>
#include <experimental/optional>

namespace mgm = mir::graphics::mesa;

mgm::RealKMSOutputContainer::RealKMSOutputContainer(
    std::vector<int> const& drm_fds,
    std::function<std::shared_ptr<PageFlipper>(int)> const& construct_page_flipper)
    : drm_fds{drm_fds},
      construct_page_flipper{construct_page_flipper}
{
}

void mgm::RealKMSOutputContainer::for_each_output(std::function<void(std::shared_ptr<KMSOutput> const&)> functor) const
{
    for(auto& output: outputs)
        functor(output);
}

void mgm::RealKMSOutputContainer::update_from_hardware_state()
{
    decltype(outputs) new_outputs;

    std::exception_ptr kms_exception;
    for (auto drm_fd : drm_fds)
    {
        auto resources =
            [](int drm_fd, std::exception_ptr& failure_reason)
            {
                try
                {
                    return std::make_unique<kms::DRMModeResources>(drm_fd);
                }
                catch (std::runtime_error const&)
                {
                    failure_reason = std::current_exception();
                    return std::unique_ptr<kms::DRMModeResources>{};
                }
            }(drm_fd, kms_exception);

        if (!resources)
        {
            break;
        }

        for (auto &&connector : resources->connectors())
        {
            // Caution: O(n²) here, but n is the number of outputs, so should
            // conservatively be << 100.
            auto existing_output = std::find_if(
                outputs.begin(),
                outputs.end(),
                [&connector, drm_fd](auto const &candidate)
                {
                    return
                        connector->connector_id == candidate->id() &&
                        drm_fd == candidate->drm_fd();
                });

            if (existing_output != outputs.end())
            {
                // We could drop this down to O(n) by being smarter about moving out
                // of the outputs vector.
                //
                // That's a bit of a faff, so just do the simple thing for now.
                new_outputs.push_back(*existing_output);
                new_outputs.back()->refresh_hardware_state();
            }
            else
            {
                new_outputs.push_back(std::make_shared<RealKMSOutput>(
                    drm_fd,
                    std::move(connector),
                    construct_page_flipper(drm_fd)));
            }
        }

    }
    if (new_outputs.empty() && kms_exception)
    {
        std::rethrow_exception(kms_exception);
    }

    outputs = new_outputs;
}
