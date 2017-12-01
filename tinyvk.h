/*
 Copyright 2017 Google Inc.
 
 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at
 
 http://www.apache.org/licenses/LICENSE-2.0
 
 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.


 Copyright (c) 2017, The Cinder Project, All rights reserved.

 This code is intended for use with the Cinder C++ library: http://libcinder.org

 Redistribution and use in source and binary forms, with or without modification, are permitted provided that
 the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, this list of conditions and
    the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
    the following disclaimer in the documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
*/

/*

NOTES:
 - Microsoft's C compiler doesn't support certain C11/C99 features, such as VLAs
 - tinyvk/tinydx is written for experimentation and fun-having - not performance
 - For simplicity, only one descriptor set can be bound at once
   - In D3D12, this means two descriptor heaps (CBVSRVUAVs and samplers)
   - For Vulkan shaders the 'set' parameter for 'layout' should always be 0
   - For D3D12 shaders the 'space' parameter for resource bindings should always be 0
 - Vulkan like idioms are used primarily with some D3D12 wherever it makes sense
 - Storage buffers created with tr_create_storage_buffer are not host visible. 
   - This was done to align the behavior on Vulkan and D3D12. Vulkan's storage 
     buffers can be host visible, but D3D12's UAV buffers are not permitted to 
     be host visible.

COMPILING & LINKING
   In one C/C++ file that #includes this file, do this:
      #define TINY_RENDERER_IMPLEMENTATION
   before the #include. That will create the implementation in that file.

*/

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined(__linux__)
    #define TINY_RENDERER_LINUX
    #define VK_USE_PLATFORM_XCB_KHR
    #include <X11/Xlib-xcb.h>
#endif

#if defined(_WIN32)
    #define TINY_RENDERER_MSW
    #define VK_USE_PLATFORM_WIN32_KHR
    // Pull in minimal Windows headers
    #if ! defined(NOMINMAX)
        #define NOMINMAX
    #endif
    #if ! defined(WIN32_LEAN_AND_MEAN)
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Windows.h>
#endif

#include <vulkan/vulkan.h>

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
namespace TINY_RENDERER_CPP_NAMESPACE {
#endif

#if defined(__cplusplus)
    #define tr_api_export extern "C"
#else 
    #define tr_api_export 
#endif

#if ! defined(TINY_RENDERER_CUSTOM_MAX)
enum {
    tr_max_instance_extensions       = 1024,
    tr_max_device_extensions         = 1024,
    tr_max_gpus                      = 4,
    tr_max_descriptors               = 32,
    tr_max_descriptor_sets           = 8,
    tr_max_render_target_attachments = 8,
    tr_max_submit_cmds               = 8,
    tr_max_submit_wait_semaphores    = 8,
    tr_max_submit_signal_semaphores  = 8,
    tr_max_present_wait_semaphores   = 8,
    tr_max_vertex_bindings           = 15,
    tr_max_vertex_attribs            = 15,
    tr_max_semantic_name_length      = 128,
    tr_max_descriptor_entries        = 256,
    tr_max_mip_levels                = 0xFFFFFFFF,
};
#endif

typedef enum tr_api {
    tr_api_vulkan = 0,
    tr_api_d3d12
} tr_api;

typedef enum tr_log_type {
    tr_log_type_info = 0,
    tr_log_type_warn,
    tr_log_type_debug,
    tr_log_type_error
} tr_log_type;

typedef void(*tr_log_fn)(tr_log_type, const char*, const char*);

/*

There's a lot of things to get right in Vulkan for it to work at all. So for now, the renderer 
will just assert in all the places where something goes wrong. Keep in mind that any memory 
allocated prior to the assert is not freed.

In the future, maybe add a error handling system so keep these values around.

*/ 
/*
typedef enum tr_result {
    // No errors
    tr_result_ok                = 0x00000000,
    // Errors
    tr_result_error_mask        = 0x0000FFFF,
    tr_result_unknown           = 0x00000001,
    tr_result_bad_ptr           = 0x00000002,
    tr_result_alloc_failed      = 0x00000004,
    tr_result_exceeded_max      = 0x00000008,
    // Components
    tr_result_component_mask    = 0x0FFF0000,
    tr_result_general           = 0x00000000,
    tr_result_renderer          = 0x00010000,
    tr_result_environment       = 0x00020000,
    tr_result_device            = 0x00040000,
    tr_result_queue             = 0x00080000,
    tr_result_surface           = 0x00100000,
    tr_result_swapchain         = 0x00200000,
    tr_result_render_target     = 0x00400000,
    tr_result_buffer            = 0x00800000,
    tr_result_texture           = 0x01000000,
    tr_result_cmd               = 0x02000000,
    tr_result_fence             = 0x04000000,
    tr_result_semaphore         = 0x08000000,
    // Internal API
    tr_result_internal_api_mask = 0xF0000000,
    tr_result_internal_api      = 0x10000000,
} tr_result;
*/

typedef enum tr_buffer_usage {
    tr_buffer_usage_index                       = 0x00000001,
    tr_buffer_usage_vertex                      = 0x00000002,
    tr_buffer_usage_indirect                    = 0x00000004,
    tr_buffer_usage_transfer_src                = 0x00000008,
    tr_buffer_usage_transfer_dst                = 0x00000010,
    tr_buffer_usage_uniform_cbv                 = 0x00000020,
    tr_buffer_usage_storage_srv                 = 0x00000040,
    tr_buffer_usage_storage_uav                 = 0x00000080,
    tr_buffer_usage_uniform_texel_srv           = 0x00000100,
    tr_buffer_usage_storage_texel_uav           = 0x00000200,
} tr_buffer_usage;

typedef enum tr_texture_type {
    tr_texture_type_1d,
    tr_texture_type_2d,
    tr_texture_type_3d,
    tr_texture_type_cube,
} tr_texture_type;

typedef enum tr_texture_usage {
    tr_texture_usage_undefined                  = 0x00000000,
    tr_texture_usage_transfer_src               = 0x00000001,
    tr_texture_usage_transfer_dst               = 0x00000002,
    tr_texture_usage_sampled_image              = 0x00000004,
    tr_texture_usage_storage_image              = 0x00000008,
    tr_texture_usage_color_attachment           = 0x00000010,
    tr_texture_usage_depth_stencil_attachment   = 0x00000020,
    tr_texture_usage_resolve_src                = 0x00000040,
    tr_texture_usage_resolve_dst                = 0x00000080,
    tr_texture_usage_present                    = 0x00000100,
} tr_texture_usage;

typedef uint32_t tr_texture_usage_flags;

typedef enum tr_format {
    tr_format_undefined = 0,
    // 1 channel
    tr_format_r8_unorm,
    tr_format_r16_unorm,
    tr_format_r16_float,
    tr_format_r32_uint,
    tr_format_r32_float,
    // 2 channel
    tr_format_r8g8_unorm,
    tr_format_r16g16_unorm,
    tr_format_r16g16_float,
    tr_format_r32g32_uint,
    tr_format_r32g32_float,
    // 3 channel
    tr_format_r8g8b8_unorm,
    tr_format_r16g16b16_unorm,
    tr_format_r16g16b16_float,
    tr_format_r32g32b32_uint,
    tr_format_r32g32b32_float,
    // 4 channel
    tr_format_b8g8r8a8_unorm,
    tr_format_r8g8b8a8_unorm,
    tr_format_r16g16b16a16_unorm,
    tr_format_r16g16b16a16_float,
    tr_format_r32g32b32a32_uint,
    tr_format_r32g32b32a32_float,
    // Depth/stencil
    tr_format_d16_unorm,
    tr_format_x8_d24_unorm_pack32,
    tr_format_d32_float,
    tr_format_s8_uint,
    tr_format_d16_unorm_s8_uint,
    tr_format_d24_unorm_s8_uint,
    tr_format_d32_float_s8_uint,
} tr_format;

typedef enum tr_descriptor_type {
    tr_descriptor_type_undefined = 0,
    tr_descriptor_type_sampler,
    tr_descriptor_type_uniform_buffer_cbv,       // CBV | VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
    tr_descriptor_type_storage_buffer_srv,       // SRV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    tr_descriptor_type_storage_buffer_uav,       // UAV | VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
    tr_descriptor_type_uniform_texel_buffer_srv, // SRV | VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
    tr_descriptor_type_storage_texel_buffer_uav, // UAV | VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
    tr_descriptor_type_texture_srv,              // SRV | VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
    tr_descriptor_type_texture_uav,              // UAV | VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
} tr_descriptor_type;

typedef enum tr_sample_count {
    tr_sample_count_1  =  1,
    tr_sample_count_2  =  2,
    tr_sample_count_4  =  4,
    tr_sample_count_8  =  8,
    tr_sample_count_16 = 16,
} tr_sample_count;

typedef enum tr_shader_stage {
    tr_shader_stage_vert          = 0x00000001,
    tr_shader_stage_tesc          = 0x00000002,
    tr_shader_stage_tese          = 0x00000004,
    tr_shader_stage_geom          = 0x00000008,
    tr_shader_stage_frag          = 0x00000010,
    tr_shader_stage_comp          = 0x00000020,
    tr_shader_stage_all_graphics  = 0x0000001F,
    tr_shader_stage_count         = 6,
} tr_shader_stage;

typedef enum tr_primitive_topo {
    tr_primitive_topo_point_list = 0,
    tr_primitive_topo_line_list,
    tr_primitive_topo_line_strip,
    tr_primitive_topo_tri_list,
    tr_primitive_topo_tri_strip,
    tr_primitive_topo_tri_fan,
    tr_primitive_topo_1_point_patch,
    tr_primitive_topo_2_point_patch,
    tr_primitive_topo_3_point_patch,
    tr_primitive_topo_4_point_patch,
} tr_primitive_topo;

typedef enum tr_index_type {
    tr_index_type_uint32 = 0,
    tr_index_type_uint16,
} tr_index_type;

typedef enum tr_semantic {
    tr_semantic_undefined = 0,
    tr_semantic_position,
    tr_semantic_normal,
    tr_semantic_color,
    tr_semantic_tangent,
    tr_semantic_bitangent,
    tr_semantic_texcoord0,
    tr_semantic_texcoord1,
    tr_semantic_texcoord2,
    tr_semantic_texcoord3,
    tr_semantic_texcoord4,
    tr_semantic_texcoord5,
    tr_semantic_texcoord6,
    tr_semantic_texcoord7,
    tr_semantic_texcoord8,
    tr_semantic_texcoord9,
} tr_semantic;

typedef enum tr_cull_mode {
    tr_cull_mode_none = 0,
    tr_cull_mode_back,
    tr_cull_mode_front,
    tr_cull_mode_both
} tr_cull_mode;

typedef enum tr_front_face {
    tr_front_face_ccw = 0,
    tr_fornt_face_cw
} tr_front_face;

typedef enum tr_pipeline_type {
  tr_pipeline_type_undefined = 0,
  tr_pipeline_type_compute,
  tr_pipeline_type_graphics
} tr_pipeline_type;

// Forward declarations
typedef struct tr_renderer tr_renderer;
typedef struct tr_render_target tr_render_target;
typedef struct tr_buffer tr_buffer;
typedef struct tr_texture tr_texture;
typedef struct tr_sampler tr_sampler;

typedef struct tr_clear_value {
    union {
        struct {
            float                       r;
            float                       g;
            float                       b;
            float                       a;
        };
        struct {
            float                       depth;
            uint32_t                    stencil;
        };
    };
} tr_clear_value;

typedef struct tr_platform_handle {
#if defined(__linux__)
    xcb_connection_t*                   connection;
    xcb_window_t                        window;
#elif defined(TINY_RENDERER_MSW)
    HINSTANCE                           hinstance;
    HWND                                hwnd;
#endif
} tr_platform_handle;

typedef struct tr_swapchain_settings {
    uint32_t                            image_count;
    tr_sample_count                     sample_count;
    uint32_t                            sample_quality;
    tr_format                           color_format;
    tr_clear_value                      color_clear_value;
    tr_format                           depth_stencil_format;
    tr_clear_value                      depth_stencil_clear_value;
} tr_swapchain_settings;

typedef struct tr_string_list {
    uint32_t        count;
    const char**    names;
} tr_string_list;

typedef struct tr_renderer_settings {
    tr_platform_handle                  handle;
    uint32_t                            width;
    uint32_t                            height;
    tr_swapchain_settings               swapchain;
    tr_log_fn                           log_fn;
    // Vulkan specific options
    tr_string_list                      instance_layers;
    tr_string_list                      instance_extensions;
    //tr_string_list                      device_layers;
    tr_string_list                      device_extensions;
    PFN_vkDebugReportCallbackEXT        vk_debug_fn;
} tr_renderer_settings;

typedef struct tr_fence {
    VkFence                             vk_fence;
} tr_fence;

typedef struct tr_semaphore {
    VkSemaphore                         vk_semaphore;
} tr_semaphore;

typedef struct tr_queue {
    tr_renderer*                        renderer;
    VkQueue                             vk_queue;
    uint32_t                            vk_queue_family_index;
} tr_queue;

typedef struct tr_renderer {
    tr_api                              api;
    tr_renderer_settings                settings;
    tr_render_target**                  swapchain_render_targets;
    uint32_t                            swapchain_image_index;
    tr_queue*                           graphics_queue;
    tr_queue*                           present_queue;
    tr_fence**                          image_acquired_fences;
    tr_semaphore**                      image_acquired_semaphores;
    tr_semaphore**                      render_complete_semaphores;
    VkInstance                          vk_instance;
    uint32_t                            vk_gpu_count;
    VkPhysicalDevice                    vk_gpus[tr_max_gpus];
    VkPhysicalDevice                    vk_active_gpu;
    uint32_t                            vk_active_gpu_index;
    VkPhysicalDeviceMemoryProperties    vk_memory_properties;
    VkPhysicalDeviceProperties          vk_active_gpu_properties;
    VkDevice                            vk_device;
    VkSurfaceKHR                        vk_surface;
    VkSwapchainKHR                      vk_swapchain;
    VkDebugReportCallbackEXT            vk_debug_report;
    bool                                vk_device_ext_VK_AMD_negative_viewport_height;
} tr_renderer;

typedef struct tr_descriptor {
    tr_descriptor_type                  type;
    uint32_t                            binding;
    uint32_t                            count;
    tr_shader_stage                     shader_stages;
    tr_buffer*                          uniform_buffers[tr_max_descriptor_entries];
    tr_texture*                         textures[tr_max_descriptor_entries];
    tr_sampler*                         samplers[tr_max_descriptor_entries];
    tr_buffer*                          buffers[tr_max_descriptor_entries];
} tr_descriptor;

typedef struct tr_descriptor_set {
    uint32_t                            descriptor_count;
    tr_descriptor*                      descriptors;
    VkDescriptorSetLayout               vk_descriptor_set_layout;
    VkDescriptorSet                     vk_descriptor_set;
    VkDescriptorPool                    vk_descriptor_pool;
} tr_descriptor_set;

typedef struct tr_cmd_pool {
    tr_renderer*                        renderer;
    VkCommandPool                       vk_cmd_pool;
} tr_cmd_pool;

typedef struct tr_cmd {
    tr_cmd_pool*                        cmd_pool;
    VkCommandBuffer                     vk_cmd_buf;
} tr_cmd;

typedef struct tr_buffer {
    tr_renderer*                        renderer;
    tr_buffer_usage                     usage;
    uint64_t                            size;
    bool                                host_visible;
    tr_index_type                       index_type;
    uint32_t                            vertex_stride;
    tr_format                           format;
    uint64_t                            first_element;
    uint64_t                            element_count;
    uint64_t                            struct_stride;
    bool                                raw;
    void*                               cpu_mapped_address;
    VkBuffer                            vk_buffer;
    VkDeviceMemory                      vk_memory;
    // Used for uniform and storage buffers
    VkDescriptorBufferInfo              vk_buffer_info;
    // Used for uniform texel and storage texel buffers
    VkBufferView                        vk_buffer_view;
    // Counter buffer
    tr_buffer*                          counter_buffer;
} tr_buffer;

typedef struct tr_texture {
    tr_renderer*                        renderer;
    tr_texture_type                     type;
    tr_texture_usage_flags              usage;
    uint32_t                            width;
    uint32_t                            height;
    uint32_t                            depth;
    tr_format                           format;
    uint32_t                            mip_levels;
    tr_sample_count                     sample_count;
    uint32_t                            sample_quality;
    tr_clear_value                      clear_value;
    bool                                host_visible;
    void*                               cpu_mapped_address;
    uint32_t                            owns_image;
    VkImage                             vk_image;
    VkDeviceMemory                      vk_memory;
    VkImageView                         vk_image_view;
    VkImageAspectFlags                  vk_aspect_mask;
    VkDescriptorImageInfo               vk_texture_view;
} tr_texture;

typedef struct tr_sampler {
    tr_renderer*                        renderer;
    VkSampler                           vk_sampler;
    VkDescriptorImageInfo               vk_sampler_view;
} tr_sampler;

typedef struct tr_shader_program {
    tr_renderer*                        renderer;
    uint32_t                            shader_stages;
    VkShaderModule                      vk_vert;
    VkShaderModule                      vk_tesc;
    VkShaderModule                      vk_tese;
    VkShaderModule                      vk_geom;
    VkShaderModule                      vk_frag;
    VkShaderModule                      vk_comp;
    const char*                         vert_entry_point;
    const char*                         tesc_entry_point;
    const char*                         tese_entry_point;
    const char*                         geom_entry_point;
    const char*                         frag_entry_point;
    const char*                         comp_entry_point;
} tr_shader_program;

typedef struct tr_vertex_attrib {
    tr_semantic                         semantic;
    uint32_t                            semantic_name_length;
    char                                semantic_name[tr_max_semantic_name_length];
    tr_format                           format;
    uint32_t                            binding;
    uint32_t                            location;
    uint32_t                            offset;
} tr_vertex_attrib;

typedef struct tr_vertex_layout {
    uint32_t                            attrib_count;
    tr_vertex_attrib                    attribs[tr_max_vertex_attribs];
} tr_vertex_layout;

typedef struct tr_pipeline_settings {
    tr_primitive_topo                   primitive_topo;
    tr_cull_mode                        cull_mode;
    tr_front_face                       front_face;
    bool                                depth;
} tr_pipeline_settings;

typedef struct tr_pipeline {
    tr_renderer*                        renderer;
    tr_pipeline_settings                settings;
    tr_pipeline_type                    type;
    VkPipelineLayout                    vk_pipeline_layout;
    VkPipeline                          vk_pipeline;
} tr_pipeline;

typedef struct tr_render_target {
    tr_renderer*                        renderer;
    uint32_t                            width;
    uint32_t                            height;
    tr_sample_count                     sample_count;
    tr_format                           color_format;
    uint32_t                            color_attachment_count;
    tr_texture*                         color_attachments[tr_max_render_target_attachments];
    tr_texture*                         color_attachments_multisample[tr_max_render_target_attachments];
    tr_format                           depth_stencil_format;
    tr_texture*                         depth_stencil_attachment;
    tr_texture*                         depth_stencil_attachment_multisample;
    VkRenderPass                        vk_render_pass;
    VkFramebuffer                       vk_framebuffer;
} tr_render_target;

typedef struct tr_mesh {
    tr_renderer*                        renderer;
    tr_buffer*                          uniform_buffer;
    tr_buffer*                          index_buffer;
    tr_buffer*                          vertex_buffer;
    tr_shader_program*                  shader_program;
    tr_pipeline*                        pipeline;
} tr_mesh;

typedef bool(*tr_image_resize_uint8_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, uint8_t* p_dst_data,
                                        uint32_t channel_cout, void* p_user_data);

typedef bool(*tr_image_resize_float_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, float* p_dst_data,
                                        uint32_t channel_cout, void* p_user_data);

// API functions
tr_api_export void tr_create_renderer(const char* app_name, const tr_renderer_settings* p_settings, tr_renderer** pp_renderer);
tr_api_export void tr_destroy_renderer(tr_renderer* p_renderer);

tr_api_export void tr_create_fence(tr_renderer* p_renderer, tr_fence** pp_fence);
tr_api_export void tr_destroy_fence(tr_renderer* p_renderer, tr_fence* p_fence);

tr_api_export void tr_create_semaphore(tr_renderer* p_renderer, tr_semaphore** pp_semaphore);
tr_api_export void tr_destroy_semaphore(tr_renderer* p_renderer, tr_semaphore* p_semaphore);

tr_api_export void tr_create_descriptor_set(tr_renderer* p_renderer, uint32_t descriptor_count, const tr_descriptor* descriptors, tr_descriptor_set** pp_descriptor_set);
tr_api_export void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

tr_api_export void tr_create_cmd_pool(tr_renderer* p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool** pp_cmd_pool);
tr_api_export void tr_destroy_cmd_pool(tr_renderer* p_renderer, tr_cmd_pool* p_cmd_pool);

tr_api_export void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd);
tr_api_export void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd);

tr_api_export void tr_create_cmd_n(tr_cmd_pool* p_cmd_pool, bool secondary, uint32_t cmd_count, tr_cmd*** ppp_cmd);
tr_api_export void tr_destroy_cmd_n(tr_cmd_pool* p_cmd_pool, uint32_t cmd_count, tr_cmd** pp_cmd);

tr_api_export void tr_create_buffer(tr_renderer* p_renderer, tr_buffer_usage usage, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export void tr_create_index_buffer(tr_renderer*p_renderer, uint64_t size, bool host_visible, tr_index_type index_type, tr_buffer** pp_buffer);
tr_api_export void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, uint32_t vertex_stride, tr_buffer** pp_buffer);
tr_api_export void tr_create_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_buffer);
tr_api_export void tr_create_rw_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_counter_buffer, tr_buffer** pp_buffer);
tr_api_export void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);

tr_api_export void tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_destroy_texture(tr_renderer* p_renderer, tr_texture*p_texture);

tr_api_export void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler);
tr_api_export void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);

tr_api_export void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size, const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
tr_api_export void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, tr_shader_program** p_shader_program);
tr_api_export void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
tr_api_export void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program);

tr_api_export void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
tr_api_export void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
tr_api_export void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);

tr_api_export void tr_create_render_target(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format color_format, uint32_t color_attachment_count, const tr_clear_value* color_clear_values, tr_format depth_stencil_format, const tr_clear_value* depth_stencil_clear_value, tr_render_target** pp_render_target);
tr_api_export void tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);

tr_api_export void tr_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

tr_api_export void tr_begin_cmd(tr_cmd* p_cmd);
tr_api_export void tr_end_cmd(tr_cmd* p_cmd);
tr_api_export void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
tr_api_export void tr_cmd_end_render(tr_cmd* p_cmd);
tr_api_export void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float, float width, float height, float min_depth, float max_depth);
tr_api_export void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
tr_api_export void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width);
tr_api_export void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value);
tr_api_export void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value);
tr_api_export void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
tr_api_export void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set);
tr_api_export void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer);
tr_api_export void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers);
tr_api_export void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
tr_api_export void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index);
tr_api_export void tr_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);
tr_api_export void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
tr_api_export void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
tr_api_export void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
tr_api_export void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
tr_api_export void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
tr_api_export void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture);

tr_api_export void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
tr_api_export void tr_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores, uint32_t signal_semaphore_count, tr_semaphore** pp_signal_semaphores);
tr_api_export void tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);
tr_api_export void tr_queue_wait_idle(tr_queue* p_queue);

tr_api_export void tr_render_target_set_color_clear_value(tr_render_target* p_render_target, uint32_t attachment_index, float r, float g, float b, float a);
tr_api_export void tr_render_target_set_depth_stencil_clear_value(tr_render_target* p_render_target, float depth, uint8_t stencil);

tr_api_export bool      tr_vertex_layout_support_format(tr_format format);
tr_api_export uint32_t  tr_vertex_layout_stride(const tr_vertex_layout* p_vertex_layout);

// Utility functions
tr_api_export uint32_t           tr_util_calc_mip_levels(uint32_t width, uint32_t height);
tr_api_export VkFormat           tr_util_to_vk_format(tr_format format);
tr_api_export tr_format          tr_util_from_vk_format(VkFormat fomat);
tr_api_export uint32_t           tr_util_format_stride(tr_format format);
tr_api_export uint32_t           tr_util_format_channel_count(tr_format format);
tr_api_export VkShaderStageFlags tr_util_to_vk_shader_stages(tr_shader_stage shader_stages);
tr_api_export void               tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
tr_api_export void               tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
tr_api_export void               tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_buffer);
tr_api_export void               tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer);
tr_api_export void               tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data, tr_buffer* p_buffer);
tr_api_export void               tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, uint32_t src_channel_count, tr_texture* p_texture, tr_image_resize_uint8_fn resize_fn, void* p_user_data);
tr_api_export void               tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data);

// =================================================================================================
// IMPLEMENTATION
// =================================================================================================

#if defined(TINY_RENDERER_IMPLEMENTATION)

#pragma comment(lib, "vulkan-1.lib")

#define TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer) \
    assert(NULL != s_tr_internal);                   \
    assert(NULL != s_tr_internal->renderer);         \
    assert(NULL != p_renderer);                      \
    assert(s_tr_internal->renderer == p_renderer);

#define TINY_RENDERER_SAFE_FREE(p_var) \
    if (NULL != p_var) {               \
       free((void*)p_var);             \
       p_var = NULL;                   \
    }

#if defined(__cplusplus)  
    #define TINY_RENDERER_DECLARE_ZERO(type, var) \
            type var = {};                        
#else
    #define TINY_RENDERER_DECLARE_ZERO(type, var) \
            type var = {0};                        
#endif

static inline uint32_t tr_max(uint32_t a, uint32_t b) 
{
    return a > b ? a : b;
}

static inline uint32_t tr_min(uint32_t a, uint32_t b) 
{
    return a < b ? a : b;
}

static inline uint32_t tr_round_up(uint32_t value, uint32_t multiple)
{
    assert(multiple);
    return ((value + multiple - 1) / multiple) * multiple;
}

// Internal utility functions (may become external one day)
VkSampleCountFlagBits tr_util_to_vk_sample_count(tr_sample_count sample_count);
VkBufferUsageFlags    tr_util_to_vk_buffer_usage(tr_buffer_usage usage);
VkImageUsageFlags     tr_util_to_vk_image_usage(tr_texture_usage usage);
VkImageLayout         tr_util_to_vk_image_layout(tr_texture_usage usage);
VkImageAspectFlags    tr_util_vk_determine_aspect_mask(VkFormat format);
bool                  tr_util_vk_get_memory_type(const VkPhysicalDeviceMemoryProperties* mem_props,  uint32_t type_bits, VkMemoryPropertyFlags flags, uint32_t* p_index);
VkFormatFeatureFlags  tr_util_vk_image_usage_to_format_features(VkImageUsageFlags usage);

// Internal init functions
void tr_internal_vk_create_instance(const char* app_name, tr_renderer* p_renderer);
void tr_internal_vk_create_surface(tr_renderer* p_renderer);
void tr_internal_vk_create_device(tr_renderer* p_renderer);
void tr_internal_vk_create_swapchain(tr_renderer* p_renderer);
void tr_internal_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_vk_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_vk_destroy_instance(tr_renderer* p_renderer);
void tr_internal_vk_destroy_surface(tr_renderer* p_renderer);
void tr_internal_vk_destroy_device(tr_renderer* p_renderer);
void tr_internal_vk_destroy_swapchain(tr_renderer* p_renderer);

// Internal create functions
void tr_internal_vk_create_fence(tr_renderer *p_renderer, tr_fence* p_fence);
void tr_internal_vk_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence);
void tr_internal_vk_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);
void tr_internal_vk_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);
void tr_internal_vk_create_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool* p_cmd_pool);
void tr_internal_vk_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool);
void tr_internal_vk_create_cmd(tr_cmd_pool *p_cmd_pool, bool secondary, tr_cmd* p_cmd);
void tr_internal_vk_destroy_cmd(tr_cmd_pool *p_cmd_pool, tr_cmd* p_cmd);
void tr_internal_vk_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_vk_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_vk_create_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_vk_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_vk_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_vk_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_vk_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* pp_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline);
void tr_internal_vk_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline);
void tr_internal_vk_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);
void tr_internal_vk_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size, const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program* p_shader_program);
void tr_internal_vk_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program);
void tr_internal_vk_create_render_target(tr_renderer* p_renderer, bool is_swapchain, tr_render_target* p_render_target);
void tr_internal_vk_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);

// Internal descriptor set functions
void tr_internal_vk_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

// Internal command buffer functions
void tr_internal_vk_begin_cmd(tr_cmd* p_cmd);
void tr_internal_vk_end_cmd(tr_cmd* p_cmd);
void tr_internal_vk_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
void tr_internal_vk_cmd_end_render(tr_cmd* p_cmd);
void tr_internal_vk_cmd_set_viewport(tr_cmd* p_cmd, float x, float, float width, float height, float min_depth, float max_depth);
void tr_internal_vk_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void tr_internal_vk_cmd_set_line_width(tr_cmd* p_cmd, float line_width);
void tr_cmd_internal_vk_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value);
void tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value);
void tr_internal_vk_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
void tr_internal_vk_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set);
void tr_internal_vk_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer);
void tr_internal_vk_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers);
void tr_internal_vk_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
void tr_internal_vk_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index);
void tr_internal_vk_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);
void tr_internal_vk_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
void tr_internal_vk_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_vk_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_vk_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
void tr_internal_vk_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture);

// Internal queue/swapchain functions
void tr_internal_vk_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
void tr_internal_vk_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores, uint32_t signal_semaphore_count, tr_semaphore** pp_signal_semaphores);
void tr_internal_vk_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);
void tr_internal_vk_queue_wait_idle(tr_queue* p_queue);


// -------------------------------------------------------------------------------------------------
// ptr_vector (begin)
// -------------------------------------------------------------------------------------------------
typedef void(*pfn_ptr_vector_destroy_elem)(void*);

typedef struct ptr_vector {
    size_t                      _size;
    size_t                      _capacity;
    void**                      _data;
    pfn_ptr_vector_destroy_elem _destroy_elem_fn;
} ptr_vector;

bool ptr_vector_create(ptr_vector** pp_vector, pfn_ptr_vector_destroy_elem pfn_destroy_elem)
{
    if (NULL == pfn_destroy_elem) {
        return false;
    }

    ptr_vector* p_vector = (ptr_vector*)calloc(1, sizeof(*p_vector));
    if (NULL == p_vector) {
        return false;
    }

    p_vector->_size = 0;
    p_vector->_capacity = 0;
    p_vector->_destroy_elem_fn = pfn_destroy_elem;
    *pp_vector = p_vector;
    
    return true;
}

bool ptr_vector_resize(ptr_vector* p_vector, size_t n) 
{
    if (NULL == p_vector) {
        return false;
    }

    size_t new_capacity = ((7 * n) / 4) + (n > 0 ? 1 : 0);
    void** new_data = NULL;
    if ((new_capacity != p_vector->_capacity) && (new_capacity > 0)) {
        new_data = (void**)calloc(new_capacity, sizeof(*new_data));
        if (NULL == new_data) {
            return false;
        }
    }

    if (NULL != new_data) {
        if (p_vector->_size > 0) {
            if (NULL == p_vector->_data) {
                return false;
            }

            void* ret = memcpy(new_data, 
                               p_vector->_data, p_vector->_size * sizeof(*(p_vector->_data)));
            if (ret != new_data) {
                return false;
            }
        }

        if (n < p_vector->_size) {
            pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;           
            for (size_t i = n; i < p_vector->_size; ++i) {
                if (NULL != destroy_elem_fn) {
                    destroy_elem_fn(p_vector->_data[i]);
                }
                p_vector->_data[i] = NULL;
            }
        }

        if (NULL != p_vector->_data) {
            free(p_vector->_data);
        }

        p_vector->_size = n;
        p_vector->_capacity = new_capacity;
        p_vector->_data = new_data;
    }
        
    return true;
}

bool ptr_vector_push_back(ptr_vector* p_vector, void* p) 
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t new_size = p_vector->_size + 1;
    if (new_size >= p_vector->_capacity) {
        bool ret = ptr_vector_resize(p_vector, new_size);
        if (! ret) {
            return false;
        }
    }

    p_vector->_size = new_size;
    p_vector->_data[new_size - 1] = p;

    return true;
}

bool ptr_vector_erase(ptr_vector* p_vector, void* p)
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t n = (size_t)-1;
    for (size_t i = 0; i < p_vector->_size; ++i) {
        if (p == p_vector->_data[i]) {
            n = i;
            break;
        }
    }

    if ((((size_t)-1) != n) && (NULL != p_vector->_data)) {
        if (n > 0) {
            for (size_t i = 0; i < (n - 1); ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }           
        }

        if (n < (p_vector->_size - 1)) {
            for (size_t i = n; i < p_vector->_size; ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        for (size_t i = (p_vector->_size - 1); i < p_vector->_capacity; ++i) {
            p_vector->_data[i] = NULL;
        }

        pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;
        if (NULL != destroy_elem_fn) {
            destroy_elem_fn(p);
        }

        p_vector->_size -= 1;

        if ((p_vector->_size < (p_vector->_capacity)) && (p_vector->_capacity > 10)) {
            bool ret = ptr_vector_resize(p_vector, p_vector->_size); 
            if (! ret) {
                return false;
            }
        }
    }

    return true; 
}

bool ptr_vector_remove(ptr_vector* p_vector, void* p)
{
    if ((NULL == p_vector) || (NULL == p)) {
        return false;
    }

    size_t n = (size_t)-1;
    for (size_t i = 0; i < p_vector->_size; ++i) {
        if (p == p_vector->_data[i]) {
            n = i;
            break;
        }
    }

    if ((((size_t)-1) != n) && (NULL != p_vector->_data)) {
        if (n > 0) {
            for (size_t i = 0; i < (n - 1); ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }           
        }

        if (n < (p_vector->_size - 1)) {
            for (size_t i = n; i < p_vector->_size; ++i) {
                p_vector->_data[i] = p_vector->_data[i + 1];
            }
        }

        for (size_t i = (p_vector->_size - 1); i < p_vector->_capacity; ++i) {
            p_vector->_data[i] = NULL;
        }

        p_vector->_size -= 1;

        if ((p_vector->_size < (p_vector->_capacity)) && (p_vector->_capacity > 10)) {
            bool ret = ptr_vector_resize(p_vector, p_vector->_size); 
            if (! ret) {
                return false;
            }
        }
    }

    return true; 
}


bool ptr_vector_destroy(ptr_vector* p_vector)
{
    if (NULL == p_vector) {
        return false;
    }

    if ((p_vector->_capacity > 0) && (NULL != p_vector->_data)) {
        if (p_vector->_size > 0) {
            pfn_ptr_vector_destroy_elem destroy_elem_fn = p_vector->_destroy_elem_fn;
            for (size_t i = 0; i < p_vector->_size; ++i) {
                if (NULL != destroy_elem_fn) {
                    destroy_elem_fn(p_vector->_data[i]);
                }
                p_vector->_data[i] = NULL;
            }
            p_vector->_size = 0;
            p_vector->_destroy_elem_fn = NULL;
        }
        p_vector->_capacity = 0;
        free(p_vector->_data);
        p_vector->_data = NULL;
    }
    free(p_vector);

    return true;
}
// -------------------------------------------------------------------------------------------------
// ptr_vector (end)
// -------------------------------------------------------------------------------------------------

// Internal singleton 
typedef struct tr_internal_data {
    tr_renderer*        renderer;   
    tr_render_target*   bound_render_target;
} tr_internal_data;

static tr_internal_data* s_tr_internal = NULL;

// Proxy log callback
static void tr_internal_log(tr_log_type type, const char* msg, const char* component)
{
    if (s_tr_internal->renderer->settings.log_fn) {
        s_tr_internal->renderer->settings.log_fn(type, msg, component);
    }
}

// These functions will need to be loaded in
static PFN_vkCreateDebugReportCallbackEXT  trVkCreateDebugReportCallbackEXT  = NULL;
static PFN_vkDestroyDebugReportCallbackEXT trVkDestroyDebugReportCallbackEXT = NULL;
static PFN_vkDebugReportMessageEXT         trVkDebugReportMessageEXT         = NULL;

// Proxy debug callback for Vulkan layers
static VKAPI_ATTR VkBool32 VKAPI_CALL tr_internal_debug_report_callback(
    VkDebugReportFlagsEXT      flags,
    VkDebugReportObjectTypeEXT objectType,
    uint64_t                   object,
    size_t                     location,
    int32_t                    messageCode,
    const char*                pLayerPrefix,
    const char*                pMessage,
    void*                      pUserData
)
{
    if ((NULL != s_tr_internal) && 
        (NULL != s_tr_internal->renderer) && 
        (NULL != s_tr_internal->renderer->settings.vk_debug_fn)) {
        return s_tr_internal->renderer->settings.vk_debug_fn(
            flags,
            objectType,
            object,
            location,
            messageCode,
            pLayerPrefix,
            pMessage,
            pUserData);
    }
    return VK_FALSE;
}

// -------------------------------------------------------------------------------------------------
// API functions
// -------------------------------------------------------------------------------------------------
void tr_create_renderer(const char *app_name, const tr_renderer_settings* settings, tr_renderer** pp_renderer)
{
    if (NULL == s_tr_internal) {
        s_tr_internal = (tr_internal_data*)calloc(1, sizeof(*s_tr_internal));
        assert(NULL != s_tr_internal);

        s_tr_internal->renderer = (tr_renderer*)calloc(1, sizeof(*(s_tr_internal->renderer)));
        assert(NULL != s_tr_internal->renderer);

        // Shorter way to get to the object
        tr_renderer* p_renderer = s_tr_internal->renderer;

        // Copy settings
        memcpy(&(p_renderer->settings), settings, sizeof(*settings));

        // Allocate storage for queues
        p_renderer->graphics_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->graphics_queue));
        assert(NULL != p_renderer->graphics_queue);
        p_renderer->present_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->present_queue));
        assert(NULL != p_renderer->present_queue);

        p_renderer->graphics_queue->renderer = p_renderer;
        p_renderer->present_queue->renderer = p_renderer;

        // Initialize the Vulkan bits
        {
            tr_internal_vk_create_instance(app_name, p_renderer);
            tr_internal_vk_create_surface(p_renderer);
            tr_internal_vk_create_device(p_renderer);
            tr_internal_vk_create_swapchain(p_renderer);
        }

        // Allocate and configure render target objects
        tr_internal_create_swapchain_renderpass(p_renderer);

        // Initialize the Vulkan bits of the render targets
        tr_internal_vk_create_swapchain_renderpass(p_renderer);

        // Allocate storage for image acquired fences
        p_renderer->image_acquired_fences = (tr_fence**)calloc(p_renderer->settings.swapchain.image_count, 
                                                               sizeof(*(p_renderer->image_acquired_fences)));
        assert(NULL != p_renderer->image_acquired_fences);
        
        // Allocate storage for image acquire semaphores
        p_renderer->image_acquired_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count, 
                                                                       sizeof(*(p_renderer->image_acquired_semaphores)));
        assert(NULL != p_renderer->image_acquired_semaphores);
        
        // Allocate storage for render complete semaphores
        p_renderer->render_complete_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count, 
                                                                        sizeof(*(p_renderer->render_complete_semaphores)));
        assert(NULL != p_renderer->render_complete_semaphores);

        // Initialize fences and semaphores
        for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i ) {
            tr_create_fence(p_renderer, &(p_renderer->image_acquired_fences[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->image_acquired_semaphores[i]));
            tr_create_semaphore(p_renderer, &(p_renderer->render_complete_semaphores[i]));
        }

        // No need to do this since, the render pass will take care of them
        //
        //// Transition the swapchain render targets to first use
        //if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
        //    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        //        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        //        // Color single-sample images are swapchain images and are not transitioned
        //
        //        // Color multi-sample
        //        tr_util_transition_image(p_renderer->graphics_queue, 
        //                                 render_target->color_attachments_multisample[0], 
        //                                 tr_texture_usage_undefined, 
        //                                 tr_texture_usage_color_attachment );
        //        // Depth/stencil
        //        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
        //            tr_util_transition_image(p_renderer->graphics_queue, 
        //                                     render_target->depth_stencil_attachment, 
        //                                     tr_texture_usage_undefined, 
        //                                     tr_texture_usage_depth_stencil_attachment );
        //        }
        //    }
        //}
        //else {
        //    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        //        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        //        // Color images are swapchain images and are not transitioned
        //
        //        // Depth/stencil
        //        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
        //            tr_util_transition_image(p_renderer->graphics_queue, 
        //                                     render_target->depth_stencil_attachment, 
        //                                     tr_texture_usage_undefined, 
        //                                     tr_texture_usage_depth_stencil_attachment );
        //        }
        //            
        //    }
        //}

        // Renderer is good! Assign it to result!
        *(pp_renderer) = p_renderer;
    }
}

void tr_destroy_renderer(tr_renderer* p_renderer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != s_tr_internal);
    
    // Destroy the swapchain render targets
    if (NULL != p_renderer->swapchain_render_targets) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_render_target(p_renderer, p_renderer->swapchain_render_targets[i]);
        }
                    
    }

    // Destroy render sync objects
    if (NULL != p_renderer->image_acquired_fences) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_fence(p_renderer, p_renderer->image_acquired_fences[i]);
        }
    }
    if (NULL != p_renderer->image_acquired_semaphores) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_semaphore(p_renderer, p_renderer->image_acquired_semaphores[i]);
        }
    }
    if (NULL != p_renderer->render_complete_semaphores) {
        for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
            tr_destroy_semaphore(p_renderer, p_renderer->render_complete_semaphores[i]);
        }
    }

    // Destroy the Vulkan bits
    tr_internal_vk_destroy_swapchain(p_renderer);
    tr_internal_vk_destroy_surface(p_renderer);
    tr_internal_vk_destroy_device(p_renderer);
    tr_internal_vk_destroy_instance(p_renderer);

    // Free all the renderer components!
    TINY_RENDERER_SAFE_FREE(p_renderer->swapchain_render_targets);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_fences);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_semaphores);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->render_complete_semaphores);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->present_queue);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->graphics_queue);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer);
    TINY_RENDERER_SAFE_FREE(s_tr_internal);
}

void tr_create_fence(tr_renderer *p_renderer, tr_fence** pp_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_fence* p_fence = (tr_fence*)calloc(1, sizeof(*p_fence));
    assert(NULL != p_fence);

    tr_internal_vk_create_fence(p_renderer, p_fence);

    *pp_fence = p_fence;
}

void tr_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_fence);

    tr_internal_vk_destroy_fence(p_renderer, p_fence);

    TINY_RENDERER_SAFE_FREE(p_fence);
}

void tr_create_semaphore(tr_renderer *p_renderer, tr_semaphore** pp_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_semaphore* p_semaphore = (tr_semaphore*)calloc(1, sizeof(*p_semaphore));
    assert(NULL != p_semaphore);

    tr_internal_vk_create_semaphore(p_renderer, p_semaphore);

    *pp_semaphore = p_semaphore;
}

void tr_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_semaphore);

    tr_internal_vk_destroy_semaphore(p_renderer, p_semaphore);

    TINY_RENDERER_SAFE_FREE(p_semaphore);
}

void tr_create_descriptor_set(tr_renderer* p_renderer, uint32_t descriptor_count, const tr_descriptor* p_descriptors, tr_descriptor_set** pp_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_descriptor_set* p_descriptor_set = (tr_descriptor_set*)calloc(1, sizeof(*p_descriptor_set));
    assert(NULL != p_descriptor_set);

    p_descriptor_set->descriptors = (tr_descriptor*)calloc(descriptor_count, sizeof(*(p_descriptor_set->descriptors)));
    assert(NULL != p_descriptor_set->descriptors);

    p_descriptor_set->descriptor_count = descriptor_count;
    memcpy(p_descriptor_set->descriptors, p_descriptors, descriptor_count * sizeof(*(p_descriptor_set->descriptors)));

    tr_internal_vk_create_descriptor_set(p_renderer, p_descriptor_set);

    *pp_descriptor_set = p_descriptor_set;
}

void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set->descriptors);

    tr_internal_vk_destroy_descriptor_set(p_renderer, p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set);
}

void tr_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool** pp_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_cmd_pool* p_cmd_pool = (tr_cmd_pool*)calloc(1, sizeof(*p_cmd_pool));
    assert(NULL != p_cmd_pool);

    p_cmd_pool->renderer = p_renderer;

    tr_internal_vk_create_cmd_pool(p_renderer, p_queue, transient, p_cmd_pool);
    
    *pp_cmd_pool = p_cmd_pool;
}

void tr_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_cmd_pool);

    tr_internal_vk_destroy_cmd_pool(p_renderer, p_cmd_pool);

    TINY_RENDERER_SAFE_FREE(p_cmd_pool);
}

void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd)
{
    assert(NULL != p_cmd_pool);

    tr_cmd* p_cmd = (tr_cmd*)calloc(1, sizeof(*p_cmd));
    assert(NULL != p_cmd);

    p_cmd->cmd_pool = p_cmd_pool;

    tr_internal_vk_create_cmd(p_cmd_pool, secondary, p_cmd);
    
    *pp_cmd = p_cmd;
}

void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool);
    assert(NULL != p_cmd);

    tr_internal_vk_destroy_cmd(p_cmd_pool, p_cmd);

    TINY_RENDERER_SAFE_FREE(p_cmd);
}

void tr_create_cmd_n(tr_cmd_pool *p_cmd_pool, bool secondary, uint32_t cmd_count, tr_cmd*** ppp_cmd)
{
    assert(NULL != ppp_cmd);

    tr_cmd** pp_cmd = (tr_cmd**)calloc(cmd_count, sizeof(*pp_cmd));
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i) {
        tr_create_cmd(p_cmd_pool, secondary, &(pp_cmd[i]));
    }

    *ppp_cmd = pp_cmd;
}

void tr_destroy_cmd_n(tr_cmd_pool *p_cmd_pool, uint32_t cmd_count, tr_cmd** pp_cmd)
{
    assert(NULL != pp_cmd);

    for (uint32_t i = 0; i < cmd_count; ++i) {
        tr_destroy_cmd(p_cmd_pool, pp_cmd[i]);
    }

    
    TINY_RENDERER_SAFE_FREE(pp_cmd);
}

void tr_create_buffer(tr_renderer* p_renderer, tr_buffer_usage usage, uint64_t size, bool host_visible, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0 );

    tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
    assert(NULL != p_buffer);

    p_buffer->renderer     = p_renderer;
    p_buffer->usage        = usage;
    p_buffer->size         = size;
    p_buffer->host_visible = host_visible;

    tr_internal_vk_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_index_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_index_type index_type, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_index, size, host_visible, pp_buffer);
    (*pp_buffer)->index_type = index_type;
}

void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_uniform_cbv, size, host_visible, pp_buffer);
}

void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, uint32_t vertex_stride, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_vertex, size, host_visible, pp_buffer);
    (*pp_buffer)->vertex_stride = vertex_stride;
}

void tr_create_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0 );

    tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
    assert(NULL != p_buffer);

    p_buffer->renderer        = p_renderer;
    p_buffer->usage           = tr_buffer_usage_storage_srv;
    p_buffer->size            = size;
    p_buffer->host_visible    = false;
    p_buffer->format          = tr_format_undefined;
    p_buffer->first_element   = first_element;
    p_buffer->element_count   = element_count;
    p_buffer->struct_stride   = struct_stride;
  
    tr_internal_vk_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_rw_structured_buffer(tr_renderer* p_renderer, uint64_t size, uint64_t first_element, uint64_t element_count, uint64_t struct_stride, bool raw, tr_buffer** pp_counter_buffer, tr_buffer** pp_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(size > 0 );

    // Create counter buffer
    if (pp_counter_buffer != NULL) {
      tr_buffer* p_counter_buffer = (tr_buffer*)calloc(1, sizeof(*p_counter_buffer));
      assert(NULL != p_counter_buffer);

      p_counter_buffer->renderer        = p_renderer;
      p_counter_buffer->usage           = tr_buffer_usage_storage_uav;
      p_counter_buffer->size            = 4;
      p_counter_buffer->host_visible    = false;
      p_counter_buffer->format          = tr_format_undefined;
      p_counter_buffer->first_element   = 0;
      p_counter_buffer->element_count   = 1;
      p_counter_buffer->struct_stride   = 4;
    
      tr_internal_vk_create_buffer(p_renderer, p_counter_buffer);

      *pp_counter_buffer = p_counter_buffer;
    }

    // Create data buffer
    {
      tr_buffer* p_buffer = (tr_buffer*)calloc(1, sizeof(*p_buffer));
      assert(NULL != p_buffer);

      p_buffer->renderer        = p_renderer;
      p_buffer->usage           = tr_buffer_usage_storage_uav;
      p_buffer->size            = size;
      p_buffer->host_visible    = false;
      p_buffer->format          = tr_format_undefined;
      p_buffer->first_element   = first_element;
      p_buffer->element_count   = element_count;
      p_buffer->struct_stride   = struct_stride;

      if (pp_counter_buffer != NULL) {
        p_buffer->counter_buffer = *pp_counter_buffer;
      }
    
      tr_internal_vk_create_buffer(p_renderer, p_buffer);

      *pp_buffer = p_buffer;
    }
}


void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_buffer);

    tr_internal_vk_destroy_buffer(p_renderer, p_buffer);

    TINY_RENDERER_SAFE_FREE(p_buffer);
}

void tr_create_texture(
    tr_renderer*             p_renderer, 
    tr_texture_type          type, 
    uint32_t                 width, 
    uint32_t                 height, 
    uint32_t                 depth, 
    tr_sample_count          sample_count,
    tr_format                format, 
    uint32_t                 mip_levels,
    const tr_clear_value*    p_clear_value, 
    bool                     host_visible, 
    tr_texture_usage_flags   usage, 
    tr_texture**             pp_texture
)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert((width > 0) && (height > 0) && (depth > 0));

    tr_texture* p_texture = (tr_texture*)calloc(1, sizeof(*p_texture));
    assert(NULL != p_texture);

    p_texture->renderer           = p_renderer;
    p_texture->type               = type;
    p_texture->usage              = usage;
    p_texture->width              = width;
    p_texture->height             = height;
    p_texture->depth              = depth;
    p_texture->format             = format;
    p_texture->mip_levels         = mip_levels;
    p_texture->sample_count       = sample_count;
    p_texture->host_visible       = host_visible;
    p_texture->cpu_mapped_address = NULL;
    p_texture->owns_image         = false;
    if (NULL != p_clear_value) {
        if (tr_texture_usage_depth_stencil_attachment == (usage & tr_texture_usage_depth_stencil_attachment)) {
            p_texture->clear_value.depth = p_clear_value->depth;
            p_texture->clear_value.stencil = p_clear_value->stencil;
        }
        else {
            p_texture->clear_value.r = p_clear_value->r;
            p_texture->clear_value.g = p_clear_value->g;
            p_texture->clear_value.b = p_clear_value->b;
            p_texture->clear_value.a = p_clear_value->a;
        }
    }

    tr_internal_vk_create_texture(p_renderer, p_texture);

    *pp_texture = p_texture;
}

void tr_create_texture_1d(
    tr_renderer*            p_renderer, 
    uint32_t                width, 
    tr_sample_count         sample_count, 
    tr_format               format, 
    bool                    host_visible, 
    tr_texture_usage_flags  usage, 
    tr_texture**            pp_texture
)
{
    tr_create_texture(p_renderer, tr_texture_type_1d, width, 1, 1, sample_count, format, 1, NULL, host_visible, usage, pp_texture);
}

void tr_create_texture_2d(
    tr_renderer*              p_renderer, 
    uint32_t                  width, 
    uint32_t                  height, 
    tr_sample_count           sample_count, 
    tr_format                 format,
    uint32_t                  mip_levels,
    const tr_clear_value*     clear_value, 
    bool                      host_visible,
    tr_texture_usage_flags    usage, 
    tr_texture**              pp_texture
)
{
    if (tr_max_mip_levels == mip_levels) {
        mip_levels = tr_util_calc_mip_levels(width, height);
    }

    tr_create_texture(p_renderer, tr_texture_type_2d, width, height, 1, sample_count, format, mip_levels, clear_value, host_visible, usage, pp_texture);
}

void tr_create_texture_3d(
    tr_renderer*            p_renderer, 
    uint32_t                width, 
    uint32_t                height, 
    uint32_t                depth, 
    tr_sample_count         sample_count, 
    tr_format               format, 
    bool                    host_visible, 
    tr_texture_usage_flags  usage, 
    tr_texture**            pp_texture
)
{
    tr_create_texture(p_renderer, tr_texture_type_3d, width, height, depth, sample_count, format, 1, NULL, host_visible, usage, pp_texture);
}

void tr_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_texture);

    tr_internal_vk_destroy_texture(p_renderer, p_texture);

    TINY_RENDERER_SAFE_FREE(p_texture);
}

void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_sampler* p_sampler = (tr_sampler*)calloc(1, sizeof(*p_sampler));
    assert(NULL != p_sampler);

    p_sampler->renderer = p_renderer;
    
    tr_internal_vk_create_sampler(p_renderer, p_sampler);

    *pp_sampler = p_sampler;
}

void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_sampler);

    tr_internal_vk_destroy_sampler(p_renderer, p_sampler);

    TINY_RENDERER_SAFE_FREE(p_sampler);
}

void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size, const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    if (vert_size > 0) {
        assert(NULL != vert_code);
    }
    if (tesc_size > 0) {
        assert(NULL != tesc_code);
    }
    if (tese_size > 0) {
        assert(NULL != tese_code);
    }
    if (geom_size > 0) {
        assert(NULL != geom_code);
    }
    if (frag_size > 0) {
        assert(NULL != frag_code);
    }
    if (comp_size > 0) {
        assert(NULL != comp_code);
    }

    tr_shader_program* p_shader_program = (tr_shader_program*)calloc(1, sizeof(*p_shader_program));
    assert(NULL != p_shader_program);
    
    p_shader_program->renderer = p_renderer;
    p_shader_program->shader_stages |= (vert_size > 0) ? tr_shader_stage_vert : 0;
    p_shader_program->shader_stages |= (tesc_size > 0) ? tr_shader_stage_tesc : 0;
    p_shader_program->shader_stages |= (tese_size > 0) ? tr_shader_stage_tese : 0;
    p_shader_program->shader_stages |= (geom_size > 0) ? tr_shader_stage_geom : 0;
    p_shader_program->shader_stages |= (frag_size > 0) ? tr_shader_stage_frag : 0;
    p_shader_program->shader_stages |= (comp_size > 0) ? tr_shader_stage_comp : 0;

    tr_internal_vk_create_shader_program(p_renderer, vert_size, vert_code, vert_enpt, tesc_size, tesc_code, tesc_enpt, tese_size, tese_code, tese_enpt, geom_size, geom_code, geom_enpt, frag_size, frag_code, frag_enpt, comp_size, comp_code, comp_enpt, p_shader_program);

    if ((vert_enpt != NULL) && (strlen(vert_enpt) > 0)) {
      p_shader_program->vert_entry_point = (const char*)calloc(strlen(vert_enpt) + 1, sizeof(char));
      assert(p_shader_program->vert_entry_point != NULL);
      strncpy((char*)p_shader_program->vert_entry_point, vert_enpt, strlen(vert_enpt));
    }

    if ((tesc_enpt != NULL) && (strlen(tesc_enpt) > 0)) {
      p_shader_program->tesc_entry_point = (const char*)calloc(strlen(tesc_enpt) + 1, sizeof(char));
      assert(p_shader_program->tesc_entry_point != NULL);
      strncpy((char*)p_shader_program->tesc_entry_point, tesc_enpt, strlen(tesc_enpt));
    }

    if ((tese_enpt != NULL) && (strlen(tese_enpt) > 0)) {
      p_shader_program->tese_entry_point = (const char*)calloc(strlen(tese_enpt) + 1, sizeof(char));
      assert(p_shader_program->tese_entry_point != NULL);
      strncpy((char*)p_shader_program->tese_entry_point, tese_enpt, strlen(tese_enpt));
    }

    if ((geom_enpt != NULL) && (strlen(geom_enpt) > 0)) {
      p_shader_program->geom_entry_point = (const char*)calloc(strlen(geom_enpt) + 1, sizeof(char));
      assert(p_shader_program->geom_entry_point != NULL);
      strncpy((char*)p_shader_program->geom_entry_point, geom_enpt, strlen(geom_enpt));
    }

    if ((frag_enpt != NULL) && (strlen(frag_enpt) > 0)) {
      p_shader_program->frag_entry_point = (const char*)calloc(strlen(frag_enpt) + 1, sizeof(char));
      assert(p_shader_program->frag_entry_point != NULL);
      strncpy((char*)p_shader_program->frag_entry_point, frag_enpt, strlen(frag_enpt));
    }

    if ((comp_enpt != NULL) && (strlen(comp_enpt) > 0)) {
      p_shader_program->comp_entry_point = (const char*)calloc(strlen(comp_enpt) + 1, sizeof(char));
      assert(p_shader_program->comp_entry_point != NULL);
      strncpy((char*)p_shader_program->comp_entry_point, comp_enpt, strlen(comp_enpt));
    }

    *pp_shader_program = p_shader_program;
}

void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const uint32_t* vert_code, const char* vert_enpt, uint32_t frag_size, const uint32_t* frag_code, const char* frag_enpt, tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, vert_size, vert_code, vert_enpt, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, frag_size, frag_code, frag_enpt, 0, NULL, NULL, pp_shader_program);
}

void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    tr_create_shader_program_n(p_renderer, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, 0, NULL, NULL, comp_size, comp_code, comp_enpt, pp_shader_program);
}

void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_internal_vk_destroy_shader_program(p_renderer, p_shader_program);
}

void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_render_target);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = (tr_pipeline*)calloc(1, sizeof(*p_pipeline));
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    tr_internal_vk_create_pipeline(p_renderer, p_shader_program, p_vertex_layout, p_descriptor_set, p_render_target, p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_graphics;

    *pp_pipeline = p_pipeline;
}

tr_api_export void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_shader_program);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = (tr_pipeline*)calloc(1, sizeof(*p_pipeline));
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    tr_internal_vk_create_compute_pipeline(p_renderer, p_shader_program, p_descriptor_set, p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_compute;

    *pp_pipeline = p_pipeline;
}

void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_pipeline);

    tr_internal_vk_destroy_pipeline(p_renderer, p_pipeline);

    TINY_RENDERER_SAFE_FREE(p_pipeline);
}

void tr_create_render_target(
    tr_renderer*            p_renderer, 
    uint32_t                width, 
    uint32_t                height, 
    tr_sample_count         sample_count, 
    tr_format               color_format, 
    uint32_t                color_attachment_count, 
    const tr_clear_value*   p_color_clear_values,
    tr_format               depth_stencil_format, 
    const tr_clear_value*   p_depth_stencil_clear_value,
    tr_render_target**      pp_render_target
)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_render_target* p_render_target = (tr_render_target*)calloc(1, sizeof(*p_render_target));
    assert(NULL != p_render_target);

    p_render_target->renderer               = p_renderer;
    p_render_target->width                  = width;
    p_render_target->height                 = height;
    p_render_target->sample_count           = sample_count;
    p_render_target->color_format           = color_format;
    p_render_target->color_attachment_count = color_attachment_count;
    p_render_target->depth_stencil_format   = depth_stencil_format;
    
    // Create attachments
    {
        // Color
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            const tr_clear_value* clear_value = (NULL != p_color_clear_values) ? &p_color_clear_values[i] : NULL;

            tr_create_texture_2d(p_renderer, 
                                 p_render_target->width, 
                                 p_render_target->height, 
                                 tr_sample_count_1,
                                 p_render_target->color_format,
                                 1,
                                 clear_value,
                                 false,
                                 (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_sampled_image),
                                 &(p_render_target->color_attachments[i]));

            if (p_render_target->sample_count > tr_sample_count_1) {
                tr_create_texture_2d(p_renderer, 
                                     p_render_target->width, 
                                     p_render_target->height, 
                                     p_render_target->sample_count,
                                     p_render_target->color_format,
                                     1,
                                     clear_value,
                                     false,
                                     (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_sampled_image),
                                     &(p_render_target->color_attachments_multisample[i]));
            }
        }

        // Depth/stencil
        if (tr_format_undefined != p_render_target->depth_stencil_format) {
            tr_create_texture_2d(p_renderer, 
                                 p_render_target->width, 
                                 p_render_target->height, 
                                 p_render_target->sample_count,
                                 p_render_target->color_format,
                                 1,
                                 p_depth_stencil_clear_value,
                                 false,
                                 (tr_texture_usage)(tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image),
                                 &(p_render_target->depth_stencil_attachment));
        }
    }

    // Create Vulkan specific objects for the render target
    tr_internal_vk_create_render_target(p_renderer, false, p_render_target);

    *pp_render_target = p_render_target;
}

void tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_render_target);

    if ((s_tr_internal->renderer == p_renderer) && (NULL != p_render_target)) {
        // Destroy color attachments
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            tr_destroy_texture(p_renderer, p_render_target->color_attachments[i]);
            if (NULL != p_render_target->color_attachments_multisample[i]) {
                tr_destroy_texture(p_renderer, p_render_target->color_attachments_multisample[i]);
            }
        }

        // Destroy depth attachment
        if (NULL != p_render_target->depth_stencil_attachment) {
            tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment);
        }

        // Destroy VkRenderPass object
        if (VK_NULL_HANDLE != p_render_target->vk_render_pass) {
            vkDestroyRenderPass(p_renderer->vk_device, p_render_target->vk_render_pass, NULL);
        }

        // Destroy VkFramebuffer object
        if (VK_NULL_HANDLE != p_render_target->vk_framebuffer) {
            vkDestroyFramebuffer(p_renderer->vk_device, p_render_target->vk_framebuffer, NULL);
        }

    }

    TINY_RENDERER_SAFE_FREE(p_render_target);
}

// -------------------------------------------------------------------------------------------------
// Descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer);
    assert(NULL != p_descriptor_set);

    tr_internal_vk_update_descriptor_set(p_renderer, p_descriptor_set);
}

// -------------------------------------------------------------------------------------------------
// Command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_begin_cmd(p_cmd);
}

void tr_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_end_cmd(p_cmd);
}

void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    s_tr_internal->bound_render_target = p_render_target;

    tr_internal_vk_cmd_begin_render(p_cmd, p_render_target);
}

void tr_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_end_render(p_cmd);

    s_tr_internal->bound_render_target = NULL;
}

void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_viewport(p_cmd, x, y, width, height, min_depth, max_depth);
}

void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_scissor(p_cmd, x, y, width, height);
}

void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_set_line_width(p_cmd,  line_width);
}

void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    tr_cmd_internal_vk_cmd_clear_color_attachment(p_cmd, attachment_index, clear_value);
}

void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
  assert(NULL != p_cmd);

  tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(p_cmd, clear_value);
}

void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);

    tr_internal_vk_cmd_bind_pipeline(p_cmd, p_pipeline);
}

void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);
    assert(NULL != p_descriptor_set);

    tr_internal_vk_cmd_bind_descriptor_sets(p_cmd, p_pipeline, p_descriptor_set);
}

void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_vk_cmd_bind_index_buffer(p_cmd, p_buffer);
}

void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd);
    assert(0 != buffer_count);
    assert(NULL != pp_buffers);

    tr_internal_vk_cmd_bind_vertex_buffers(p_cmd, buffer_count, pp_buffers);
}

void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_draw(p_cmd, vertex_count, first_vertex);
}

void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd);

    tr_internal_vk_cmd_draw_indexed(p_cmd, index_count, first_index);
}

void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
}

void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_texture);

    // Vulkan doesn't have an VkImageLayout corresponding to tr_texture_usage_storage, so
    // just ignore transitions into or out of tr_texture_usage_storage.
    if ((old_usage == tr_texture_usage_storage_image) || (new_usage == tr_texture_usage_storage_image)) {
      return;
    }

    tr_internal_vk_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
}

void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    // Vulkan render passes take care of transitions, so just ignore this for now...
}

void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
  // Vulkan render passes take care of transitions, so just ignore this for now...
}

void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    assert(NULL != p_cmd);
    tr_internal_vk_cmd_dispatch(p_cmd, group_count_x, group_count_y, group_count_z);
}

void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd != NULL);
    assert(p_buffer != NULL);
    assert(p_texture != NULL);

    tr_internal_vk_cmd_copy_buffer_to_texture2d(p_cmd, width, height, row_pitch, buffer_offset, mip_level, p_buffer, p_texture);
}

void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_internal_vk_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
}

void tr_queue_submit(
    tr_queue*      p_queue, 
    uint32_t       cmd_count,
    tr_cmd**       pp_cmds,
    uint32_t       wait_semaphore_count,
    tr_semaphore** pp_wait_semaphores,
    uint32_t       signal_semaphore_count,
    tr_semaphore** pp_signal_semaphores
)
{
    assert(NULL != p_queue);
    assert(cmd_count > 0);
    assert(NULL != pp_cmds);
    if (wait_semaphore_count > 0) {
        assert(NULL != pp_wait_semaphores);
    }
    if (signal_semaphore_count > 0) {
        assert(NULL != pp_signal_semaphores);
    }

    tr_internal_vk_queue_submit(p_queue, 
                                cmd_count, 
                                pp_cmds, 
                                wait_semaphore_count, 
                                pp_wait_semaphores, 
                                signal_semaphore_count, 
                                pp_signal_semaphores);
}

void tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
    assert(NULL != p_queue);
    if (wait_semaphore_count > 0) {
        assert(NULL != pp_wait_semaphores);
    }

    tr_internal_vk_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
}

void tr_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue);

    tr_internal_vk_queue_wait_idle(p_queue);
}

void tr_render_target_set_color_clear_value(tr_render_target* p_render_target, uint32_t attachment_index, float r, float g, float b, float a)
{
    assert(NULL != p_render_target);
    assert(attachment_index < p_render_target->color_attachment_count);

    p_render_target->color_attachments[attachment_index]->clear_value.r = r;
    p_render_target->color_attachments[attachment_index]->clear_value.g = g;
    p_render_target->color_attachments[attachment_index]->clear_value.b = b;
    p_render_target->color_attachments[attachment_index]->clear_value.a = a;
}

void tr_render_target_set_depth_stencil_clear_value(tr_render_target* p_render_target, float depth, uint8_t stencil)
{
    assert(NULL != p_render_target);

    p_render_target->depth_stencil_attachment->clear_value.depth = depth;
    p_render_target->depth_stencil_attachment->clear_value.stencil = stencil;
}

bool tr_vertex_layout_support_format(tr_format format)
{
    bool result = false;
    switch (format) {
        // 1 channel
        case tr_format_r8_unorm            : result = true; break;
        case tr_format_r16_unorm           : result = true; break;
        case tr_format_r16_float           : result = true; break;
        case tr_format_r32_uint            : result = true; break;
        case tr_format_r32_float           : result = true; break;
        // 2 channel
        case tr_format_r8g8_unorm          : result = true; break;
        case tr_format_r16g16_unorm        : result = true; break;
        case tr_format_r16g16_float        : result = true; break;
        case tr_format_r32g32_uint         : result = true; break;
        case tr_format_r32g32_float        : result = true; break;
        // 3 channel
        case tr_format_r8g8b8_unorm        : result = true; break;
        case tr_format_r16g16b16_unorm     : result = true; break;
        case tr_format_r16g16b16_float     : result = true; break;
        case tr_format_r32g32b32_uint      : result = true; break;
        case tr_format_r32g32b32_float     : result = true; break;
        // 4 channel
        case tr_format_r8g8b8a8_unorm      : result = true; break;
        case tr_format_r16g16b16a16_unorm  : result = true; break;
        case tr_format_r16g16b16a16_float  : result = true; break;
        case tr_format_r32g32b32a32_uint   : result = true; break;
        case tr_format_r32g32b32a32_float  : result = true; break;
    }
    return result;
}

uint32_t tr_vertex_layout_stride(const tr_vertex_layout* p_vertex_layout)
{
    assert(NULL != p_vertex_layout);

    uint32_t result = 0;
    for (uint32_t i = 0; i < p_vertex_layout->attrib_count; ++i) {
        result += tr_util_format_stride(p_vertex_layout->attribs[i].format);
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------------------

// This function always returns zero for Vulkan for now because
// of how the counter are configured.
uint64_t tr_util_calc_storage_counter_offset(uint64_t buffer_size)
{
    uint64_t result = 0;
    return result;
}

uint32_t tr_util_calc_mip_levels(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
        return 0;

    uint32_t result = 1;
    while (width > 1 || height > 1)
    {
        width >>= 1;
        height >>= 1;
        result++;
    }
    return result;
}

tr_api_export VkFormat tr_util_to_vk_format(tr_format format)
{
    VkFormat result = VK_FORMAT_UNDEFINED;
    switch (format) {
        // 1 channel
        case tr_format_r8_unorm            : result = VK_FORMAT_R8_UNORM; break;
        case tr_format_r16_unorm           : result = VK_FORMAT_R16_UNORM; break;
        case tr_format_r16_float           : result = VK_FORMAT_R16_SFLOAT; break;
        case tr_format_r32_uint            : result = VK_FORMAT_R32_UINT; break;
        case tr_format_r32_float           : result = VK_FORMAT_R32_SFLOAT; break;
        // 2 channel
        case tr_format_r8g8_unorm          : result = VK_FORMAT_R8G8_UNORM; break;
        case tr_format_r16g16_unorm        : result = VK_FORMAT_R16G16_UNORM; break;
        case tr_format_r16g16_float        : result = VK_FORMAT_R16G16_SFLOAT; break;
        case tr_format_r32g32_uint         : result = VK_FORMAT_R32G32_UINT; break;
        case tr_format_r32g32_float        : result = VK_FORMAT_R32G32_SFLOAT; break;
        // 3 channel
        case tr_format_r8g8b8_unorm        : result = VK_FORMAT_R8G8B8_UNORM; break;
        case tr_format_r16g16b16_unorm     : result = VK_FORMAT_R16G16B16_UNORM; break;
        case tr_format_r16g16b16_float     : result = VK_FORMAT_R16G16B16_SFLOAT; break;
        case tr_format_r32g32b32_uint      : result = VK_FORMAT_R32G32B32_UINT; break;
        case tr_format_r32g32b32_float     : result = VK_FORMAT_R32G32B32_SFLOAT; break;
        // 4 channel
        case tr_format_b8g8r8a8_unorm      : result = VK_FORMAT_B8G8R8A8_UNORM; break;
        case tr_format_r8g8b8a8_unorm      : result = VK_FORMAT_R8G8B8A8_UNORM; break;
        case tr_format_r16g16b16a16_unorm  : result = VK_FORMAT_R16G16B16A16_UNORM; break;
        case tr_format_r16g16b16a16_float  : result = VK_FORMAT_R16G16B16A16_SFLOAT; break;
        case tr_format_r32g32b32a32_uint   : result = VK_FORMAT_R32G32B32A32_UINT; break;
        case tr_format_r32g32b32a32_float  : result = VK_FORMAT_R32G32B32A32_SFLOAT; break;
        // Depth/stencil
        case tr_format_d16_unorm           : result = VK_FORMAT_D16_UNORM; break;
        case tr_format_x8_d24_unorm_pack32 : result = VK_FORMAT_X8_D24_UNORM_PACK32; break;
        case tr_format_d32_float           : result = VK_FORMAT_D32_SFLOAT; break;
        case tr_format_s8_uint             : result = VK_FORMAT_S8_UINT; break;
        case tr_format_d16_unorm_s8_uint   : result = VK_FORMAT_D16_UNORM_S8_UINT; break;
        case tr_format_d24_unorm_s8_uint   : result = VK_FORMAT_D24_UNORM_S8_UINT; break;
        case tr_format_d32_float_s8_uint   : result = VK_FORMAT_D32_SFLOAT_S8_UINT; break;
    }
    return result;
}

tr_format tr_util_from_vk_format(VkFormat format)
{
    tr_format result = tr_format_undefined;
    switch (format) {
        // 1 channel
        case VK_FORMAT_R8_UNORM            : result = tr_format_r8_unorm; break;
        case VK_FORMAT_R16_UNORM           : result = tr_format_r16_unorm; break;
        case VK_FORMAT_R16_SFLOAT          : result = tr_format_r16_float; break;
        case VK_FORMAT_R32_UINT            : result = tr_format_r32_uint; break;
        case VK_FORMAT_R32_SFLOAT          : result = tr_format_r32_float; break;
        // 2 channel
        case VK_FORMAT_R8G8_UNORM          : result = tr_format_r8g8_unorm; break;
        case VK_FORMAT_R16G16_UNORM        : result = tr_format_r16g16_unorm; break;
        case VK_FORMAT_R16G16_SFLOAT       : result = tr_format_r16g16_float; break;
        case VK_FORMAT_R32G32_UINT         : result = tr_format_r32g32_uint; break;
        case VK_FORMAT_R32G32_SFLOAT       : result = tr_format_r32g32_float; break;
        // 3 channel
        case VK_FORMAT_R8G8B8_UNORM        : result = tr_format_r8g8b8_unorm; break;
        case VK_FORMAT_R16G16B16_UNORM     : result = tr_format_r16g16b16_unorm; break;
        case VK_FORMAT_R16G16B16_SFLOAT    : result = tr_format_r16g16b16_float; break;
        case VK_FORMAT_R32G32B32_UINT      : result = tr_format_r32g32b32_uint; break;
        case VK_FORMAT_R32G32B32_SFLOAT    : result = tr_format_r32g32b32_float; break;
        // 4 channel
        case VK_FORMAT_B8G8R8A8_UNORM      : result = tr_format_b8g8r8a8_unorm; break;
        case VK_FORMAT_R8G8B8A8_UNORM      : result = tr_format_r8g8b8a8_unorm; break;
        case VK_FORMAT_R16G16B16A16_UNORM  : result = tr_format_r16g16b16a16_unorm; break;
        case VK_FORMAT_R16G16B16A16_SFLOAT : result = tr_format_r16g16b16a16_float; break;
        case VK_FORMAT_R32G32B32A32_UINT   : result = tr_format_r32g32b32a32_uint; break;
        case VK_FORMAT_R32G32B32A32_SFLOAT : result = tr_format_r32g32b32a32_float; break;
        // Depth/stencil
        case VK_FORMAT_D16_UNORM           : result = tr_format_d16_unorm; break;
        case VK_FORMAT_X8_D24_UNORM_PACK32 : result = tr_format_x8_d24_unorm_pack32; break;
        case VK_FORMAT_D32_SFLOAT          : result = tr_format_d32_float; break;
        case VK_FORMAT_S8_UINT             : result = tr_format_s8_uint; break;
        case VK_FORMAT_D16_UNORM_S8_UINT   : result = tr_format_d16_unorm_s8_uint; break;
        case VK_FORMAT_D24_UNORM_S8_UINT   : result = tr_format_d24_unorm_s8_uint; break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT  : result = tr_format_d32_float_s8_uint; break;
    }
    return result;
}

VkShaderStageFlags tr_util_to_vk_shader_stages(tr_shader_stage shader_stages)
{
    VkShaderStageFlags result = 0;
    if (tr_shader_stage_all_graphics == (shader_stages & tr_shader_stage_all_graphics)) {
        result = VK_SHADER_STAGE_ALL_GRAPHICS;
    }
    else {
        if (tr_shader_stage_vert == (shader_stages & tr_shader_stage_vert)) {
            result |= VK_SHADER_STAGE_VERTEX_BIT;
        }
        if (tr_shader_stage_tesc == (shader_stages & tr_shader_stage_tesc)) {
            result |= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
        }
        if (tr_shader_stage_tese == (shader_stages & tr_shader_stage_tese)) {
            result |= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
        }
        if (tr_shader_stage_geom == (shader_stages & tr_shader_stage_geom)) {
            result |= VK_SHADER_STAGE_GEOMETRY_BIT;
        }
        if (tr_shader_stage_frag == (shader_stages & tr_shader_stage_frag)) {
            result |= VK_SHADER_STAGE_FRAGMENT_BIT;
        }
        if (tr_shader_stage_comp == (shader_stages & tr_shader_stage_comp)) {
            result |= VK_SHADER_STAGE_COMPUTE_BIT;
        }
    }
    return result;
}

uint32_t tr_util_format_stride(tr_format format)
{
    uint32_t result = 0;
    switch (format) {
        // 1 channel
        case tr_format_r8_unorm            : result = 1; break;
        case tr_format_r16_unorm           : result = 2; break;
        case tr_format_r16_float           : result = 2; break;
        case tr_format_r32_uint            : result = 4; break;
        case tr_format_r32_float           : result = 4; break;
        // 2 channel
        case tr_format_r8g8_unorm          : result = 2; break;
        case tr_format_r16g16_unorm        : result = 4; break;
        case tr_format_r16g16_float        : result = 4; break;
        case tr_format_r32g32_uint         : result = 8; break;
        case tr_format_r32g32_float        : result = 8; break;
        // 3 channel
        case tr_format_r8g8b8_unorm        : result = 3; break;
        case tr_format_r16g16b16_unorm     : result = 6; break;
        case tr_format_r16g16b16_float     : result = 6; break;
        case tr_format_r32g32b32_uint      : result = 12; break;
        case tr_format_r32g32b32_float     : result = 12; break;
        // 4 channel
        case tr_format_b8g8r8a8_unorm      : result = 4; break;
        case tr_format_r8g8b8a8_unorm      : result = 4; break;
        case tr_format_r16g16b16a16_unorm  : result = 8; break;
        case tr_format_r16g16b16a16_float  : result = 8; break;
        case tr_format_r32g32b32a32_uint   : result = 16; break;
        case tr_format_r32g32b32a32_float  : result = 16; break;
        // Depth/stencil
        case tr_format_d16_unorm           : result = 0; break;
        case tr_format_x8_d24_unorm_pack32 : result = 0; break;
        case tr_format_d32_float           : result = 0; break;
        case tr_format_s8_uint             : result = 0; break;
        case tr_format_d16_unorm_s8_uint   : result = 0; break;
        case tr_format_d24_unorm_s8_uint   : result = 0; break;
        case tr_format_d32_float_s8_uint   : result = 0; break;
    }
    return result;
}

uint32_t tr_util_format_channel_count(tr_format format)
{
    uint32_t result = 0;
    switch (format) {
        // 1 channel
        case tr_format_r8_unorm            : result = 1; break;
        case tr_format_r16_unorm           : result = 1; break;
        case tr_format_r16_float           : result = 1; break;
        case tr_format_r32_uint            : result = 1; break;
        case tr_format_r32_float           : result = 1; break;
        // 2 channel
        case tr_format_r8g8_unorm          : result = 2; break;
        case tr_format_r16g16_unorm        : result = 2; break;
        case tr_format_r16g16_float        : result = 2; break;
        case tr_format_r32g32_uint         : result = 2; break;
        case tr_format_r32g32_float        : result = 2; break;
        // 3 channel
        case tr_format_r8g8b8_unorm        : result = 3; break;
        case tr_format_r16g16b16_unorm     : result = 3; break;
        case tr_format_r16g16b16_float     : result = 3; break;
        case tr_format_r32g32b32_uint      : result = 3; break;
        case tr_format_r32g32b32_float     : result = 3; break;
        // 4 channel
        case tr_format_b8g8r8a8_unorm      : result = 4; break;
        case tr_format_r8g8b8a8_unorm      : result = 4; break;
        case tr_format_r16g16b16a16_unorm  : result = 4; break;
        case tr_format_r16g16b16a16_float  : result = 4; break;
        case tr_format_r32g32b32a32_uint   : result = 4; break;
        case tr_format_r32g32b32a32_float  : result = 4; break;
        // Depth/stencil
        case tr_format_d16_unorm           : result = 0; break;
        case tr_format_x8_d24_unorm_pack32 : result = 0; break;
        case tr_format_d32_float           : result = 0; break;
        case tr_format_s8_uint             : result = 0; break;
        case tr_format_d16_unorm_s8_uint   : result = 0; break;
        case tr_format_d24_unorm_s8_uint   : result = 0; break;
        case tr_format_d32_float_s8_uint   : result = 0; break;
    }
    return result;
}

void tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);
    
    tr_begin_cmd(p_cmd);
    tr_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);
}

void tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_queue);
    assert(NULL != p_texture);

    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);
    
    tr_begin_cmd(p_cmd);
    tr_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);
}

bool tr_image_resize_uint8_t(
    uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* src_data,
    uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, uint8_t* dst_data,
    uint32_t channel_cout, void* user_data
)
{
    float dx = (float)src_width / (float)dst_width;
    float dy = (float)src_height / (float)dst_height;

    const uint8_t src_pixel_stride = channel_cout;
    const uint8_t dst_pixel_stride = channel_cout;

    uint8_t* dst_row = dst_data;
    for (uint32_t y = 0; y < dst_height; ++y) {
        float src_x = 0;
        float src_y = (float)y * dy;
        uint8_t* dst_pixel = dst_row;
        for (uint32_t x = 0; x < dst_width; ++x) {
            const uint32_t src_offset = ((uint32_t)src_y * src_row_stride) + ((uint32_t)src_x * src_pixel_stride);
            const uint8_t* src_pixel = src_data + src_offset;           
            for (uint32_t c = 0; c < channel_cout; ++c) {
                *(dst_pixel + c) = *(src_pixel + c);
            }
            src_x += dx;
            dst_pixel += dst_pixel_stride;
        }
        dst_row += dst_row_stride;
    }

    return true;
}

void tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_counter_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_counter_buffer);
    assert(NULL != p_counter_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_counter_buffer->renderer, tr_buffer_usage_transfer_src, p_counter_buffer->size, true, &buffer);
    uint32_t* mapped_ptr = (uint32_t*)buffer->cpu_mapped_address;
    *(mapped_ptr) = count;
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_counter_buffer, tr_buffer_usage_storage_uav, tr_buffer_usage_transfer_dst);
    TINY_RENDERER_DECLARE_ZERO(VkBufferCopy, region);
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = (VkDeviceSize)4;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_counter_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_counter_buffer, tr_buffer_usage_transfer_dst, tr_buffer_usage_storage_uav);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_counter_buffer->renderer, buffer);
}

void tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->vk_buffer);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, p_buffer->size, true, &buffer);
    memset(buffer->cpu_mapped_address, 0, buffer->size);
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    TINY_RENDERER_DECLARE_ZERO(VkBufferCopy, region);
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = (VkDeviceSize)p_buffer->size;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->vk_buffer);
    assert(p_buffer->size >= size);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, size, true, &buffer);
    memcpy(buffer->cpu_mapped_address, p_src_data, size);
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    TINY_RENDERER_DECLARE_ZERO(VkBufferCopy, region);
    region.srcOffset = 0;
    region.dstOffset = 0;
    region.size      = (VkDeviceSize)size;
    vkCmdCopyBuffer(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_buffer->vk_buffer, 1, &region);
    tr_internal_vk_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, uint32_t src_channel_count, tr_texture* p_texture, tr_image_resize_uint8_fn resize_fn, void* p_user_data)
{
    assert(NULL != p_queue);
    assert(NULL != p_src_data);
    assert(NULL != p_texture);
    assert(NULL != p_texture->vk_image);
    assert((src_width > 0) && (src_height > 0) && (src_row_stride > 0));
    assert(tr_sample_count_1 == p_texture->sample_count);

    uint8_t* p_expanded_src_data = NULL;
    const uint32_t dst_channel_count = tr_util_format_channel_count(p_texture->format);
    assert(src_channel_count < dst_channel_count);

    if (src_channel_count < dst_channel_count) {
        uint32_t expanded_row_stride = src_width * dst_channel_count;
        uint32_t expanded_size = expanded_row_stride * src_height;
        p_expanded_src_data = (uint8_t*)calloc(1, expanded_size);
        assert(NULL != p_expanded_src_data);

        const uint32_t src_pixel_stride = src_channel_count;
        const uint32_t expanded_pixel_stride = dst_channel_count;
        const uint8_t* src_row = p_src_data;
        uint8_t* expanded_row = p_expanded_src_data;
        for (uint32_t y = 0; y < src_height; ++y) {
            const uint8_t* src_pixel = src_row;
            uint8_t* expanded_pixel = expanded_row;
            for (uint32_t x = 0; x < src_width; ++x) {
                uint32_t c = 0; 
                for (; c < src_channel_count; ++c) {
                    *(expanded_pixel + c) = *(src_pixel + c);
                }
                for (; c < dst_channel_count; ++c) {
                    *(expanded_pixel + c) = 0xFF;
                }
                src_pixel += src_pixel_stride;
                expanded_pixel += expanded_pixel_stride;
            }
            src_row += src_row_stride;
            expanded_row += expanded_row_stride;
        }
        src_row_stride = expanded_row_stride;
        src_channel_count = dst_channel_count;
        p_src_data = p_expanded_src_data;
    }

    // Get memory requirements that covers all mip levels
    TINY_RENDERER_DECLARE_ZERO(VkMemoryRequirements, mem_reqs);
    vkGetImageMemoryRequirements(p_texture->renderer->vk_device, p_texture->vk_image, &mem_reqs);
    // Create temporary buffer big enough to fit all mip levels
    tr_buffer* buffer = NULL;
    tr_create_buffer(p_texture->renderer, tr_buffer_usage_transfer_src, mem_reqs.size, true, &buffer);
    //
    // If you're coming from D3D12, you might want to do something like:
    //
    //   // Get resource layout for all mip levels
    //   VkSubresourceLayout* subres_layouts = (VkSubresourceLayout*)calloc(p_texture->mip_levels, sizeof(*subres_layouts));
    //   assert(NULL != subres_layouts);
    //   VkImageAspectFlags aspect_mask = tr_util_vk_determine_aspect_mask(tr_util_to_vk_format(p_texture->format));
    //   for (uint32_t i = 0; i < p_texture->mip_levels; ++i) {
    //       TINY_RENDERER_DECLARE_ZERO(VkImageSubresource, img_sub_res);
    //       img_sub_res.aspectMask = aspect_mask;
    //       img_sub_res.mipLevel   = i;
    //       img_sub_res.arrayLayer = 0;
    //       vkGetImageSubresourceLayout(p_texture->renderer->vk_device, p_texture->vk_image, &img_sub_res, &(subres_layouts[i]));
    //   } 
    //
    // ...unfortunately, in Vulkan this approach may be slightly 
    // problematic, because you can only call vkGetImageSubresourceLayout on
    // an image that was created with VK_IMAGE_TILING_LINEAR. The validation
    // layers will issue an error if vkGetImageSubresourceLayout get called
    // on an image created with VK_IMAGE_TILING_OPTIMAL. For now, since the
    // total size of the memory is correct, we'll just calculate the row pitch
    // and offset for each mip level manually.
    // 
    
    // Use default simple resize if a resize function was not supplied
    if (NULL == resize_fn) {
        resize_fn = &tr_image_resize_uint8_t;
    }
    // Resize image into appropriate mip level
    uint32_t dst_width = p_texture->width;
    uint32_t dst_height = p_texture->height;
    VkDeviceSize buffer_offset = 0;
    for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
        uint32_t dst_row_stride = src_row_stride >> mip_level;
        uint8_t* p_dst_data = (uint8_t*)buffer->cpu_mapped_address + buffer_offset;
        resize_fn(src_width, src_height, src_row_stride, p_src_data, dst_width, dst_height, dst_row_stride, p_dst_data, dst_channel_count, p_user_data);
        buffer_offset += dst_row_stride * dst_height;
        dst_width >>= 1;
        dst_height >>= 1;
    }

    // Copy buffer to texture
    buffer_offset = 0;
    VkFormat format = tr_util_to_vk_format(p_texture->format);
    VkImageAspectFlags aspect_mask = tr_util_vk_determine_aspect_mask(format);
    {
        const uint32_t region_count = p_texture->mip_levels;
        VkBufferImageCopy* regions = (VkBufferImageCopy*)calloc(region_count, sizeof(*regions));
        assert(NULL != regions);

        dst_width = p_texture->width;
        dst_height = p_texture->height;
        for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
            regions[mip_level].bufferOffset                    = buffer_offset;
            regions[mip_level].bufferRowLength                 = dst_width;
            regions[mip_level].bufferImageHeight               = dst_height;
            regions[mip_level].imageSubresource.aspectMask     = aspect_mask;
            regions[mip_level].imageSubresource.mipLevel       = mip_level;
            regions[mip_level].imageSubresource.baseArrayLayer = 0;
            regions[mip_level].imageSubresource.layerCount     = 1;
            regions[mip_level].imageOffset.x                   = 0;
            regions[mip_level].imageOffset.y                   = 0;
            regions[mip_level].imageOffset.z                   = 0;
            regions[mip_level].imageExtent.width               = dst_width;
            regions[mip_level].imageExtent.height              = dst_height;
            regions[mip_level].imageExtent.depth               = 1;
            buffer_offset += (src_row_stride >> mip_level) * dst_height;
            dst_width >>= 1;
            dst_height >>= 1;
        }
        
        tr_cmd_pool* p_cmd_pool = NULL;
        tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

        tr_cmd* p_cmd = NULL;
        tr_create_cmd(p_cmd_pool, false, &p_cmd);

        tr_begin_cmd(p_cmd);
        //
        // Vulkan textures are created with VK_IMAGE_LAYOUT_UNDEFFINED (tr_texture_usage_undefined)
        //
        tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_undefined, tr_texture_usage_transfer_dst);
        vkCmdCopyBufferToImage(p_cmd->vk_cmd_buf, buffer->vk_buffer, p_texture->vk_image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, region_count, regions);
        tr_internal_vk_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_transfer_dst, tr_texture_usage_sampled_image);
        tr_end_cmd(p_cmd);

        tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
        tr_queue_wait_idle(p_queue);

        tr_destroy_cmd(p_cmd_pool, p_cmd);
        tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

        tr_destroy_buffer(p_texture->renderer, buffer);

        TINY_RENDERER_SAFE_FREE(regions);
    }

    TINY_RENDERER_SAFE_FREE(p_expanded_src_data);
}

void tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data)
{
}

// -------------------------------------------------------------------------------------------------
// Internal utility functions
// -------------------------------------------------------------------------------------------------
VkSampleCountFlagBits tr_util_to_vk_sample_count(tr_sample_count sample_count) 
{
    VkSampleCountFlagBits result = VK_SAMPLE_COUNT_1_BIT;
    switch (sample_count) {
        case tr_sample_count_1  : result = VK_SAMPLE_COUNT_1_BIT;  break;
        case tr_sample_count_2  : result = VK_SAMPLE_COUNT_2_BIT;  break;
        case tr_sample_count_4  : result = VK_SAMPLE_COUNT_4_BIT;  break;
        case tr_sample_count_8  : result = VK_SAMPLE_COUNT_8_BIT;  break;
        case tr_sample_count_16 : result = VK_SAMPLE_COUNT_16_BIT; break;
    }
    return result;
}

VkBufferUsageFlags tr_util_to_vk_buffer_usage(tr_buffer_usage usage)
{
    VkBufferUsageFlags result = 0;
    if (tr_buffer_usage_index == (usage & tr_buffer_usage_index)) {
        result |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }
    if (tr_buffer_usage_vertex == (usage & tr_buffer_usage_vertex)) {
        result |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }
    if (tr_buffer_usage_indirect == (usage & tr_buffer_usage_indirect)) {
        result |= VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
    }
    if (tr_buffer_usage_transfer_src == (usage & tr_buffer_usage_transfer_src)) {
        result |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }
    if (tr_buffer_usage_transfer_dst == (usage & tr_buffer_usage_transfer_dst)) {
        result |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }
    if (tr_buffer_usage_uniform_cbv == (usage & tr_buffer_usage_uniform_cbv)) {
        result |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }
    if (tr_buffer_usage_storage_srv == (usage & tr_buffer_usage_storage_srv)) {
        result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (tr_buffer_usage_storage_uav == (usage & tr_buffer_usage_storage_uav)) {
        result |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    }
    if (tr_buffer_usage_uniform_texel_srv == (usage & tr_buffer_usage_uniform_texel_srv)) {
        result |= VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT;
    }
    if (tr_buffer_usage_storage_texel_uav == (usage & tr_buffer_usage_storage_texel_uav)) {
        result |= VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT;
    }
    return result;
}

VkImageUsageFlags tr_util_to_vk_image_usage(tr_texture_usage_flags usage)
{
    VkImageUsageFlags result = 0;
    if (tr_texture_usage_transfer_src == (usage & tr_texture_usage_transfer_src)) {
        result |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    if (tr_texture_usage_transfer_dst == (usage & tr_texture_usage_transfer_dst)) {
        result |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }
    if (tr_texture_usage_sampled_image == (usage & tr_texture_usage_sampled_image)) {
        result |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    if (tr_texture_usage_storage_image == (usage & tr_texture_usage_storage_image)) {
        result |= VK_IMAGE_USAGE_STORAGE_BIT;
    }
    if (tr_texture_usage_color_attachment == (usage & tr_texture_usage_color_attachment)) {
        result |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
    if (tr_texture_usage_depth_stencil_attachment == (usage & tr_texture_usage_depth_stencil_attachment)) {
        result |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return result;
}


VkImageLayout tr_util_to_vk_image_layout(tr_texture_usage usage)
{
    VkImageLayout result = VK_IMAGE_LAYOUT_UNDEFINED;
    switch(usage) {
        case tr_texture_usage_transfer_src             : result = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
        case tr_texture_usage_transfer_dst             : result = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
        case tr_texture_usage_sampled_image            : result = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
        case tr_texture_usage_storage_image            : result = VK_IMAGE_LAYOUT_GENERAL; break;
        case tr_texture_usage_color_attachment         : result = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; break;
        case tr_texture_usage_depth_stencil_attachment : result = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
        case tr_texture_usage_present                  : result = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; break;
    }
    return result;
}

VkImageAspectFlags tr_util_vk_determine_aspect_mask(VkFormat format)
{
    VkImageAspectFlags result = 0;
    switch( format ) {
        // Depth
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
        // Stencil
        case VK_FORMAT_S8_UINT:
            result = VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
        // Depth/stencil
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            result = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
        // Assume everything else is Color
        default:
            result = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
    }
    return result;
}

bool tr_util_vk_get_memory_type(const VkPhysicalDeviceMemoryProperties* mem_props, uint32_t type_bits, VkMemoryPropertyFlags flags, uint32_t* p_index)
{
    bool found = false; 
    for (uint32_t i = 0; i < mem_props->memoryTypeCount; ++i) {
        if (type_bits & 1) {
            if (flags == (mem_props->memoryTypes[i].propertyFlags & flags)) {
                if (p_index) {
                    *p_index = i;
                }
                found = true;
                break;
            }
        }
        type_bits >>= 1;
    }
    return found;
}

VkFormatFeatureFlags tr_util_vk_image_usage_to_format_features(VkImageUsageFlags usage)
{
    VkFormatFeatureFlags result = (VkFormatFeatureFlags)0;
    if (VK_IMAGE_USAGE_SAMPLED_BIT== (usage & VK_IMAGE_USAGE_SAMPLED_BIT)) {
        result |= VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT;
    }
    if (VK_IMAGE_USAGE_STORAGE_BIT== (usage & VK_IMAGE_USAGE_STORAGE_BIT)) {
        result |= VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT;
    }
    if (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT== (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)) {
        result |= VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT;
    }
    if (VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT== (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
        result |= VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
// Internal init functions
// -------------------------------------------------------------------------------------------------
bool tr_internal_vk_find_queue_family(VkPhysicalDevice gpu, const VkQueueFlags queue_flags, uint32_t* p_queue_family_index, VkQueueFamilyProperties* p_queue_family_properties )
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
    if (0 == count) {
        return false;
    }

    VkQueueFamilyProperties* properties = (VkQueueFamilyProperties*)calloc(count, sizeof(*properties));
    assert(NULL != properties);

    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, properties);

    VkBool32 found = VK_FALSE;
    for (uint32_t index = 0; 0 < count; ++index) {
        if (queue_flags == (properties[index].queueFlags & queue_flags)) {
            found = VK_TRUE;
            if (NULL != p_queue_family_index) {
                *p_queue_family_index = index;
            }
            if (NULL != p_queue_family_properties) {
                memcpy(p_queue_family_properties, &properties[index], sizeof(*p_queue_family_properties));
            }
            break;
        }
    }

    TINY_RENDERER_SAFE_FREE(properties);

    return (VK_TRUE == found) ?  true : false;
}

bool tr_internal_vk_find_present_queue_family(VkPhysicalDevice gpu, VkSurfaceKHR surface, uint32_t* p_queue_family_index)
{
    uint32_t count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
    if (0 == count) {
        return false;
    }

    VkBool32 found = VK_FALSE;
    for (uint32_t index = 0; index < count; ++index) {
        VkBool32 supports_present = VK_FALSE;
        VkResult res = vkGetPhysicalDeviceSurfaceSupportKHR(gpu, index, surface, &supports_present);
        if ((VK_SUCCESS == res) && (VK_TRUE == supports_present)) {
            found = VK_TRUE;
            if (NULL != p_queue_family_index) {
                *p_queue_family_index = index;
            }
            break;
        }
    }

    return (VK_TRUE == found) ?  true : false;
}

void tr_internal_vk_create_instance(const char* app_name, tr_renderer* p_renderer)
{
    uint32_t count = 0;
    VkLayerProperties layers[100];
    VkExtensionProperties exts[100];
    vkEnumerateInstanceLayerProperties(&count, NULL);
    vkEnumerateInstanceLayerProperties(&count, layers);
    for (uint32_t i =0; i < count; ++i) {
        tr_internal_log(tr_log_type_info, layers[i].layerName, "vkinstance-layer");
    }
    vkEnumerateInstanceExtensionProperties(NULL, &count, NULL);
    vkEnumerateInstanceExtensionProperties(NULL, &count, exts);
    for (uint32_t i =0; i < count; ++i) {
        tr_internal_log(tr_log_type_info, exts[i].extensionName, "vkinstance-ext");
    }
    
    TINY_RENDERER_DECLARE_ZERO(VkApplicationInfo, app_info);
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = NULL;
    app_info.pApplicationName   = app_name;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName        = "tinyvk";
    app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 3);

    // Instance
    {
        // Extensions
        uint32_t extension_count = 0;
        const char* extensions[tr_max_instance_extensions] = { 0 };
        // Copy extensions if they're present
        if (p_renderer->settings.device_extensions.count > 0) {
          for (; extension_count < p_renderer->settings.instance_extensions.count; ++extension_count) {
            extensions[extension_count] = p_renderer->settings.instance_extensions.names[extension_count];
          }
        }
        else {
          // Use default extensions
          extensions[extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
#if defined(TINY_RENDERER_LINUX)
          extensions[extension_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(TINY_RENDERER_MSW)
          extensions[extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#endif
        }

        for (uint32_t i = 0; i < p_renderer->settings.instance_layers.count; ++i) {
          if (extension_count >= tr_max_instance_extensions) {
            break;
          }

          int cmp = strcmp(p_renderer->settings.instance_layers.names[i], "VK_LAYER_LUNARG_standard_validation");
          if (cmp == 0) {
            extensions[extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
          }
        }

        TINY_RENDERER_DECLARE_ZERO(VkInstanceCreateInfo, create_info);
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pNext                   = NULL;
        create_info.flags                   = 0;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledLayerCount       = p_renderer->settings.instance_layers.count;
        create_info.ppEnabledLayerNames     = p_renderer->settings.instance_layers.names;
        create_info.enabledExtensionCount   = extension_count;
        create_info.ppEnabledExtensionNames = extensions;
        VkResult vk_res = vkCreateInstance(&create_info, NULL, &(p_renderer->vk_instance));
        assert(VK_SUCCESS == vk_res);
    }

    // Debug
    {
        trVkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkCreateDebugReportCallbackEXT");
        trVkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkDestroyDebugReportCallbackEXT");
        trVkDebugReportMessageEXT         = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkDebugReportMessageEXT");

        if ((NULL != trVkCreateDebugReportCallbackEXT) && (NULL != trVkDestroyDebugReportCallbackEXT) && (NULL != trVkDebugReportMessageEXT)) {
            TINY_RENDERER_DECLARE_ZERO(VkDebugReportCallbackCreateInfoEXT, create_info);
            create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
            create_info.pNext       = NULL;
            create_info.pfnCallback = tr_internal_debug_report_callback;
            create_info.pUserData   = NULL;
            create_info.flags       = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | 
                                      VK_DEBUG_REPORT_WARNING_BIT_EXT | 
                                      VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
                                      VK_DEBUG_REPORT_ERROR_BIT_EXT |
                                      VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            VkResult res = trVkCreateDebugReportCallbackEXT(p_renderer->vk_instance, &create_info, NULL, &(p_renderer->vk_debug_report));
            if (VK_SUCCESS != res) {
                tr_internal_log(tr_log_type_error, "vkCreateDebugReportCallbackEXT failed - disabling Vulkan debug callbacks", "tr_internal_vk_init_instance");
                trVkCreateDebugReportCallbackEXT  = NULL;
                trVkDestroyDebugReportCallbackEXT = NULL;
                trVkDebugReportMessageEXT         = NULL;
            }
        }
    }
}

void tr_internal_vk_create_surface(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_instance);

#if defined(TINY_RENDERER_LINUX)
    TINY_RENDERER_DECLARE_ZERO(VkXcbSurfaceCreateInfoKHR, create_info);
    create_info.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    create_info.pNext      = NULL;
    create_info.flags      = 0;
    create_info.connection = p_renderer->settings.handle.connection;
    create_info.window     = p_renderer->settings.handle.window;
    VkResult vk_res = vkCreateXcbSurfaceKHR(p_renderer->vk_instance, &create_info, NULL, &(p_renderer->vk_surface));
    assert(VK_SUCCESS == vk_res);
#elif defined(TINY_RENDERER_MSW)
    TINY_RENDERER_DECLARE_ZERO(VkWin32SurfaceCreateInfoKHR, create_info);
    create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    create_info.pNext     = NULL;
    create_info.flags     = 0;
    create_info.hinstance = p_renderer->settings.handle.hinstance;
    create_info.hwnd      = p_renderer->settings.handle.hwnd;
    VkResult vk_res = vkCreateWin32SurfaceKHR(p_renderer->vk_instance, &create_info, NULL, &(p_renderer->vk_surface));
    assert(VK_SUCCESS == vk_res);
#endif
}

void tr_internal_vk_create_device(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_instance);

    VkResult vk_res = vkEnumeratePhysicalDevices(p_renderer->vk_instance, &(p_renderer->vk_gpu_count), NULL);
    assert(VK_SUCCESS == vk_res);
    assert(p_renderer->vk_gpu_count > 0);

    if (p_renderer->vk_gpu_count > tr_max_gpus) {
        p_renderer->vk_gpu_count = tr_max_gpus;
    }

    vk_res = vkEnumeratePhysicalDevices(p_renderer->vk_instance, &(p_renderer->vk_gpu_count), p_renderer->vk_gpus);
    assert(VK_SUCCESS == vk_res);

    // Find gpu that supports both graphics and present
    p_renderer->vk_active_gpu_index = UINT32_MAX;
    for (uint32_t gpu_index = 0; gpu_index < p_renderer->vk_gpu_count; ++gpu_index) {
        VkPhysicalDevice gpu = p_renderer->vk_gpus[gpu_index];

        // Make sure GPU supports graphics queue
        uint32_t graphics_queue_family_index = UINT32_MAX;
        if (! tr_internal_vk_find_queue_family(gpu, VK_QUEUE_GRAPHICS_BIT, &graphics_queue_family_index, NULL)) {
            continue;
        }

        // Make sure GPU supports present
        uint32_t present_queue_family_index = UINT32_MAX;
        if (! tr_internal_vk_find_present_queue_family(gpu, p_renderer->vk_surface, &present_queue_family_index)) {
            continue;
        }
            
        if ((UINT32_MAX != graphics_queue_family_index) && (UINT32_MAX != present_queue_family_index)) {
            p_renderer->vk_active_gpu = gpu;
            p_renderer->vk_active_gpu_index = gpu_index;
            p_renderer->graphics_queue->vk_queue_family_index = graphics_queue_family_index;
            p_renderer->present_queue->vk_queue_family_index = present_queue_family_index;
            break;
        }
    }

    // This can happen on systems with multiple GPUs (like laptops) and Vulkan decided
    // to pick the integrated GPU instead of the discrete one.
    assert(VK_NULL_HANDLE != p_renderer->vk_active_gpu);

    uint32_t count = 0;
    VkExtensionProperties exts[100];
    vkEnumerateDeviceExtensionProperties(p_renderer->vk_active_gpu, NULL, &count, NULL);
    vkEnumerateDeviceExtensionProperties(p_renderer->vk_active_gpu, NULL, &count, exts);
    for (uint32_t i =0; i < count; ++i) {
        tr_internal_log(tr_log_type_info, exts[i].extensionName, "vkdevice-ext");
    }

    // Get memory properties
    vkGetPhysicalDeviceMemoryProperties(p_renderer->vk_active_gpu, &(p_renderer->vk_memory_properties));

    // Get device properties
    vkGetPhysicalDeviceProperties(p_renderer->vk_active_gpu, &(p_renderer->vk_active_gpu_properties));

    float queue_priorites[1] = {1.0f};
    uint32_t queue_create_infos_count = 1;
    TINY_RENDERER_DECLARE_ZERO(VkDeviceQueueCreateInfo, queue_create_infos[2]);
    queue_create_infos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_infos[0].pNext            = NULL;
    queue_create_infos[0].flags            = 0;
    queue_create_infos[0].queueFamilyIndex = p_renderer->graphics_queue->vk_queue_family_index;
    queue_create_infos[0].queueCount       = 1;
    queue_create_infos[0].pQueuePriorities = queue_priorites;
    if (p_renderer->graphics_queue->vk_queue_family_index != p_renderer->present_queue->vk_queue_family_index) {
        queue_create_infos_count               = 2;
        queue_create_infos[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[1].pNext            = NULL;
        queue_create_infos[1].flags            = 0;
        queue_create_infos[1].queueFamilyIndex = p_renderer->present_queue->vk_queue_family_index;
        queue_create_infos[1].queueCount       = 1;
        queue_create_infos[1].pQueuePriorities = queue_priorites;
    }

    // Device extensions
    uint32_t extension_count = 0;
    const char* extensions[tr_max_instance_extensions] = { 0 };
    // Copy extensions if they're present
    if (p_renderer->settings.device_extensions.count > 0) {
      for (; extension_count < p_renderer->settings.device_extensions.count; ++extension_count) {
        extensions[extension_count] = p_renderer->settings.device_extensions.names[extension_count];
      }
    }
    else {
      // Use default extensions
      extensions[extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
      extensions[extension_count++] = VK_KHR_MAINTENANCE1_EXTENSION_NAME;
    }

    VkPhysicalDeviceFeatures gpu_features = { 0 };
    vkGetPhysicalDeviceFeatures(p_renderer->vk_active_gpu, &gpu_features);
    gpu_features.multiViewport  = VK_FALSE;
    gpu_features.geometryShader = VK_TRUE;
        
    TINY_RENDERER_DECLARE_ZERO(VkDeviceCreateInfo, create_info);
    create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    create_info.pNext                   = NULL;
    create_info.flags                   = 0;
    create_info.queueCreateInfoCount    = queue_create_infos_count;
    create_info.pQueueCreateInfos       = queue_create_infos;
    create_info.enabledLayerCount       = 0;
    create_info.ppEnabledLayerNames     = NULL;
    create_info.enabledExtensionCount   = extension_count;
    create_info.ppEnabledExtensionNames = extensions;
    create_info.pEnabledFeatures        = &gpu_features;
    vk_res = vkCreateDevice(p_renderer->vk_active_gpu, &create_info, NULL, &(p_renderer->vk_device));
    assert(VK_SUCCESS == vk_res);

    vkGetDeviceQueue(p_renderer->vk_device, p_renderer->graphics_queue->vk_queue_family_index, 0, &(p_renderer->graphics_queue->vk_queue));
    assert(VK_NULL_HANDLE != p_renderer->graphics_queue->vk_queue);

    vkGetDeviceQueue(p_renderer->vk_device, p_renderer->present_queue->vk_queue_family_index, 0, &(p_renderer->present_queue->vk_queue));
    assert(VK_NULL_HANDLE != p_renderer->present_queue->vk_queue);
}

void tr_internal_vk_create_swapchain(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_active_gpu);

    // Most GPUs will not go beyond VK_SAMPLE_COUNT_8_BIT
    assert(0 != (p_renderer->vk_active_gpu_properties.limits.framebufferColorSampleCounts & p_renderer->settings.swapchain.sample_count));

    // Image count
    {
        if (0 == p_renderer->settings.swapchain.image_count) {
            p_renderer->settings.swapchain.image_count = 2;
        }

        TINY_RENDERER_DECLARE_ZERO(VkSurfaceCapabilitiesKHR, caps);
        VkResult vk_res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &caps);
        assert(VK_SUCCESS == vk_res);

        if (p_renderer->settings.swapchain.image_count < caps.minImageCount) {
          p_renderer->settings.swapchain.image_count = caps.minImageCount;
        }

        if ((caps.maxImageCount > 0) && (p_renderer->settings.swapchain.image_count > caps.maxImageCount)) {
            p_renderer->settings.swapchain.image_count = caps.maxImageCount;
        }
    }

    // Surface format
    TINY_RENDERER_DECLARE_ZERO(VkSurfaceFormatKHR, surface_format);
    surface_format.format = VK_FORMAT_UNDEFINED;
    {
        uint32_t count = 0;
        VkSurfaceFormatKHR* formats = NULL;
    
        // Get surface formats count
        VkResult vk_res = vkGetPhysicalDeviceSurfaceFormatsKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, NULL);
        assert(VK_SUCCESS == vk_res);
        
        // Allocate and get surface formats
        formats = (VkSurfaceFormatKHR*)calloc(count, sizeof(*formats));
        vk_res = vkGetPhysicalDeviceSurfaceFormatsKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, formats);
        assert(VK_SUCCESS == vk_res);

        if ((1 == count) && (VK_FORMAT_UNDEFINED == formats[0].format)) {
            surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
            surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        }
        else {
            VkFormat requested_format = tr_util_to_vk_format(p_renderer->settings.swapchain.color_format);
            for (uint32_t i = 0; i  < count; ++i) {
                if ((requested_format ==  formats[i].format) && (VK_COLOR_SPACE_SRGB_NONLINEAR_KHR == formats[i].colorSpace)) {
                    surface_format.format = requested_format;
                    surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
                    break;
                }
            }

            // Default to VK_FORMAT_B8G8R8A8_UNORM if requested format isn't found
            if (VK_FORMAT_UNDEFINED == surface_format.format) {
                surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
                surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
            }
        }

        assert(VK_FORMAT_UNDEFINED != surface_format.format);

        // Free formats
        TINY_RENDERER_SAFE_FREE(formats);
    }

    // Present modes
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    {
        uint32_t count = 0;
        VkPresentModeKHR* modes = NULL;
        // Get present mode count
        VkResult vk_res = vkGetPhysicalDeviceSurfacePresentModesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, NULL);
        assert(VK_SUCCESS == vk_res);

        // Allocate and get present modes
        modes = (VkPresentModeKHR*)calloc(count, sizeof(*modes));
        vk_res = vkGetPhysicalDeviceSurfacePresentModesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, modes);
        assert(VK_SUCCESS == vk_res);

        for (uint32_t i = 0; i < count; ++i) {
            if (VK_PRESENT_MODE_IMMEDIATE_KHR == modes[i]) {
                present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                break;
            }
        }

        // Free modes
        TINY_RENDERER_SAFE_FREE(modes);
    }

    // Swapchain
    {
        VkExtent2D extent = {0};
        extent.width = p_renderer->settings.width;
        extent.height = p_renderer->settings.height;

        VkSharingMode sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
        uint32_t queue_family_index_count = 0;
        uint32_t queue_family_indices[2] = {0};
        if (p_renderer->graphics_queue->vk_queue_family_index != p_renderer->present_queue->vk_queue_family_index) {
            sharing_mode = VK_SHARING_MODE_CONCURRENT;
            queue_family_index_count = 2;
            queue_family_indices[0] = p_renderer->graphics_queue->vk_queue_family_index;
            queue_family_indices[1] = p_renderer->present_queue->vk_queue_family_index;
        }

        TINY_RENDERER_DECLARE_ZERO(VkSwapchainCreateInfoKHR, create_info);
        create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        create_info.pNext                 = NULL;
        create_info.flags                 = 0;
        create_info.surface               = p_renderer->vk_surface;
        create_info.minImageCount         = p_renderer->settings.swapchain.image_count;
        create_info.imageFormat           = surface_format.format;
        create_info.imageColorSpace       = surface_format.colorSpace;
        create_info.imageExtent           = extent;
        create_info.imageArrayLayers      = 1;
        create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        create_info.imageSharingMode      = sharing_mode;
        create_info.queueFamilyIndexCount = queue_family_index_count;
        create_info.pQueueFamilyIndices   = queue_family_indices;
        create_info.preTransform          = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        create_info.compositeAlpha        = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        create_info.presentMode           = present_mode;
        create_info.clipped               = VK_TRUE;
        create_info.oldSwapchain          = NULL;
        VkResult vk_res = vkCreateSwapchainKHR(p_renderer->vk_device, &create_info, NULL, &(p_renderer->vk_swapchain));
        assert(VK_SUCCESS == vk_res);

        p_renderer->settings.swapchain.color_format = tr_util_from_vk_format(surface_format.format);

        // Make sure depth/stencil format is supported - fall back to VK_FORMAT_D16_UNORM if not
        VkFormat vk_depth_stencil_format = tr_util_to_vk_format(p_renderer->settings.swapchain.depth_stencil_format);
        if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
            TINY_RENDERER_DECLARE_ZERO(VkImageFormatProperties, properties);
            vk_res = vkGetPhysicalDeviceImageFormatProperties(p_renderer->vk_active_gpu, 
                                                           vk_depth_stencil_format,
                                                           VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                                                           0, 
                                                           &properties);
            // Fall back to something that's guaranteed to work
            if (VK_SUCCESS != vk_res) {
                p_renderer->settings.swapchain.depth_stencil_format = tr_format_d16_unorm;
            }
        }
    }
}

void tr_internal_create_swapchain_renderpass(tr_renderer* p_renderer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    p_renderer->swapchain_render_targets = (tr_render_target**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*p_renderer->swapchain_render_targets));
    assert(NULL != p_renderer->swapchain_render_targets);

    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        p_renderer->swapchain_render_targets[i] = (tr_render_target*)calloc(1, sizeof(*(p_renderer->swapchain_render_targets[i])));
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->renderer               = p_renderer;
        render_target->width                  = p_renderer->settings.width;
        render_target->height                 = p_renderer->settings.height;
        render_target->sample_count           = (tr_sample_count)p_renderer->settings.swapchain.sample_count;
        render_target->color_format           = p_renderer->settings.swapchain.color_format;
        render_target->color_attachment_count = 1;
        render_target->depth_stencil_format   = p_renderer->settings.swapchain.depth_stencil_format;

        render_target->color_attachments[0] = (tr_texture*)calloc(1, sizeof(*render_target->color_attachments[0]));
        assert(NULL != render_target->color_attachments[0]);

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
            render_target->color_attachments_multisample[0] = (tr_texture*)calloc(1, sizeof(*render_target->color_attachments_multisample[0]));
            assert(NULL != render_target->color_attachments_multisample[0]);
        }

        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
            render_target->depth_stencil_attachment = (tr_texture*)calloc(1, sizeof(*render_target->depth_stencil_attachment));
            assert(NULL != render_target->depth_stencil_attachment);

            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
                render_target->depth_stencil_attachment_multisample = (tr_texture*)calloc(1, sizeof(*render_target->depth_stencil_attachment_multisample));
                assert(NULL != render_target->depth_stencil_attachment_multisample);
            }
        }
    }

    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->color_attachments[0]->type          = tr_texture_type_2d;
        render_target->color_attachments[0]->usage         = (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_present);
        render_target->color_attachments[0]->width         = p_renderer->settings.width;
        render_target->color_attachments[0]->height        = p_renderer->settings.height;
        render_target->color_attachments[0]->depth         = 1;
        render_target->color_attachments[0]->format        = p_renderer->settings.swapchain.color_format;
        render_target->color_attachments[0]->mip_levels    = 1;
        render_target->color_attachments[0]->clear_value.r = p_renderer->settings.swapchain.color_clear_value.r;
        render_target->color_attachments[0]->clear_value.g = p_renderer->settings.swapchain.color_clear_value.g;
        render_target->color_attachments[0]->clear_value.b = p_renderer->settings.swapchain.color_clear_value.b;
        render_target->color_attachments[0]->clear_value.a = p_renderer->settings.swapchain.color_clear_value.a;
        render_target->color_attachments[0]->sample_count  = tr_sample_count_1;

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
            render_target->color_attachments_multisample[0]->type          = tr_texture_type_2d;
            render_target->color_attachments_multisample[0]->usage         = (tr_texture_usage)(tr_texture_usage_color_attachment | tr_texture_usage_sampled_image);
            render_target->color_attachments_multisample[0]->width         = p_renderer->settings.width;
            render_target->color_attachments_multisample[0]->height        = p_renderer->settings.height;
            render_target->color_attachments_multisample[0]->depth         = 1;
            render_target->color_attachments_multisample[0]->format        = p_renderer->settings.swapchain.color_format;
            render_target->color_attachments_multisample[0]->mip_levels    = 1;
            render_target->color_attachments_multisample[0]->clear_value.r = p_renderer->settings.swapchain.color_clear_value.r;
            render_target->color_attachments_multisample[0]->clear_value.g = p_renderer->settings.swapchain.color_clear_value.g;
            render_target->color_attachments_multisample[0]->clear_value.b = p_renderer->settings.swapchain.color_clear_value.b;
            render_target->color_attachments_multisample[0]->clear_value.a = p_renderer->settings.swapchain.color_clear_value.a;
            render_target->color_attachments_multisample[0]->sample_count  = render_target->sample_count;
        }

        if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
            render_target->depth_stencil_attachment->type                = tr_texture_type_2d;
            render_target->depth_stencil_attachment->usage               = (tr_texture_usage)(tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image);
            render_target->depth_stencil_attachment->width               = p_renderer->settings.width;
            render_target->depth_stencil_attachment->height              = p_renderer->settings.height;
            render_target->depth_stencil_attachment->depth               = 1;
            render_target->depth_stencil_attachment->format              = p_renderer->settings.swapchain.depth_stencil_format;
            render_target->depth_stencil_attachment->mip_levels          = 1;
            render_target->depth_stencil_attachment->clear_value.depth   = p_renderer->settings.swapchain.depth_stencil_clear_value.depth;
            render_target->depth_stencil_attachment->clear_value.stencil = p_renderer->settings.swapchain.depth_stencil_clear_value.stencil;
            render_target->depth_stencil_attachment->sample_count        = tr_sample_count_1;
            
            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
              render_target->depth_stencil_attachment_multisample->type                = tr_texture_type_2d;
              render_target->depth_stencil_attachment_multisample->usage               = (tr_texture_usage)(tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image);
              render_target->depth_stencil_attachment_multisample->width               = p_renderer->settings.width;
              render_target->depth_stencil_attachment_multisample->height              = p_renderer->settings.height;
              render_target->depth_stencil_attachment_multisample->depth               = 1;
              render_target->depth_stencil_attachment_multisample->format              = p_renderer->settings.swapchain.depth_stencil_format;
              render_target->depth_stencil_attachment_multisample->mip_levels          = 1;
              render_target->depth_stencil_attachment_multisample->clear_value.depth   = p_renderer->settings.swapchain.depth_stencil_clear_value.depth;
              render_target->depth_stencil_attachment_multisample->clear_value.stencil = p_renderer->settings.swapchain.depth_stencil_clear_value.stencil;
              render_target->depth_stencil_attachment_multisample->sample_count        = render_target->sample_count;            
            }
        }
    }
}

void tr_internal_vk_create_swapchain_renderpass(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    uint32_t image_count = 0;
    VkResult vk_res = vkGetSwapchainImagesKHR(p_renderer->vk_device, p_renderer->vk_swapchain, &image_count, NULL);
    assert(VK_SUCCESS == vk_res);

    assert(image_count == p_renderer->settings.swapchain.image_count);

    VkImage* swapchain_images = (VkImage*)calloc(image_count, sizeof(*swapchain_images));
    assert(NULL != swapchain_images);

    vk_res = vkGetSwapchainImagesKHR(p_renderer->vk_device, p_renderer->vk_swapchain, &image_count, swapchain_images);
    assert(VK_SUCCESS == vk_res);

    // Populate the vk_image field and create the Vulkan texture objects
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->color_attachments[0]->vk_image = swapchain_images[i];
        tr_internal_vk_create_texture(p_renderer, render_target->color_attachments[0]);

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
            tr_internal_vk_create_texture(p_renderer, render_target->color_attachments_multisample[0]);
        }

        if (NULL != render_target->depth_stencil_attachment) {
            tr_internal_vk_create_texture(p_renderer, render_target->depth_stencil_attachment);
            
            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
                tr_internal_vk_create_texture(p_renderer, render_target->depth_stencil_attachment_multisample);
            }
        }
    }

    // Initialize Vulkan render target objects
    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        tr_internal_vk_create_render_target(p_renderer, true, render_target);
    }

    TINY_RENDERER_SAFE_FREE(swapchain_images);
}

void tr_internal_vk_destroy_instance(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_instance);

    if ((NULL != trVkDestroyDebugReportCallbackEXT) && (VK_NULL_HANDLE !=  p_renderer->vk_debug_report)) {
        trVkDestroyDebugReportCallbackEXT(p_renderer->vk_instance, p_renderer->vk_debug_report, NULL);
    }
    vkDestroyInstance(p_renderer->vk_instance, NULL);

}

void tr_internal_vk_destroy_surface(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_surface);

    vkDestroySurfaceKHR(p_renderer->vk_instance, p_renderer->vk_surface, NULL);
}

void tr_internal_vk_destroy_device(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_surface);

    vkDestroyDevice(p_renderer->vk_device, NULL);
}

void tr_internal_vk_destroy_swapchain(tr_renderer* p_renderer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_swapchain);
    assert(VK_NULL_HANDLE != p_renderer->vk_surface);

    vkDestroySwapchainKHR(p_renderer->vk_device, p_renderer->vk_swapchain, NULL);
}

// -------------------------------------------------------------------------------------------------
// Internal create functions
// -------------------------------------------------------------------------------------------------
void tr_internal_vk_create_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{   
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    TINY_RENDERER_DECLARE_ZERO(VkFenceCreateInfo, create_info);
    create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    VkResult vk_res = vkCreateFence(p_renderer->vk_device,  &create_info, NULL, &(p_fence->vk_fence));
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{   
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_fence->vk_fence);

    vkDestroyFence(p_renderer->vk_device, p_fence->vk_fence, NULL);
}

void tr_internal_vk_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{   
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    TINY_RENDERER_DECLARE_ZERO(VkSemaphoreCreateInfo, create_info);
    create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    create_info.pNext = NULL;
    create_info.flags = 0;
    VkResult vk_res = vkCreateSemaphore(p_renderer->vk_device,  &create_info, NULL, &(p_semaphore->vk_semaphore));
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{   
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_semaphore->vk_semaphore);

    vkDestroySemaphore(p_renderer->vk_device, p_semaphore->vk_semaphore, NULL);
}

void tr_internal_vk_create_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    enum {max_types = 12};

    TINY_RENDERER_DECLARE_ZERO(VkDescriptorPoolSize, pool_sizes_by_type[max_types]);
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_SAMPLER               ].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE         ].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_STORAGE_IMAGE         ].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER  ].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER  ].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER        ].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER        ].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    pool_sizes_by_type[VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT      ].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;

    VkDescriptorSetLayoutBinding* bindings = (VkDescriptorSetLayoutBinding*)calloc(p_descriptor_set->descriptor_count, sizeof(*bindings));
    assert(NULL != bindings);

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        const tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        VkDescriptorSetLayoutBinding* binding = &(bindings[i]);
        uint32_t type_index = UINT32_MAX;
        switch (descriptor->type) {
            case tr_descriptor_type_sampler                  : type_index = VK_DESCRIPTOR_TYPE_SAMPLER; break;
            case tr_descriptor_type_uniform_buffer_cbv       : type_index = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER; break;
            case tr_descriptor_type_storage_buffer_srv       : type_index = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
            case tr_descriptor_type_storage_buffer_uav       : type_index = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; break;
            case tr_descriptor_type_uniform_texel_buffer_srv : type_index = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER; break;
            case tr_descriptor_type_storage_texel_buffer_uav : type_index = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER; break;
            case tr_descriptor_type_texture_srv              : type_index = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; break;
            case tr_descriptor_type_texture_uav              : type_index = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; break;
        }
        if (UINT32_MAX != type_index) {
            binding->binding            = descriptor->binding;
            binding->descriptorType     = (VkDescriptorType)type_index;
            binding->descriptorCount    = descriptor->count;
            binding->stageFlags         = tr_util_to_vk_shader_stages(descriptor->shader_stages);
            binding->pImmutableSamplers = NULL;

            pool_sizes_by_type[type_index].descriptorCount += descriptor->count;
        }
    }

    uint32_t pool_size_count = 0;
    TINY_RENDERER_DECLARE_ZERO(VkDescriptorPoolSize, pool_sizes[max_types]);
    for (uint32_t i = 0; i < max_types; ++i) {
        if (pool_sizes_by_type[i].descriptorCount > 0) {
            pool_sizes[pool_size_count].type = pool_sizes_by_type[i].type;
            pool_sizes[pool_size_count].descriptorCount = pool_sizes_by_type[i].descriptorCount;
            ++pool_size_count;
        }
    }

    assert(0 != pool_size_count);

    // Descriptor pool
    {
        TINY_RENDERER_DECLARE_ZERO(VkDescriptorPoolCreateInfo, create_info);
        create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        create_info.pNext         = NULL;
        create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT ;
        create_info.maxSets       = 1;
        create_info.poolSizeCount = pool_size_count;
        create_info.pPoolSizes    = pool_sizes;
        VkResult vk_res = vkCreateDescriptorPool(p_renderer->vk_device, &create_info, NULL, &(p_descriptor_set->vk_descriptor_pool));
        assert(VK_SUCCESS == vk_res);
    }

    // Descriptor set layout
    {
        TINY_RENDERER_DECLARE_ZERO(VkDescriptorSetLayoutCreateInfo, create_info);
        create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        create_info.pNext        = NULL;
        create_info.flags        = 0;
        create_info.bindingCount = p_descriptor_set->descriptor_count;
        create_info.pBindings    = bindings;
        VkResult vk_res = vkCreateDescriptorSetLayout(p_renderer->vk_device, &create_info, NULL, &(p_descriptor_set->vk_descriptor_set_layout));
        assert(VK_SUCCESS == vk_res);
    }

    // Allocate descriptor set
    {
        TINY_RENDERER_DECLARE_ZERO(VkDescriptorSetAllocateInfo, alloc_info);
        alloc_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.pNext              = NULL;
        alloc_info.descriptorPool     = p_descriptor_set->vk_descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts        = &(p_descriptor_set->vk_descriptor_set_layout);
        VkResult vk_res = vkAllocateDescriptorSets(p_renderer->vk_device, &alloc_info, &(p_descriptor_set->vk_descriptor_set));
        assert(VK_SUCCESS == vk_res);
    }

    TINY_RENDERER_SAFE_FREE(bindings);
}

void tr_internal_vk_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_descriptor_set->vk_descriptor_set_layout);
    assert(VK_NULL_HANDLE != p_descriptor_set->vk_descriptor_set);
    assert(VK_NULL_HANDLE != p_descriptor_set->vk_descriptor_pool);

    VkResult vk_res = vkFreeDescriptorSets(p_renderer->vk_device, p_descriptor_set->vk_descriptor_pool, 1, &(p_descriptor_set->vk_descriptor_set));
    assert(VK_SUCCESS == vk_res);

    vkDestroyDescriptorSetLayout(p_renderer->vk_device, p_descriptor_set->vk_descriptor_set_layout, NULL);
    vkDestroyDescriptorPool(p_renderer->vk_device, p_descriptor_set->vk_descriptor_pool, NULL);
}

void tr_internal_vk_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool* p_cmd_pool)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(p_renderer->graphics_queue->vk_queue_family_index == p_queue->vk_queue_family_index);
    assert((p_queue->vk_queue_family_index == p_renderer->graphics_queue->vk_queue_family_index) ||
           (p_queue->vk_queue_family_index == p_renderer->present_queue->vk_queue_family_index));

    TINY_RENDERER_DECLARE_ZERO(VkCommandPoolCreateInfo, create_info);
    create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    create_info.pNext            = NULL;
    create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    create_info.queueFamilyIndex = p_queue->vk_queue_family_index;
    if (transient) {
        create_info.flags |= VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    }
    VkResult vk_res = vkCreateCommandPool(p_renderer->vk_device, &create_info, NULL, &(p_cmd_pool->vk_cmd_pool));
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_cmd_pool->vk_cmd_pool);

    vkDestroyCommandPool(p_renderer->vk_device, p_cmd_pool->vk_cmd_pool, NULL);
}

void tr_internal_vk_create_cmd(tr_cmd_pool *p_cmd_pool, bool secondary, tr_cmd* p_cmd)
{
    assert(VK_NULL_HANDLE != p_cmd_pool->renderer->vk_device);
    assert(VK_NULL_HANDLE != p_cmd_pool->vk_cmd_pool);

    TINY_RENDERER_DECLARE_ZERO(VkCommandBufferAllocateInfo, alloc_info);
    alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.pNext              = NULL;
    alloc_info.commandPool        = p_cmd_pool->vk_cmd_pool;
    alloc_info.level              = secondary ? VK_COMMAND_BUFFER_LEVEL_SECONDARY : VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = 1;
    VkResult vk_res = vkAllocateCommandBuffers(p_cmd_pool->renderer->vk_device, &alloc_info, &(p_cmd->vk_cmd_buf));
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_destroy_cmd(tr_cmd_pool *p_cmd_pool, tr_cmd* p_cmd)
{
    assert(VK_NULL_HANDLE != p_cmd_pool->renderer->vk_device);
    assert(VK_NULL_HANDLE != p_cmd_pool->vk_cmd_pool);
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    vkFreeCommandBuffers(p_cmd_pool->renderer->vk_device, p_cmd_pool->vk_cmd_pool, 1, &(p_cmd->vk_cmd_buf));
}

void tr_internal_vk_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    // Align the buffer size to multiples of the dynamic uniform buffer minimum size
    if (p_buffer->usage & tr_buffer_usage_uniform_cbv) {
        // Make minimum size 256 bytes to match D3D12
        p_buffer->size = tr_round_up(tr_max((uint32_t)p_buffer->size, 256), 
                                     (uint32_t)(p_renderer->vk_active_gpu_properties.limits.minUniformBufferOffsetAlignment));
    }

    TINY_RENDERER_DECLARE_ZERO(VkBufferCreateInfo, create_info);
    create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    create_info.pNext                 = NULL;
    create_info.flags                 = 0;
    create_info.size                  = p_buffer->size;
    create_info.usage                 = tr_util_to_vk_buffer_usage(p_buffer->usage);
    create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    create_info.queueFamilyIndexCount = 0;
    create_info.pQueueFamilyIndices   = NULL;

    // Make it easy to copy to and from buffer
    create_info.usage |= (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);

    VkResult vk_res = vkCreateBuffer(p_renderer->vk_device, &create_info, NULL, &(p_buffer->vk_buffer));
    assert(VK_SUCCESS == vk_res);

    TINY_RENDERER_DECLARE_ZERO(VkMemoryRequirements, mem_reqs);
    vkGetBufferMemoryRequirements(p_renderer->vk_device, p_buffer->vk_buffer, &mem_reqs);

    VkMemoryPropertyFlags mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (p_buffer->host_visible) {
        mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    }

    uint32_t memory_type_index = UINT32_MAX;
    bool found_memmory = tr_util_vk_get_memory_type(&p_renderer->vk_memory_properties, mem_reqs.memoryTypeBits, mem_flags, &memory_type_index);
    assert(found_memmory);

    TINY_RENDERER_DECLARE_ZERO(VkMemoryAllocateInfo, alloc_info);
    alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.pNext           = NULL;
    alloc_info.allocationSize  = mem_reqs.size;
    alloc_info.memoryTypeIndex = memory_type_index;
    vk_res = vkAllocateMemory(p_renderer->vk_device, &alloc_info, NULL, &(p_buffer->vk_memory));
    assert(VK_SUCCESS == vk_res);

    vk_res = vkBindBufferMemory(p_renderer->vk_device, p_buffer->vk_buffer, p_buffer->vk_memory, 0);
    assert(VK_SUCCESS == vk_res);

    if (p_buffer->host_visible) {
        vk_res = vkMapMemory(p_renderer->vk_device, p_buffer->vk_memory, 0, VK_WHOLE_SIZE, 0, &(p_buffer->cpu_mapped_address));
        assert(VK_SUCCESS == vk_res);
    }

    switch (p_buffer->usage) {
        case tr_buffer_usage_uniform_texel_srv:
        case tr_buffer_usage_storage_texel_uav: {
            TINY_RENDERER_DECLARE_ZERO(VkBufferViewCreateInfo, buffer_view_create_info);
            buffer_view_create_info.sType   = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
            buffer_view_create_info.pNext   = NULL;
            buffer_view_create_info.flags   = 0;
            buffer_view_create_info.buffer  = p_buffer->vk_buffer;
            buffer_view_create_info.format  = tr_util_to_vk_format(p_buffer->format);
            buffer_view_create_info.range   = VK_WHOLE_SIZE;
            vk_res = vkCreateBufferView(p_renderer->vk_device, &buffer_view_create_info, NULL, &(p_buffer->vk_buffer_view));
        }
        break;

        case tr_buffer_usage_uniform_cbv:
        case tr_buffer_usage_storage_srv:
        case tr_buffer_usage_storage_uav: {
            p_buffer->vk_buffer_info.buffer = p_buffer->vk_buffer;
            p_buffer->vk_buffer_info.offset = 0;
            p_buffer->vk_buffer_info.range  = VK_WHOLE_SIZE;
        }
        break;
    }
}

void tr_internal_vk_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_buffer->vk_buffer);
    
    vkDestroyBuffer(p_renderer->vk_device, p_buffer->vk_buffer, NULL);
}

void tr_internal_vk_create_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    p_texture->renderer = p_renderer;

    if (VK_NULL_HANDLE == p_texture->vk_image) {
        VkImageType image_type = VK_IMAGE_TYPE_2D;
        switch (p_texture->type) {
            case tr_texture_type_1d   : image_type = VK_IMAGE_TYPE_1D; break;
            case tr_texture_type_2d   : image_type = VK_IMAGE_TYPE_2D; break;
            case tr_texture_type_3d   : image_type = VK_IMAGE_TYPE_3D; break;
            case tr_texture_type_cube : image_type = VK_IMAGE_TYPE_2D; break;
        }

        TINY_RENDERER_DECLARE_ZERO(VkImageCreateInfo, create_info);
        create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        create_info.pNext                 = NULL;
        create_info.flags                 = 0;
        create_info.imageType             = image_type;
        create_info.format                = tr_util_to_vk_format(p_texture->format);
        create_info.extent.width          = p_texture->width;
        create_info.extent.height         = p_texture->height;
        create_info.extent.depth          = p_texture->depth;
        create_info.mipLevels             = p_texture->mip_levels;
        create_info.arrayLayers           = p_texture->host_visible ? 1 : 1;
        create_info.samples               = tr_util_to_vk_sample_count(p_texture->sample_count);
        create_info.tiling                = (0 != p_texture->host_visible) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
        create_info.usage                 = tr_util_to_vk_image_usage(p_texture->usage);
        create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
        create_info.queueFamilyIndexCount = 0;
        create_info.pQueueFamilyIndices   = NULL;
        create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
        if (VK_IMAGE_USAGE_SAMPLED_BIT & create_info.usage) {
            // Make it easy to copy to and from textures
            create_info.usage |= (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT);
        }
        // Verify that GPU supports this format
        TINY_RENDERER_DECLARE_ZERO(VkFormatProperties, format_props);
        vkGetPhysicalDeviceFormatProperties(p_renderer->vk_active_gpu, create_info.format, &format_props);
        VkFormatFeatureFlags format_features = tr_util_vk_image_usage_to_format_features(create_info.usage);
        if (p_texture->host_visible) {
            VkFormatFeatureFlags flags = format_props.linearTilingFeatures & format_features;
            assert((0 != flags) && "Format is not supported for host visible images");
        }
        else {
            VkFormatFeatureFlags flags = format_props.optimalTilingFeatures & format_features;
            assert((0 != flags) && "Format is not supported for GPU local images (i.e. not host visible images)");
        }
        // Apply some bounds to the image
        TINY_RENDERER_DECLARE_ZERO(VkImageFormatProperties, image_format_props);
        VkResult vk_res = vkGetPhysicalDeviceImageFormatProperties(p_renderer->vk_active_gpu, create_info.format,
            create_info.imageType, create_info.tiling, create_info.usage, create_info.flags, &image_format_props);
        assert(VK_SUCCESS == vk_res);
        if (create_info.mipLevels > 1) {
            p_texture->mip_levels = tr_min(p_texture->mip_levels, image_format_props.maxMipLevels);
            create_info.mipLevels = p_texture->mip_levels;
        }
        // Create image
        vk_res = vkCreateImage(p_renderer->vk_device, &create_info, NULL, &(p_texture->vk_image));
        assert(VK_SUCCESS == vk_res);

        TINY_RENDERER_DECLARE_ZERO(VkMemoryRequirements, mem_reqs);
        vkGetImageMemoryRequirements(p_renderer->vk_device, p_texture->vk_image, &mem_reqs);

        VkMemoryPropertyFlags mem_flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        if (p_texture->host_visible) {
            mem_flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        uint32_t memory_type_index = UINT32_MAX;
        bool found_memory = tr_util_vk_get_memory_type(&p_renderer->vk_memory_properties, mem_reqs.memoryTypeBits, mem_flags, &memory_type_index);
        assert(found_memory);

        TINY_RENDERER_DECLARE_ZERO(VkMemoryAllocateInfo, alloc_info);
        alloc_info.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.pNext           = NULL;
        alloc_info.allocationSize  = mem_reqs.size;
        alloc_info.memoryTypeIndex = memory_type_index;
        vk_res = vkAllocateMemory(p_renderer->vk_device, &alloc_info, NULL, &(p_texture->vk_memory));
        assert(VK_SUCCESS == vk_res);

        vk_res = vkBindImageMemory(p_renderer->vk_device, p_texture->vk_image, p_texture->vk_memory, 0);
        assert(VK_SUCCESS == vk_res);

        if (p_texture->host_visible) {
            vk_res = vkMapMemory(p_renderer->vk_device, p_texture->vk_memory, 0, VK_WHOLE_SIZE, 0, &(p_texture->cpu_mapped_address));
            assert(VK_SUCCESS == vk_res);
        }

        p_texture->owns_image = true;
    }

    // Create image view
    {
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
        switch (p_texture->type) {
            case tr_texture_type_1d   : view_type = VK_IMAGE_VIEW_TYPE_1D;   break;
            case tr_texture_type_2d   : view_type = VK_IMAGE_VIEW_TYPE_2D;   break;
            case tr_texture_type_3d   : view_type = VK_IMAGE_VIEW_TYPE_3D;   break;
            case tr_texture_type_cube : view_type = VK_IMAGE_VIEW_TYPE_CUBE; break;
        }

        TINY_RENDERER_DECLARE_ZERO(VkImageViewCreateInfo, create_info);
        create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        create_info.pNext                           = NULL;
        create_info.flags                           = 0;
        create_info.image                           = p_texture->vk_image;
        create_info.viewType                        = view_type;
        create_info.format                          = tr_util_to_vk_format(p_texture->format);
        create_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
        create_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
        create_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
        create_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
        create_info.subresourceRange.aspectMask     = tr_util_vk_determine_aspect_mask(tr_util_to_vk_format(p_texture->format));
        create_info.subresourceRange.baseMipLevel   = 0;
        create_info.subresourceRange.levelCount     = p_texture->mip_levels;
        create_info.subresourceRange.baseArrayLayer = 0;
        create_info.subresourceRange.layerCount     = 1;
        VkResult vk_res = vkCreateImageView(p_renderer->vk_device, &create_info, NULL, &(p_texture->vk_image_view));
        assert(VK_SUCCESS == vk_res);

        p_texture->vk_aspect_mask = create_info.subresourceRange.aspectMask;
    }

    p_texture->vk_texture_view.imageView = p_texture->vk_image_view;
    p_texture->vk_texture_view.imageLayout = (p_texture->usage & tr_texture_usage_storage_image) ? VK_IMAGE_LAYOUT_GENERAL
                                                                                                 : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
}

void tr_internal_vk_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_texture->vk_image);
    assert(VK_NULL_HANDLE != p_texture->vk_image_view);
    if (p_texture->owns_image) {
        assert(VK_NULL_HANDLE != p_texture->vk_memory);
    }

    if (VK_NULL_HANDLE != p_texture->vk_memory) {
        vkFreeMemory(p_renderer->vk_device, p_texture->vk_memory, NULL);
    }

    if ((VK_NULL_HANDLE != p_texture->vk_image) && (p_texture->owns_image)) {
        vkDestroyImage(p_renderer->vk_device, p_texture->vk_image, NULL);
    }

    if (VK_NULL_HANDLE != p_texture->vk_image_view) {
        vkDestroyImageView(p_renderer->vk_device, p_texture->vk_image_view, NULL);
    }
}

void tr_internal_vk_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    TINY_RENDERER_DECLARE_ZERO(VkSamplerCreateInfo, create_info);
    create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    create_info.pNext                   = NULL;
    create_info.flags                   = 0;
    create_info.magFilter               = VK_FILTER_LINEAR;
    create_info.minFilter               = VK_FILTER_LINEAR;
    create_info.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    create_info.addressModeU            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeV            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.addressModeW            = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    create_info.mipLodBias              = 0.0f;
    create_info.anisotropyEnable        = VK_FALSE;
    create_info.maxAnisotropy           = 1.0f;
    create_info.compareEnable           = VK_FALSE;
    create_info.compareOp               = VK_COMPARE_OP_NEVER;
    create_info.minLod                  = 0.0f;
    create_info.maxLod                  = 0.0f;
    create_info.borderColor             = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    create_info.unnormalizedCoordinates = VK_FALSE;
    VkResult vk_res = vkCreateSampler(p_renderer->vk_device, &create_info, NULL, &(p_sampler->vk_sampler));
    assert(VK_SUCCESS == vk_res);

    p_sampler->vk_sampler_view.sampler = p_sampler->vk_sampler;
}

void tr_internal_vk_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_sampler->vk_sampler);

    vkDestroySampler(p_renderer->vk_device, p_sampler->vk_sampler, NULL);
}

void tr_internal_vk_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t tesc_size, const void* tesc_code, const char* tesc_enpt, uint32_t tese_size, const void* tese_code, const char* tese_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program* p_shader_program)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    for (uint32_t i = 0; i < tr_shader_stage_count; ++i) {
        tr_shader_stage stage_mask = (tr_shader_stage)(1 << i);
        if (stage_mask == (p_shader_program->shader_stages & stage_mask)) {
            TINY_RENDERER_DECLARE_ZERO(VkShaderModuleCreateInfo, create_info);
            create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            create_info.pNext = NULL;
            create_info.flags = 0;  
            switch(stage_mask) {
                case tr_shader_stage_vert: {
                    create_info.codeSize = vert_size;
                    create_info.pCode = (const uint32_t*)vert_code;                 
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_vert));
                    assert(VK_SUCCESS == vk_res);
                } break;
                case tr_shader_stage_tesc: {
                    create_info.codeSize = tesc_size;
                    create_info.pCode = (const uint32_t*)tesc_code;
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_tesc));
                    assert(VK_SUCCESS == vk_res);
                } break;
                case tr_shader_stage_tese: {
                    create_info.codeSize = tese_size;
                    create_info.pCode = (const uint32_t*)tese_code;
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_tese));
                    assert(VK_SUCCESS == vk_res);
                } break;
                case tr_shader_stage_geom: {
                    create_info.codeSize = geom_size;
                    create_info.pCode = (const uint32_t*)geom_code;
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_geom));
                    assert(VK_SUCCESS == vk_res);
                } break;
                case tr_shader_stage_frag: {
                    create_info.codeSize = frag_size;
                    create_info.pCode = (const uint32_t*)frag_code;
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_frag));
                    assert(VK_SUCCESS == vk_res);
                } break;
                case tr_shader_stage_comp: {
                    create_info.codeSize = comp_size;
                    create_info.pCode = (const uint32_t*)comp_code;
                    VkResult vk_res = vkCreateShaderModule(p_renderer->vk_device, &create_info, NULL, &(p_shader_program->vk_comp));
                    assert(VK_SUCCESS == vk_res);
                } break;
            }
        }
    }   
}

void tr_internal_vk_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    if (VK_NULL_HANDLE != p_shader_program->vk_vert) {
        vkDestroyShaderModule(p_renderer->vk_device, p_shader_program->vk_vert, NULL);
    }

    if (VK_NULL_HANDLE != p_shader_program->vk_tesc) {
        vkDestroyShaderModule(p_renderer->vk_device, p_shader_program->vk_tesc, NULL);
    }

    if (VK_NULL_HANDLE != p_shader_program->vk_tese) {
        vkDestroyShaderModule(p_renderer->vk_device, p_shader_program->vk_tese, NULL);
    }

    if (VK_NULL_HANDLE != p_shader_program->vk_geom) {
        vkDestroyShaderModule(p_renderer->vk_device, p_shader_program->vk_geom, NULL);
    }

    if (VK_NULL_HANDLE != p_shader_program->vk_frag) {
        vkDestroyShaderModule(p_renderer->vk_device, p_shader_program->vk_frag, NULL);
    }
}

void tr_internal_vk_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert((VK_NULL_HANDLE != p_shader_program->vk_vert) || (VK_NULL_HANDLE != p_shader_program->vk_tesc) || (VK_NULL_HANDLE != p_shader_program->vk_tese) || (VK_NULL_HANDLE != p_shader_program->vk_geom) || (VK_NULL_HANDLE != p_shader_program->vk_frag));
    assert(VK_NULL_HANDLE != p_render_target->vk_render_pass);

    // Pipeline layout
    {
        TINY_RENDERER_DECLARE_ZERO(VkPipelineLayoutCreateInfo, create_info);
        create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pNext                  = NULL;
        create_info.flags                  = 0;
        create_info.setLayoutCount         = (NULL != p_descriptor_set) ? 1 : 0;
        create_info.pSetLayouts            = (NULL != p_descriptor_set) ? &(p_descriptor_set->vk_descriptor_set_layout) : NULL;
        create_info.pushConstantRangeCount = 0;
        create_info.pPushConstantRanges    = NULL;
        VkResult vk_res = vkCreatePipelineLayout(p_renderer->vk_device, &create_info, NULL, &(p_pipeline->vk_pipeline_layout));
        assert(VK_SUCCESS == vk_res);
    }

    // Pipeline
    {
        uint32_t stage_count = 0;
        TINY_RENDERER_DECLARE_ZERO(VkPipelineShaderStageCreateInfo, stages[5]);
        for (uint32_t i = 0; i < 5; ++i) {
            tr_shader_stage stage_mask = (tr_shader_stage)(1 << i);
            if (stage_mask == (p_shader_program->shader_stages & stage_mask)) {
                stages[stage_count].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
                stages[stage_count].pNext = NULL;
                stages[stage_count].flags = 0;
                stages[stage_count].pSpecializationInfo = NULL;         
                switch(stage_mask) {
                    case tr_shader_stage_vert: {
                        stages[stage_count].pName  = p_shader_program->vert_entry_point;
                        stages[stage_count].stage  = VK_SHADER_STAGE_VERTEX_BIT; 
                        stages[stage_count].module = p_shader_program->vk_vert;
                    } break;
                    case tr_shader_stage_tesc: {
                        stages[stage_count].pName  = p_shader_program->tesc_entry_point;
                        stages[stage_count].stage = VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT; 
                        stages[stage_count].module = p_shader_program->vk_tesc;
                    } break;
                    case tr_shader_stage_tese: {
                        stages[stage_count].pName  = p_shader_program->tese_entry_point;
                        stages[stage_count].stage = VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
                        stages[stage_count].module = p_shader_program->vk_tese;
                    } break;
                    case tr_shader_stage_geom: {
                        stages[stage_count].pName  = p_shader_program->geom_entry_point;
                        stages[stage_count].stage = VK_SHADER_STAGE_GEOMETRY_BIT; 
                        stages[stage_count].module = p_shader_program->vk_geom;
                    } break;
                    case tr_shader_stage_frag: {
                        stages[stage_count].pName  = p_shader_program->frag_entry_point;
                        stages[stage_count].stage = VK_SHADER_STAGE_FRAGMENT_BIT; 
                        stages[stage_count].module = p_shader_program->vk_frag;
                    } break;
                }
                ++stage_count;
            }
        }

        // Make sure there's a shader
        assert(0 != stage_count);
        
        // Make sure there's attributes
        assert(0 != p_vertex_layout->attrib_count);

        uint32_t input_binding_count = 0;
        VkVertexInputBindingDescription input_bindings[tr_max_vertex_bindings] = {0};
        uint32_t input_attribute_count = 0;
        VkVertexInputAttributeDescription input_attributes[tr_max_vertex_attribs] = {0};
        // Ignore everything that's beyond tr_max_vertex_attribs
        uint32_t attrib_count = p_vertex_layout->attrib_count > tr_max_vertex_attribs ? tr_max_vertex_attribs : p_vertex_layout->attrib_count;
        // Initial values
        uint32_t binding_value = UINT32_MAX;
        for (uint32_t i = 0; i < attrib_count; ++i) {
            const tr_vertex_attrib* attrib = &(p_vertex_layout->attribs[i]);
            if (binding_value != attrib->binding) {
                binding_value = attrib->binding;
                ++input_binding_count;
            }

            input_bindings[input_binding_count - 1].binding = binding_value;
            input_bindings[input_binding_count - 1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            input_bindings[input_binding_count - 1].stride += tr_util_format_stride(attrib->format);
            
            input_attributes[input_attribute_count].location = attrib->location;
            input_attributes[input_attribute_count].binding  = attrib->binding;
            input_attributes[input_attribute_count].format   = tr_util_to_vk_format(attrib->format);
            input_attributes[input_attribute_count].offset   = attrib->offset;
            ++input_attribute_count;
        }
        
        TINY_RENDERER_DECLARE_ZERO(VkPipelineVertexInputStateCreateInfo, vi);
        vi.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vi.pNext                                    = NULL;
        vi.flags                                    = 0;
        vi.vertexBindingDescriptionCount            = input_binding_count;
        vi.pVertexBindingDescriptions               = input_bindings;
        vi.vertexAttributeDescriptionCount          = input_attribute_count;
        vi.pVertexAttributeDescriptions             = input_attributes;
                                                
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        switch(p_pipeline_settings->primitive_topo) {
            case tr_primitive_topo_point_list     : topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST; break;
            case tr_primitive_topo_line_list      : topology = VK_PRIMITIVE_TOPOLOGY_LINE_LIST; break;
            case tr_primitive_topo_line_strip     : topology = VK_PRIMITIVE_TOPOLOGY_LINE_STRIP; break;
            case tr_primitive_topo_tri_strip      : topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP; break;
            case tr_primitive_topo_tri_fan        : topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN; break;
            case tr_primitive_topo_1_point_patch  : topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
            case tr_primitive_topo_2_point_patch  : topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
            case tr_primitive_topo_3_point_patch  : topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
            case tr_primitive_topo_4_point_patch  : topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST; break;
        }
        TINY_RENDERER_DECLARE_ZERO(VkPipelineInputAssemblyStateCreateInfo, ia);
        ia.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        ia.pNext                                    = NULL;
        ia.flags                                    = 0;
        ia.topology                                 = topology;
        ia.primitiveRestartEnable                   = VK_FALSE;

        TINY_RENDERER_DECLARE_ZERO(VkPipelineTessellationStateCreateInfo, ts);
        ts.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
        ts.pNext                                    = NULL;
        ts.flags                                    = 0;
        ts.patchControlPoints                       = 0;
        switch (p_pipeline_settings->primitive_topo) {
          case tr_primitive_topo_1_point_patch: ts.patchControlPoints = 1; break;
          case tr_primitive_topo_2_point_patch: ts.patchControlPoints = 2; break;
          case tr_primitive_topo_3_point_patch: ts.patchControlPoints = 3; break;
          case tr_primitive_topo_4_point_patch: ts.patchControlPoints = 4; break;
        }

        TINY_RENDERER_DECLARE_ZERO(VkPipelineViewportStateCreateInfo, vs);
        vs.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        vs.pNext                                    = NULL;
        vs.flags                                    = 0;
        vs.viewportCount                            = 1;
        vs.pViewports                               = NULL;
        vs.scissorCount                             = 1;
        vs.pScissors                                = NULL;

        VkCullModeFlags cull_mode = VK_CULL_MODE_NONE;
        switch(p_pipeline_settings->cull_mode) {
            case tr_cull_mode_back  : cull_mode = VK_CULL_MODE_BACK_BIT; break;
            case tr_cull_mode_front : cull_mode = VK_CULL_MODE_FRONT_BIT; break;
            case tr_cull_mode_both  : cull_mode = VK_CULL_MODE_FRONT_AND_BACK; break;
        }
        VkFrontFace front_face = (tr_fornt_face_cw == p_pipeline_settings->front_face) ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
        TINY_RENDERER_DECLARE_ZERO(VkPipelineRasterizationStateCreateInfo, rs);
        rs.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rs.pNext                                    = NULL;
        rs.flags                                    = 0;
        rs.depthClampEnable                         = VK_FALSE;
        rs.rasterizerDiscardEnable                  = VK_FALSE;
        rs.polygonMode                              = VK_POLYGON_MODE_FILL;
        rs.cullMode                                 = cull_mode;
        rs.frontFace                                = front_face;
        rs.depthBiasEnable                          = VK_FALSE;
        rs.depthBiasConstantFactor                  = 0.0f;
        rs.depthBiasClamp                           = 0.0f;
        rs.depthBiasSlopeFactor                     = 0.0f;
        rs.lineWidth                                = 1.0f;

        TINY_RENDERER_DECLARE_ZERO(VkPipelineMultisampleStateCreateInfo, ms);
        ms.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        ms.pNext                                    = NULL;
        ms.flags                                    = 0;
        ms.rasterizationSamples                     = tr_util_to_vk_sample_count(p_render_target->sample_count);
        ms.sampleShadingEnable                      = VK_FALSE;
        ms.minSampleShading                         = 0.0f;
        ms.pSampleMask                              = 0;
        ms.alphaToCoverageEnable                    = VK_FALSE;
        ms.alphaToOneEnable                         = VK_FALSE;

        TINY_RENDERER_DECLARE_ZERO(VkPipelineDepthStencilStateCreateInfo, ds);
        ds.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds.pNext                                    = NULL;
        ds.flags                                    = 0;
        ds.depthTestEnable                          = p_pipeline_settings->depth ? VK_TRUE : VK_FALSE;
        ds.depthWriteEnable                         = p_pipeline_settings->depth ? VK_TRUE : VK_FALSE;
        ds.depthCompareOp                           = VK_COMPARE_OP_LESS;
        ds.depthBoundsTestEnable                    = VK_FALSE;
        ds.stencilTestEnable                        = VK_FALSE;
        ds.front.failOp                             = VK_STENCIL_OP_KEEP;
        ds.front.passOp                             = VK_STENCIL_OP_KEEP;
        ds.front.depthFailOp                        = VK_STENCIL_OP_KEEP;
        ds.front.compareOp                          = VK_COMPARE_OP_NEVER; 
        ds.front.compareMask                        = 0;
        ds.front.writeMask                          = 0;
        ds.front.reference                          = 0;
        ds.back.failOp                              = VK_STENCIL_OP_KEEP;
        ds.back.passOp                              = VK_STENCIL_OP_KEEP;
        ds.back.depthFailOp                         = VK_STENCIL_OP_KEEP;
        ds.back.compareOp                           = VK_COMPARE_OP_NEVER; 
        ds.back.compareMask                         = 0;
        ds.back.writeMask                           = 0;
        ds.back.reference                           = 0;
        ds.minDepthBounds                           = 0.0f;
        ds.maxDepthBounds                           = 0.0;

        TINY_RENDERER_DECLARE_ZERO(VkPipelineColorBlendAttachmentState, cbas);
        cbas.colorWriteMask                         = 0xf;
        cbas.blendEnable                            = VK_FALSE;
        cbas.alphaBlendOp                           = VK_BLEND_OP_ADD;
        cbas.colorBlendOp                           = VK_BLEND_OP_ADD;
        cbas.srcColorBlendFactor                    = VK_BLEND_FACTOR_ZERO;
        cbas.dstColorBlendFactor                    = VK_BLEND_FACTOR_ZERO;
        cbas.srcAlphaBlendFactor                    = VK_BLEND_FACTOR_ZERO;
        cbas.dstAlphaBlendFactor                    = VK_BLEND_FACTOR_ZERO;
        TINY_RENDERER_DECLARE_ZERO(VkPipelineColorBlendStateCreateInfo, cb);
        cb.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        cb.pNext                                    = NULL;
        cb.flags                                    = 0;
        cb.logicOpEnable                            = VK_FALSE;
        cb.logicOp                                  = VK_LOGIC_OP_NO_OP;
        cb.attachmentCount                          = 1;
        cb.pAttachments                             = &cbas;
        cb.blendConstants[0]                        = 0.0f;
        cb.blendConstants[1]                        = 0.0f;
        cb.blendConstants[2]                        = 0.0f;
        cb.blendConstants[3]                        = 0.0f;

        TINY_RENDERER_DECLARE_ZERO(VkDynamicState, dyn_states[9]);
        dyn_states[0]                               = VK_DYNAMIC_STATE_VIEWPORT;
        dyn_states[1]                               = VK_DYNAMIC_STATE_SCISSOR;
        dyn_states[2]                               = VK_DYNAMIC_STATE_LINE_WIDTH;
        dyn_states[3]                               = VK_DYNAMIC_STATE_DEPTH_BIAS;
        dyn_states[4]                               = VK_DYNAMIC_STATE_BLEND_CONSTANTS;
        dyn_states[5]                               = VK_DYNAMIC_STATE_DEPTH_BOUNDS;
        dyn_states[6]                               = VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK;
        dyn_states[7]                               = VK_DYNAMIC_STATE_STENCIL_WRITE_MASK;
        dyn_states[8]                               = VK_DYNAMIC_STATE_STENCIL_REFERENCE;
        TINY_RENDERER_DECLARE_ZERO(VkPipelineDynamicStateCreateInfo, dy);
        dy.sType                                    = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dy.pNext                                    = NULL;
        dy.flags                                    = 0;
        dy.dynamicStateCount                        = 9;
        dy.pDynamicStates                           = dyn_states;

        TINY_RENDERER_DECLARE_ZERO(VkGraphicsPipelineCreateInfo, create_info);
        create_info.sType                           = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        create_info.pNext                           = NULL;
        create_info.flags                           = 0;
        create_info.stageCount                      = stage_count;
        create_info.pStages                         = stages;
        create_info.pVertexInputState               = &vi;
        create_info.pInputAssemblyState             = &ia;
        create_info.pTessellationState              = &ts;
        create_info.pViewportState                  = &vs;
        create_info.pRasterizationState             = &rs;
        create_info.pMultisampleState               = &ms;
        create_info.pDepthStencilState              = &ds;
        create_info.pColorBlendState                = &cb;
        create_info.pDynamicState                   = &dy;
        create_info.layout                          = p_pipeline->vk_pipeline_layout;
        create_info.renderPass                      = p_render_target->vk_render_pass;
        create_info.subpass                         = 0;
        create_info.basePipelineHandle              = VK_NULL_HANDLE;
        create_info.basePipelineIndex               = -1;
        VkResult vk_res = vkCreateGraphicsPipelines(p_renderer->vk_device, VK_NULL_HANDLE, 1, &create_info, NULL, &(p_pipeline->vk_pipeline));
        assert(VK_SUCCESS == vk_res);
    }
}

void tr_internal_vk_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    assert(p_renderer->vk_device != VK_NULL_HANDLE);
    assert(p_shader_program->vk_comp != VK_NULL_HANDLE);

    // Pipeline layout
    {
        TINY_RENDERER_DECLARE_ZERO(VkPipelineLayoutCreateInfo, create_info);
        create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        create_info.pNext                  = NULL;
        create_info.flags                  = 0;
        create_info.setLayoutCount         = (NULL != p_descriptor_set) ? 1 : 0;
        create_info.pSetLayouts            = (NULL != p_descriptor_set) ? &(p_descriptor_set->vk_descriptor_set_layout) : NULL;
        create_info.pushConstantRangeCount = 0;
        create_info.pPushConstantRanges    = NULL;
        VkResult vk_res = vkCreatePipelineLayout(p_renderer->vk_device, &create_info, NULL, &(p_pipeline->vk_pipeline_layout));
        assert(VK_SUCCESS == vk_res);
    }

    // Pipeline
    {
      TINY_RENDERER_DECLARE_ZERO(VkPipelineShaderStageCreateInfo , stage);
      stage.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
      stage.pNext               = NULL;
      stage.flags               = 0;
      stage.stage               = VK_SHADER_STAGE_COMPUTE_BIT;
      stage.module              = p_shader_program->vk_comp;
      stage.pName               = p_shader_program->comp_entry_point;
      stage.pSpecializationInfo = NULL;  

      TINY_RENDERER_DECLARE_ZERO(VkComputePipelineCreateInfo, create_info);
      create_info.sType               = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
      create_info.pNext               = NULL;
      create_info.flags               = 0;
      create_info.stage               = stage;
      create_info.layout              = p_pipeline->vk_pipeline_layout;
      create_info.basePipelineHandle  = 0;
      create_info.basePipelineIndex   = 0;
      VkResult vk_res = vkCreateComputePipelines(p_renderer->vk_device, VK_NULL_HANDLE, 1, &create_info, NULL, &(p_pipeline->vk_pipeline));
      assert(VK_SUCCESS == vk_res);
    }
}

void tr_internal_vk_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_pipeline->vk_pipeline);
    assert(VK_NULL_HANDLE != p_pipeline->vk_pipeline_layout);

    vkDestroyPipeline(p_renderer->vk_device, p_pipeline->vk_pipeline, NULL);
    vkDestroyPipelineLayout(p_renderer->vk_device, p_pipeline->vk_pipeline_layout, NULL);
}

void tr_internal_vk_create_render_pass(tr_renderer* p_renderer, bool is_swapchain, tr_render_target* p_render_target)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);

    uint32_t color_attachment_count = p_render_target->color_attachment_count;
    uint32_t depth_stencil_attachment_count = (tr_format_undefined != p_render_target->depth_stencil_format) ? 1 : 0;

    VkAttachmentDescription* attachments = NULL;
    VkAttachmentReference* color_attachment_refs = NULL;
    VkAttachmentReference* depth_stencil_attachment_ref = NULL;
    // Resolve is for color attachments only, there's no mapping in 
    // the subpass description for a depth stencil resolve.
    VkAttachmentReference* resolve_attachment_refs = NULL;

    // Fill out attachment descriptions and references
    if (p_render_target->sample_count > tr_sample_count_1) {
        uint32_t attachment_description_count = (2 * color_attachment_count) + depth_stencil_attachment_count;
        attachments = (VkAttachmentDescription*)calloc(attachment_description_count, sizeof(*attachments));
        assert(NULL != attachments);

        if (color_attachment_count > 0) {
            color_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*color_attachment_refs));
            assert(NULL != color_attachment_refs);

            resolve_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*resolve_attachment_refs));
            assert(NULL != resolve_attachment_refs);
        }
        if (depth_stencil_attachment_count > 0) {
            depth_stencil_attachment_ref = (VkAttachmentReference*)calloc(1, sizeof(*depth_stencil_attachment_ref));
            assert(NULL != depth_stencil_attachment_ref);
        }

        // Color
        for (uint32_t i = 0; i < color_attachment_count; ++i) {
            const uint32_t ssidx = 2*i;
            const uint32_t msidx = ssidx + 1;

            // descriptions
            attachments[ssidx].flags          = 0;
            attachments[ssidx].format         = tr_util_to_vk_format(p_render_target->color_format);
            attachments[ssidx].samples        = tr_util_to_vk_sample_count(tr_sample_count_1);
            attachments[ssidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[ssidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[ssidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[ssidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[ssidx].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[ssidx].finalLayout    = is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            attachments[msidx].flags          = 0;
            attachments[msidx].format         = tr_util_to_vk_format(p_render_target->color_format);
            attachments[msidx].samples        = tr_util_to_vk_sample_count(p_render_target->sample_count);
            attachments[msidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[msidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[msidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[msidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[msidx].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[msidx].finalLayout    = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // references
            color_attachment_refs[i].attachment   = msidx;
            color_attachment_refs[i].layout       = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            resolve_attachment_refs[i].attachment = ssidx;
            resolve_attachment_refs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        // Depth stencil
        if (depth_stencil_attachment_count > 0) {
            uint32_t idx = (2 * color_attachment_count);

            /// Descriptions
            attachments[idx].flags          = 0;
            attachments[idx].format         = tr_util_to_vk_format(p_render_target->depth_stencil_format);
            attachments[idx].samples        = tr_util_to_vk_sample_count(p_render_target->sample_count);
            attachments[idx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[idx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[idx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[idx].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[idx].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // References
            depth_stencil_attachment_ref[0].attachment = idx;
            depth_stencil_attachment_ref[0].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }
    else {
        uint32_t attachment_description_count = color_attachment_count + depth_stencil_attachment_count;
        attachments = (VkAttachmentDescription*)calloc(attachment_description_count, sizeof(*attachments));
        assert(NULL != attachments);
        
        if (color_attachment_count > 0) {
            color_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*color_attachment_refs));
            assert(NULL != color_attachment_refs);
        }
        if (depth_stencil_attachment_count > 0) {
            depth_stencil_attachment_ref = (VkAttachmentReference*)calloc(1, sizeof(*depth_stencil_attachment_ref));
            assert(NULL != depth_stencil_attachment_ref);
        }

        // Color
        for (uint32_t i = 0; i < color_attachment_count; ++i) {
            const uint32_t ssidx = i;

            // descriptions
            attachments[ssidx].flags          = 0;
            attachments[ssidx].format         = tr_util_to_vk_format(p_render_target->color_format);
            attachments[ssidx].samples        = tr_util_to_vk_sample_count(tr_sample_count_1);
            attachments[ssidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[ssidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[ssidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[ssidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[ssidx].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[ssidx].finalLayout    = is_swapchain ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // references
            color_attachment_refs[i].attachment = ssidx;
            color_attachment_refs[i].layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        }

        // Depth stencil
        if (depth_stencil_attachment_count > 0) {
            uint32_t idx = color_attachment_count;
            attachments[idx].flags          = 0;
            attachments[idx].format         = tr_util_to_vk_format(p_render_target->depth_stencil_format);
            attachments[idx].samples        = tr_util_to_vk_sample_count(p_render_target->sample_count);
            attachments[idx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[idx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[idx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
            attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachments[idx].initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;
            attachments[idx].finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            depth_stencil_attachment_ref[0].attachment = idx;
            depth_stencil_attachment_ref[0].layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        }
    }
            
    TINY_RENDERER_DECLARE_ZERO(VkSubpassDescription, subpass);
    subpass.flags                   = 0;
    subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.inputAttachmentCount    = 0;
    subpass.pInputAttachments       = NULL;
    subpass.colorAttachmentCount    = color_attachment_count;
    subpass.pColorAttachments       = color_attachment_refs;
    subpass.pResolveAttachments     = resolve_attachment_refs;
    subpass.pDepthStencilAttachment = depth_stencil_attachment_ref;
    subpass.preserveAttachmentCount = 0;
    subpass.pPreserveAttachments    = NULL;

    // Create self-dependency in case image or memory barrier is issued within subpass
    TINY_RENDERER_DECLARE_ZERO(VkSubpassDependency, subpass_dependency);
    subpass_dependency.srcSubpass      = 0;
    subpass_dependency.dstSubpass      = 0;
    subpass_dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpass_dependency.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpass_dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    uint32_t attachment_count = (p_render_target->sample_count > tr_sample_count_1) ?  (2 * color_attachment_count) : color_attachment_count;
    attachment_count += depth_stencil_attachment_count;

    TINY_RENDERER_DECLARE_ZERO(VkRenderPassCreateInfo, create_info);
    create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    create_info.pNext           = NULL;
    create_info.flags           = 0;
    create_info.attachmentCount = attachment_count;
    create_info.pAttachments    = attachments;
    create_info.subpassCount    = 1;
    create_info.pSubpasses      = &subpass;
    create_info.dependencyCount = 1;
    create_info.pDependencies   = &subpass_dependency;
    
    VkResult vk_res = vkCreateRenderPass(p_renderer->vk_device, &create_info, NULL, &(p_render_target->vk_render_pass));
    assert(VK_SUCCESS == vk_res);

    TINY_RENDERER_SAFE_FREE(attachments);
    TINY_RENDERER_SAFE_FREE(color_attachment_refs);
    TINY_RENDERER_SAFE_FREE(resolve_attachment_refs);
    TINY_RENDERER_SAFE_FREE(depth_stencil_attachment_ref);
}

void tr_internal_vk_create_framebuffer(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_render_target->vk_render_pass);

    uint32_t attachment_count = p_render_target->color_attachment_count;
    if (p_render_target->sample_count > tr_sample_count_1) {
        attachment_count *= 2;
    }
    if (tr_format_undefined != p_render_target->depth_stencil_format) {
        attachment_count += 1;
    }

    VkImageView* attachments = (VkImageView*)calloc(attachment_count, sizeof(*attachments));
    assert(NULL != attachments);

    VkImageView* iter_attachments = attachments;
    if (p_render_target->sample_count > tr_sample_count_1) {
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            // single sample
            *iter_attachments = p_render_target->color_attachments[i]->vk_image_view;
            ++iter_attachments;
            // multi sample
            *iter_attachments = p_render_target->color_attachments_multisample[i]->vk_image_view;
            ++iter_attachments;
        }
    }
    else {
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            *iter_attachments = p_render_target->color_attachments[i]->vk_image_view;
            ++iter_attachments;
        }
    }

    // Depth/stencil
    if (tr_format_undefined != p_render_target->depth_stencil_format) {
        if (p_render_target->sample_count > tr_sample_count_1) {
            *iter_attachments = p_render_target->depth_stencil_attachment_multisample->vk_image_view;
            ++iter_attachments;
        }
        else {
            *iter_attachments = p_render_target->depth_stencil_attachment->vk_image_view;
            ++iter_attachments;
        }
    }
    
    TINY_RENDERER_DECLARE_ZERO(VkFramebufferCreateInfo, create_info);
    create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    create_info.pNext           = NULL;
    create_info.flags           = 0;
    create_info.renderPass      = p_render_target->vk_render_pass;
    create_info.attachmentCount = attachment_count;
    create_info.pAttachments    = attachments;
    create_info.width           = p_render_target->width;;
    create_info.height          = p_render_target->height;
    create_info.layers          = 1;
    VkResult vk_res = vkCreateFramebuffer(p_renderer->vk_device, &create_info, NULL, &(p_render_target->vk_framebuffer));
    assert(VK_SUCCESS == vk_res);

    TINY_RENDERER_SAFE_FREE(attachments);
}

void tr_internal_vk_create_render_target(tr_renderer* p_renderer, bool is_swapchain, tr_render_target* p_render_target)
{
    tr_internal_vk_create_render_pass(p_renderer, is_swapchain, p_render_target);
    tr_internal_vk_create_framebuffer(p_renderer, p_render_target);
}

void tr_internal_vk_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_render_target->vk_render_pass);

    vkDestroyFramebuffer(p_renderer->vk_device, p_render_target->vk_framebuffer, NULL);
    vkDestroyRenderPass(p_renderer->vk_device, p_render_target->vk_render_pass, NULL);
}

// -------------------------------------------------------------------------------------------------
// Internal descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_internal_vk_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_descriptor_set->vk_descriptor_set);

    // Not really efficient, just write less frequently ;)
    uint32_t write_count = 0;
    uint32_t sampler_view_count = 0;
    uint32_t image_view_count = 0;
    uint32_t buffer_view_count = 0;
    for (uint32_t descriptor_index = 0; descriptor_index < p_descriptor_set->descriptor_count; ++descriptor_index) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[descriptor_index]);
        switch (descriptor->type) {
            case tr_descriptor_type_sampler: {
                sampler_view_count += descriptor->count;
                ++write_count;
            }
            break;

            case tr_descriptor_type_uniform_buffer_cbv:
            case tr_descriptor_type_storage_buffer_srv:
            case tr_descriptor_type_storage_buffer_uav:
            case tr_descriptor_type_uniform_texel_buffer_srv:
            case tr_descriptor_type_storage_texel_buffer_uav: {
                buffer_view_count += descriptor->count;
                ++write_count;
            }
            break;

            case tr_descriptor_type_texture_srv:
            case tr_descriptor_type_texture_uav: {
                image_view_count += descriptor->count;
                ++write_count;
            }
            break;
        }
    }
    // Bail if there's nothing to write
    if (0 == write_count) {
        return;
    }

    VkWriteDescriptorSet* writes = (VkWriteDescriptorSet*)calloc(write_count, sizeof(*writes));
    assert(NULL != writes);
    VkDescriptorImageInfo* sampler_views = (VkDescriptorImageInfo*)calloc(sampler_view_count, sizeof(*sampler_views));
    assert(NULL != sampler_views);
    VkDescriptorImageInfo* image_views = (VkDescriptorImageInfo*)calloc(image_view_count, sizeof(*image_views));
    assert(NULL != image_views);
    VkDescriptorBufferInfo* buffer_views = (VkDescriptorBufferInfo*)calloc(buffer_view_count, sizeof(*buffer_views));
    assert(NULL != buffer_views);

    uint32_t write_index = 0;
    uint32_t sampler_view_index = 0;
    uint32_t texture_view_index = 0;
    uint32_t buffer_view_index = 0;
    for (uint32_t descriptor_index = 0; descriptor_index < p_descriptor_set->descriptor_count; ++descriptor_index) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[descriptor_index]);
        if ((NULL == descriptor->samplers) && (NULL == descriptor->textures) && (NULL == descriptor->uniform_buffers)) {
            continue;
        }

        writes[write_index].sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[write_index].pNext           = NULL;
        writes[write_index].dstSet          = p_descriptor_set->vk_descriptor_set;
        writes[write_index].dstBinding      = descriptor->binding;
        writes[write_index].dstArrayElement = 0;
        writes[write_index].descriptorCount = descriptor->count;
        switch (descriptor->type) {
            case tr_descriptor_type_sampler: {
                assert(NULL != descriptor->samplers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
                writes[write_index].pImageInfo = &(sampler_views[sampler_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(sampler_views[sampler_view_index]), 
                           &(descriptor->samplers[i]->vk_sampler_view),
                           sizeof(descriptor->samplers[i]->vk_sampler_view));
                    ++sampler_view_index;
                }
            }
            break;

            case tr_descriptor_type_uniform_buffer_cbv: {
                assert(NULL != descriptor->uniform_buffers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                writes[write_index].pBufferInfo = &(buffer_views[buffer_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(buffer_views[buffer_view_index]), 
                           &(descriptor->uniform_buffers[i]->vk_buffer_info),
                           sizeof(descriptor->uniform_buffers[i]->vk_buffer_info));
                    ++buffer_view_index;
                }
            }
            break;

            case tr_descriptor_type_storage_buffer_srv: {
                assert(NULL != descriptor->buffers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[write_index].pBufferInfo = &(buffer_views[buffer_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(buffer_views[buffer_view_index]), 
                           &(descriptor->buffers[i]->vk_buffer_info),
                           sizeof(descriptor->buffers[i]->vk_buffer_info));
                    ++buffer_view_index;
                }
            }
            break;

            case tr_descriptor_type_storage_buffer_uav: {
                assert(NULL != descriptor->buffers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
                writes[write_index].pBufferInfo = &(buffer_views[buffer_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(buffer_views[buffer_view_index]), 
                           &(descriptor->buffers[i]->vk_buffer_info),
                           sizeof(descriptor->buffers[i]->vk_buffer_info));
                    ++buffer_view_index;
                }
            }
            break;

            case tr_descriptor_type_uniform_texel_buffer_srv: {
                assert(NULL != descriptor->buffers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                writes[write_index].pBufferInfo = &(buffer_views[buffer_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(buffer_views[buffer_view_index]), 
                           &(descriptor->buffers[i]->vk_buffer_info),
                           sizeof(descriptor->buffers[i]->vk_buffer_info));
                    ++buffer_view_index;
                }
            }
            break;

            case tr_descriptor_type_storage_texel_buffer_uav: {
                assert(NULL != descriptor->buffers);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                writes[write_index].pBufferInfo = &(buffer_views[buffer_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(buffer_views[buffer_view_index]), 
                           &(descriptor->buffers[i]->vk_buffer_info),
                           sizeof(descriptor->buffers[i]->vk_buffer_info));
                    ++buffer_view_index;
                }
            }
            break;

            case tr_descriptor_type_texture_srv: {
                assert(NULL != descriptor->textures);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
                writes[write_index].pImageInfo = &(image_views[texture_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(image_views[texture_view_index]), 
                           &(descriptor->textures[i]->vk_texture_view),
                           sizeof(descriptor->textures[i]->vk_texture_view));
                    ++texture_view_index;
                }
            }
            break;

            case tr_descriptor_type_texture_uav: {
                assert(NULL != descriptor->textures);

                writes[write_index].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
                writes[write_index].pImageInfo = &(image_views[texture_view_index]);
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    memcpy(&(image_views[texture_view_index]), 
                           &(descriptor->textures[i]->vk_texture_view),
                           sizeof(descriptor->textures[i]->vk_texture_view));
                    ++texture_view_index;
                }
            }
            break;

        }

        ++write_index;
    }

    uint32_t copy_count = 0;
    VkCopyDescriptorSet* copies = NULL;

    vkUpdateDescriptorSets(p_renderer->vk_device, write_count, writes, copy_count, copies);

    TINY_RENDERER_SAFE_FREE(sampler_views);
    TINY_RENDERER_SAFE_FREE(image_views);
    TINY_RENDERER_SAFE_FREE(buffer_views);
    TINY_RENDERER_SAFE_FREE(writes);
}

// -------------------------------------------------------------------------------------------------
// Internal command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_internal_vk_begin_cmd(tr_cmd* p_cmd)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    TINY_RENDERER_DECLARE_ZERO(VkCommandBufferBeginInfo, begin_info);
    begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;;
    begin_info.pNext            = NULL;
    begin_info.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    begin_info.pInheritanceInfo = NULL;
    VkResult vk_res = vkBeginCommandBuffer(p_cmd->vk_cmd_buf, &begin_info);
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_end_cmd(tr_cmd* p_cmd)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    VkResult vk_res = vkEndCommandBuffer(p_cmd->vk_cmd_buf);
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);
    assert(VK_NULL_HANDLE != p_render_target->vk_render_pass);
    assert(VK_NULL_HANDLE != p_render_target->vk_framebuffer);

    TINY_RENDERER_DECLARE_ZERO(VkRect2D, render_area);
    render_area.offset.x = 0;
    render_area.offset.y = 0;
    render_area.extent.width = p_render_target->width;
    render_area.extent.height = p_render_target->height;

    // Multiply by 2 in case there's multisampling
    TINY_RENDERER_DECLARE_ZERO(VkClearValue, clear_values[(2 * tr_max_render_target_attachments) + 1]);
    
    uint32_t color_count = p_render_target->color_attachment_count;
    for (uint32_t i = 0; i < color_count; ++i) {
        clear_values[i].color.float32[0] = p_render_target->color_attachments[i]->clear_value.r;
        clear_values[i].color.float32[1] = p_render_target->color_attachments[i]->clear_value.g;
        clear_values[i].color.float32[2] = p_render_target->color_attachments[i]->clear_value.b;
        clear_values[i].color.float32[3] = p_render_target->color_attachments[i]->clear_value.a;
        if (p_render_target->sample_count > tr_sample_count_1) {
            clear_values[i].color.float32[0] = p_render_target->color_attachments_multisample[i]->clear_value.r;
            clear_values[i].color.float32[1] = p_render_target->color_attachments_multisample[i]->clear_value.g;
            clear_values[i].color.float32[2] = p_render_target->color_attachments_multisample[i]->clear_value.b;
            clear_values[i].color.float32[3] = p_render_target->color_attachments_multisample[i]->clear_value.a;
        }
    }
    uint32_t depth_stencil_count = (tr_format_undefined != p_render_target->depth_stencil_format) ? 1 : 0;
    if (depth_stencil_count > 0) {
        clear_values[color_count].depthStencil.depth = p_render_target->depth_stencil_attachment->clear_value.depth;
        clear_values[color_count].depthStencil.stencil = p_render_target->depth_stencil_attachment->clear_value.stencil;
    }

    uint32_t clear_value_count = color_count;
    if (p_render_target->sample_count > tr_sample_count_1) {
        clear_value_count *= 2;
    }
    clear_value_count += depth_stencil_count;

    TINY_RENDERER_DECLARE_ZERO(VkRenderPassBeginInfo, begin_info);
    begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    begin_info.pNext           = NULL;
    begin_info.renderPass      = p_render_target->vk_render_pass;
    begin_info.framebuffer     = p_render_target->vk_framebuffer;
    begin_info.renderArea      = render_area;
    begin_info.clearValueCount = clear_value_count;
    begin_info.pClearValues    = clear_values;

    vkCmdBeginRenderPass(p_cmd->vk_cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void tr_internal_vk_cmd_end_render(tr_cmd* p_cmd)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    vkCmdEndRenderPass(p_cmd->vk_cmd_buf);
}

void tr_internal_vk_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    TINY_RENDERER_DECLARE_ZERO(VkViewport, viewport);

    // Because some IHVs decide to implement negative viewport height differently
    if (p_cmd->cmd_pool->renderer->vk_device_ext_VK_AMD_negative_viewport_height) {
      viewport.x = x;
      viewport.y = 0;
      viewport.width = width;
      viewport.height = -height;
      viewport.minDepth = min_depth;
      viewport.maxDepth = max_depth;
    }
    else {
      viewport.x = x;
      viewport.y = height;
      viewport.width = width;
      viewport.height = -height;
      viewport.minDepth = min_depth;
      viewport.maxDepth = max_depth;
    }
    vkCmdSetViewport(p_cmd->vk_cmd_buf, 0, 1, &viewport);
}

void tr_internal_vk_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    TINY_RENDERER_DECLARE_ZERO(VkRect2D, rect);
    rect.offset.x = x;
    rect.offset.x = y;
    rect.extent.width = width;
    rect.extent.height = height;
    vkCmdSetScissor(p_cmd->vk_cmd_buf, 0, 1, &rect);
}

void tr_internal_vk_cmd_set_line_width(tr_cmd* p_cmd, float line_width)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    vkCmdSetLineWidth(p_cmd->vk_cmd_buf, line_width);
}


void tr_cmd_internal_vk_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);
    assert(NULL != s_tr_internal->bound_render_target);

    TINY_RENDERER_DECLARE_ZERO(VkClearAttachment, attachment);
    attachment.aspectMask                  = VK_IMAGE_ASPECT_COLOR_BIT;
    attachment.colorAttachment             = attachment_index;
    attachment.clearValue.color.float32[0] = clear_value->r;
    attachment.clearValue.color.float32[1] = clear_value->g;
    attachment.clearValue.color.float32[2] = clear_value->b;
    attachment.clearValue.color.float32[3] = clear_value->a;
    
    TINY_RENDERER_DECLARE_ZERO(VkClearRect, rect);
    rect.baseArrayLayer     = 0;
    rect.layerCount         = 1;
    rect.rect.offset.x      = 0;
    rect.rect.offset.y      = 0;
    rect.rect.extent.width  = s_tr_internal->bound_render_target->width;
    rect.rect.extent.height = s_tr_internal->bound_render_target->height;
    
    vkCmdClearAttachments(p_cmd->vk_cmd_buf, 1, &attachment, 1, &rect);
}

void tr_cmd_internal_vk_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
  assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);
  assert(NULL != s_tr_internal->bound_render_target);

  TINY_RENDERER_DECLARE_ZERO(VkClearAttachment, attachment);
  attachment.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  attachment.clearValue.depthStencil.depth = clear_value->depth;
  attachment.clearValue.depthStencil.stencil = clear_value->stencil;

  TINY_RENDERER_DECLARE_ZERO(VkClearRect, rect);
  rect.baseArrayLayer = 0;
  rect.layerCount = 1;
  rect.rect.offset.x = 0;
  rect.rect.offset.y = 0;
  rect.rect.extent.width = s_tr_internal->bound_render_target->width;
  rect.rect.extent.height = s_tr_internal->bound_render_target->height;

  vkCmdClearAttachments(p_cmd->vk_cmd_buf, 1, &attachment, 1, &rect);
}

void tr_internal_vk_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(p_cmd != NULL);
    assert(p_cmd->vk_cmd_buf != VK_NULL_HANDLE);
    assert(p_pipeline != NULL);

    VkPipelineBindPoint pipeline_bind_point 
        = (p_pipeline->type == tr_pipeline_type_compute) ? VK_PIPELINE_BIND_POINT_COMPUTE
                                                         : VK_PIPELINE_BIND_POINT_GRAPHICS;

    vkCmdBindPipeline(p_cmd->vk_cmd_buf, pipeline_bind_point, p_pipeline->vk_pipeline);

    //switch (p_pipeline->type) {
    //  case tr_pipeline_type_compute:
    //    vkCmdBindPipeline(p_cmd->vk_cmd_buf, VK_PIPELINE_BIND_POINT_COMPUTE, p_pipeline->vk_pipeline);
    //    break;
    //  case tr_pipeline_type_graphics:
    //    vkCmdBindPipeline(p_cmd->vk_cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, p_pipeline->vk_pipeline);
    //    break;
    //  default: break;
    //}
}

void tr_internal_vk_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set)
{
    assert(p_cmd != NULL);
    assert(p_cmd->vk_cmd_buf != VK_NULL_HANDLE);
    assert(p_pipeline != NULL);
    assert(p_pipeline->vk_pipeline_layout != VK_NULL_HANDLE);
    assert(p_descriptor_set != NULL);
    assert(p_descriptor_set->vk_descriptor_set != VK_NULL_HANDLE);

    VkPipelineBindPoint pipeline_bind_point 
        = (p_pipeline->type == tr_pipeline_type_compute) ? VK_PIPELINE_BIND_POINT_COMPUTE
                                                         : VK_PIPELINE_BIND_POINT_GRAPHICS;

    // @TODO: Add dynamic offsets support
    vkCmdBindDescriptorSets(p_cmd->vk_cmd_buf, pipeline_bind_point, 
                            p_pipeline->vk_pipeline_layout, 0, 
                            1, &(p_descriptor_set->vk_descriptor_set), 0, NULL);
}

void tr_internal_vk_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    VkIndexType vk_index_type = (tr_index_type_uint16 == p_buffer->index_type) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
    vkCmdBindIndexBuffer(p_cmd->vk_cmd_buf, p_buffer->vk_buffer, 0, vk_index_type);
}

void tr_internal_vk_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    const uint32_t max_buffers = p_cmd->cmd_pool->renderer->vk_active_gpu_properties.limits.maxVertexInputBindings;
    uint32_t capped_buffer_count = buffer_count > max_buffers ? max_buffers : buffer_count;

    // No upper bound for this, so use 64 for now
    assert(capped_buffer_count < 64);

    TINY_RENDERER_DECLARE_ZERO(VkBuffer, buffers[64]);
    TINY_RENDERER_DECLARE_ZERO(VkDeviceSize, offsets[64]);
    
    for (uint32_t i = 0; i < capped_buffer_count; ++i) {
        buffers[i] = pp_buffers[i]->vk_buffer;
    }

    vkCmdBindVertexBuffers(p_cmd->vk_cmd_buf, 0, capped_buffer_count, buffers, offsets);
}

void tr_internal_vk_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    vkCmdDraw(p_cmd->vk_cmd_buf, vertex_count, 1, first_vertex, 0);
}

void tr_internal_vk_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);

    vkCmdDrawIndexed(p_cmd->vk_cmd_buf, index_count, 1, first_index, 0, 0);
}

void tr_internal_vk_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(p_cmd != NULL);
    assert(p_cmd->vk_cmd_buf != VK_NULL_HANDLE);

    VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    VkDependencyFlags dependency_flags = 0;
    TINY_RENDERER_DECLARE_ZERO(VkBufferMemoryBarrier , barrier);
    barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    barrier.pNext               = NULL;
    barrier.srcQueueFamilyIndex = 0;
    barrier.dstQueueFamilyIndex = 0;
    barrier.buffer              = p_buffer->vk_buffer;
    barrier.offset              = 0;
    barrier.size                = VK_WHOLE_SIZE;

    switch (old_usage) {
        case tr_buffer_usage_transfer_src: {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }
        break;

        case tr_buffer_usage_transfer_dst: {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_uniform_texel_srv: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_texel_uav: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_uniform_cbv: {
            barrier.srcAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_srv: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_uav: {
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_index: {
            barrier.srcAccessMask = VK_ACCESS_INDEX_READ_BIT;
        }
        break;

        case tr_buffer_usage_vertex: {
            barrier.srcAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        break;

        case tr_buffer_usage_indirect: {
            barrier.srcAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        break;
    }

    switch (new_usage) {
        case tr_buffer_usage_transfer_src: {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }
        break;

        case tr_buffer_usage_transfer_dst: {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_uniform_texel_srv: {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_texel_uav: {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_uniform_cbv: {
            barrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_srv: {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case tr_buffer_usage_storage_uav: {
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
        }
        break;

        case tr_buffer_usage_index: {
            barrier.dstAccessMask = VK_ACCESS_INDEX_READ_BIT;
        }
        break;

        case tr_buffer_usage_vertex: {
            barrier.dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
        }
        break;

        case tr_buffer_usage_indirect: {
            barrier.dstAccessMask = VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
        }
        break;
    }

    vkCmdPipelineBarrier(p_cmd->vk_cmd_buf,
                         src_stage_mask,
                         dst_stage_mask,
                         dependency_flags,
                         0,
                         NULL,
                         1,
                         &barrier,
                         0,
                         NULL);
}

void tr_internal_vk_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(VK_NULL_HANDLE != p_cmd->vk_cmd_buf);
    assert(VK_NULL_HANDLE != p_texture->vk_image);

    VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkDependencyFlags dependency_flags = 0;
    TINY_RENDERER_DECLARE_ZERO(VkImageMemoryBarrier, barrier);

    barrier.oldLayout = tr_util_to_vk_image_layout(old_usage);
    barrier.newLayout = tr_util_to_vk_image_layout(new_usage);

    barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.pNext                           = NULL;
    barrier.oldLayout                       = tr_util_to_vk_image_layout(old_usage);
    barrier.newLayout                       = tr_util_to_vk_image_layout(new_usage);
    barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
    barrier.image                           = p_texture->vk_image;
    barrier.subresourceRange.aspectMask     = p_texture->vk_aspect_mask;
    barrier.subresourceRange.baseMipLevel   = 0;
    barrier.subresourceRange.levelCount     = p_texture->mip_levels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount     = 1;

    // oldLayout
    switch (barrier.oldLayout) 
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            barrier.srcAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:  {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_PREINITIALIZED: {
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        }
        break;  
    }

    // newImageLayout
    switch (barrier.newLayout) 
    {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            barrier.dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        }

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT;
        }
        break;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
        }
        break;                                            
    }

    vkCmdPipelineBarrier(p_cmd->vk_cmd_buf,
                         src_stage_mask,
                         dst_stage_mask,
                         dependency_flags,
                         0,
                         NULL,
                         0,
                         NULL,
                         1,
                         &barrier);
}

void tr_internal_vk_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->vk_cmd_buf);
    
    if (p_render_target->sample_count > tr_sample_count_1) {
        if (1 == p_render_target->color_attachment_count) {
            tr_texture* ss_attachment = p_render_target->color_attachments[0];
            tr_texture* ms_attachment = p_render_target->color_attachments_multisample[0];

            // This means we're dealing with a multisample swapchain
            if (tr_texture_usage_present == (ss_attachment->usage & tr_texture_usage_present)) {
                if ((tr_texture_usage_present == old_usage) && (tr_texture_usage_color_attachment == new_usage)) {
                    tr_internal_vk_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_present, tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) && (tr_texture_usage_present == new_usage)) {
                    tr_internal_vk_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_color_attachment, tr_texture_usage_present);
                }
            }
        }
    }
    else {
        if (1 == p_render_target->color_attachment_count) {
            tr_texture* attachment = p_render_target->color_attachments[0];
            // This means we're dealing with a single sample swapchain
            if (tr_texture_usage_present == (attachment->usage & tr_texture_usage_present)) {
                if ((tr_texture_usage_present == old_usage) && (tr_texture_usage_color_attachment == new_usage)) {
                    tr_internal_vk_cmd_image_transition(p_cmd, attachment, tr_texture_usage_present, tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) && (tr_texture_usage_present == new_usage)) {
                    tr_internal_vk_cmd_image_transition(p_cmd, attachment, tr_texture_usage_color_attachment, tr_texture_usage_present);
                }
            }
        }
    }
}

void tr_internal_vk_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    assert(p_cmd != NULL);
    assert(p_cmd->vk_cmd_buf != VK_NULL_HANDLE);

    vkCmdDispatch(p_cmd->vk_cmd_buf, group_count_x, group_count_y, group_count_z);
}

void tr_internal_vk_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd != NULL);
    assert(p_cmd->vk_cmd_buf != VK_NULL_HANDLE);

    VkBufferImageCopy regions = { 0 };
    regions.bufferOffset                    = buffer_offset;
    regions.bufferRowLength                 = width;
    regions.bufferImageHeight               = height;
    regions.imageSubresource.aspectMask     = p_texture->vk_aspect_mask;
    regions.imageSubresource.mipLevel       = mip_level;
    regions.imageSubresource.baseArrayLayer = 0;
    regions.imageSubresource.layerCount     = 1;
    regions.imageOffset.x                   = 0;
    regions.imageOffset.y                   = 0;
    regions.imageOffset.z                   = 0;
    regions.imageExtent.width               = width;
    regions.imageExtent.height              = height;
    regions.imageExtent.depth               = 1;

    vkCmdCopyBufferToImage(p_cmd->vk_cmd_buf, p_buffer->vk_buffer, p_texture->vk_image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions);
}

// -------------------------------------------------------------------------------------------------
// Internal queue functions
// -------------------------------------------------------------------------------------------------
void tr_internal_vk_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
    assert(VK_NULL_HANDLE != p_renderer->vk_device);
    assert(VK_NULL_HANDLE != p_renderer->vk_swapchain);

    VkSemaphore semaphore = (NULL != p_signal_semaphore) ? p_signal_semaphore->vk_semaphore : VK_NULL_HANDLE;
    VkFence fence = (NULL != p_fence) ? p_fence->vk_fence : VK_NULL_HANDLE;

    VkResult vk_res = vkAcquireNextImageKHR(p_renderer->vk_device, 
                                            p_renderer->vk_swapchain, 
                                            UINT64_MAX, 
                                            semaphore, 
                                            fence, 
                                            &(p_renderer->swapchain_image_index));
    assert(VK_SUCCESS == vk_res);

    vk_res = vkWaitForFences(p_renderer->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
    assert(VK_SUCCESS == vk_res);

    vk_res = vkResetFences(p_renderer->vk_device, 1, &fence);
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_queue_submit(
    tr_queue*      p_queue, 
    uint32_t       cmd_count,
    tr_cmd**       pp_cmds,
    uint32_t       wait_semaphore_count,
    tr_semaphore** pp_wait_semaphores,
    uint32_t       signal_semaphore_count,
    tr_semaphore** pp_signal_semaphores
)
{
    assert(VK_NULL_HANDLE != p_queue->vk_queue);

    TINY_RENDERER_DECLARE_ZERO(VkCommandBuffer, cmds[tr_max_submit_cmds]);
    cmd_count = cmd_count > tr_max_submit_cmds ? tr_max_submit_cmds : cmd_count;
    for (uint32_t i = 0; i < cmd_count; ++i) {
        cmds[i] = pp_cmds[i]->vk_cmd_buf;
    }

    TINY_RENDERER_DECLARE_ZERO(VkSemaphore, wait_semaphores[tr_max_submit_wait_semaphores]);
    TINY_RENDERER_DECLARE_ZERO(VkPipelineStageFlags, wait_masks[tr_max_submit_wait_semaphores]);
    wait_semaphore_count = wait_semaphore_count > tr_max_submit_wait_semaphores ? tr_max_submit_wait_semaphores : wait_semaphore_count;
    for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
        wait_semaphores[i] = pp_wait_semaphores[i]->vk_semaphore;
        wait_masks[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    }

    TINY_RENDERER_DECLARE_ZERO(VkSemaphore, signal_semaphores[tr_max_submit_signal_semaphores]);
    signal_semaphore_count = signal_semaphore_count > tr_max_submit_signal_semaphores ? tr_max_submit_signal_semaphores : signal_semaphore_count;
    for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
        signal_semaphores[i] = pp_signal_semaphores[i]->vk_semaphore;
    }

    TINY_RENDERER_DECLARE_ZERO(VkSubmitInfo, submit_info);
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = NULL;
    submit_info.waitSemaphoreCount   = wait_semaphore_count;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = wait_masks;
    submit_info.commandBufferCount   = cmd_count;
    submit_info.pCommandBuffers      = cmds;
    submit_info.signalSemaphoreCount = signal_semaphore_count;
    submit_info.pSignalSemaphores    = signal_semaphores;
    VkResult vk_res = vkQueueSubmit(p_queue->vk_queue, 1, &submit_info, VK_NULL_HANDLE);
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
    assert(VK_NULL_HANDLE != p_queue->vk_queue);

    tr_renderer* renderer = p_queue->renderer;

    TINY_RENDERER_DECLARE_ZERO(VkSemaphore, wait_semaphores[tr_max_present_wait_semaphores]);
    wait_semaphore_count = wait_semaphore_count > tr_max_present_wait_semaphores ? wait_semaphore_count : wait_semaphore_count;
    for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
        wait_semaphores[i] = pp_wait_semaphores[i]->vk_semaphore;
    }

    TINY_RENDERER_DECLARE_ZERO(VkPresentInfoKHR, present_info);
    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = NULL;
    present_info.waitSemaphoreCount = wait_semaphore_count;
    present_info.pWaitSemaphores    = wait_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &(renderer->vk_swapchain);
    present_info.pImageIndices      = &(renderer->swapchain_image_index);
    present_info.pResults           = NULL;

    VkResult vk_res = vkQueuePresentKHR(s_tr_internal->renderer->present_queue->vk_queue, &present_info);
    assert(VK_SUCCESS == vk_res);
}

void tr_internal_vk_queue_wait_idle(tr_queue* p_queue)
{
    assert(VK_NULL_HANDLE != p_queue->vk_queue);

    VkResult vk_res = vkQueueWaitIdle(p_queue->vk_queue);
    assert(VK_SUCCESS == vk_res);
}

#endif // TINY_RENDERER_IMPLEMENTATION

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
} // namespace TINY_RENDERER_CPP_NAMESPACE
#endif
