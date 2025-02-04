/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir_test_framework/main.h"

#include <libgen.h>

#include <iostream>

int main(int argc, char* argv[])
{
    unsetenv("WAYLAND_DISPLAY");    // We don't want to conflict with any existing server
    // Override this standard gtest message
    char path[] = __FILE__;
    std::cout << "Running main() from " << basename(path) << std::endl;

    return mir_test_framework::main(argc, argv);
}
