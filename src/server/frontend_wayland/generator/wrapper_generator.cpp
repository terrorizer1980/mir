/*
 * Copyright © 2017 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored By: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "wrapper_generator.h"
#include "emitter.h"
#include "argument.h"

#include <libxml++/libxml++.h>
#include <functional>
#include <vector>
#include <experimental/optional>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <locale>
#include <stdio.h>

const std::vector<std::string> cpp_reserved_keywords = {"namespace"}; // add to this on an as-needed basis

// remove the path from a file path, leaving only the base name
std::string file_name_from_path(std::string const& path)
{
    size_t i = path.find_last_of("/");
    if (i == std::string::npos)
        return path;
    else
        return path.substr(i + 1);
}

// make sure the name is not a C++ reserved word, could be expanded to get rid of invalid characters if that was needed
std::string sanitize_name(std::string const& name)
{
    std::string ret = name;
    for (auto const& i: cpp_reserved_keywords)
    {
        if (i == name)
        {
            ret = name + "_";
        }
    }
    return ret;
}

Emitter emit_comment_header(std::string const& input_file_path)
{
    return Lines{
        "/*",
        " * AUTOGENERATED - DO NOT EDIT",
        " *",
        {" * This header is generated by wrapper_generator.cpp from ", file_name_from_path(input_file_path)},
        " * To regenerate, run the “refresh-wayland-wrapper” target.",
        " */",
    };
}

// converts any string into a valid, all upper case macro name (replacing special chars with underscores)
std::string macro_string(std::string const& name)
{
    std::string macro_name = "";
    for (unsigned i = 0; i < name.size(); i++)
    {
        char c = name[i];
        if ((c >= 'a' && c <= 'z') ||
            (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9' && i > 0))
        {
            macro_name += std::toupper(c, std::locale("C"));
        }
        else
        {
            macro_name += '_';
        }
    }
    return macro_name;
}

Emitter emit_include_guard_top(std::string const& macro)
{
    return Lines{
        {"#ifndef ", macro},
        {"#define ", macro},
    };
}

Emitter emit_include_guard_bottom(std::string const& macro)
{
    return Lines{
        {"#endif // ", macro}
    };
}

Emitter emit_required_headers(std::string const& custom_header)
{
    return Lines{
        "#include <experimental/optional>",
        "#include <boost/throw_exception.hpp>",
        "#include <boost/exception/diagnostic_information.hpp>",
        "",
        {"#include \"", custom_header, "\""},
        "",
        "#include \"mir/fd.h\"",
        "#include \"mir/log.h\"",
    };
}

std::string camel_case_string(std::string const& name)
{
    std::string camel_cased_name;
    camel_cased_name = std::string{std::toupper(name[0], std::locale("C"))} + name.substr(1);
    auto next_underscore_offset = name.find('_');
    while (next_underscore_offset != std::string::npos)
    {
        if (next_underscore_offset < camel_cased_name.length())
        {
            camel_cased_name = camel_cased_name.substr(0, next_underscore_offset) +
                               std::toupper(camel_cased_name[next_underscore_offset + 1], std::locale("C")) +
                               camel_cased_name.substr(next_underscore_offset + 2);
        }
        next_underscore_offset = camel_cased_name.find('_', next_underscore_offset);
    }
    return camel_cased_name;
}

void emit_indented_lines(std::ostream& out, std::string const& indent,
                         std::initializer_list<std::initializer_list<std::string>> lines)
{
    for (auto const& line : lines)
    {
        out << indent;
        for (auto const& fragment : line)
        {
            out << fragment;
        }
        out << "\n";
    }
}

class Interface;

class Method
{
public:
    Method(xmlpp::Element const& node)
        : name{node.get_attribute_value("name")}
    {
        for (auto const& child : node.get_children("arg"))
        {
            auto arg_node = dynamic_cast<xmlpp::Element const*>(child);
            arguments.emplace_back(std::ref(*arg_node));
        }
    }

    // TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
    Emitter emit_virtual_prototype(bool is_global) const
    {
        std::vector<Emitter> args;
        if (is_global)
        {
            args.push_back("struct wl_client* client");
            args.push_back("struct wl_resource* resource");
        }
        for (auto& i : arguments)
        {
            args.push_back(i.cpp_prototype());
        }

        return {"virtual void ", name, "(", List{args, ", "}, ") = 0;"};
    }

    // TODO: Decide whether to resolve wl_resource* to wrapped types (ie: Region, Surface, etc).
    Emitter emit_thunk(std::string const& interface_type, bool is_global) const
    {
        std::vector<Emitter> c_args{
            {"struct wl_client*", (is_global ? " client" : "")},
            "struct wl_resource* resource"};
        for (auto const& arg : arguments)
            c_args.push_back(arg.c_prototype());

        std::vector<Emitter> thunk_converters;
        for (auto const& arg : arguments)
        {
            if (auto converter = arg.thunk_converter())
                thunk_converters.push_back(converter.value());
        }

        std::vector<Emitter> call_args;
        if (is_global)
        {
            call_args.push_back("client");
            call_args.push_back("resource");
        }
        for (auto& arg : arguments)
            call_args.push_back(arg.thunk_call_fragment());

        return {"static void ", name, "_thunk(", List{c_args, ", "}, ")",
            Block{
                {"auto me = static_cast<", interface_type, "*>(wl_resource_get_user_data(resource));"},
                Lines{thunk_converters},
                "try",
                Block{
                    {"me->", name, "(", List{call_args, ", "}, ");"}
                },
                "catch (...)",
                Block{
                    {"::mir::log(",
                        List{{
                                "::mir::logging::Severity::critical",
                                "    \"frontend:Wayland\"",
                                "    std::current_exception()",
                                {"    \"Exception processing ", interface_type, "::", name, "() request\""}
                            }, ","},
                        ");"}
                }
            }
        };
    }

    Emitter emit_vtable_initialiser() const
    {
        return {name, "_thunk"};
    }

private:
    std::string const name;
    std::vector<Argument> arguments;

public:
};

class Interface
{
public:
    Interface(
        xmlpp::Element const& node,
        std::function<std::string(std::string)> const& name_transform,
        std::unordered_set<std::string> const& constructable_interfaces)
        : wl_name{node.get_attribute_value("name")},
          generated_name{name_transform(wl_name)},
          is_global{constructable_interfaces.count(wl_name) == 0}
    {
        for (auto method_node : node.get_children("request"))
        {
            auto method = dynamic_cast<xmlpp::Element*>(method_node);
            methods.emplace_back(std::ref(*method));
        }
    }

    void emit_constructor(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        if (is_global)
        {
            emit_constructor_for_global(out, indent);
        }
        else
        {
            emit_constructor_for_regular(out, indent, has_vtable);
        }
    }

    void emit_bind(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        emit_indented_lines(out, indent, {
            {"static void bind_thunk(struct wl_client* client, void* data, uint32_t version, uint32_t id)"},
            {"{"},
        });
        emit_indented_lines(out, indent + "    ", {
            {"auto me = static_cast<", generated_name, "*>(data);"},
            {"auto resource = wl_resource_create(client, &", wl_name, "_interface,"},
            {"                                   std::min(version, me->max_version), id);"},
            {"if (resource == nullptr)"},
            {"{"},
            {"    wl_client_post_no_memory(client);"},
            {"    BOOST_THROW_EXCEPTION((std::bad_alloc{}));"},
            {"}"},
        });
        if (has_vtable)
        {
            emit_indented_lines(out, indent + "    ",
                {{"wl_resource_set_implementation(resource, get_vtable(), me, nullptr);"}});
        }
        emit_indented_lines(out, indent + "    ", {
            {"try"},
            {"{"},
            {"  me->bind(client, resource);"},
            {"}"},
            {"catch(...)"},
            {"{"},
            {"    ::mir::log("},
            {"        ::mir::logging::Severity::critical,"},
            {"        \"frontend:Wayland\","},
            {"        std::current_exception(),"},
            {"        \"Exception processing ", generated_name, "::bind() request\");"},
            {"}"},
        });
        emit_indented_lines(out, indent, {
            {"}"}
        });
    }

    void emit_get_vtable(std::ostream& out, std::string const& indent)
    {
        emit_indented_lines(out, indent, {
            {"static inline struct " + wl_name + "_interface const* get_vtable()"},
            {"{"},
            {"    static struct " + wl_name + "_interface const vtable = {"},
        });
        for (auto const& method : methods)
        {
            auto emitter = method.emit_vtable_initialiser();
            emitter.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
            std::cout << ",\n";
        }
        emit_indented_lines(out, indent, {
            {"    };"},
            {"    return &vtable;"},
            {"}"},
        });
    }

    void emit_class()
    {
        auto& out = std::cout;
        out << "class " << generated_name << std::endl;
        out << "{" << std::endl;
        out << "protected:" << std::endl;

        emit_constructor(out, "    ", !methods.empty());
        if (is_global)
        {
            emit_indented_lines(out, "    ", {
                {"virtual ~", generated_name, "()"},
                {"{"},
                {"    wl_global_destroy(global);"},
                {"}"},
            });
        }
        else
        {
            emit_indented_lines(out, "    ", {
                {"virtual ~", generated_name, "() = default;"},
            });
        }
        out << '\n';

        if (is_global)
        {
            emit_indented_lines(out, "    ", {
                {"virtual void bind(struct wl_client* client, struct wl_resource* resource) { (void)client; (void)resource; }"}
            });
        }
        for (auto const& method : methods)
        {
            auto emitter = method.emit_virtual_prototype(is_global);
            emitter.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
            std::cout << "\n";
        }
        out << std::endl;

        if (is_global)
        {
            emit_indented_lines(out, "    ", {
                {"struct wl_global* const global;"},
                {"uint32_t const max_version;"},
            });
        }
        else
        {
            emit_indented_lines(out, "    ", {
                {"struct wl_client* const client;"},
                {"struct wl_resource* const resource;"}
            });
        }
        out << '\n';

        if (!methods.empty())
        {
            out << "private:" << std::endl;
        }

        for (auto const& method : methods)
        {
            auto emitter = method.emit_thunk(generated_name, is_global);
            emitter.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
            std::cout << "\n";
        }

        if (is_global)
        {
            emit_bind(out, "    ", !methods.empty());
            if (!methods.empty())
                out << '\n';
        }

        if (!methods.empty())
        {
            if (!is_global)
            {
                emit_indented_lines(out, "    ", {
                    {"static void resource_destroyed_thunk(wl_resource* resource)"},
                    {"{"},
                    {"    delete static_cast<", generated_name, "*>(wl_resource_get_user_data(resource));"},
                    {"}"}
                });
                out << '\n';
            }

            emit_get_vtable(out, "    ");
        }
        out << "};" << '\n';
    }

private:
    void emit_constructor_for_global(std::ostream& out, std::string const& indent)
    {
        emit_indented_lines(out, indent, {
            {generated_name, "(struct wl_display* display, uint32_t max_version)"},
            {"    : global{wl_global_create(display, &", wl_name, "_interface, max_version,"},
            {"                              this, &", generated_name, "::bind_thunk)},"},
            {"        max_version{max_version}"},
            {"{" },
            {"    if (global == nullptr)"},
            {"    {" },
            {"        BOOST_THROW_EXCEPTION((std::runtime_error{"},
            {"            \"Failed to export ", wl_name, " interface\"}));"},
            {"    }" },
            {"}"},
        });
    }

    void emit_constructor_for_regular(std::ostream& out, std::string const& indent, bool has_vtable)
    {
        emit_indented_lines(out, indent, {
            { generated_name, "(struct wl_client* client, struct wl_resource* parent, uint32_t id)" },
            { "    : client{client}," },
            { "      resource{wl_resource_create(client, &", wl_name, "_interface, wl_resource_get_version(parent), id)}" },
            { "{" }
        });
        emit_indented_lines(out, indent + "    ", {
            { "if (resource == nullptr)" },
            { "{" },
            { "    wl_resource_post_no_memory(parent);" },
            { "    BOOST_THROW_EXCEPTION((std::bad_alloc{}));" },
            { "}" },
        });
        if (has_vtable)
        {
            emit_indented_lines(out, indent + "    ",
                {{ "wl_resource_set_implementation(resource, get_vtable(), this, &resource_destroyed_thunk);" }});
        }
        emit_indented_lines(out, indent, {
            { "}" }
        });
    }

    std::string const wl_name;
    std::string const generated_name;
    bool const is_global;
    std::vector<Method> methods;
};

// arguments are:
//  0: binary
//  1: name prefix (such as wl_)
//  2: header to include (such as wayland-server.h)
//  3: input file path
int main(int argc, char** argv)
{
    if (argc != 4)
    {
        exit(1);
    }

    std::string const prefix{argv[1]};

    auto name_transform = [prefix](std::string protocol_name)
    {
        std::string transformed_name = protocol_name;
        if (protocol_name.find(prefix) == 0) // if the first instance of prefix is at the start of protocol_name
        {
            // cut off the prefix
            transformed_name = protocol_name.substr(prefix.length());
        }
        return camel_case_string(transformed_name);
    };

    std::string const input_file_path{argv[3]};
    xmlpp::DomParser parser(input_file_path);

    auto document = parser.get_document();

    auto root_node = document->get_root_node();

    auto constructor_nodes = root_node->find("//arg[@type='new_id']");
    std::unordered_set<std::string> constructible_interfaces;
    for (auto const node : constructor_nodes)
    {
        auto arg = dynamic_cast<xmlpp::Element const*>(node);
        constructible_interfaces.insert(arg->get_attribute_value("interface"));
    }

    auto emitter = emit_comment_header(input_file_path);
    emitter.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
    std::cout << "\n";

    std::cout << std::endl;

    std::string const include_guard_macro = macro_string("MIR_FRONTEND_WAYLAND_" + file_name_from_path(input_file_path) + "_WRAPPER");
    auto emitter0 = emit_include_guard_top(include_guard_macro);
    emitter0.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
    std::cout << "\n";

    std::cout << std::endl;

    std::string const custom_header{argv[2]};
    auto emitter1 = emit_required_headers(custom_header);
    emitter1.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
    std::cout << "\n";

    std::cout << std::endl;

    std::cout << "namespace mir" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "namespace frontend" << std::endl;
    std::cout << "{" << std::endl;
    std::cout << "namespace wayland" << std::endl;
    std::cout << "{" << std::endl;

    for (auto top_level : root_node->get_children("interface"))
    {
        auto interface = dynamic_cast<xmlpp::Element*>(top_level);

        if (interface->get_attribute_value("name") == "wl_display" ||
            interface->get_attribute_value("name") == "wl_registry")
        {
            // These are special, and don't need binding.
            continue;
        }
        Interface(*interface, name_transform, constructible_interfaces).emit_class();

        std::cout << std::endl << std::endl;
    }
    std::cout << "}" << std::endl;
    std::cout << "}" << std::endl;
    std::cout << "}" << std::endl;

    std::cout << std::endl;

    auto emitter2 = emit_include_guard_bottom(include_guard_macro);
    emitter2.emit({std::cout, std::make_shared<bool>(false), "\t\t"});
    std::cout << "\n";

    return 0;
}
