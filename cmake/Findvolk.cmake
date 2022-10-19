# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_package(volk CONFIG QUIET)

if(TARGET volk::volk)
  return()
endif()

message(STATUS "volk not found via CMake. Trying PkgConfig...")

find_path(VOLK_INC_DIR NAMES volk.h REQUIRED)

add_library(volk STATIC ${VOLK_INC_DIR}/volk.c)
target_include_directories(volk INTERFACE ${VULKAN_INC_DIR})
target_link_libraries(volk PRIVATE ${CMAKE_DL_LIBS})

add_library(volk::volk ALIAS volk)
