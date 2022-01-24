/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/throw_exception.hpp>
#include <thread>
#include <future>

#include "mir/system_executor.h"
#include "mir/test/signal.h"
#include "mir/test/current_thread_name.h"

using namespace std::literals::chrono_literals;
using namespace testing;

namespace mt = mir::test;

TEST(SystemExecutor, executes_work)
{
    auto const done = std::make_shared<mt::Signal>();
    mir::system_executor.spawn([done]() { done->raise(); });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(SystemExecutor, can_execute_work_from_within_work_item)
{
    auto const done = std::make_shared<mt::Signal>();

    mir::system_executor.spawn(
        [done]()
        {
            mir::system_executor.spawn([done]() { done->raise(); });
        });

    EXPECT_TRUE(done->wait_for(60s));
}

TEST(SystemExecutor, work_executed_from_within_work_item_is_not_blocked_by_work_item_blocking)
{
    auto const done = std::make_shared<mt::Signal>();
    auto const waited_for_done = std::make_shared<mt::Signal>();

    mir::system_executor.spawn(
        [done, waited_for_done]()
        {
            mir::system_executor.spawn([done]() { done->raise(); });
            if (!waited_for_done->wait_for(60s))
            {
                FAIL() << "Spawned work failed to execute";
            }
        });

    EXPECT_TRUE(done->wait_for(60s));
    waited_for_done->raise();
}

TEST(SystemExecutorDeathTest, unhandled_exception_in_work_item_causes_termination_by_default)
{
    EXPECT_DEATH(
        {
            mir::system_executor.spawn(
                []()
                {
                    BOOST_THROW_EXCEPTION((std::runtime_error{"Oops, unhandled exception"}));
                });
            std::this_thread::sleep_for(std::chrono::seconds{60});
        },
        ""
    );
}

TEST(SystemExecutor, can_set_unhandled_exception_handler)
{
    static std::promise<std::exception_ptr> exception_pipe;

    auto exception = exception_pipe.get_future();

    mir::system_executor.set_unhandled_exception_handler(
        []()
        {
            exception_pipe.set_value(std::current_exception());
        });

    mir::system_executor.spawn([]() { throw std::runtime_error{"Boop!"}; });

    EXPECT_THAT(exception.wait_for(std::chrono::seconds{60}), Eq(std::future_status::ready));

    try
    {
        std::rethrow_exception(exception.get());
    }
    catch (std::runtime_error const& err)
    {
        EXPECT_THAT(err.what(), StrEq("Boop!"));
    }
}

#ifndef MIR_DONT_USE_PTHREAD_GETNAME_NP
TEST(SystemExecutor, executor_threads_have_sensible_name)
#else
TEST(SystemExecutor, DISABLED_executor_threads_have_sensible_names)
#endif
{
    auto thread_name_provider = std::make_shared<std::promise<std::string>>();
    auto thread_name = thread_name_provider->get_future();

    mir::system_executor.spawn(
        [thread_name_provider]()
        {
            thread_name_provider->set_value(mt::current_thread_name());
        });

    ASSERT_THAT(thread_name.wait_for(std::chrono::seconds{60}), Eq(std::future_status::ready));
    EXPECT_THAT(thread_name.get(), MatchesRegex("Mir/Workqueue.*"));
}
