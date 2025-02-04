/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/shared_library_prober.h"
#include "mir/shared_library.h"

#include <boost/filesystem.hpp>

#include <system_error>
#include <cstring>
#include <algorithm>

namespace
{
std::error_code boost_to_std_error(boost::system::error_code const& ec)
{
    if (ec)
    {
        if (ec.category() != boost::system::system_category())
        {
            throw std::logic_error{"Boost error from unexpected category: " +
                                   ec.message()};
        }
        return std::error_code{ec.value(), std::system_category()};
    }
    return std::error_code{};
}

// Libraries can be of the form libname.so(.X.Y)
bool path_has_library_extension(boost::filesystem::path const& path)
{
    return path.extension().string() == ".so" ||
           path.string().find(".so.") != std::string::npos;
}

}

namespace
{
bool greater_soname_version(boost::filesystem::path const& lhs, boost::filesystem::path const& rhs)
{
    auto lhbuf = strrchr(lhs.c_str(), '.');
    auto rhbuf = strrchr(rhs.c_str(), '.');

    if (!rhbuf) return lhbuf;
    if (!lhbuf) return false;

    return strtol(++lhbuf, 0, 0) > strtol(++rhbuf, 0, 0);
}
}

void mir::select_libraries_for_path(
    std::string const& path,
    std::function<Selection(std::shared_ptr<mir::SharedLibrary> const&)> const& selector,
    mir::SharedLibraryProberReport& report)
{
    report.probing_path(path);
    // We use the error_code overload because we want to throw a std::system_error
    boost::system::error_code ec;

    boost::filesystem::directory_iterator iterator{path, ec};
    if (ec)
    {
        std::system_error error(boost_to_std_error(ec), path);
        report.probing_failed(path, error);
        throw error;
    }

    std::vector<boost::filesystem::path> libraries;
    for (; iterator != boost::filesystem::directory_iterator() ; ++iterator)
    {
        if (path_has_library_extension(iterator->path()))
            libraries.push_back(iterator->path().string());
    }

    std::sort(libraries.begin(), libraries.end(), &greater_soname_version);

    for(auto& lib : libraries)
    {
        try
        {
            report.loading_library(lib);
            auto const shared_lib = std::make_shared<mir::SharedLibrary>(lib.string());

            if (selector(shared_lib) == Selection::quit)
                return;
        }
        catch (std::runtime_error const& err)
        {
            report.loading_failed(lib, err);
        }
    }
}

std::vector<std::shared_ptr<mir::SharedLibrary>>
mir::libraries_for_path(std::string const& path, mir::SharedLibraryProberReport& report)
{
    std::vector<std::shared_ptr<mir::SharedLibrary>> result;

    select_libraries_for_path(
        path,
        [&](std::shared_ptr<mir::SharedLibrary> const& shared_lib)
            { result.push_back(shared_lib); return Selection::persist; },
        report);

    return result;
}
