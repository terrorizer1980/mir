add_library(mirudev OBJECT
  udev_wrapper.cpp
)

target_link_libraries(
  mirudev

  PUBLIC
    PkgConfig::UDEV
)
