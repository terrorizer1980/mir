/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "multithread_harness.h"

#include "src/server/compositor/stream.h"
#include "mir/graphics/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <future>
#include <thread>
#include <mutex>
#include <atomic>

namespace mc = mir::compositor;
namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mt = mir::testing;
namespace geom = mir::geometry;

namespace
{

struct SwapperSwappingStress : public ::testing::Test
{
    void SetUp()
    {
        stream = std::make_shared<mc::Stream>(
            geom::Size{380, 210}, mir_pixel_format_abgr_8888);
    }

    std::shared_ptr<mc::Stream> stream;
};

} // namespace

TEST_F(SwapperSwappingStress, swapper)
{
    std::atomic_bool done(false);

    auto f = std::async(std::launch::async,
                [&]
                {
                    std::shared_ptr<mg::Buffer> buffer = nullptr;
                    for(auto i=0u; i < 400; i++)
                    {
                        stream->submit_buffer(buffer);
                        std::this_thread::yield();
                    }
                    done = true;
                });

    auto g = std::async(std::launch::async,
                [&]
                {
                    while (!done)
                    {
                        auto b = stream->lock_compositor_buffer(0);
                        std::this_thread::yield();
                        b.reset();
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 100; i++)
                    {
                        stream->allow_framedropping(true);
                        std::this_thread::yield();
                        stream->allow_framedropping(false);
                        std::this_thread::yield();
                    }
                });

    f.wait();
    g.wait();
    h.wait();
}

TEST_F(SwapperSwappingStress, different_swapper_types)
{
    std::atomic_bool done(false);

    auto f = std::async(std::launch::async,
                [&]
                {
                    std::shared_ptr<mg::Buffer> buffer = nullptr;
                    for(auto i=0u; i < 400; i++)
                    {
                        stream->submit_buffer(buffer);
                        std::this_thread::yield();
                    }
                    done = true;
                });

    auto g = std::async(std::launch::async,
                [&]
                {
                    while (!done)
                    {
                        auto b = stream->lock_compositor_buffer(0);
                        std::this_thread::yield();
                        b.reset();
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 200; i++)
                    {
                        stream->allow_framedropping(true);
                        std::this_thread::yield();
                        stream->allow_framedropping(false);
                        std::this_thread::yield();
                    }
                });

    f.wait();
    g.wait();
    h.wait();
}
