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

# Build type specific configurations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # Strip debug symbols
    target_link_options(dumper PRIVATE -s)

    # Optimization for size
    target_compile_options(dumper PRIVATE -Oz)

    # Remove unused sections
    target_compile_options(dumper PRIVATE -ffunction-sections -fdata-sections)
    target_link_options(dumper PRIVATE -Wl,--gc-sections)

    # Hide internal symbols
    target_compile_options(dumper PRIVATE -fvisibility=hidden)

    # Remove RPATH
    set_target_properties(dumper PROPERTIES
        BUILD_WITH_INSTALL_RPATH FALSE
        INSTALL_RPATH ""
    )
else()
    # Debug build: keep symbols, no optimization
    target_compile_options(dumper PRIVATE -O0 -g)
endif()
