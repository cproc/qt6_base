# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Find the XKB_COMMON_X11 library

# Will make the target PkgConfig::XKB_COMMON_X11 available when found.
if(TARGET PkgConfig::XKB_COMMON_X11)
    set(XKB_COMMON_X11_FOUND TRUE)
    return()
endif()

find_library(XKB_COMMON_X11_LIBRARY NAMES "xkbcommon-x11")
find_path(XKB_COMMON_X11_INCLUDE_DIR NAMES "xkbcommon/xkbcommon-x11.h" DOC "The XKB_COMMON_X11 Include path")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XKB_COMMON_X11 DEFAULT_MSG XKB_COMMON_X11_INCLUDE_DIR XKB_COMMON_X11_LIBRARY)

mark_as_advanced(XKB_COMMON_X11_INCLUDE_DIR XKB_COMMON_X11_LIBRARY)

if(XKB_COMMON_X11_FOUND)
    add_library(PkgConfig::XKB_COMMON_X11 INTERFACE IMPORTED)
    target_link_libraries(PkgConfig::XKB_COMMON_X11 INTERFACE "${XKB_COMMON_X11_LIBRARY}")
    target_include_directories(PkgConfig::XKB_COMMON_X11 INTERFACE "${XKB_COMMON_X11_INCLUDE_DIR}")
endif()
