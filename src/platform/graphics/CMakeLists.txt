add_library(mirplatformgraphicscommon OBJECT

  egl_extensions.cpp
  egl_resources.cpp
  egl_error.cpp
  display_configuration.cpp
  gamma_curves.cpp
  buffer_basic.cpp
  pixel_format_utils.cpp
  ${PROJECT_SOURCE_DIR}/src/include/platform/mir/graphics/pixel_format_utils.h
  overlapping_output_grouping.cpp
  atomic_frame.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/display.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/texture.h
  texture.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program.h
  program.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/program_factory.h
  program_factory.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_wayland_allocator.h
  egl_wayland_allocator.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/renderer/sw/pixel_source.h
  cpu_buffers.cpp
  egl_logger.cpp
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/egl_logger.h
  ${PROJECT_SOURCE_DIR}/include/platform/mir/graphics/dmabuf_buffer.h
)

target_include_directories(
  mirplatformgraphicscommon

  PUBLIC
    ${CMAKE_CURRENT_BINARY_DIR}
  PRIVATE
    ${DRM_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/include/renderers/gl
)

target_link_libraries(
  mirplatformgraphicscommon

  PUBLIC
    mirwayland
    PkgConfig::EGL
    PkgConfig::GLESv2
)

set(symbol_map ${CMAKE_CURRENT_SOURCE_DIR}/symbols.map)
