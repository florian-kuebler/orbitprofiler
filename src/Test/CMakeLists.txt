# Copyright (c) 2021 The Orbit Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cmake_minimum_required(VERSION 3.15)

add_library(Test_Main OBJECT EXCLUDE_FROM_ALL test_main.cpp Path.cpp)
target_include_directories(Test_Main PUBLIC include/)
target_link_libraries(Test_Main PUBLIC OrbitBase GTest::GTest)
add_library(Test::Main ALIAS Test_Main)

if(TARGET Qt5::Core)
  add_library(Test_QtCoreMain OBJECT EXCLUDE_FROM_ALL test_qtcore_main.cpp Path.cpp)
  target_include_directories(Test_QtCoreMain PUBLIC include/)
  target_link_libraries(Test_QtCoreMain PUBLIC OrbitBase GTest::GTest Qt5::Core)
  add_library(Test::QtCoreMain ALIAS Test_QtCoreMain)
endif()

if(TARGET Qt5::Widgets)
  add_library(Test_QtGuiMain OBJECT EXCLUDE_FROM_ALL test_qtgui_main.cpp Path.cpp)
  target_include_directories(Test_QtGuiMain PUBLIC include/)
  target_link_libraries(Test_QtGuiMain PUBLIC OrbitBase GTest::GTest Qt5::Widgets absl::strings)
  add_library(Test::QtGuiMain ALIAS Test_QtGuiMain)
endif()

add_executable(TestTests StringViewTest.cpp)
target_link_libraries(TestTests PRIVATE Test::Main)
target_compile_options(TestTests PRIVATE ${STRICT_COMPILE_FLAGS})
register_test(TestTests)