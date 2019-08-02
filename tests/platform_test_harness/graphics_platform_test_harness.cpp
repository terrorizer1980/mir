#include "mir/graphics/platform.h"
#include "mir/graphics/display.h"
#include "mir/graphics/display_buffer.h"
#include "mir/options/program_option.h"
#include "mir/shared_library.h"
#include "mir/renderer/gl/context_source.h"
#include "mir/renderer/gl/context.h"
#include "mir/main_loop.h"

#include "mir/default_server_configuration.h"

#include "mir/emergency_cleanup.h"
//#include <gtest/gtest.h>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>
#include <mir/renderer/gl/render_target.h>
#include <thread>
#include <EGL/egl.h>
#include <GL/gl.h>
#include <unistd.h>

namespace mg = mir::graphics;

namespace
{/*
class TestLogger
{
public:
    class TestScope
    {
        friend class TestLogger;
        TestScope(TestLogger& parent)
            : parent{parent}
        {
        }
    private:
        TestLogger& parent;
    };

    TestScope enter_test_scope(std::string test_name)
    {
        return TestScope{*this};
    }
};
*/  
//TODO: With a little bit of reworking of the CMake files it would be possible to
//just depend on the relevant object libraries and not pull in the whole DefaultServerConfiguration
class MinimalServerEnvironment : private mir::DefaultServerConfiguration
{
public:
    MinimalServerEnvironment()
        : DefaultServerConfiguration{1, argv}
    {
    }

    ~MinimalServerEnvironment()
    {
        if (main_loop_thread.joinable())
        {
            the_main_loop()->stop();
            main_loop_thread.join();
        }
    }

    auto options() const -> std::shared_ptr<mir::options::Option>
    {
        return std::make_shared<mir::options::ProgramOption>();
    }

    auto console_services() -> std::shared_ptr<mir::ConsoleServices>
    {
        // The ConsoleServices may require a running mainloop.
        if (!main_loop_thread.joinable())
            main_loop_thread = std::thread{[this]() { the_main_loop()->run(); }};

        return the_console_services();
    }

    auto display_report() -> std::shared_ptr<mir::graphics::DisplayReport>
    {
        return the_display_report();
    }

    auto logger() -> std::shared_ptr<mir::logging::Logger>
    {
        return the_logger();
    }

    auto emergency_cleanup_registry() -> std::shared_ptr<mir::EmergencyCleanupRegistry>
    {
        return the_emergency_cleanup();
    }

    auto initial_display_configuration() -> std::shared_ptr<mir::graphics::DisplayConfigurationPolicy>
    {
        return the_display_configuration_policy();
    }

    auto gl_config() -> std::shared_ptr<mir::graphics::GLConfig>
    {
        return the_gl_config();
    }
private:
    std::thread main_loop_thread;
    static char const* argv[];
};
char const* MinimalServerEnvironment::argv[] = {"graphics_platform_test_harness", nullptr};

std::string describe_probe_result(mg::PlatformPriority priority)
{
    if (priority == mg::PlatformPriority::unsupported)
    {
        return "UNSUPPORTED";
    }
    else if (priority == mg::PlatformPriority::dummy)
    {
        return "DUMMY";
    }
    else if (priority < mg::PlatformPriority::supported)
    {
        return std::string{"SUPPORTED - "} + std::to_string(mg::PlatformPriority::supported - priority);
    }
    else if (priority == mg::PlatformPriority::supported)
    {
        return "SUPPORTED";
    }
    else if (priority < mg::PlatformPriority::best)
    {
        return std::string{"SUPPORTED + "} + std::to_string(priority - mg::PlatformPriority::supported);
    }
    else if (priority == mg::PlatformPriority::best)
    {
        return "BEST";
    }
    return std::string{"BEST + "} + std::to_string(priority - mg::PlatformPriority::best);
}

bool test_probe(mir::SharedLibrary const& dso, MinimalServerEnvironment& config)
{
    try
    {
        auto probe_fn =
            dso.load_function<mg::PlatformProbe>("probe_graphics_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        auto const result = probe_fn(
            config.console_services(),
            *std::dynamic_pointer_cast<mir::options::ProgramOption>(config.options()));

        std::cout << "Probe result: " << describe_probe_result(result) << "(" << result << ")" << std::endl;

        return result > mg::PlatformPriority::dummy;
    }
    catch (std::exception const& err)
    {
        std::cout << "Probing failed: " << boost::diagnostic_information(err) << std::endl;
        return false;
    }
    catch (...)
    {
        std::cout << "Probing failed with broken exception!" << std::endl;
        return false;
    }
}

auto test_display_platform_construction(mir::SharedLibrary const& dso, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::DisplayPlatform>
{
    try
    {
        auto create_display_platform = dso.load_function<mg::CreateDisplayPlatform>(
            "create_display_platform", MIR_SERVER_GRAPHICS_PLATFORM_VERSION);

        auto display = create_display_platform(
            env.options(),
            env.emergency_cleanup_registry(),
            env.console_services(),
            env.display_report(),
            env.logger());

        std::cout << "Successfully constructed DisplayPlatform" << std::endl;

        return display;
    }
    catch (std::exception const& err)
    {
        std::cout << "Display construction failed: " << boost::diagnostic_information(err) << std::endl;
        throw;
    }
    catch (...)
    {
        std::cout << "Display construction failed with broken exception!" << std::endl;
        throw;
    }
}

auto test_display_construction(mir::graphics::DisplayPlatform& platform, MinimalServerEnvironment& env)
    -> std::shared_ptr<mir::graphics::Display>
{
    try
    {
        auto display = platform.create_display(
            env.initial_display_configuration(),
            env.gl_config());

        std::cout << "Successfully created display" << std::endl;

        return display;
    }
    catch (std::exception const& err)
    {
        std::cout << "Display construction failed: " << boost::diagnostic_information(err) << std::endl;
        throw;
    }
    catch (...)
    {
        std::cout << "Display construction failed with broken exception!" << std::endl;
        throw;
    }
}

auto test_display_supports_gl(mg::Display& display) -> bool
{
    if (dynamic_cast<mir::renderer::gl::ContextSource*>(display.native_display()))
    {
        std::cout << "Display supports GL context creation" << std::endl;
        return true;
    }
    std::cout << "Display does not support GL context creation" << std::endl;
    return false;
}

void for_each_display_buffer(mg::Display& display, std::function<void(mg::DisplayBuffer&)> functor)
{
    display.for_each_display_sync_group(
        [db_functor = std::move(functor)](mg::DisplaySyncGroup& sync_group)
        {
            sync_group.for_each_display_buffer(db_functor);
        });
}

auto test_display_has_at_least_one_enabled_output(mg::Display& display) -> bool
{
    int output_count{0};
    for_each_display_buffer(display, [&output_count](auto&) { ++output_count; });
    if (output_count > 0)
    {
        std::cout << "Display has " << output_count << " enabled outputs" << std::endl;
    }
    else
    {
        std::cout << "Display has no enabled outputs!" << std::endl;
    }

    return output_count > 0;
}

auto test_display_buffers_support_gl(mg::Display& display) -> bool
{
    bool all_support_gl{true};
    for_each_display_buffer(
        display,
        [&all_support_gl](mg::DisplayBuffer& db)
        {
            all_support_gl &=
                dynamic_cast<mir::renderer::gl::RenderTarget*>(db.native_display_buffer()) != nullptr;
        });

    if (all_support_gl)
    {
        std::cout << "DisplayBuffers support GL rendering" << std::endl;
    }
    else
    {
        std::cout << "DisplayBuffers do *not* support GL rendering" << std::endl;
    }

    return all_support_gl;
}

auto dump_egl_config(mg::Display& display) -> bool
{
    auto& context_source = dynamic_cast<mir::renderer::gl::ContextSource&>(display);
    auto ctx = context_source.create_gl_context();
    
    ctx->make_current();

    auto const dpy = eglGetCurrentDisplay();
    std::cout << "EGL Information: " << std::endl;
    std::cout << "EGL Client APIs: " << eglQueryString(dpy, EGL_CLIENT_APIS) << std::endl;
    std::cout << "EGL Vendor: " << eglQueryString(dpy, EGL_VENDOR) << std::endl;
    std::cout << "EGL Version: " << eglQueryString(dpy, EGL_VERSION) << std::endl;
    std::cout << "EGL Extensions: " << eglQueryString(dpy, EGL_EXTENSIONS) << std::endl;

    return true;
}

auto hex_to_gl(char colour) -> GLclampf
{
    return static_cast<float>(colour) / 255;
}

void basic_display_swapping(mg::Display& display)
{
    for_each_display_buffer(
        display,
        [](mg::DisplayBuffer& db)
        {
            auto& gl_buffer = dynamic_cast<mir::renderer::gl::RenderTarget&>(db);
            gl_buffer.make_current();
            
            for (int i = 0; i < 5; ++i)
            {
                glClearColor(
                    hex_to_gl(0xe9),
                    hex_to_gl(0x54),
                    hex_to_gl(0x20),
                    1.0f);

                glClear(GL_COLOR_BUFFER_BIT);

                gl_buffer.swap_buffers();

                sleep(1);

                glClearColor(
                    hex_to_gl(0x77),
                    hex_to_gl(0x21),
                    hex_to_gl(0x6f),
                    1.0f);

                glClear(GL_COLOR_BUFFER_BIT);

                gl_buffer.swap_buffers();

                sleep(1);
            }
        });
}
}

int main(int argc, char const** argv)
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " PLATFORM_DSO" << std::endl;
        return -1;
    }

    mir::SharedLibrary platform_dso{argv[1]};

    MinimalServerEnvironment config;

    bool success = true;
    success &= test_probe(platform_dso, config);
    if (success)
    {
        if (auto display_platform = test_display_platform_construction(platform_dso, config))
        {
            if(auto display = test_display_construction(*display_platform, config))
            {
                success &= test_display_supports_gl(*display);
                success &= dump_egl_config(*display);
                success &= test_display_has_at_least_one_enabled_output(*display);
                success &= test_display_buffers_support_gl(*display);
                basic_display_swapping(*display);
            }
            else
            {
                success = false;
            }
        }
        else
        {
            success = false;
        }
    }
    return success ? 0 : -1;
}
