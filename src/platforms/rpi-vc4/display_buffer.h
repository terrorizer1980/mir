//
// Created by chris on 26/6/19.
//

#ifndef MIR_DISPLAYBUFFER_H
#define MIR_DISPLAYBUFFER_H

#include "mir/graphics/display_buffer.h"
#include "mir/renderer/gl/render_target.h"
#include "mir/graphics/display.h"

#include <EGL/egl.h>
#include <bcm_host.h>

namespace mir
{
namespace graphics
{
namespace rpi
{
class DisplayBuffer
    : public graphics::DisplayBuffer,
      public graphics::DisplaySyncGroup,
      public graphics::NativeDisplayBuffer,
      public renderer::gl::RenderTarget
{
public:
    DisplayBuffer(
        geometry::Size const& size,
        DISPMANX_DISPLAY_HANDLE_T display,
        EGLDisplay dpy,
        EGLConfig config,
        EGLContext ctx);

    void for_each_display_buffer(std::function<void(graphics::DisplayBuffer&)> const& f) override;
    void post() override;
    std::chrono::milliseconds recommended_sleep() const override;

    geometry::Rectangle view_area() const override;
    bool overlay(RenderableList const& renderlist) override;
    glm::mat2 transformation() const override;
    NativeDisplayBuffer* native_display_buffer() override;

    void make_current() override;
    void release_current() override;
    void swap_buffers() override;
    void bind() override;

private:
    geometry::Rectangle const view;
    EGLDisplay const dpy;
    EGLContext const ctx;
    EGLSurface const surface;
};
}
}
}

#endif  // MIR_DISPLAYBUFFER_H
