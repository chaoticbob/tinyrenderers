cmake_minimum_required(VERSION 3.0)

project(samples)

include_directories(${tinyrenders_include_dir})
include_directories(${CMAKE_SOURCE_DIR}/third_party/stb)


include_directories(${glfw_INCLUDE_DIRS})
link_libraries(glfw)

include_directories(${VULKAN_INCLUDE_DIR})
link_libraries(${VULKAN_LIBRARY})

function(add_vk sample_name)
    set(target_name "${sample_name}_VK")
    add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                  ${CMAKE_CURRENT_SOURCE_DIR}/assets/${sample_name}.hlsl
                                  ${CMAKE_SOURCE_DIR}/tinyvk.h)
    set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/assets/${sample_name}.hlsl
                                PROPERTIES HEADER_FILE_ONLY TRUE)
    if(UNIX)
		target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK)
        target_link_libraries(${target_name} PRIVATE X11-xcb)
    elseif(WIN32)
		target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK -D_CRT_SECURE_NO_WARNINGS)
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
        set_target_properties(${target_name} PROPERTIES FOLDER "hlsl-shader-tracking/vk")
    endif()
endfunction()

add_vk(structs_01)
add_vk(structs_02)
add_vk(structs_03)
add_vk(structs_04)
add_vk(opvariable_init)

if(WIN32)
    function(add_dx sample_name)
        set(target_name "${sample_name}_DX")
        add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                      ${CMAKE_CURRENT_SOURCE_DIR}/assets/${sample_name}.hlsl
                                      ${CMAKE_SOURCE_DIR}/tinydx.h)
        set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/assets/${sample_name}.hlsl
                                    PROPERTIES HEADER_FILE_ONLY TRUE)
        target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_DX -D_CRT_SECURE_NO_WARNINGS)
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
        set_target_properties(${target_name} PROPERTIES FOLDER "hlsl-shader-tracking/dx")
    endfunction()

    add_dx(structs_01)
    add_dx(structs_02)
    add_dx(structs_03)
    add_dx(structs_04)
	add_dx(opvariable_init)
endif()
