#------------------------------------------------------------------------------
# Top-level CMake file for ut of bcos-tars-protocol
# ------------------------------------------------------------------------------
# Copyright (C) 2021 FISCO BCOS.
# SPDX-License-Identifier: Apache-2.0
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# ------------------------------------------------------------------------------
file(GLOB_RECURSE SOURCES "*.cpp")

# cmake settings
set(TEST_BINARY_NAME test-bcos-tars-protocol)

add_executable(${TEST_BINARY_NAME} ${SOURCES})
target_include_directories(${TEST_BINARY_NAME} PRIVATE . ${CMAKE_SOURCE_DIR})

find_package(Boost REQUIRED unit_test_framework)
find_package(jsoncpp REQUIRED)

target_compile_options(${TEST_BINARY_NAME} PRIVATE -Wno-unused-variable)
target_link_libraries(${TEST_BINARY_NAME} PRIVATE ${TARS_PROTOCOL_TARGET} ${CRYPTO_TARGET} Boost::unit_test_framework jsoncpp_static)
set_source_files_properties("main.cpp" PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
set_target_properties(${TEST_BINARY_NAME} PROPERTIES UNITY_BUILD "ON")
add_test(NAME test-tars-protocol WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMAND ${TEST_BINARY_NAME})
