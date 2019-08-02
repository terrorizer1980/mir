//
// Created by chris on 21/6/19.
//

#ifndef MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_
#define MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_

#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir/graphics/wayland_allocator.h"
#include "mir/graphics/egl_extensions.h"

namespace mir
{
class Executor;

namespace renderer
{
namespace gl
{
class Context;
}
}

namespace graphics
{
class Display;

namespace rpi
{
class BufferAllocator :
	public GraphicBufferAllocator,
	public WaylandAllocator
{
public:
    BufferAllocator(graphics::Display const& output);

    std::shared_ptr<Buffer> alloc_buffer(BufferProperties const &buffer_properties) override;
    std::vector<MirPixelFormat> supported_pixel_formats() override;
    std::shared_ptr<Buffer> alloc_buffer(geometry::Size size, uint32_t native_format, uint32_t native_flags) override;
    std::shared_ptr<Buffer> alloc_software_buffer(geometry::Size size, MirPixelFormat format) override;

    void bind_display(wl_display* display, std::shared_ptr<Executor> wayland_executor) override;
    std::shared_ptr<Buffer> buffer_from_resource(
        wl_resource* resource,
	std::function<void()>&& ,
	std::function<void()>&&) override;

private:
    std::shared_ptr<EGLExtensions> const egl_extensions;
    std::shared_ptr<renderer::gl::Context> const ctx;
    std::shared_ptr<Executor> wayland_executor;
};
}
}
}
#endif  // MIR_GRAPHICS_RPI_VC4_BUFFER_ALLOCATOR_H_
