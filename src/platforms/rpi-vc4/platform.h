//
// Created by chris on 20/6/19.
//

#ifndef MIR_RPI_VC4_PLATFORM_H_
#define MIR_RPI_VC4_PLATFORM_H_

#include "mir/graphics/platform.h"

#include "display_platform.h"
#include "rendering_platform.h"

namespace mir
{
namespace graphics
{
namespace rpi
{
class Platform : public graphics::Platform
{
public:
    Platform();

    auto create_buffer_allocator(Display const &output) -> UniqueModulePtr<GraphicBufferAllocator> override;
    auto make_ipc_operations() const -> UniqueModulePtr<PlatformIpcOperations> override;
    auto native_rendering_platform() ->NativeRenderingPlatform * override;
    auto create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const &initial_conf_policy,
        std::shared_ptr<GLConfig> const &gl_config) -> UniqueModulePtr<Display> override;
    auto native_display_platform() -> NativeDisplayPlatform *override;
    auto extensions() const -> std::vector<ExtensionDescription> override;

private:
    std::unique_ptr<DisplayPlatform> const display_platform;
    std::unique_ptr<RenderingPlatform> const render_platform;
};
}
}
}


#endif  // MIR_RPI_VC4_PLATFORM_H_
