//
// Created by chris on 20/6/19.
//

#ifndef MIR_RENDERINGPLATFORM_H
#define MIR_RENDERINGPLATFORM_H

#include "mir/graphics/platform.h"

namespace mir
{
namespace graphics
{
namespace rpi
{
class RenderingPlatform : public graphics::RenderingPlatform
{
public:
    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator(Display const &output) override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
    NativeRenderingPlatform *native_rendering_platform() override;
};
}
}
}

#endif  // MIR_RENDERINGPLATFORM_H
