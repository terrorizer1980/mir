/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "compositor_report.h"
#include "mir/logging/logger.h"

using namespace mir;
using namespace mir::compositor;
using namespace mir::time;

namespace
{
    const char * const component = "compositor";
    const auto min_report_interval = std::chrono::seconds(1);
}

logging::CompositorReport::CompositorReport(
    std::shared_ptr<Logger> const& logger,
    std::shared_ptr<Clock> const& clock)
    : logger(logger),
      clock(clock),
      last_report(now())
{
}

logging::CompositorReport::TimePoint logging::CompositorReport::now() const
{
    return clock->sample();
}

void logging::CompositorReport::added_display(int width, int height,
                                              int x, int y,
                                              SubCompositorId id)
{
    char msg[128];
    snprintf(msg, sizeof msg, "Added display %p: %dx%d %+d%+d",
             id, width, height, x, y);
    logger->log(Logger::informational, msg, component);
}

void logging::CompositorReport::began_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.start_of_frame = t;
    inst.latency_sum += t - last_scheduled;
}

void logging::CompositorReport::Instance::log(
    Logger& logger, SubCompositorId id)
{
    // The first report is a valid sample, but don't log anything because
    // we need at least two samples for valid deltas.
    if (last_reported_total_time_sum > TimePoint())
    {
        long long dt =
            std::chrono::duration_cast<std::chrono::microseconds>(
                total_time_sum - last_reported_total_time_sum
            ).count();
        auto dn = nframes - last_reported_nframes;
        long long df =
            std::chrono::duration_cast<std::chrono::microseconds>(
                frame_time_sum - last_reported_frame_time_sum
            ).count();
        long long dl =
            std::chrono::duration_cast<std::chrono::microseconds>(
                latency_sum - last_reported_latency_sum
            ).count();

        // Keep everything premultiplied by 1000 to guarantee accuracy
        // and avoid floating point.
        long frames_per_1000sec = dt ? dn * 1000000000LL / dt : 0;
        long avg_frame_time_usec = dn ? df / dn : 0;
        long avg_latency_usec = dn ? dl / dn : 0;
        long dt_msec = dt / 1000L;

        char msg[128];
        snprintf(msg, sizeof msg, "Display %p averaged %ld.%03ld FPS, "
                 "%ld.%03ld ms/frame, "
                 "latency %ld.%03ld ms, "
                 "%ld frames over %ld.%03ld sec",
                 id,
                 frames_per_1000sec / 1000,
                 frames_per_1000sec % 1000,
                 avg_frame_time_usec / 1000,
                 avg_frame_time_usec % 1000,
                 avg_latency_usec / 1000,
                 avg_latency_usec % 1000,
                 dn,
                 dt_msec / 1000,
                 dt_msec % 1000
                 );

        logger.log(Logger::informational, msg, component);
    }

    last_reported_total_time_sum = total_time_sum;
    last_reported_frame_time_sum = frame_time_sum;
    last_reported_latency_sum = latency_sum;
    last_reported_nframes = nframes;
}

void logging::CompositorReport::finished_frame(SubCompositorId id)
{
    std::lock_guard<std::mutex> lock(mutex);
    auto& inst = instance[id];

    auto t = now();
    inst.total_time_sum += t - inst.end_of_frame;
    inst.frame_time_sum += t - inst.start_of_frame;
    inst.end_of_frame = t;
    inst.nframes++;

    /*
     * The exact reporting interval doesn't matter because we count everything
     * as a Reimann sum. Results will simply be the average over the interval.
     */
    if ((t - last_report) >= min_report_interval)
    {
        last_report = t;

        for (auto& i : instance)
            i.second.log(*logger, i.first);
    }
}

void logging::CompositorReport::started()
{
    logger->log(Logger::informational, "Started", component);
}

void logging::CompositorReport::stopped()
{
    logger->log(Logger::informational, "Stopped", component);

    std::lock_guard<std::mutex> lock(mutex);
    instance.clear();
}

void logging::CompositorReport::scheduled()
{
    std::lock_guard<std::mutex> lock(mutex);
    last_scheduled = now();
}
