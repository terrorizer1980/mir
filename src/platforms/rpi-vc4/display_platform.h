//
// Created by chris on 20/6/19.
//

#ifndef MIR_DISPLAY_PLATFORM_H
#define MIR_DISPLAY_PLATFORM_H

#include "mir/graphics/platform.h"

#include <EGL/egl.h>

namespace mir
{
namespace graphics
{
namespace rpi
{
class DisplayPlatform : public graphics::DisplayPlatform
{
public:
    DisplayPlatform();

    auto create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config)
        ->UniqueModulePtr<graphics::Display> override;
    NativeDisplayPlatform* native_display_platform() override;
    std::vector<ExtensionDescription> extensions() const override;

private:
    EGLDisplay dpy;
};
}
}
}

#endif  // MIR_DISPLAY_PLATFORM_H
