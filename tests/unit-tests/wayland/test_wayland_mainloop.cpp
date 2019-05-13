/*
 * Copyright © 2019 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "src/server/frontend_wayland/wayland_executor.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <wayland-server-core.h>

#include "mir/test/signal.h"

namespace mt = mir::test;
namespace mf = mir::frontend;

using namespace testing;

class WaylandMainloopTest : public Test {
public:
    WaylandMainloopTest()
        : display{wl_display_create()}
    {
    }

    ~WaylandMainloopTest()
    {
        wl_display_destroy(display);
    }

    wl_display* const display;
};

TEST_F(WaylandMainloopTest, does_not_dispatch_before_start)
{
    mf::WaylandExecutor ml{display};

    auto dispatched = std::make_shared<mt::Signal>();

    ml.spawn([dispatched]() { dispatched->raise(); });

    EXPECT_FALSE(dispatched->wait_for(std::chrono::seconds{1}));
}

TEST_F(WaylandMainloopTest, dispatches_after_start)
{
    mf::WaylandExecutor ml{display};

    auto dispatched = std::make_shared<mt::Signal>();

    ml.spawn([dispatched]() { dispatched->raise(); });

    ml.start();

    EXPECT_TRUE(dispatched->wait_for(std::chrono::seconds{1}));
}

