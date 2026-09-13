# ch30-fuzzing-2/structured/CMakeLists.txt
#
# Wires the FuzzedDataProvider harness (fdp_harness.cc + target.c) and the
# libprotobuf-mutator harness (lpm_harness.cc + parse_config.c) against the
# clang-18 fuzzer runtime shipped on Ubuntu 24.04.
#
# Usage:
#     mkdir -p build && cd build
#     CC=clang-18 CXX=clang++-18 cmake ..
#     cmake --build . -j
#     ./fdp_fuzz -runs=100000
#     ./lpm_fuzz -runs=100000
#
# Mitigation flags — this directory intentionally compiles targets with:
#   -fsanitize=fuzzer,address,undefined   fuzzer runtime + ASan/UBSan oracles
#   -g -O1                                 keep frames + inlining sane for triage
#   -fno-omit-frame-pointer                for readable ASan stacks
#   -fstack-protector-strong               ON on the target as a safety net (we
#                                          still want the fuzzer to find the bug
#                                          via the ASan redzone, not just SSP)
# and does NOT pass -D_FORTIFY_SOURCE=3, because we want the raw memcpy to
# fire the ASan redzone instead of being caught by FORTIFY at compile time.

cmake_minimum_required(VERSION 3.20)
project(ch30_structured LANGUAGES C CXX)

set(CMAKE_C_STANDARD   11)
set(CMAKE_CXX_STANDARD 17)

if(NOT CMAKE_C_COMPILER_ID STREQUAL "Clang")
    message(FATAL_ERROR "libFuzzer harnesses require clang; got ${CMAKE_C_COMPILER_ID}")
endif()

set(FUZZ_FLAGS
    -g -O1
    -fno-omit-frame-pointer
    -fstack-protector-strong
    -fsanitize=fuzzer,address,undefined)

# --- FDP harness -------------------------------------------------------------
add_executable(fdp_fuzz fdp_harness.cc target.c)
target_compile_options(fdp_fuzz PRIVATE ${FUZZ_FLAGS})
target_link_options   (fdp_fuzz PRIVATE ${FUZZ_FLAGS})

# --- LPM harness -------------------------------------------------------------
# Requires:
#   apt install protobuf-compiler libprotobuf-dev libprotobuf-mutator-dev
find_program(PROTOC protoc)
find_library(PROTOBUF          protobuf)
find_library(PROTOBUF_MUTATOR  protobuf-mutator)
find_library(PROTOBUF_MUTATOR_LF protobuf-mutator-libfuzzer)

if(PROTOC AND PROTOBUF AND PROTOBUF_MUTATOR AND PROTOBUF_MUTATOR_LF)
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/config.pb.cc
                ${CMAKE_CURRENT_BINARY_DIR}/config.pb.h
        COMMAND ${PROTOC} --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
                          -I${CMAKE_CURRENT_SOURCE_DIR}
                          ${CMAKE_CURRENT_SOURCE_DIR}/config.proto
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/config.proto)

    add_executable(lpm_fuzz
        lpm_harness.cc parse_config.c
        ${CMAKE_CURRENT_BINARY_DIR}/config.pb.cc)
    target_include_directories(lpm_fuzz PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        /usr/include)
    target_compile_options(lpm_fuzz PRIVATE ${FUZZ_FLAGS})
    target_link_options   (lpm_fuzz PRIVATE ${FUZZ_FLAGS})
    target_link_libraries (lpm_fuzz PRIVATE
        ${PROTOBUF_MUTATOR_LF} ${PROTOBUF_MUTATOR} ${PROTOBUF})
else()
    message(WARNING "libprotobuf-mutator toolchain not found; skipping lpm_fuzz")
endif()
