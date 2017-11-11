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
                                  ${CMAKE_SOURCE_DIR}/mesh.h    
                                  ${CMAKE_SOURCE_DIR}/tinyvk.h)
    target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK)
    if(UNIX)
        target_link_libraries(${target_name} PRIVATE X11-xcb)
    elseif(WIN32)
        set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
        set_target_properties(${target_name} PROPERTIES FOLDER "show-and-tell/vk")        
    endif()     
endfunction()

add_vk(ChessSet)

if(WIN32)
    function(add_dx sample_name)
        set(target_name "${sample_name}_DX")
        add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                      ${CMAKE_SOURCE_DIR}/mesh.h
                                      ${CMAKE_SOURCE_DIR}/tinydx.h)
        target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_DX)
            set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
            set_target_properties(${target_name} PROPERTIES FOLDER "show-and-tell/dx")
    endfunction()

    add_dx(ChessSet)
endif()
