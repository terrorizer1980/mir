/*
 * AUTOGENERATED - DO NOT EDIT
 *
 * This file is generated from text-input-unstable-v3.xml
 * To regenerate, run the “refresh-wayland-wrapper” target.
 */

#ifndef MIR_FRONTEND_WAYLAND_TEXT_INPUT_UNSTABLE_V3_XML_WRAPPER
#define MIR_FRONTEND_WAYLAND_TEXT_INPUT_UNSTABLE_V3_XML_WRAPPER

#include <optional>

#include "mir/fd.h"
#include <wayland-server-core.h>

#include "mir/wayland/wayland_base.h"

namespace mir
{
namespace wayland
{

class TextInputV3;
class TextInputManagerV3;

class TextInputV3 : public Resource
{
public:
    static char const constexpr* interface_name = "zwp_text_input_v3";

    static TextInputV3* from(struct wl_resource*);

    TextInputV3(struct wl_resource* resource, Version<1>);
    virtual ~TextInputV3();

    void send_enter_event(struct wl_resource* surface) const;
    void send_leave_event(struct wl_resource* surface) const;
    void send_preedit_string_event(std::optional<std::string> const& text, int32_t cursor_begin, int32_t cursor_end) const;
    void send_commit_string_event(std::optional<std::string> const& text) const;
    void send_delete_surrounding_text_event(uint32_t before_length, uint32_t after_length) const;
    void send_done_event(uint32_t serial) const;

    struct wl_client* const client;
    struct wl_resource* const resource;

    struct ChangeCause
    {
        static uint32_t const input_method = 0;
        static uint32_t const other = 1;
    };

    struct ContentHint
    {
        static uint32_t const none = 0x0;
        static uint32_t const completion = 0x1;
        static uint32_t const spellcheck = 0x2;
        static uint32_t const auto_capitalization = 0x4;
        static uint32_t const lowercase = 0x8;
        static uint32_t const uppercase = 0x10;
        static uint32_t const titlecase = 0x20;
        static uint32_t const hidden_text = 0x40;
        static uint32_t const sensitive_data = 0x80;
        static uint32_t const latin = 0x100;
        static uint32_t const multiline = 0x200;
    };

    struct ContentPurpose
    {
        static uint32_t const normal = 0;
        static uint32_t const alpha = 1;
        static uint32_t const digits = 2;
        static uint32_t const number = 3;
        static uint32_t const phone = 4;
        static uint32_t const url = 5;
        static uint32_t const email = 6;
        static uint32_t const name = 7;
        static uint32_t const password = 8;
        static uint32_t const pin = 9;
        static uint32_t const date = 10;
        static uint32_t const time = 11;
        static uint32_t const datetime = 12;
        static uint32_t const terminal = 13;
    };

    struct Opcode
    {
        static uint32_t const enter = 0;
        static uint32_t const leave = 1;
        static uint32_t const preedit_string = 2;
        static uint32_t const commit_string = 3;
        static uint32_t const delete_surrounding_text = 4;
        static uint32_t const done = 5;
    };

    struct Thunks;

    static bool is_instance(wl_resource* resource);

private:
    virtual void enable() = 0;
    virtual void disable() = 0;
    virtual void set_surrounding_text(std::string const& text, int32_t cursor, int32_t anchor) = 0;
    virtual void set_text_change_cause(uint32_t cause) = 0;
    virtual void set_content_type(uint32_t hint, uint32_t purpose) = 0;
    virtual void set_cursor_rectangle(int32_t x, int32_t y, int32_t width, int32_t height) = 0;
    virtual void commit() = 0;
};

class TextInputManagerV3 : public Resource
{
public:
    static char const constexpr* interface_name = "zwp_text_input_manager_v3";

    static TextInputManagerV3* from(struct wl_resource*);

    TextInputManagerV3(struct wl_resource* resource, Version<1>);
    virtual ~TextInputManagerV3();

    struct wl_client* const client;
    struct wl_resource* const resource;

    struct Thunks;

    static bool is_instance(wl_resource* resource);

    class Global : public wayland::Global
    {
    public:
        Global(wl_display* display, Version<1>);

        auto interface_name() const -> char const* override;

    private:
        virtual void bind(wl_resource* new_zwp_text_input_manager_v3) = 0;
        friend TextInputManagerV3::Thunks;
    };

private:
    virtual void get_text_input(struct wl_resource* id, struct wl_resource* seat) = 0;
};

}
}

#endif // MIR_FRONTEND_WAYLAND_TEXT_INPUT_UNSTABLE_V3_XML_WRAPPER
