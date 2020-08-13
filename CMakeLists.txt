cmake_minimum_required(VERSION 3.0)

project(tinyrenderers)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/lib")
set(CMAKE_PDB_OUTPUT_DIRECTORY     "${CMAKE_SOURCE_DIR}/lib")

if (GGP_TOOLCHAIN_PATH)
    set(GGP TRUE)
endif()

# Determine Vulkan's include directory
if (NOT VULKAN_INCLUDE_DIR)
    if (LINUX)
        if (GGP)
            set(VULKAN_INCLUDE_DIR "${GGP_SYSROOT_PATH}/usr/local/include")
        else()
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/x86_64/include")
            endif()
        endif()
    elseif (WIN32)
        if (DEFINED ENV{VULKAN_SDK})
            set(VULKAN_INCLUDE_DIR "$ENV{VULKAN_SDK}/Include")
        endif()
    endif()
endif()

# Determine Vulkan's library directory
if (NOT VULKAN_LIBRARY_DIR)
    if (LINUX)
        if (GGP)
            set(VULKAN_LIBRARY_DIR "${GGP_SYSROOT_PATH}/usr/local/lib")
        else()
            if (DEFINED ENV{VULKAN_SDK})
                set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/x86_64/lib")
            endif()
        endif()
    elseif (WIN32)
        if (DEFINED ENV{VULKAN_SDK})
            set(VULKAN_LIBRARY_DIR "$ENV{VULKAN_SDK}/Lib")
        endif()
    endif()
endif()

# Bail if Vulkan's include directory is not set
if (NOT VULKAN_INCLUDE_DIR)
    message(FATAL_ERROR "VULKAN_INCLUDE_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

# Bail if Vulkan's library directory is not set
if (NOT VULKAN_LIBRARY_DIR)
    message(FATAL_ERROR "VULKAN_LIBRARY_DIR not specified and could not be determined using environment variable VULKAN_SDK")
endif()

message("Found Vulkan: ${VULKAN_DIR}")
message("  Vulkan include directory: ${VULKAN_INCLUDE_DIR}")
message("  Vulkan library directory: ${VULKAN_LIBRARY_DIR}")

set(GLFW_BUILD_EXAMPLES FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    FALSE CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     FALSE CACHE BOOL "" FORCE)
set(GLFW_VULKAN_STATIC  FALSE CACHE BOOL "" FORCE)
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/glm)
add_subdirectory(third_party/tinyobjloader)

set(tinyrenders_include_dir "${CMAKE_SOURCE_DIR}")
add_subdirectory(samples)
add_subdirectory(demos)