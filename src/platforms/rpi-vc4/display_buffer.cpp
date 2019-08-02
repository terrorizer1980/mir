//
// Created by chris on 26/6/19.
//

#include "display_buffer.h"

#include "mir/graphics/egl_error.h"

#include <boost/throw_exception.hpp>

namespace mg = mir::graphics;

namespace
{
typedef struct {
    DISPMANX_ELEMENT_HANDLE_T element;
    int width;   /* This is necessary because dispmanx elements are not queriable. */
    int height;
} EGL_DISPMANX_WINDOW_T;


auto surface_for_display(
    DISPMANX_DISPLAY_HANDLE_T display,
    mir::geometry::Size const& size,
    EGLDisplay dpy,
    EGLConfig config) -> EGLSurface
{
    VC_RECT_T source_rect, dest_rect;

    dest_rect.x = 0;
    dest_rect.y = 0;
    dest_rect.width = size.width.as_uint32_t();
    dest_rect.height = size.height.as_uint32_t();

    source_rect.x = 0;
    source_rect.y = 0;
    source_rect.width = size.width.as_uint32_t() << 16;
    source_rect.height = size.height.as_uint32_t() << 16;

    DISPMANX_UPDATE_HANDLE_T update = vc_dispmanx_update_start(0);

    DISPMANX_ELEMENT_HANDLE_T display_element =
        vc_dispmanx_element_add(
            update,
            display,
            0,
            &dest_rect,
            0,
            &source_rect,
            DISPMANX_PROTECTION_NONE,
            0,
            0,
            DISPMANX_NO_ROTATE);

    vc_dispmanx_update_submit_sync(update);

    static EGL_DISPMANX_WINDOW_T native_window {
        display_element,
        size.width.as_int(),
        size.height.as_int()
    };

    auto const surface = eglCreateWindowSurface(
        dpy,
        config,
        reinterpret_cast<EGLNativeWindowType>(&native_window), // Who typedef'd EGLNativeWindowType as unsigned long?!
        NULL);

    if (surface == EGL_NO_SURFACE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to create EGL surface for display")));
    }

    return surface;
}
}

mg::rpi::DisplayBuffer::DisplayBuffer(
    geometry::Size const& size,
    DISPMANX_DISPLAY_HANDLE_T display,
    EGLDisplay dpy,
    EGLConfig config,
    EGLContext ctx)
    : view{{0, 0}, size},
      dpy{dpy},
      ctx{ctx},
      surface{surface_for_display(display, size, dpy, config)}
{
}

void mg::rpi::DisplayBuffer::for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
{
    f(*this);
}

void mg::rpi::DisplayBuffer::post()
{
}

std::chrono::milliseconds mg::rpi::DisplayBuffer::recommended_sleep() const
{
    return std::chrono::milliseconds();
}
mir::geometry::Rectangle mg::rpi::DisplayBuffer::view_area() const
{
    return view;
}
bool mg::rpi::DisplayBuffer::overlay(mg::RenderableList const& /*renderlist*/)
{
    return false;
}
glm::mat2 mg::rpi::DisplayBuffer::transformation() const
{
    return glm::mat2();
}
mg::NativeDisplayBuffer* mg::rpi::DisplayBuffer::native_display_buffer()
{
    return this;
}

void mg::rpi::DisplayBuffer::make_current()
{
    if (eglMakeCurrent(dpy, surface, surface, ctx) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to make context current"));
    }
}

void mg::rpi::DisplayBuffer::release_current()
{
    if (eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION(mg::egl_error("Failed to release context"));
    }
}

void mg::rpi::DisplayBuffer::swap_buffers()
{
    if (eglSwapBuffers(dpy, surface) != EGL_TRUE)
    {
        BOOST_THROW_EXCEPTION((mg::egl_error("Failed to swap buffers")));
    }
}

void mg::rpi::DisplayBuffer::bind()
{
}
