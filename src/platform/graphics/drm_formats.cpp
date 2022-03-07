/*
 * Copyright © 2022 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "mir/graphics/drm_formats.h"

#include <cstdint>
#include <xf86drm.h>
#include <drm_fourcc.h>
#include <unordered_map>
#include <stdexcept>

namespace mg = mir::graphics;

namespace
{
std::array const format_name_map = {
#define STRINGIFY(val) \
    std::pair<uint32_t, char const*>{val, #val},

#include "drm-formats"

#undef STRINGIFY
};

constexpr auto drm_format_to_string(uint32_t format) -> char const*
{
#define STRINGIFY(val) \
    case val:          \
        return #val;

    if (!(format & DRM_FORMAT_BIG_ENDIAN))
    {
        switch (format)
        {
#include "drm-formats"

            default:
                return "Unknown DRM format; rebuild Mir against newer DRM headers?";
        }

    }
#undef STRINGIFY

#define STRINGIFY_BIG_ENDIAN(val) \
    case val:                    \
        return #val " (big endian)";

    switch (format & (~DRM_FORMAT_BIG_ENDIAN))
    {
#include "drm-formats-big-endian"

        default:
            return "Unknown DRM format; rebuild Mir against newer DRM headers?";
    }
#undef STRINGIFY_BIGENDIAN
}

struct RGBComponentInfo
{
    uint32_t red_bits;
    uint32_t green_bits;
    uint32_t blue_bits;
    std::optional<uint32_t> alpha_bits;
};

struct FormatInfo
{
    uint32_t format;
    bool has_alpha;
    uint32_t opaque_equivalent;
    uint32_t alpha_equivalent;
    std::optional<RGBComponentInfo> components;
};

constexpr bool is_component_name(char c)
{
    return c == 'R' || c == 'G' || c == 'B' ||
           c == 'Y' || c == 'U' || c == 'V' ||
           c == 'A' || c == 'X' || c == 'C';
}

constexpr std::optional<size_t> component_index(char component, char const* const specifier)
{
    for (int i = 0; specifier[i]; ++i)
    {
        if (specifier[i] == component)
        {
            return i;
        }
    }
    return {};
}

constexpr RGBComponentInfo parse_rgb_component_sizes(
    int red_index,
    int green_index,
    int blue_index,
    std::optional<int> alpha_index,
    char const* component_specifier)
{
    RGBComponentInfo info;

    // component is of the form [component list][digits]
    char const* component = component_specifier;
    while(is_component_name(*component))
    {
        ++component;
    }
    // component is now just [digits]
    int component_num = 0;
    while (*component)
    {
        uint32_t parsed_count = *component - '0';
        if (parsed_count == 1 && *(component + 1))
        {
            /* For RGB formats there are two cases where we can find a '1':
             * - In 1555 / 5551 formats, where the 1 *is* the size
             * - In formats with 10- or 16-bit channels, where we have a second digit
             */
            switch (*(component + 1))
            {
                case '5':
                    break;
                case '0':
                case '6':
                    parsed_count *= 10;
                    ++component;
                    parsed_count += *component - '0';
                    break;
                default:
                    throw std::logic_error{"Expected channel size >= 10 to be either 10 or 16"};
            }
        }

        if (red_index == component_num)
        {
            info.red_bits = parsed_count;
        }
        else if (green_index == component_num)
        {
            info.green_bits = parsed_count;
        }
        else if (blue_index == component_num)
        {
            info.blue_bits = parsed_count;
        }
        else if (alpha_index == component_num)
        {
            info.alpha_bits = parsed_count;
        }
    }

    return info;
}

constexpr FormatInfo info_from_specifier(char const* const component_specifier)
{
    FormatInfo info;

    std::optional<int> const red_index = component_index('R', component_specifier);
    std::optional<int> const green_index = component_index('G', component_specifier);
    std::optional<int> const blue_index = component_index('B', component_specifier);
    std::optional<int> const alpha_index = component_index('A', component_specifier);

    // Sanity check: we have either RGB or YUV, but not both
    if ((red_index || green_index || blue_index))
    {
        if (component_index('Y', component_specifier) ||
            component_index('U', component_specifier) ||
            component_index('V', component_specifier))
        {
            throw std::logic_error{"Invalid specifier passed to info_from_specifier"};
        }
    }

    // There are some DRM formats that don't include all colour components, but they're not sensible
    // for display.
    if (red_index && green_index && blue_index)
    {
        info.components = parse_rgb_component_sizes(
            *red_index,
            *green_index,
            *blue_index,
            alpha_index,
            component_specifier);
    }

    return info;
}

std::unordered_map<uint32_t, FormatInfo const> const formats = {
    {
        DRM_FORMAT_ARGB8888,
        {
            DRM_FORMAT_ARGB8888,
            true,
            DRM_FORMAT_XRGB8888,
            DRM_FORMAT_ARGB8888,
        }
    },
    {
        DRM_FORMAT_XRGB4444,
        {
            DRM_FORMAT_XRGB4444,
            false,
            DRM_FORMAT_XRGB4444,
            DRM_FORMAT_ARGB4444,
        }
    },
    {
        DRM_FORMAT_XBGR4444,
        {
            DRM_FORMAT_XBGR4444,
            false,
            DRM_FORMAT_XBGR4444,
            DRM_FORMAT_ABGR4444,
        }
    },
    {
        DRM_FORMAT_RGBX4444,
        {
            DRM_FORMAT_RGBX4444,
            false,
            DRM_FORMAT_RGBX4444,
            DRM_FORMAT_RGBA4444,
        }
    },
    {
        DRM_FORMAT_BGRX4444,
        {
            DRM_FORMAT_BGRX4444,
            false,
            DRM_FORMAT_BGRX4444,
            DRM_FORMAT_BGRA4444,
        }
    },
    {
        DRM_FORMAT_ARGB4444,
        {
            DRM_FORMAT_ARGB4444,
            true,
            DRM_FORMAT_XRGB4444,
            DRM_FORMAT_ARGB4444,
        }
    },
    {
        DRM_FORMAT_ABGR4444,
        {
            DRM_FORMAT_ABGR4444,
            true,
            DRM_FORMAT_XBGR4444,
            DRM_FORMAT_ABGR4444,
        }
    },
    {
        DRM_FORMAT_RGBA4444,
        {
            DRM_FORMAT_RGBA4444,
            true,
            DRM_FORMAT_RGBX4444,
            DRM_FORMAT_RGBA4444,
        }
    },
    {
        DRM_FORMAT_BGRA4444,
        {
            DRM_FORMAT_BGRA4444,
            true,
            DRM_FORMAT_BGRX4444,
            DRM_FORMAT_BGRA4444,
        }
    },
    {
        DRM_FORMAT_XRGB1555,
        {
            DRM_FORMAT_XRGB1555,
            false,
            DRM_FORMAT_XRGB1555,
            DRM_FORMAT_ARGB1555,
        }
    },
    {
        DRM_FORMAT_XBGR1555,
        {
            DRM_FORMAT_XBGR1555,
            false,
            DRM_FORMAT_XBGR1555,
            DRM_FORMAT_ABGR1555,
        }
    },
    {
        DRM_FORMAT_RGBX5551,
        {
            DRM_FORMAT_RGBX5551,
            false,
            DRM_FORMAT_RGBX5551,
            DRM_FORMAT_RGBA5551,
        }
    },
    {
        DRM_FORMAT_BGRX5551,
        {
            DRM_FORMAT_BGRX5551,
            true,
            DRM_FORMAT_BGRX5551,
            DRM_FORMAT_BGRA5551,
        }
    },
    {
        DRM_FORMAT_ARGB1555,
        {
            DRM_FORMAT_ARGB1555,
            true,
            DRM_FORMAT_XRGB1555,
            DRM_FORMAT_ARGB1555,
        }
    },
    {
        DRM_FORMAT_ABGR1555,
        {
            DRM_FORMAT_ABGR1555,
            true,
            DRM_FORMAT_XBGR1555,
            DRM_FORMAT_ABGR1555,
        }
    },
    {
        DRM_FORMAT_RGBA5551,
        {
            DRM_FORMAT_RGBA5551,
            true,
            DRM_FORMAT_RGBX5551,
            DRM_FORMAT_RGBA5551,
        }
    },
    {
        DRM_FORMAT_BGRA5551,
        {
            DRM_FORMAT_BGRA5551,
            true,
            DRM_FORMAT_BGRX5551,
            DRM_FORMAT_BGRA5551,
        }
    },
    {
        DRM_FORMAT_RGB565,
        {
            DRM_FORMAT_RGB565,
            false,
            DRM_FORMAT_RGB565,
            DRM_FORMAT_INVALID,
        }
    },
    {
        DRM_FORMAT_BGR565,
        {
            DRM_FORMAT_BGR565,
            false,
            DRM_FORMAT_BGR565,
            DRM_FORMAT_INVALID,
        }
    },
    {
        DRM_FORMAT_RGB888,
        {
            DRM_FORMAT_RGB888,
            false,
            DRM_FORMAT_RGB888,
            DRM_FORMAT_INVALID,
        }
    },
    {
        DRM_FORMAT_BGR888,
        {
            DRM_FORMAT_BGR888,
            false,
            DRM_FORMAT_BGR888,
            DRM_FORMAT_INVALID,
        }
    },

p RGB */
DRM_FORMAT_XRGB8888 fourcc_code('X', 'R', '2', '4') /* [31:0] x:R:G:B 8:8:8:8 little endian */
DRM_FORMAT_XBGR8888 fourcc_code('X', 'B', '2', '4') /* [31:0] x:B:G:R 8:8:8:8 little endian */
DRM_FORMAT_RGBX8888 fourcc_code('R', 'X', '2', '4') /* [31:0] R:G:B:x 8:8:8:8 little endian */
DRM_FORMAT_BGRX8888 fourcc_code('B', 'X', '2', '4') /* [31:0] B:G:R:x 8:8:8:8 little endian */

DRM_FORMAT_ARGB8888 fourcc_code('A', 'R', '2', '4') /* [31:0] A:R:G:B 8:8:8:8 little endian */
DRM_FORMAT_ABGR8888 fourcc_code('A', 'B', '2', '4') /* [31:0] A:B:G:R 8:8:8:8 little endian */
DRM_FORMAT_RGBA8888 fourcc_code('R', 'A', '2', '4') /* [31:0] R:G:B:A 8:8:8:8 little endian */
DRM_FORMAT_BGRA8888 fourcc_code('B', 'A', '2', '4') /* [31:0] B:G:R:A 8:8:8:8 little endian */

DRM_FORMAT_XRGB2101010  fourcc_code('X', 'R', '3', '0') /* [31:0] x:R:G:B 2:10:10:10 little endian */
DRM_FORMAT_XBGR2101010  fourcc_code('X', 'B', '3', '0') /* [31:0] x:B:G:R 2:10:10:10 little endian */
DRM_FORMAT_RGBX1010102  fourcc_code('R', 'X', '3', '0') /* [31:0] R:G:B:x 10:10:10:2 little endian */
DRM_FORMAT_BGRX1010102  fourcc_code('B', 'X', '3', '0') /* [31:0] B:G:R:x 10:10:10:2 little endian */

DRM_FORMAT_ARGB2101010  fourcc_code('A', 'R', '3', '0') /* [31:0] A:R:G:B 2:10:10:10 little endian */
DRM_FORMAT_ABGR2101010  fourcc_code('A', 'B', '3', '0') /* [31:0] A:B:G:R 2:10:10:10 little endian */
DRM_FORMAT_RGBA1010102  fourcc_code('R', 'A', '3', '0') /* [31:0] R:G:B:A 10:10:10:2 little endian */
DRM_FORMAT_BGRA1010102  fourcc_code('B', 'A', '3', '0') /* [31:0] B:G:R:A 10:10:10:2 little endian */
i
}

};

constexpr mg::DRMFormat::DRMFormat(uint32_t fourcc_format)
    : format{fourcc_format}
{
}

auto mg::DRMFormat::name() const -> const char*
{
    return drm_format_to_string(format);
}



auto mg::drm_modifier_to_string(uint64_t modifier) -> std::string
{
#ifdef HAVE_DRM_GET_MODIFIER_NAME
    struct CStrDeleter
    {
    public:
        void operator()(char* c_str)
        {
            if (c_str)
            {
                free (c_str);
            }
        }
    };

    auto const vendor =
        [&]() -> std::string
        {
            std::unique_ptr<char[], CStrDeleter> vendor{drmGetFormatModifierVendor(modifier)};
            if (vendor)
            {
                return vendor.get();
            }
            return "(UNKNOWN VENDOR)";
        }();

    auto const name =
        [&]() -> std::string
        {
            std::unique_ptr<char[], CStrDeleter> name{drmGetFormatModifierName(modifier)};
            if (name)
            {
                return name.get();
            }
            return "(UNKNOWN MODIFIER)";
        }();

    return vendor + ":" + name;
}
#else
#define STRINGIFY(val) \
    case val:          \
        return #val;

    switch (modifier)
    {
#ifdef DRM_FORMAT_MOD_INVALID
        STRINGIFY(DRM_FORMAT_MOD_INVALID)
#endif
#ifdef DRM_FORMAT_MOD_LINEAR
        STRINGIFY(DRM_FORMAT_MOD_LINEAR)
#endif
#ifdef I915_FORMAT_MOD_X_TILED
        STRINGIFY(I915_FORMAT_MOD_X_TILED)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED
        STRINGIFY(I915_FORMAT_MOD_Y_TILED)
#endif
#ifdef I915_FORMAT_MOD_Yf_TILED
        STRINGIFY(I915_FORMAT_MOD_Yf_TILED)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_CCS)
#endif
#ifdef I915_FORMAT_MOD_Yf_TILED_CCS
        STRINGIFY(I915_FORMAT_MOD_Yf_TILED_CCS)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_GEN12_RC_CCS)
#endif
#ifdef I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS
        STRINGIFY(I915_FORMAT_MOD_Y_TILED_GEN12_MC_CCS)
#endif
#ifdef DRM_FORMAT_MOD_SAMSUNG_64_32_TILE
        STRINGIFY(DRM_FORMAT_MOD_SAMSUNG_64_32_TILE)
#endif
#ifdef DRM_FORMAT_MOD_SAMSUNG_16_16_TILE
        STRINGIFY(DRM_FORMAT_MOD_SAMSUNG_16_16_TILE)
#endif
#ifdef DRM_FORMAT_MOD_QCOM_COMPRESSED
        STRINGIFY(DRM_FORMAT_MOD_QCOM_COMPRESSED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SUPER_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SUPER_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SPLIT_TILED)
#endif
#ifdef DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED
        STRINGIFY(DRM_FORMAT_MOD_VIVANTE_SPLIT_SUPER_TILED)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_TEGRA_TILED)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_ONE_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_ONE_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_TWO_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_TWO_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_FOUR_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_FOUR_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_EIGHT_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_EIGHT_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_SIXTEEN_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_SIXTEEN_GOB)
#endif
#ifdef DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_THIRTYTWO_GOB
        STRINGIFY(DRM_FORMAT_MOD_NVIDIA_16BX2_BLOCK_THIRTYTWO_GOB)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_VC4_T_TILED)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND32
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND32)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND64
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND64)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND128
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND128)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_SAND256
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_SAND256)
#endif
#ifdef DRM_FORMAT_MOD_BROADCOM_UIF
        STRINGIFY(DRM_FORMAT_MOD_BROADCOM_UIF)
#endif
#ifdef DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED
        STRINGIFY(DRM_FORMAT_MOD_ARM_16X16_BLOCK_U_INTERLEAVED)
#endif
#ifdef DRM_FORMAT_MOD_ALLWINNER_TILED
        STRINGIFY(DRM_FORMAT_MOD_ALLWINNER_TILED)
#endif

        default:
            return "(unknown)";
    }

#undef STRINGIFY
}
#endif

