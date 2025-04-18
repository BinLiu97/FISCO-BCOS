cmake_minimum_required(VERSION 3.14)

project(bcos-executor VERSION ${VERSION})

find_package(TBB CONFIG REQUIRED)
find_package(wedprcrypto REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(evmc REQUIRED)
find_package(evmone REQUIRED)
find_package(intx REQUIRED)
find_package(ethash REQUIRED)
find_package(Boost REQUIRED serialization program_options context)
find_package(jsoncpp REQUIRED)

file(GLOB_RECURSE SRCS src/*.cpp)
add_library(${EXECUTOR_TARGET} ${SRCS})
if(WITH_WASM)
    target_link_libraries(${EXECUTOR_TARGET} PUBLIC jsoncpp_static ${LEDGER_TARGET} ${CODEC_TARGET} ${TOOL_TARGET} ${CRYPTO_TARGET} ${TABLE_TARGET} wedprcrypto::fisco-bcos bcos-protocol
            Boost::context evmone fbwasm evmc::loader evmc::instructions wabt GroupSig Paillier blst)
else()
    target_link_libraries(${EXECUTOR_TARGET} PUBLIC jsoncpp_static ${LEDGER_TARGET} ${CODEC_TARGET} ${TOOL_TARGET} ${CRYPTO_TARGET} ${TABLE_TARGET} wedprcrypto::fisco-bcos bcos-protocol
            Boost::context evmone evmc::loader evmc::instructions GroupSig Paillier blst)
endif()

set_source_files_properties(
    "src/vm/kzgPrecompiled.cpp"
    "src/vm/VMFactory.cpp"
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
set_target_properties(${EXECUTOR_TARGET} PROPERTIES UNITY_BUILD "ON")

if (TOOLS)
    add_subdirectory(tools)
endif()

if (TESTS)
    enable_testing()
    set(ENV{CTEST_OUTPUT_ON_FAILURE} True)
    add_subdirectory(test/unittest)
endif()
