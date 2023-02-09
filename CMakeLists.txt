cmake_minimum_required (VERSION 3.20.0)

project("VulkanRHI")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PLATFORM_WINDOWS "Windows Platform" OFF)


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
find_package(Boost REQUIRED COMPONENTS system filesystem)
find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")
find_package(tinyobjloader CONFIG REQUIRED)

add_executable(VulkanRHI ${MAIN_SRC_FILES})

target_compile_definitions(VulkanRHI PRIVATE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1)

target_link_libraries(VulkanRHI
    glfw
    glm::glm
    ${VULKAN_LIBRARY}
    unofficial::vulkan-memory-allocator::vulkan-memory-allocator
    ${Boost_LIBRARIES}
    tinyobjloader::tinyobjloader
)


target_include_directories(VulkanRHI
    PUBLIC ${LIB_DIR}
    PUBLIC ${IMGUI_DIR}
    PUBLIC ${VulkanInclude}
    PUBLIC ${Boost_INCLUDE_DIR}
    PUBLIC ${STB_INCLUDE_DIRS}
)


# find_program(GLSLC_PROGRAM glslc REQUIRED)
# set(GLSL_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources/Shader/GLSL)
# message("[GLSLC] Compile Files In ${GLSL_SHADER_DIR} Begin")
# file(GLOB GLSL_SHADERS ${GLSL_SHADER_DIR}/*.vert ${GLSL_SHADER_DIR}/*.frag)
# foreach(GLSL_SHADER ${GLSL_SHADERS})
#     get_filename_component(SHADER_NAME ${GLSL_SHADER} NAME)
#     set(GLSL_SHADER_SPIRV_PATH ${GLSL_SHADER_DIR}/SPIR-V/${SHADER_NAME}.spv)
#     execute_process(COMMAND ${GLSLC_PROGRAM} ${GLSL_SHADER} -o ${GLSL_SHADER_SPIRV_PATH})
#     message("glsl compiling ${GLSL_SHADER} to ${GLSL_SHADER_SPIRV_PATH}")
# endforeach()
# message("[GLSLC] Compile Files In ${GLSL_SHADER_DIR} Finish")

message("CMAKE_CXX_STANDARD ${CMAKE_CXX_STANDARD} ")