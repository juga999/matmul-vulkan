
cmake_minimum_required(VERSION 3.16)

project(matmul_vulkan
    VERSION 1.0.0
    LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
find_program(GLSLC glslc REQUIRED)

# Build shared library
add_library(matmul_vulkan SHARED
    src/matrix_buffer.cpp
    src/matmul_context.cpp
    src/matmul_vulkan.cpp
)

add_custom_command(
    OUTPUT ${CMAKE_BINARY_DIR}/matmul.spv
    COMMAND ${GLSLC}
            ${CMAKE_SOURCE_DIR}/src/matmul.comp
            -O
            -o ${CMAKE_BINARY_DIR}/matmul.spv
    DEPENDS ${CMAKE_SOURCE_DIR}/src/matmul.comp
)

add_custom_target(
    shaders ALL
    DEPENDS ${CMAKE_BINARY_DIR}/matmul.spv
)

add_dependencies(matmul_vulkan shaders)

# Library properties
set_target_properties(matmul_vulkan PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    OUTPUT_NAME matmul_vulkan

    # Hide C++ implementation symbols by default
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(matmul_vulkan PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

target_include_directories(matmul_vulkan PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(matmul_vulkan PRIVATE Vulkan::Vulkan)

