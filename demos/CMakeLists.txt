cmake_minimum_required(VERSION 3.0)

project(samples)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/third_party/imgui)

include_directories(${tinyrenders_include_dir})
include_directories(${CMAKE_SOURCE_DIR}/third_party/stb)
include_directories(${CMAKE_SOURCE_DIR}/third_party/tinyobjloader)
include_directories(${IMGUI_DIR})

include_directories(${tinyrenders_dir} ${tinyrenders_include_dir})
include_directories(${glfw_INCLUDE_DIRS})
link_libraries(glfw glm)

include_directories(${VULKAN_INCLUDE_DIR})
link_libraries(${VULKAN_LIBRARY})

function(add_vk sample_name)
  set(target_name "${sample_name}_VK")
  add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                ${tinyrenders_dir}/tinyvk.h
                                ${tinyrenders_include_dir}/camera.h
                                ${tinyrenders_include_dir}/cbuffer.h
                                ${tinyrenders_include_dir}/entity.h
                                ${tinyrenders_include_dir}/mesh.h
                                ${tinyrenders_include_dir}/transform.h
                                ${IMGUI_DIR}/imgui.h
                                ${IMGUI_DIR}/imgui.cpp
                                ${IMGUI_DIR}/imgui_draw.cpp)
  if(UNIX)
    target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK)
    target_compile_options(${target_name} PRIVATE -std=c++14)
    target_link_libraries(${target_name} PRIVATE X11-xcb)
  elseif(WIN32)
  	target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_VK -D_CRT_SECURE_NO_WARNINGS)
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
    set_target_properties(${target_name} PROPERTIES FOLDER "demos/vk")        
  endif()     
endfunction()

add_vk(ChessSet)
add_vk(TriangleTessellation)
add_vk(Shaders)
add_vk(BRDF)

if(WIN32)
  function(add_dx sample_name)
    set(target_name "${sample_name}_DX")
    add_executable(${target_name} ${CMAKE_CURRENT_SOURCE_DIR}/src/${sample_name}.cpp
                                  ${tinyrenders_dir}/tinydx.h
                                  ${tinyrenders_include_dir}/camera.h
                                  ${tinyrenders_include_dir}/cbuffer.h
                                  ${tinyrenders_include_dir}/entity.h
                                  ${tinyrenders_include_dir}/mesh.h
                                  ${tinyrenders_include_dir}/transform.h
                                  ${IMGUI_DIR}/imgui.h
                                  ${IMGUI_DIR}/imgui.cpp
                                  ${IMGUI_DIR}/imgui_draw.cpp)
    target_compile_definitions(${target_name} PRIVATE -DTINY_RENDERER_DX -D_CRT_SECURE_NO_WARNINGS)
    set_target_properties(${target_name} PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup /SUBSYSTEM:Windows /INCREMENTAL:NO")
    set_target_properties(${target_name} PROPERTIES FOLDER "demos/dx")
  endfunction()

  add_dx(ChessSet)
  add_dx(TriangleTessellation)
  add_dx(Shaders)
  add_dx(BRDF)
endif()
