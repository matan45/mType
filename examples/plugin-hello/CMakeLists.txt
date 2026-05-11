# Standalone build for the mType native-plugin "hello" example (MYT-289).
# This project is intentionally independent of the main mType build — it only
# needs the public C ABI header at mType/plugin/PluginHostApi.h.

cmake_minimum_required(VERSION 3.15)
project(hello_plugin C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(hello SHARED hello.cpp)

target_include_directories(hello PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../mType/plugin
)

# Output a flat name on every platform so the .mt script can use the same
# literal across hosts (hello.dll on Windows, hello.so on Linux, hello.dylib
# on macOS — the consumer picks the right one).
set_target_properties(hello PROPERTIES
    PREFIX ""
    OUTPUT_NAME "hello"
)

if(UNIX AND NOT APPLE)
    set_target_properties(hello PROPERTIES SUFFIX ".so")
endif()
