//
// Created by chris on 20/6/19.
//

#include <boost/throw_exception.hpp>

#include "mir/module_deleter.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"

#include "rendering_platform.h"

#include "buffer_allocator.h"

namespace mg = mir::graphics;

auto mg::rpi::RenderingPlatform::create_buffer_allocator(Display const &output)
  ->mir::UniqueModulePtr<GraphicBufferAllocator>
{
    return mir::make_module_ptr<rpi::BufferAllocator>(output);
}

auto mir::graphics::rpi::RenderingPlatform::make_ipc_operations() const
  -> mir::UniqueModulePtr<PlatformIpcOperations>
{
    BOOST_THROW_EXCEPTION((std::runtime_error{"rpi-vc4 platform does not support mirclient"}));
}

auto mir::graphics::rpi::RenderingPlatform::native_rendering_platform()
  -> mir::graphics::NativeRenderingPlatform *
{
    return nullptr;
}
