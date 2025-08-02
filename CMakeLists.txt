cmake_minimum_required(VERSION 3.16)
project(ShaderPlayground LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find Vulkan SDK
find_package(Vulkan REQUIRED)

# Qt6 Widgets
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# Enable CMake's Automoc feature for Qt
set(CMAKE_AUTOMOC ON)

# Define sources: private CPPs and public headers
file(GLOB_RECURSE CPP_SOURCES
        ${CMAKE_SOURCE_DIR}/src/*/Private/*.cpp
)
file(GLOB_RECURSE HEADER_FILES
        ${CMAKE_SOURCE_DIR}/src/*/Public/*.h
)

# Add the main.cpp file specifically
list(APPEND CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/Private/main.cpp)
list(APPEND CPP_SOURCES ${CMAKE_SOURCE_DIR}/src/VulkanCore/Private/MainWindow.cpp)

add_executable(app ${CPP_SOURCES} ${HEADER_FILES}
        src/VulkanCore/Public/QtVulkanWindow.h
        src/VulkanCore/Private/QtVulkanWindow.cpp
)

# Include public directories
target_include_directories(app PRIVATE
        ${Vulkan_INCLUDE_DIR}
        ${Qt6Widgets_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/src/FileWatcher/Public
        ${CMAKE_SOURCE_DIR}/src/ShaderLoader/Public
        ${CMAKE_SOURCE_DIR}/src/VulkanCore/Public
)

# Link libraries
target_link_libraries(app PRIVATE
        ${Vulkan_LIBRARIES}
        Qt6::Widgets
)

# Enable C++23 features
target_compile_features(app PRIVATE cxx_std_23)
