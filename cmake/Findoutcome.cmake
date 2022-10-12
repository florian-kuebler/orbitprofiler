# Copyright (c) 2022 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

add_library(outcome INTERFACE IMPORTED)
target_include_directories(outcome SYSTEM INTERFACE ${CMAKE_SOURCE_DIR}/third_party/Outcome/include)
add_library(outcome::outcome ALIAS outcome)

