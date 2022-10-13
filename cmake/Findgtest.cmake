# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

find_package(gtest CONFIG QUIET)

if(TARGET gtest::gtest)
  return()
endif()

find_package(GTest REQUIRED)

add_library(gtest::gtest INTERFACE IMPORTED)
target_link_libraries(gtest::gtest INTERFACE
  GTest::gtest
  GTest::gmock)
