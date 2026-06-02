cmake_minimum_required(VERSION 3.22.1)
project("dumper")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(dumper SHARED
    jni_bridge.cpp
    core/binary_stream.cpp
    core/metadata.cpp
    core/il2cpp.cpp
    core/elf.cpp
    core/section_helper.cpp
    core/il2cpp_executor.cpp
    core/il2cpp_decompiler.cpp
    core/struct_generator.cpp
    core/config.cpp
    #core/boyer_moore.cpp
    core/custom_attribute_reader.cpp
)

target_link_libraries(dumper android log)
target_include_directories(dumper PRIVATE ${CMAKE_SOURCE_DIR}/core)
