cmake_minimum_required(VERSION 3.0)

project(samples)

include_directories(${tinyrenders_include_dir})
include_directories(${CMAKE_SOURCE_DIR}/third_party/stb)
include_directories(${CMAKE_SOURCE_DIR}/third_party/tinyobjloader)

include_directories(${glfw_INCLUDE_DIRS})
link_libraries(glfw glm)

include_directories(${VULKAN_INCLUDE_DIR})
link_libraries(${VULKAN_LIBRARY})

function(add_vk sample_name)
    set(target_name "${sample_name}_VK")
    add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                  ${CMAKE_SOURCE_DIR}/camera.h
                                  ${CMAKE_SOURCE_DIR}/cbuffer.h
                                  ${CMAKE_SOURCE_DIR}/entity.h
                                  ${CMAKE_SOURCE_DIR}/mesh.h
                                  ${CMAKE_SOURCE_DIR}/tinyvk.h
                                  ${CMAKE_SOURCE_DIR}/transform.h)
    if (GGP)
        target_compile_definitions(${target_name} PRIVATE __ggp__ _GNU_SOURCE GLFW_INCLUDE_NONE TINY_RENDERER_VK)
        target_compile_options(${target_name} PRIVATE -std=c++14)
        target_link_libraries(${target_name} PRIVATE m ggp vulkan)
	elseif(UNIX)
        target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK)
        target_compile_options(${target_name} PRIVATE -std=c++14)
        target_link_directories(${target_name} PRIVATE ${VULKAN_LIBRARY_DIR})
        target_link_libraries(${target_name} PRIVATE X11-xcb vulkan)
    elseif(WIN32)
        target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK -D_CRT_SECURE_NO_WARNINGS)
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
        set_target_properties(${target_name} PROPERTIES FOLDER "demos/vk")
        target_link_libraries(${target_name} PUBLIC "${VULKAN_LIBRARY_DIR}/vulkan-1.lib")
    endif()     
endfunction()

add_vk(ChessSet)
add_vk(TriangleTessellation)

if(WIN32)
    function(add_dx sample_name)
        set(target_name "${sample_name}_DX")
        add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                      ${CMAKE_SOURCE_DIR}/camera.h
                                      ${CMAKE_SOURCE_DIR}/cbuffer.h
									  ${CMAKE_SOURCE_DIR}/entity.h
                                      ${CMAKE_SOURCE_DIR}/mesh.h
                                      ${CMAKE_SOURCE_DIR}/tinydx.h
                                      ${CMAKE_SOURCE_DIR}/transform.h)
        target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_DX -D_CRT_SECURE_NO_WARNINGS)
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
        set_target_properties(${target_name} PROPERTIES FOLDER "demos/dx")
    endfunction()

    add_dx(ChessSet)
    add_dx(TriangleTessellation)    
endif()
