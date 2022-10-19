# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_package(Libssh2 CONFIG QUIET)

if(TARGET Libssh2::libssh2)
  return()
endif()

message(STATUS "Libssh2 not found via CMake. Trying PkgConfig...")

find_package(PkgConfig REQUIRED)

pkg_check_modules(LIBSSH2 REQUIRED IMPORTED_TARGET libssh2)

add_library(Libssh2::libssh2 ALIAS PkgConfig::LIBSSH2)
