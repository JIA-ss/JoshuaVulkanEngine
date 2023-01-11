cmake_minimum_required (VERSION 3.20.0)

project("ImguiTest")

option(PLATFORM_WINDOWS "Windows Platform" OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

SET(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Lib)
SET(GUID_DIR ${LIB_DIR}/guid)
SET(IMGUI_DIR ${LIB_DIR}/imgui)
SET(VULKAN_DIR ${LIB_DIR}/Vulkan)

message("CMAKE_TOOLCHAIN_FILE ${CMAKE_TOOLCHAIN_FILE}")

# Vulkan Library
SET(VULKAN_LIBRARY ${VULKAN_DIR}/Lib/vulkan-1.lib
                ${VULKAN_DIR}/Lib/VkLayer_utils.lib)
SET(VulkanInclude ${VULKAN_DIR}/Include)

FILE(
    GLOB_RECURSE MAIN_SRC_FILES
    ${GUID_DIR}/*.h
    ${GUID_DIR}/*.cpp
    ${IMGUI_DIR}/*.h
    ${IMGUI_DIR}/*.cpp


    #source files
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/*.h
    ${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp
)

if (PLATFORM_WINDOWS)
message("PLATFORM_WINDOWS DEFINED")
add_definitions("-DGUID_WINDOWS")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Src)

find_package(glfw3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)

add_executable(ImguiTest ${MAIN_SRC_FILES})
target_link_libraries(ImguiTest 
    glfw
    glm::glm
    ${VULKAN_LIBRARY}
    unofficial::vulkan-memory-allocator::vulkan-memory-allocator
)
target_include_directories(ImguiTest
    PUBLIC ${LIB_DIR}
    PUBLIC ${IMGUI_DIR}
    PUBLIC ${VulkanInclude}
)