/*
 * Copyright © 2012 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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

#include "mir_test/multithread_harness.h"

#include <vector>
#include <thread>

namespace mt = mir::testing;

void test_func (mt::SynchronizedThread<int, int>* synchronizer, std::shared_ptr<int>, int* data) {
    *data = 1;
    synchronizer->child_wait();
    *data = 2;
    synchronizer->child_wait();
}

TEST(Synchronizer, thread_stop_start) {
    int data = 0;

    auto thread_start_time = std::chrono::system_clock::now();
    auto abs_timeout = thread_start_time + std::chrono::milliseconds(500);
    mt::SynchronizedThread<int,int> t1(abs_timeout, test_func, nullptr, &data);

    t1.stabilize();
    EXPECT_EQ(data, 1);
    t1.activate();

    t1.stabilize();
    EXPECT_EQ(data, 2);
    t1.activate();
}

void test_func_pause (mt::SynchronizedThread<int, int>* synchronizer, std::shared_ptr<int>, int* data) {
    for(;;)
    {
        *data = *data+1;
        if (synchronizer->child_check()) break;
    } 
}

TEST(Synchronizer, thread_pause_req) {
    int data = 0;
    auto thread_start_time = std::chrono::system_clock::now();
    auto abs_timeout = thread_start_time + std::chrono::milliseconds(500);
    mt::SynchronizedThread<int, int> t1(abs_timeout, test_func_pause, nullptr, &data);

    t1.stabilize();
    EXPECT_EQ(data, 1);
    t1.activate();

    t1.stabilize();
    EXPECT_EQ(data, 2);
    t1.activate();

    t1.stabilize();
    t1.kill_thread();
    t1.activate();
    
}
