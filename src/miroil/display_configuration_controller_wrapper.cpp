/*
 * Copyright (C) 2021 Canonical, Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranties of MERCHANTABILITY,
 * SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <miroil/display_configuration_controller_wrapper.h>
#include "mir/shell/display_configuration_controller.h"

namespace miroil {

void DisplayConfigurationControllerWrapper::set_base_configuration(std::shared_ptr<mir::graphics::DisplayConfiguration> const& conf)
{
    wrapped->set_base_configuration(conf);
}

DisplayConfigurationControllerWrapper::DisplayConfigurationControllerWrapper(std::shared_ptr<mir::shell::DisplayConfigurationController> const & wrapped)
    : wrapped(wrapped)
{
}

}
