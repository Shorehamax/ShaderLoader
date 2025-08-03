cmake_minimum_required(VERSION 3.16)
project(ShaderLoader)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Main application
add_executable(app src/Private/main_triangle_fixed.cpp)

# Link libraries to the main application
target_link_libraries(app
    Vulkan::Vulkan
    glfw
)

# Set output directory
set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
