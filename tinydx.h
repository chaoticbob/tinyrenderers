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
 - Requires C++
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
   In one C++ file that #includes this file, do this:
      #define TINY_RENDERER_IMPLEMENTATION
   before the #include. That will create the implementation in that file.

*/

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if ! defined(_WIN32)
    #error "Windows is needed!"
#endif

//
// C++ is the only language supported by D3D12:
//     https://msdn.microsoft.com/en-us/library/windows/desktop/dn899120(v=vs.85).aspx
//
#if ! defined(__cplusplus)
    #error "D3D12 requires C++! Sorry!"
#endif 

#define TINY_RENDERER_MSW
// Pull in minimal Windows headers
#if ! defined(NOMINMAX)
    #define NOMINMAX
#endif
#if ! defined(WIN32_LEAN_AND_MEAN)
    #define WIN32_LEAN_AND_MEAN
#endif
#include <Windows.h>

#include <d3d12.h>
#include <dxgi1_5.h>

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
namespace TINY_RENDERER_CPP_NAMESPACE {
#endif

#define tr_api_export 

#if ! defined(TINY_RENDERER_CUSTOM_MAX)
enum {
    tr_max_instance_extensions       = 256,
    tr_max_device_extensions         = 256,
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
    tr_api_d3d12 = 0,
    tr_api_vulkan
} tr_api;

typedef enum tr_log_type {
    tr_log_type_info = 0,
    tr_log_type_warn,
    tr_log_type_debug,
    tr_log_type_error
} tr_log_type;

typedef void(*tr_log_fn)(tr_log_type, const char*, const char*);

/*

There's a lot of things to get right in D3D12 for it to work at all. So for now, the renderer 
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
    tr_buffer_usage_counter_uav                 = 0x00000400,
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
    tr_shader_stage_hull          = tr_shader_stage_tesc,
    tr_shader_stage_domn          = tr_shader_stage_tese,
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
    tr_front_face_cw
} tr_front_face;

// Has no effect in DX12, just here for consistency
typedef enum tr_tessellation_domain_origin {
    tr_tessellation_domain_origin_upper_left = 0,
    tr_tessellation_domain_origin_lower_left = 1,
} tr_tessellation_domain_origin;

typedef enum tr_pipeline_type {
  tr_pipeline_type_undefined = 0,
  tr_pipeline_type_compute,
  tr_pipeline_type_graphics
} tr_pipeline_type;

typedef enum tr_dx_shader_target {
    tr_dx_shader_target_5_0 = 0,
    tr_dx_shader_target_5_1,
    tr_dx_shader_target_6_0,
} tr_dx_shader_target;

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
#if defined(TINY_RENDERER_MSW)
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
    uint32_t                            count;
    const char**                        names;
} tr_string_list;

typedef struct tr_renderer_settings {
    tr_platform_handle                  handle;
    uint32_t                            width;
    uint32_t                            height;
    tr_swapchain_settings               swapchain;
    tr_log_fn                           log_fn;
    D3D_FEATURE_LEVEL                   dx_feature_level;
    tr_dx_shader_target                 dx_shader_target;
} tr_renderer_settings;

typedef struct tr_fence {
    ID3D12Fence*                        dx_fence;
} tr_fence;

typedef struct tr_semaphore {
    void*                               dx_semaphore;
} tr_semaphore;

typedef struct tr_queue {
    tr_renderer*                        renderer;
    ID3D12CommandQueue*                 dx_queue;       
    HANDLE                              dx_wait_idle_fence_event;
    ID3D12Fence*                        dx_wait_idle_fence;
    UINT64                              dx_wait_idle_fence_value;
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
#if defined(_DEBUG)
    ID3D12Debug*                        dx_debug_ctrl;
#endif
    // Use IDXGIFactory4 for now since IDXGIFactory5
    // creates problems for the Visual Studio graphics
    // debugger.
    IDXGIFactory4*                      dx_factory;
    uint32_t                            dx_gpu_count;
    IDXGIAdapter3*                      dx_gpus[tr_max_gpus];
    IDXGIAdapter3*                      dx_active_gpu;
    ID3D12Device*                       dx_device;
    // Use IDXGISwapChain3 for now since IDXGISwapChain4
    // isn't supported by older devices.
    IDXGISwapChain3*                    dx_swapchain;
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
    uint32_t                            dx_heap_offset;
    uint32_t                            dx_root_parameter_index;
} tr_descriptor;

typedef struct tr_descriptor_set {
    uint32_t                            descriptor_count;
    tr_descriptor*                      descriptors;
    ID3D12DescriptorHeap*               dx_cbvsrvuav_heap;
    ID3D12DescriptorHeap*               dx_sampler_heap;
} tr_descriptor_set;

typedef struct tr_cmd_pool {
    tr_renderer*                        renderer;
    ID3D12CommandAllocator*             dx_cmd_alloc;
} tr_cmd_pool;

typedef struct tr_cmd {
    tr_cmd_pool*                        cmd_pool;
    ID3D12GraphicsCommandList*          dx_cmd_list;
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
    ID3D12Resource*                     dx_resource;
    D3D12_CONSTANT_BUFFER_VIEW_DESC     dx_cbv_view_desc;
    D3D12_SHADER_RESOURCE_VIEW_DESC     dx_srv_view_desc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC    dx_uav_view_desc;
    D3D12_INDEX_BUFFER_VIEW             dx_index_buffer_view;  
    D3D12_VERTEX_BUFFER_VIEW            dx_vertex_buffer_view;
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
    ID3D12Resource*                     dx_resource;
    D3D12_SHADER_RESOURCE_VIEW_DESC     dx_srv_view_desc;
    D3D12_UNORDERED_ACCESS_VIEW_DESC    dx_uav_view_desc;
} tr_texture;

typedef struct tr_sampler {
    tr_renderer*                        renderer;
    D3D12_SAMPLER_DESC	                dx_sampler_desc;
} tr_sampler;

typedef struct tr_shader_program {
    tr_renderer*                        renderer;
    uint32_t                            shader_stages;
    ID3DBlob*                           dx_vert;
    ID3DBlob*                           dx_hull;
    ID3DBlob*                           dx_domn;
    ID3DBlob*                           dx_geom;
    ID3DBlob*                           dx_frag;
    ID3DBlob*                           dx_comp;
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
    tr_tessellation_domain_origin       tessellation_domain_origin; // Has no effect in DX, here for consistency
} tr_pipeline_settings;

typedef struct tr_pipeline {
    tr_renderer*                        renderer;
    tr_pipeline_settings                settings;
    tr_pipeline_type                    type;
    ID3D12RootSignature*                dx_root_signature;
    ID3D12PipelineState*                dx_pipeline_state;
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
    ID3D12DescriptorHeap*               dx_rtv_heap;
    ID3D12DescriptorHeap*               dx_dsv_heap;
} tr_render_target;

typedef struct tr_mesh {
    tr_renderer*                        renderer;
    tr_buffer*                          uniform_buffer;
    tr_buffer*                          index_buffer;
    tr_buffer*                          vertex_buffer;
    tr_shader_program*                  shader_program;
    tr_pipeline*                        pipeline;
} tr_mesh;

typedef bool(*tr_image_resize_uint8_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, uint8_t* dst_data,
                                        uint32_t channel_cout, void* user_data);

typedef bool(*tr_image_resize_float_fn)(uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* src_data, 
                                        uint32_t dst_width, uint32_t dst_height, uint32_t dst_row_stride, float* dst_data,
                                        uint32_t channel_cout, void* user_data);

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

tr_api_export void tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format format, uint32_t mip_levels, const tr_clear_value* p_clear_value, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage_flags usage, tr_texture** pp_texture);
tr_api_export void tr_destroy_texture(tr_renderer* p_renderer, tr_texture*p_texture);

tr_api_export void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler);
tr_api_export void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);

tr_api_export void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size, const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
tr_api_export void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, tr_shader_program** pp_shader_program);
tr_api_export void tr_create_shader_program_compute(tr_renderer* p_renderer, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program);
tr_api_export void tr_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program);

tr_api_export void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
tr_api_export void tr_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline);
tr_api_export void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);

tr_api_export void tr_create_render_target(tr_renderer*p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format color_format, uint32_t color_attachment_count, const tr_clear_value* p_color_clear_values, tr_format depth_stencil_format, const tr_clear_value* p_depth_stencil_clear_value, tr_render_target** pp_render_target);
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
tr_api_export uint64_t    tr_util_calc_storage_counter_offset(uint64_t buffer_size);
tr_api_export uint32_t    tr_util_calc_mip_levels(uint32_t width, uint32_t height);
tr_api_export DXGI_FORMAT tr_util_to_dx_format(tr_format format);
tr_api_export tr_format   tr_util_from_dx_format(DXGI_FORMAT fomat);
tr_api_export uint32_t    tr_util_format_stride(tr_format format);
tr_api_export uint32_t    tr_util_format_channel_count(tr_format format);
tr_api_export void        tr_util_transition_buffer(tr_queue* p_queue, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
tr_api_export void        tr_util_transition_image(tr_queue* p_queue, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
tr_api_export void        tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_buffer);
tr_api_export void        tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer);
tr_api_export void        tr_util_update_buffer(tr_queue* p_queue, uint64_t size, const void* p_src_data, tr_buffer* p_buffer);
tr_api_export void        tr_util_update_texture_uint8(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const uint8_t* p_src_data, uint32_t src_channel_count, tr_texture* p_texture, tr_image_resize_uint8_fn resize_fn, void* p_user_data);
tr_api_export void        tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data);

// =================================================================================================
// IMPLEMENTATION
// =================================================================================================

#if defined(TINY_RENDERER_IMPLEMENTATION)

#include <d3dcompiler.h>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer) \
    assert(NULL != s_tr_internal);                   \
    assert(NULL != s_tr_internal->renderer);         \
    assert(NULL != p_renderer);                      \
    assert(s_tr_internal->renderer == p_renderer);

#define TINY_RENDERER_SAFE_FREE(p_var) \
    if (NULL != p_var) {               \
       free(p_var);                    \
       p_var = NULL;                   \
    }

#if defined(__cplusplus)  
    #define TINY_RENDERER_DECLARE_ZERO(type, var) \
            type var = {};                        
#else
    #define TINY_RENDERER_DECLARE_ZERO(type, var) \
            type var = {0};                        
#endif

#define TINY_RENDERER_SAFE_RELEASE(p_var) \
    if (NULL != p_var) {               \
       p_var->Release();               \
       p_var = NULL;                   \
    }

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
D3D12_RESOURCE_STATES tr_util_to_dx_resource_state_texture(tr_texture_usage_flags usage);

// Internal init functions
void tr_internal_dx_create_device(tr_renderer* p_renderer);
void tr_internal_dx_create_swapchain(tr_renderer* p_renderer);
void tr_internal_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_dx_create_swapchain_renderpass(tr_renderer* p_renderer);
void tr_internal_dx_destroy_device(tr_renderer* p_renderer);
void tr_internal_dx_destroy_swapchain(tr_renderer* p_renderer);


// Internal create functions
void tr_internal_dx_create_fence(tr_renderer *p_renderer, tr_fence* p_fence);
void tr_internal_dx_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence);
void tr_internal_dx_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);
void tr_internal_dx_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);
void tr_internal_dx_create_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);
void tr_internal_dx_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);
void tr_internal_dx_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool* p_cmd_pool);
void tr_internal_dx_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool);
void tr_internal_dx_create_cmd(tr_cmd_pool *p_cmd_pool, bool secondary, tr_cmd* p_cmd);
void tr_internal_dx_destroy_cmd(tr_cmd_pool *p_cmd_pool, tr_cmd* p_cmd);
void tr_internal_dx_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_dx_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);
void tr_internal_dx_create_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_dx_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture);
void tr_internal_dx_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_dx_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler);
void tr_internal_dx_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline);
void tr_internal_dx_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline);
void tr_internal_dx_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline);
void tr_internal_dx_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size, const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program* p_shader_program);
void tr_internal_dx_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program);
void tr_internal_dx_create_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);
void tr_internal_dx_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);

// Internal descriptor set functions
void tr_internal_dx_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set);

// Internal command buffer functions
void tr_internal_dx_begin_cmd(tr_cmd* p_cmd);
void tr_internal_dx_end_cmd(tr_cmd* p_cmd);
void tr_internal_dx_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
void tr_internal_dx_cmd_end_render(tr_cmd* p_cmd);
void tr_internal_dx_cmd_set_viewport(tr_cmd* p_cmd, float x, float, float width, float height, float min_depth, float max_depth);
void tr_internal_dx_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void tr_cmd_internal_dx_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value);
void tr_cmd_internal_dx_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value);
void tr_internal_dx_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
void tr_internal_dx_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set);
void tr_internal_dx_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer);
void tr_internal_dx_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers);
void tr_internal_dx_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex);
void tr_internal_dx_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index);
void tr_internal_dx_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);
void tr_internal_dx_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_texture, tr_buffer_usage old_usage, tr_buffer_usage new_usage);
void tr_internal_dx_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_dx_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_dx_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage);
void tr_internal_dx_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);
void tr_internal_dx_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture);

// Internal queue/swapchain functions
void tr_internal_dx_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
void tr_internal_dx_queue_submit(tr_queue* p_queue, uint32_t cmd_count, tr_cmd** pp_cmds, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores, uint32_t signal_semaphore_count, tr_semaphore** pp_signal_semaphores);
void tr_internal_dx_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);
void tr_internal_dx_queue_wait_idle(tr_queue* p_queue);

// Functions points for functions that need to be loaded
PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER           fnD3D12CreateRootSignatureDeserializer          = NULL;
PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE           fnD3D12SerializeVersionedRootSignature          = NULL;
PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER fnD3D12CreateVersionedRootSignatureDeserializer = NULL;

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

        // Allocate storage for graphics queue
        p_renderer->graphics_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->graphics_queue));
        assert(NULL != p_renderer->graphics_queue);
        // Writes to swapchain back buffers need to happen on the same queue as the one that
        // a swapchain uses to present. So just point the present queue at the graphics queue.
        p_renderer->present_queue = p_renderer->graphics_queue;

        p_renderer->graphics_queue->renderer = p_renderer;
        p_renderer->present_queue->renderer = p_renderer;

        // Initialize the D3D12 bits
        {
            tr_internal_dx_create_device(p_renderer);
            tr_internal_dx_create_swapchain(p_renderer);
        }

        // Allocate and configure render target objects
        tr_internal_create_swapchain_renderpass(p_renderer);

        // Initialize the D3D12 bits of the render targets
        tr_internal_dx_create_swapchain_renderpass(p_renderer);

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
    tr_internal_dx_destroy_swapchain(p_renderer);
    //tr_internal_dx_destroy_surface(p_renderer);
    tr_internal_dx_destroy_device(p_renderer);
    //tr_internal_dx_destroy_instance(p_renderer);

    // Free all the renderer components!
    TINY_RENDERER_SAFE_FREE(p_renderer->swapchain_render_targets);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_fences);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_semaphores);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->render_complete_semaphores);
    // No need to destroy the present queue since it's just a pointer to the graphics queue
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->graphics_queue);
    TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer);
    TINY_RENDERER_SAFE_FREE(s_tr_internal);
}

void tr_create_fence(tr_renderer *p_renderer, tr_fence** pp_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_fence* p_fence = (tr_fence*)calloc(1, sizeof(*p_fence));
    assert(NULL != p_fence);

    tr_internal_dx_create_fence(p_renderer, p_fence);

    *pp_fence = p_fence;
}

void tr_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_fence);

    tr_internal_dx_destroy_fence(p_renderer, p_fence);

    TINY_RENDERER_SAFE_FREE(p_fence);
}

void tr_create_semaphore(tr_renderer *p_renderer, tr_semaphore** pp_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_semaphore* p_semaphore = (tr_semaphore*)calloc(1, sizeof(*p_semaphore));
    assert(NULL != p_semaphore);

    tr_internal_dx_create_semaphore(p_renderer, p_semaphore);

    *pp_semaphore = p_semaphore;
}

void tr_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_semaphore);

    tr_internal_dx_destroy_semaphore(p_renderer, p_semaphore);

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

    for (uint32_t i = 0; i < descriptor_count; ++i) {
        p_descriptor_set->descriptors[i].dx_root_parameter_index = 0xFFFFFFFF;
    }

    tr_internal_dx_create_descriptor_set(p_renderer, p_descriptor_set);

    *pp_descriptor_set = p_descriptor_set;
}

void tr_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set->descriptors);

    tr_internal_dx_destroy_descriptor_set(p_renderer, p_descriptor_set);

    TINY_RENDERER_SAFE_FREE(p_descriptor_set);
}

void tr_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool** pp_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_cmd_pool* p_cmd_pool = (tr_cmd_pool*)calloc(1, sizeof(*p_cmd_pool));
    assert(NULL != p_cmd_pool);

    p_cmd_pool->renderer = p_renderer;

    tr_internal_dx_create_cmd_pool(p_renderer, p_queue, transient, p_cmd_pool);
    
    *pp_cmd_pool = p_cmd_pool;
}

void tr_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_cmd_pool);

    tr_internal_dx_destroy_cmd_pool(p_renderer, p_cmd_pool);

    TINY_RENDERER_SAFE_FREE(p_cmd_pool);
}

void tr_create_cmd(tr_cmd_pool* p_cmd_pool, bool secondary, tr_cmd** pp_cmd)
{
    assert(NULL != p_cmd_pool);

    tr_cmd* p_cmd = (tr_cmd*)calloc(1, sizeof(*p_cmd));
    assert(NULL != p_cmd);

    p_cmd->cmd_pool = p_cmd_pool;

    tr_internal_dx_create_cmd(p_cmd_pool, secondary, p_cmd);
    
    *pp_cmd = p_cmd;
}

void tr_destroy_cmd(tr_cmd_pool* p_cmd_pool, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool);
    assert(NULL != p_cmd);

    tr_internal_dx_destroy_cmd(p_cmd_pool, p_cmd);

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
    
    tr_internal_dx_create_buffer(p_renderer, p_buffer);

    *pp_buffer = p_buffer;
}

void tr_create_index_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_index_type index_type, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_index, size, host_visible, pp_buffer);
    (*pp_buffer)->index_type = index_type;
    (*pp_buffer)->dx_index_buffer_view.Format = (tr_index_type_uint16 == index_type) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

void tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_uniform_cbv, size, host_visible, pp_buffer);
}

void tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, uint32_t vertex_stride, tr_buffer** pp_buffer)
{
    tr_create_buffer(p_renderer, tr_buffer_usage_vertex, size, host_visible, pp_buffer);
    (*pp_buffer)->vertex_stride = vertex_stride;
    (*pp_buffer)->dx_vertex_buffer_view.StrideInBytes = vertex_stride;
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
    p_buffer->struct_stride   = raw ? 0 : struct_stride;
    p_buffer->raw             = raw;
   
    tr_internal_dx_create_buffer(p_renderer, p_buffer);

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
      p_counter_buffer->usage           = tr_buffer_usage_counter_uav;
      p_counter_buffer->size            = 4;
      p_counter_buffer->host_visible    = false;
      p_counter_buffer->format          = tr_format_undefined;
      p_counter_buffer->first_element   = 0;
      p_counter_buffer->element_count   = 1;
      p_counter_buffer->struct_stride   = 4;
    
      tr_internal_dx_create_buffer(p_renderer, p_counter_buffer);

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
      p_buffer->struct_stride   = raw ? 0 : struct_stride;
      p_buffer->raw             = raw;

      if (pp_counter_buffer != NULL) {
        p_buffer->counter_buffer = *pp_counter_buffer;
      }
    
      tr_internal_dx_create_buffer(p_renderer, p_buffer);

      *pp_buffer = p_buffer;
    }
}

void tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_buffer);

    tr_internal_dx_destroy_buffer(p_renderer, p_buffer);

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
    p_texture->host_visible       = false;
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

    tr_internal_dx_create_texture(p_renderer, p_texture);

    *pp_texture = p_texture;

    if (host_visible) {
        tr_internal_log(tr_log_type_warn, "D3D12 does not support host visible textures, memory of resulting texture will not be mapped for CPU visibility", "tr_create_texture");
    }
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
    tr_renderer*             p_renderer, 
    uint32_t                 width, 
    uint32_t                 height, 
    tr_sample_count          sample_count, 
    tr_format                format, 
    uint32_t                 mip_levels,
    const tr_clear_value*    p_clear_value,  
    bool                     host_visible,
    tr_texture_usage_flags   usage, 
    tr_texture**             pp_texture
)
{
    if (tr_max_mip_levels == mip_levels) {
        mip_levels = tr_util_calc_mip_levels(width, height);
    }

    tr_create_texture(p_renderer, tr_texture_type_2d, width, height, 1, sample_count, format, mip_levels, p_clear_value, host_visible, usage, pp_texture);
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

    tr_internal_dx_destroy_texture(p_renderer, p_texture);

    TINY_RENDERER_SAFE_FREE(p_texture);
}

void tr_create_sampler(tr_renderer* p_renderer, tr_sampler** pp_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_sampler* p_sampler = (tr_sampler*)calloc(1, sizeof(*p_sampler));
    assert(NULL != p_sampler);

    p_sampler->renderer = p_renderer;

    tr_internal_dx_create_sampler(p_renderer, p_sampler);

    *pp_sampler = p_sampler;
}

void tr_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_sampler);

    tr_internal_dx_destroy_sampler(p_renderer, p_sampler);

    TINY_RENDERER_SAFE_FREE(p_sampler);
}

void tr_create_shader_program_n(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size, const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program** pp_shader_program)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    if (vert_size > 0) {
        assert(NULL != vert_code);
    }
    if (hull_size > 0) {
        assert(NULL != hull_code);
    }
    if (domn_size > 0) {
        assert(NULL != domn_code);
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
    p_shader_program->shader_stages |= (hull_size > 0) ? tr_shader_stage_tesc : 0;
    p_shader_program->shader_stages |= (domn_size > 0) ? tr_shader_stage_tese : 0;
    p_shader_program->shader_stages |= (geom_size > 0) ? tr_shader_stage_geom : 0;
    p_shader_program->shader_stages |= (frag_size > 0) ? tr_shader_stage_frag : 0;
    p_shader_program->shader_stages |= (comp_size > 0) ? tr_shader_stage_comp : 0;

    tr_internal_dx_create_shader_program(p_renderer, vert_size, vert_code, vert_enpt, hull_size, hull_code, hull_enpt, domn_size, domn_code, domn_enpt, geom_size, geom_code, geom_enpt, frag_size, frag_code, frag_enpt, comp_size, comp_code, comp_enpt, p_shader_program);

    *pp_shader_program = p_shader_program;
}

void tr_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, tr_shader_program** pp_shader_program)
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

    tr_internal_dx_destroy_shader_program(p_renderer, p_shader_program);
}

void tr_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline** pp_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_shader_program);
    assert(NULL != p_render_target);
    assert(NULL != p_pipeline_settings);

    tr_pipeline* p_pipeline = (tr_pipeline*)calloc(1, sizeof(*p_pipeline));
    assert(NULL != p_pipeline);

    memcpy(&(p_pipeline->settings), p_pipeline_settings, sizeof(*p_pipeline_settings));

    tr_internal_dx_create_pipeline(p_renderer, p_shader_program, p_vertex_layout, p_descriptor_set, p_render_target, p_pipeline_settings, p_pipeline);
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

    tr_internal_dx_create_compute_pipeline(p_renderer, p_shader_program, p_descriptor_set, p_pipeline_settings, p_pipeline);
    p_pipeline->type = tr_pipeline_type_compute;

    *pp_pipeline = p_pipeline;
}

void tr_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);
    assert(NULL != p_pipeline);

    tr_internal_dx_destroy_pipeline(p_renderer, p_pipeline);

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
    tr_internal_dx_create_render_target(p_renderer, p_render_target);

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
        if (NULL != p_render_target->depth_stencil_attachment_multisample) {
          tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment_multisample);
        }
        if (NULL != p_render_target->depth_stencil_attachment) {
            tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment);
        }

        /*
        // Destroy VkRenderPass object
        if (VK_NULL_HANDLE != p_render_target->vk_render_pass) {
            vkDestroyRenderPass(p_renderer->vk_device, p_render_target->vk_render_pass, NULL);
        }

        // Destroy VkFramebuffer object
        if (VK_NULL_HANDLE != p_render_target->vk_framebuffer) {
            vkDestroyFramebuffer(p_renderer->vk_device, p_render_target->vk_framebuffer, NULL);
        }
        */
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

    tr_internal_dx_update_descriptor_set(p_renderer, p_descriptor_set);
}

// -------------------------------------------------------------------------------------------------
// Command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_dx_begin_cmd(p_cmd);
}

void tr_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_dx_end_cmd(p_cmd);
}

void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    s_tr_internal->bound_render_target = p_render_target;

    tr_internal_dx_cmd_begin_render(p_cmd, p_render_target);
}

void tr_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd);

    tr_internal_dx_cmd_end_render(p_cmd);

    s_tr_internal->bound_render_target = NULL;
}

void tr_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth)
{
    assert(NULL != p_cmd);

    tr_internal_dx_cmd_set_viewport(p_cmd, x, y, width, height, min_depth, max_depth);
}

void tr_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(NULL != p_cmd);

    tr_internal_dx_cmd_set_scissor(p_cmd, x, y, width, height);
}

void tr_cmd_set_line_width(tr_cmd* p_cmd, float line_width)
{
    // Purposely does nothing
}

void tr_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd);

    tr_cmd_internal_dx_cmd_clear_color_attachment(p_cmd, attachment_index, clear_value);
}

void tr_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
  assert(NULL != p_cmd);

  tr_cmd_internal_dx_cmd_clear_depth_stencil_attachment(p_cmd, clear_value);
}

void tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);

    tr_internal_dx_cmd_bind_pipeline(p_cmd, p_pipeline);
}

void tr_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd);
    assert(NULL != p_pipeline);
    assert(NULL != p_descriptor_set);

    tr_internal_dx_cmd_bind_descriptor_sets(p_cmd, p_pipeline, p_descriptor_set);
}

void tr_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_dx_cmd_bind_index_buffer(p_cmd, p_buffer);
}

void tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd);
    assert(0 != buffer_count);
    assert(NULL != pp_buffers);

    tr_internal_dx_cmd_bind_vertex_buffers(p_cmd, buffer_count, pp_buffers);
}

void tr_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd);

    tr_internal_dx_cmd_draw(p_cmd, vertex_count, first_vertex);
}


void tr_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd);

    tr_internal_dx_cmd_draw_indexed(p_cmd, index_count, first_index);
}

void tr_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_buffer);

    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, old_usage, new_usage);
}

void tr_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_texture);

    tr_internal_dx_cmd_image_transition(p_cmd, p_texture, old_usage, new_usage);
}

void tr_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd);
    assert(NULL != p_render_target);

    tr_internal_dx_cmd_render_target_transition(p_cmd, p_render_target, old_usage, new_usage);
}

void tr_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
  assert(NULL != p_cmd);
  assert(NULL != p_render_target);

  tr_internal_dx_cmd_depth_stencil_transition(p_cmd, p_render_target, old_usage, new_usage);
}

void tr_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    assert(NULL != p_cmd);
    tr_internal_dx_cmd_dispatch(p_cmd, group_count_x, group_count_y, group_count_z);
}

void tr_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd != NULL);
    assert(p_buffer != NULL);
    assert(p_texture != NULL);

    tr_internal_dx_cmd_copy_buffer_to_texture2d(p_cmd, width, height, row_pitch, buffer_offset, mip_level, p_buffer, p_texture);
}

void tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
    TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer);

    tr_internal_dx_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
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

    tr_internal_dx_queue_submit(p_queue, 
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

    tr_internal_dx_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
}

void tr_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue);

    tr_internal_dx_queue_wait_idle(p_queue);
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
uint64_t tr_util_calc_storage_counter_offset(uint64_t buffer_size)
{
		uint64_t alignment = D3D12_UAV_COUNTER_PLACEMENT_ALIGNMENT;
		uint64_t result = (buffer_size + (alignment - 1)) & ~(alignment - 1);
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

DXGI_FORMAT tr_util_to_dx_format(tr_format format)
{
    DXGI_FORMAT result = DXGI_FORMAT_UNKNOWN;
    switch (format) {
        // 1 channel
        case tr_format_r8_unorm            : result = DXGI_FORMAT_R8_UNORM; break;
        case tr_format_r16_unorm           : result = DXGI_FORMAT_R16_UNORM; break;
        case tr_format_r16_float           : result = DXGI_FORMAT_R16_FLOAT; break;
        case tr_format_r32_uint            : result = DXGI_FORMAT_R32_UINT; break;
        case tr_format_r32_float           : result = DXGI_FORMAT_R32_FLOAT; break;
        // 2 channel
        case tr_format_r8g8_unorm          : result = DXGI_FORMAT_R8G8_UNORM; break;
        case tr_format_r16g16_unorm        : result = DXGI_FORMAT_R16G16_UNORM; break;
        case tr_format_r16g16_float        : result = DXGI_FORMAT_R16G16_FLOAT; break;
        case tr_format_r32g32_uint         : result = DXGI_FORMAT_R32G32_UINT; break;
        case tr_format_r32g32_float        : result = DXGI_FORMAT_R32G32_FLOAT; break;
        // 3 channel
        case tr_format_r32g32b32_uint      : result = DXGI_FORMAT_R32G32B32_UINT; break;
        case tr_format_r32g32b32_float     : result = DXGI_FORMAT_R32G32B32_FLOAT; break;
        // 4 channel
        case tr_format_b8g8r8a8_unorm      : result = DXGI_FORMAT_B8G8R8A8_UNORM; break;
        case tr_format_r8g8b8a8_unorm      : result = DXGI_FORMAT_R8G8B8A8_UNORM; break;
        case tr_format_r16g16b16a16_unorm  : result = DXGI_FORMAT_R16G16B16A16_UNORM; break;
        case tr_format_r16g16b16a16_float  : result = DXGI_FORMAT_R16G16B16A16_FLOAT; break;
        case tr_format_r32g32b32a32_uint   : result = DXGI_FORMAT_R32G32B32A32_UINT; break;
        case tr_format_r32g32b32a32_float  : result = DXGI_FORMAT_R32G32B32A32_FLOAT; break;
        // Depth/stencil
        case tr_format_d16_unorm           : result = DXGI_FORMAT_D16_UNORM; break;
        case tr_format_x8_d24_unorm_pack32 : result = DXGI_FORMAT_X32_TYPELESS_G8X24_UINT; break;
        case tr_format_d32_float           : result = DXGI_FORMAT_D32_FLOAT; break;
        case tr_format_d24_unorm_s8_uint   : result = DXGI_FORMAT_D24_UNORM_S8_UINT; break;
        case tr_format_d32_float_s8_uint   : result = DXGI_FORMAT_D32_FLOAT_S8X24_UINT; break;
    }
    return result;
}

tr_format tr_util_from_dx_format(DXGI_FORMAT format)
{
    tr_format result = tr_format_undefined;
    switch (format) {
        // 1 channel
        case DXGI_FORMAT_R8_UNORM                : result = tr_format_r8_unorm; break;
        case DXGI_FORMAT_R16_UNORM               : result = tr_format_r16_unorm; break;
        case DXGI_FORMAT_R16_FLOAT               : result = tr_format_r16_float; break;
        case DXGI_FORMAT_R32_UINT                : result = tr_format_r32_uint; break;
        case DXGI_FORMAT_R32_FLOAT               : result = tr_format_r32_float; break;
        // 2 channel
        case DXGI_FORMAT_R8G8_UNORM              : result = tr_format_r8g8_unorm; break;
        case DXGI_FORMAT_R16G16_UNORM            : result = tr_format_r16g16_unorm; break;
        case DXGI_FORMAT_R16G16_FLOAT            : result = tr_format_r16g16_float; break;
        case DXGI_FORMAT_R32G32_UINT             : result = tr_format_r32g32_uint; break;
        case DXGI_FORMAT_R32G32_FLOAT            : result = tr_format_r32g32_float; break;
        // 3 channel
        case DXGI_FORMAT_R32G32B32_UINT          : result = tr_format_r32g32b32_uint; break;
        case DXGI_FORMAT_R32G32B32_FLOAT         : result = tr_format_r32g32b32_float; break;
        // 4 channel
        case DXGI_FORMAT_B8G8R8A8_UNORM          : result = tr_format_b8g8r8a8_unorm; break;
        case DXGI_FORMAT_R8G8B8A8_UNORM          : result = tr_format_r8g8b8a8_unorm; break;
        case DXGI_FORMAT_R16G16B16A16_UNORM      : result = tr_format_r16g16b16a16_unorm; break;
        case DXGI_FORMAT_R16G16B16A16_FLOAT      : result = tr_format_r16g16b16a16_float; break;
        case DXGI_FORMAT_R32G32B32A32_UINT       : result = tr_format_r32g32b32a32_uint; break;
        case DXGI_FORMAT_R32G32B32A32_FLOAT      : result = tr_format_r32g32b32a32_float; break;
        // Depth/stencil
        case DXGI_FORMAT_D16_UNORM               : result = tr_format_d16_unorm; break;
        case DXGI_FORMAT_X32_TYPELESS_G8X24_UINT : result = tr_format_x8_d24_unorm_pack32; break;
        case DXGI_FORMAT_D32_FLOAT               : result = tr_format_d32_float; break;
        case DXGI_FORMAT_D24_UNORM_S8_UINT       : result = tr_format_d24_unorm_s8_uint; break;
        case DXGI_FORMAT_D32_FLOAT_S8X24_UINT    : result = tr_format_d32_float_s8_uint; break;
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

    //
    // D3D12 textures are created with the following resources states (tr_texture_usage_sampled_image):
    //     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    //
    // So if the old_usage is tr_texture_usage_undefined and new_usage is tr_texture_usage_sampled_image 
    // just skip it!
    //
    if ((old_usage == tr_texture_usage_undefined) && (new_usage == tr_texture_usage_sampled_image)) {
      return;
    }
    
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

void tr_util_set_storage_buffer_count(tr_queue* p_queue, uint64_t count_offset, uint32_t count, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->dx_resource);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, 256, true, &buffer);
    uint32_t* mapped_ptr = (uint32_t*)buffer->cpu_mapped_address;
    *(mapped_ptr) = count;
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_storage_uav, tr_buffer_usage_transfer_dst);
    p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, count_offset,
                                         buffer->dx_resource, 0,
                                         4);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, tr_buffer_usage_storage_uav);
    tr_end_cmd(p_cmd);

    tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
    tr_queue_wait_idle(p_queue);

    tr_destroy_cmd(p_cmd_pool, p_cmd);
    tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

    tr_destroy_buffer(p_buffer->renderer, buffer);
}

void tr_util_clear_buffer(tr_queue* p_queue, tr_buffer* p_buffer)
{
    assert(NULL != p_queue);
    assert(NULL != p_buffer);
    assert(NULL != p_buffer->dx_resource);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, p_buffer->size, true, &buffer);
    memset(buffer->cpu_mapped_address, 0, buffer->size);
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, 0,
                                         buffer->dx_resource, 0,
                                         p_buffer->size);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
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
    assert(NULL != p_buffer->dx_resource);
    assert(p_buffer->size >= size);

    tr_buffer* buffer = NULL;
    tr_create_buffer(p_buffer->renderer, tr_buffer_usage_transfer_src, size, true, &buffer);
    memcpy(buffer->cpu_mapped_address, p_src_data, size);
    
    tr_cmd_pool* p_cmd_pool = NULL;
    tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

    tr_cmd* p_cmd = NULL;
    tr_create_cmd(p_cmd_pool, false, &p_cmd);

    tr_begin_cmd(p_cmd);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, p_buffer->usage, tr_buffer_usage_transfer_dst);
    p_cmd->dx_cmd_list->CopyBufferRegion(p_buffer->dx_resource, 0,
                                         buffer->dx_resource, 0,
                                         size);
    tr_internal_dx_cmd_buffer_transition(p_cmd, p_buffer, tr_buffer_usage_transfer_dst, p_buffer->usage);
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
    assert(NULL != p_texture->dx_resource);
    assert((src_width > 0) && (src_height > 0) && (src_row_stride > 0));
    assert(tr_sample_count_1 == p_texture->sample_count);

    uint8_t* p_expanded_src_data = NULL;
    const uint32_t dst_channel_count = tr_util_format_channel_count(p_texture->format);
    assert(src_channel_count <= dst_channel_count);

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

    // Get resource layout and memory requirements for all mip levels
    TINY_RENDERER_DECLARE_ZERO(D3D12_RESOURCE_DESC, tex_resource_desc);
    tex_resource_desc.Dimension              = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    tex_resource_desc.Alignment              = 0;
    tex_resource_desc.Width                  = (UINT)p_texture->width;
    tex_resource_desc.Height                 = (UINT)p_texture->height;
    tex_resource_desc.DepthOrArraySize       = (UINT16)p_texture->depth;
    tex_resource_desc.MipLevels              = (UINT16)p_texture->mip_levels;
    tex_resource_desc.Format                 = tr_util_to_dx_format(p_texture->format);
    tex_resource_desc.SampleDesc.Count       = (UINT)p_texture->sample_count;
    tex_resource_desc.SampleDesc.Quality     = (UINT)p_texture->sample_quality;
    tex_resource_desc.Layout                 = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    tex_resource_desc.Flags                  = D3D12_RESOURCE_FLAG_NONE;    
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT* subres_layouts = (D3D12_PLACED_SUBRESOURCE_FOOTPRINT*)calloc(p_texture->mip_levels, sizeof(*subres_layouts));
    UINT* subres_rowcounts = (UINT*)calloc(p_texture->mip_levels, sizeof(*subres_rowcounts));
    UINT64* subres_row_strides = (UINT64*)calloc(p_texture->mip_levels, sizeof(*subres_row_strides));
    UINT64 buffer_size = 0;
    p_queue->renderer->dx_device->GetCopyableFootprints(&tex_resource_desc, 0, p_texture->mip_levels, 0, subres_layouts, subres_rowcounts, subres_row_strides, &buffer_size);
    // Create temporary buffer big enough to fit all mip levels
    tr_buffer* buffer = NULL;
    tr_create_buffer(p_texture->renderer, tr_buffer_usage_transfer_src, buffer_size, true, &buffer);
    // Use default simple resize if a resize function was not supplied
    if (NULL == resize_fn) {
        resize_fn = &tr_image_resize_uint8_t;
    }
    // Resize image into appropriate mip level
    uint32_t dst_width = p_texture->width;
    uint32_t dst_height = p_texture->height;
    for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT* subres_layout = &subres_layouts[mip_level];
        uint32_t dst_row_stride = subres_layout->Footprint.RowPitch;
        uint8_t* p_dst_data = (uint8_t*)buffer->cpu_mapped_address + subres_layout->Offset;
        resize_fn(src_width, src_height, src_row_stride, p_src_data, dst_width, dst_height, dst_row_stride, p_dst_data, dst_channel_count, p_user_data);
        dst_width >>= 1;
        dst_height >>= 1;
    }

    // Copy buffer to texture
    {      
        tr_cmd_pool* p_cmd_pool = NULL;
        tr_create_cmd_pool(p_queue->renderer, p_queue, true, &p_cmd_pool);

        tr_cmd* p_cmd = NULL;
        tr_create_cmd(p_cmd_pool, false, &p_cmd);

        tr_begin_cmd(p_cmd);
        //
        // D3D12 textures are created with the following resources states (tr_texture_usage_sampled_image):
        //     D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
        //
        tr_internal_dx_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_sampled_image, tr_texture_usage_transfer_dst);
        for (uint32_t mip_level = 0; mip_level < p_texture->mip_levels; ++mip_level) {
		        const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& layout = subres_layouts[mip_level];
            TINY_RENDERER_DECLARE_ZERO(D3D12_TEXTURE_COPY_LOCATION, src);
		        src.pResource       = buffer->dx_resource;
		        src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		        src.PlacedFootprint = layout;
            TINY_RENDERER_DECLARE_ZERO(D3D12_TEXTURE_COPY_LOCATION, dst);
		        dst.pResource        = p_texture->dx_resource;
		        dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		        dst.SubresourceIndex = mip_level;

		        p_cmd->dx_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
        }        
        tr_internal_dx_cmd_image_transition(p_cmd, p_texture, tr_texture_usage_transfer_dst, tr_texture_usage_sampled_image);
        tr_end_cmd(p_cmd);

        tr_queue_submit(p_queue, 1, &p_cmd, 0, NULL, 0, NULL);
        tr_queue_wait_idle(p_queue);

        tr_destroy_cmd(p_cmd_pool, p_cmd);
        tr_destroy_cmd_pool(p_queue->renderer, p_cmd_pool);

        tr_destroy_buffer(p_texture->renderer, buffer);
    }

    TINY_RENDERER_SAFE_FREE(subres_layouts);
    TINY_RENDERER_SAFE_FREE(subres_rowcounts);
    TINY_RENDERER_SAFE_FREE(subres_row_strides);
}

void tr_util_update_texture_float(tr_queue* p_queue, uint32_t src_width, uint32_t src_height, uint32_t src_row_stride, const float* p_src_data, uint32_t channels, tr_texture* p_texture, tr_image_resize_float_fn resize_fn, void* p_user_data)
{
}

// -------------------------------------------------------------------------------------------------
// Internal utility functions
// -------------------------------------------------------------------------------------------------
D3D12_RESOURCE_STATES tr_util_to_dx_resource_state_buffer(tr_buffer_usage usage)
{
    D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
    if (tr_buffer_usage_transfer_src == (usage & tr_buffer_usage_transfer_src)) {
        result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (tr_buffer_usage_transfer_dst == (usage & tr_buffer_usage_transfer_dst)) {
        result |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (tr_buffer_usage_storage_srv == (usage & tr_buffer_usage_storage_srv)) {
        result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (tr_buffer_usage_storage_uav == (usage & tr_buffer_usage_storage_uav)) {
        result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    return result;
}

D3D12_RESOURCE_STATES tr_util_to_dx_resource_state_texture(tr_texture_usage_flags usage)
{
    D3D12_RESOURCE_STATES result = D3D12_RESOURCE_STATE_COMMON;
    if (tr_texture_usage_transfer_src == (usage & tr_texture_usage_transfer_src)) {
        result |= D3D12_RESOURCE_STATE_COPY_SOURCE;
    }
    if (tr_texture_usage_transfer_dst == (usage & tr_texture_usage_transfer_dst)) {
        result |= D3D12_RESOURCE_STATE_COPY_DEST;
    }
    if (tr_texture_usage_sampled_image == (usage & tr_texture_usage_sampled_image)) {
        result |= D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
    if (tr_texture_usage_storage_image == (usage & tr_texture_usage_storage_image)) {
        result |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
    }
    if (tr_texture_usage_color_attachment == (usage & tr_texture_usage_color_attachment)) {
        result |= D3D12_RESOURCE_STATE_RENDER_TARGET;
    }
    if (tr_texture_usage_depth_stencil_attachment == (usage & tr_texture_usage_depth_stencil_attachment)) {
        result |= D3D12_RESOURCE_STATE_DEPTH_WRITE;
    }
    if (tr_texture_usage_resolve_src == (usage & tr_texture_usage_resolve_src)) {
        result |= D3D12_RESOURCE_STATE_RESOLVE_SOURCE;
    }
    if (tr_texture_usage_resolve_dst == (usage & tr_texture_usage_resolve_dst)) {
        result |= D3D12_RESOURCE_STATE_RESOLVE_DEST;
    }
    return result;
}

// -------------------------------------------------------------------------------------------------
// Internal init functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_create_device(tr_renderer* p_renderer)
{
#if defined( _DEBUG )
    if (SUCCEEDED(D3D12GetDebugInterface(__uuidof(p_renderer->dx_debug_ctrl), (void**)&(p_renderer->dx_debug_ctrl)))) {
        p_renderer->dx_debug_ctrl->EnableDebugLayer();
    }
#endif

    UINT flags = 0;
#if defined( _DEBUG )
    flags = DXGI_CREATE_FACTORY_DEBUG;
#endif
    HRESULT hres = CreateDXGIFactory2(flags, __uuidof(p_renderer->dx_factory), (void**)&(p_renderer->dx_factory));
    assert(SUCCEEDED(hres));

    D3D_FEATURE_LEVEL gpu_feature_levels[tr_max_gpus];
    for (uint32_t i = 0; i < tr_max_gpus; ++i) {
      gpu_feature_levels[i] = (D3D_FEATURE_LEVEL)0;
    }

    IDXGIAdapter1* adapter = NULL;
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != p_renderer->dx_factory->EnumAdapters1(i, &adapter); ++i) {
        TINY_RENDERER_DECLARE_ZERO(DXGI_ADAPTER_DESC1, desc);
        adapter->GetDesc1(&desc);
		// Skip software adapters
		if (desc.Flags &DXGI_ADAPTER_FLAG_SOFTWARE) {
			adapter->Release();
			continue;
		}
        // Make sure the adapter can support a D3D12 device
        if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(p_renderer->dx_device), NULL))) {
            hres = adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres)) {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_12_1;
                ++p_renderer->dx_gpu_count;
            }
            adapter->Release();
        }
        else if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, __uuidof(p_renderer->dx_device), NULL))) {
            hres = adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
            if (SUCCEEDED(hres)) {
                gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_12_0;
                ++p_renderer->dx_gpu_count;
            }
            adapter->Release();
        }
		else if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_1, __uuidof(p_renderer->dx_device), NULL))) {
			hres = adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
			if (SUCCEEDED(hres)) {
				gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_11_1;
				++p_renderer->dx_gpu_count;
			}
			adapter->Release();
		}
		else if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, __uuidof(p_renderer->dx_device), NULL))) {
			hres = adapter->QueryInterface(__uuidof(IDXGIAdapter3), (void**)&(p_renderer->dx_gpus[p_renderer->dx_gpu_count]));
			if (SUCCEEDED(hres)) {
				gpu_feature_levels[p_renderer->dx_gpu_count] = D3D_FEATURE_LEVEL_11_0;
				++p_renderer->dx_gpu_count;
			}
			adapter->Release();
		}
    }
    assert(p_renderer->dx_gpu_count > 0);

    D3D_FEATURE_LEVEL target_feature_level = D3D_FEATURE_LEVEL_12_1;
    for (uint32_t i = 0; i < p_renderer->dx_gpu_count; ++i) {
      if (gpu_feature_levels[i] == D3D_FEATURE_LEVEL_12_1) {
        p_renderer->dx_active_gpu = p_renderer->dx_gpus[i];
        break;
      }
    }

    if (p_renderer->dx_active_gpu == NULL) {
      for (uint32_t i = 0; i < p_renderer->dx_gpu_count; ++i) {
        if (gpu_feature_levels[i] == D3D_FEATURE_LEVEL_12_0) {
          p_renderer->dx_active_gpu = p_renderer->dx_gpus[i];
          target_feature_level = D3D_FEATURE_LEVEL_12_0;
          break;
        }
      }
    }

    assert(p_renderer->dx_active_gpu != NULL);

    // Load functions
    {
        HMODULE module = ::GetModuleHandle( TEXT( "d3d12.dll" ) );
        fnD3D12CreateRootSignatureDeserializer 
            = (PFN_D3D12_CREATE_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(module, 
                                                                           "D3D12SerializeVersionedRootSignature");

        fnD3D12SerializeVersionedRootSignature 
            = (PFN_D3D12_SERIALIZE_VERSIONED_ROOT_SIGNATURE)GetProcAddress(module,
                                                                           "D3D12SerializeVersionedRootSignature");
        
        fnD3D12CreateVersionedRootSignatureDeserializer 
            = (PFN_D3D12_CREATE_VERSIONED_ROOT_SIGNATURE_DESERIALIZER)GetProcAddress(module, 
                                                                                     "D3D12CreateVersionedRootSignatureDeserializer");
    }

    if ((fnD3D12CreateRootSignatureDeserializer == NULL) ||
        (fnD3D12SerializeVersionedRootSignature == NULL) ||
        (fnD3D12CreateVersionedRootSignatureDeserializer == NULL)) 
    {
      target_feature_level = D3D_FEATURE_LEVEL_12_0;
    }

    hres = D3D12CreateDevice(p_renderer->dx_active_gpu, target_feature_level, __uuidof(p_renderer->dx_device), (void**)(&p_renderer->dx_device));
    assert(SUCCEEDED(hres));

    p_renderer->settings.dx_feature_level = target_feature_level;

    // Queues
    {
        TINY_RENDERER_DECLARE_ZERO(D3D12_COMMAND_QUEUE_DESC, desc);
        desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
        desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        hres = p_renderer->dx_device->CreateCommandQueue(&desc, 
                                                         __uuidof(p_renderer->graphics_queue->dx_queue), 
                                                         (void**)&(p_renderer->graphics_queue->dx_queue));
        assert(SUCCEEDED(hres));
    }

    // Create fence
    {
        hres = p_renderer->dx_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, 
                                                  __uuidof(p_renderer->graphics_queue->dx_wait_idle_fence), (void**)&(p_renderer->graphics_queue->dx_wait_idle_fence));
        assert(SUCCEEDED(hres));
        p_renderer->graphics_queue->dx_wait_idle_fence_value = 1;

        p_renderer->graphics_queue->dx_wait_idle_fence_event = CreateEvent(NULL, FALSE, FALSE, NULL);
        assert(NULL != p_renderer->graphics_queue->dx_wait_idle_fence_event);
    }
}

void tr_internal_dx_create_swapchain(tr_renderer* p_renderer)
{
    assert(NULL != p_renderer->dx_device);

    TINY_RENDERER_DECLARE_ZERO(DXGI_SWAP_CHAIN_DESC1, desc);
    desc.Width              = p_renderer->settings.width;
    desc.Height             = p_renderer->settings.height;
    desc.Format             = tr_util_to_dx_format(p_renderer->settings.swapchain.color_format);
    desc.Stereo             = false;
    desc.SampleDesc.Count   = 1; // If multisampling is needed, we'll resolve it later
    desc.SampleDesc.Quality = p_renderer->settings.swapchain.sample_quality;
    desc.BufferUsage        = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.BufferCount        = p_renderer->settings.swapchain.image_count;
    desc.Scaling            = DXGI_SCALING_STRETCH;
    desc.SwapEffect         = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.AlphaMode          = DXGI_ALPHA_MODE_UNSPECIFIED;
    desc.Flags              = 0;

    if ((desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL) || (desc.SwapEffect == DXGI_SWAP_EFFECT_FLIP_DISCARD))  {
      if (desc.BufferCount < 2) {
        desc.BufferCount = 2;
      }      
    }

    if (desc.BufferCount > DXGI_MAX_SWAP_CHAIN_BUFFERS) {
      desc.BufferCount = DXGI_MAX_SWAP_CHAIN_BUFFERS;
    }

    p_renderer->settings.swapchain.image_count = desc.BufferCount;

    IDXGISwapChain1* swapchain;
    HRESULT hres = p_renderer->dx_factory->CreateSwapChainForHwnd(
            p_renderer->present_queue->dx_queue, p_renderer->settings.handle.hwnd,
            &desc, NULL, NULL, &swapchain);
    assert(SUCCEEDED(hres));

    hres = p_renderer->dx_factory->MakeWindowAssociation(
        p_renderer->settings.handle.hwnd, DXGI_MWA_NO_ALT_ENTER);
    assert(SUCCEEDED(hres));

    hres = swapchain->QueryInterface(__uuidof(p_renderer->dx_swapchain), (void**)&(p_renderer->dx_swapchain));
    assert(SUCCEEDED(hres));
    swapchain->Release();
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
            render_target->color_attachments_multisample[0]->usage         = tr_texture_usage_color_attachment;
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
            render_target->depth_stencil_attachment->usage               = tr_texture_usage_depth_stencil_attachment;
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
              render_target->depth_stencil_attachment_multisample->usage               = tr_texture_usage_depth_stencil_attachment;
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

void tr_internal_dx_create_swapchain_renderpass(tr_renderer* p_renderer)
{
    assert(NULL != p_renderer->dx_device);
    assert(NULL != p_renderer->dx_swapchain);

    ID3D12Resource** swapchain_images = (ID3D12Resource**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*swapchain_images));
    assert(NULL != swapchain_images);
    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i ) {
        HRESULT hres = p_renderer->dx_swapchain->GetBuffer(i, __uuidof(*swapchain_images), (void**)&(swapchain_images[i]));
        assert(SUCCEEDED(hres));
    }

    // Populate the vk_image field and create the Vulkan texture objects
    for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        render_target->color_attachments[0]->dx_resource = swapchain_images[i];
        tr_internal_dx_create_texture(p_renderer, render_target->color_attachments[0]);

        if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
            tr_internal_dx_create_texture(p_renderer, render_target->color_attachments_multisample[0]);
        }

        if (NULL != render_target->depth_stencil_attachment) {
            tr_internal_dx_create_texture(p_renderer, render_target->depth_stencil_attachment);

            if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
              tr_internal_dx_create_texture(p_renderer, render_target->depth_stencil_attachment_multisample);
            }
        }
    }

    // Initialize Vulkan render target objects
    for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
        tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
        tr_internal_dx_create_render_target(p_renderer, render_target);
    }

    TINY_RENDERER_SAFE_FREE(swapchain_images);
}

void tr_internal_dx_destroy_device(tr_renderer* p_renderer)
{
    TINY_RENDERER_SAFE_RELEASE(p_renderer->graphics_queue->dx_queue);
    TINY_RENDERER_SAFE_RELEASE(p_renderer->present_queue->dx_queue);

    TINY_RENDERER_SAFE_RELEASE(p_renderer->dx_device);

    for (uint32_t i = 0; i < p_renderer->dx_gpu_count; ++i) {
        TINY_RENDERER_SAFE_RELEASE(p_renderer->dx_gpus[i]);
    }

    TINY_RENDERER_SAFE_RELEASE(p_renderer->dx_factory);
#if defined(_DEBUG)
    TINY_RENDERER_SAFE_RELEASE(p_renderer->dx_debug_ctrl);
#endif
}

void tr_internal_dx_destroy_swapchain(tr_renderer* p_renderer)
{
    TINY_RENDERER_SAFE_RELEASE(p_renderer->dx_swapchain);
}

// -------------------------------------------------------------------------------------------------
// Internal create functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_create_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{   
}

void tr_internal_dx_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{   
}

void tr_internal_dx_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{   
}

void tr_internal_dx_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{   
}

void tr_internal_dx_create_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer->dx_device);

    uint32_t cbvsrvuav_count = 0;
    uint32_t sampler_count = 0;

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        uint32_t count = p_descriptor_set->descriptors[i].count;
        switch (p_descriptor_set->descriptors[i].type) {
            case tr_descriptor_type_sampler                 : sampler_count   += count; break;
            case tr_descriptor_type_uniform_buffer_cbv       : cbvsrvuav_count += count; break;
            case tr_descriptor_type_storage_buffer_srv       : cbvsrvuav_count += count; break;
            case tr_descriptor_type_storage_buffer_uav       : cbvsrvuav_count += count; break;
            case tr_descriptor_type_texture_srv              : cbvsrvuav_count += count; break;
            case tr_descriptor_type_texture_uav              : cbvsrvuav_count += count; break;
            case tr_descriptor_type_uniform_texel_buffer_srv : cbvsrvuav_count += count; break;
            case tr_descriptor_type_storage_texel_buffer_uav : cbvsrvuav_count += count; break;
        }
    }

    if (cbvsrvuav_count > 0) {
        TINY_RENDERER_DECLARE_ZERO(D3D12_DESCRIPTOR_HEAP_DESC, desc);
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.NumDescriptors = cbvsrvuav_count;
        desc.NodeMask       = 0;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(&desc,
            __uuidof(p_descriptor_set->dx_cbvsrvuav_heap), (void**)&(p_descriptor_set->dx_cbvsrvuav_heap));
        assert(SUCCEEDED(hres));   
    }

    if (sampler_count > 0) {
        TINY_RENDERER_DECLARE_ZERO(D3D12_DESCRIPTOR_HEAP_DESC, desc);
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
        desc.NumDescriptors = sampler_count;
        desc.NodeMask       = 0;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(&desc,
            __uuidof(p_descriptor_set->dx_sampler_heap), (void**)&(p_descriptor_set->dx_sampler_heap));
        assert(SUCCEEDED(hres));
    }

    // Assign heap offsets
    uint32_t cbvsrvuav_heap_offset = 0;
    uint32_t sampler_heap_offset = 0;
    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        switch (p_descriptor_set->descriptors[i].type) {
            case tr_descriptor_type_sampler: {
                descriptor->dx_heap_offset = sampler_heap_offset;
                sampler_heap_offset += descriptor->count;
            }
            break;


            case tr_descriptor_type_uniform_buffer_cbv:
            case tr_descriptor_type_storage_buffer_srv:
            case tr_descriptor_type_storage_buffer_uav:
            case tr_descriptor_type_uniform_texel_buffer_srv:
            case tr_descriptor_type_storage_texel_buffer_uav:
            case tr_descriptor_type_texture_srv:
            case tr_descriptor_type_texture_uav: {
                descriptor->dx_heap_offset = cbvsrvuav_heap_offset;
                cbvsrvuav_heap_offset += descriptor->count;
            }
            break;
        }
    }
}

void tr_internal_dx_destroy_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    TINY_RENDERER_SAFE_RELEASE(p_descriptor_set->dx_cbvsrvuav_heap);
    TINY_RENDERER_SAFE_RELEASE(p_descriptor_set->dx_sampler_heap);
}

void tr_internal_dx_create_cmd_pool(tr_renderer *p_renderer, tr_queue* p_queue, bool transient, tr_cmd_pool* p_cmd_pool)
{
    assert(NULL != p_renderer->dx_device);

    HRESULT hres = p_renderer->dx_device->CreateCommandAllocator(
        D3D12_COMMAND_LIST_TYPE_DIRECT,
        __uuidof(p_cmd_pool->dx_cmd_alloc), (void**)&(p_cmd_pool->dx_cmd_alloc));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_destroy_cmd_pool(tr_renderer *p_renderer, tr_cmd_pool* p_cmd_pool)
{
    TINY_RENDERER_SAFE_RELEASE(p_cmd_pool->dx_cmd_alloc);
}

void tr_internal_dx_create_cmd(tr_cmd_pool *p_cmd_pool, bool secondary, tr_cmd* p_cmd)
{
    assert(NULL != p_cmd_pool->dx_cmd_alloc);
    assert(NULL != p_cmd_pool->renderer);
    
    ID3D12PipelineState* initialState = NULL;
    HRESULT hres = p_cmd_pool->renderer->dx_device->CreateCommandList( 
        0, D3D12_COMMAND_LIST_TYPE_DIRECT, p_cmd_pool->dx_cmd_alloc, initialState, 
        __uuidof(p_cmd->dx_cmd_list), (void**)&(p_cmd->dx_cmd_list));
    assert(SUCCEEDED(hres));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    p_cmd->dx_cmd_list->Close();
}

void tr_internal_dx_destroy_cmd(tr_cmd_pool *p_cmd_pool, tr_cmd* p_cmd)
{
    TINY_RENDERER_SAFE_RELEASE(p_cmd->dx_cmd_list);
}

void tr_internal_dx_create_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    assert(NULL != p_renderer->dx_device);

    // Align the buffer size to multiples of 256
    if (p_buffer->usage & tr_buffer_usage_uniform_cbv) {
        p_buffer->size = tr_round_up((uint32_t)(p_buffer->size), 256);
    }

    D3D12_RESOURCE_DIMENSION res_dim = D3D12_RESOURCE_DIMENSION_BUFFER;

    TINY_RENDERER_DECLARE_ZERO(D3D12_HEAP_PROPERTIES, heap_props);
    heap_props.Type                 = D3D12_HEAP_TYPE_DEFAULT;
    heap_props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heap_props.CreationNodeMask     = 1;
    heap_props.VisibleNodeMask      = 1;

    D3D12_HEAP_FLAGS heap_flags     = D3D12_HEAP_FLAG_NONE;

    TINY_RENDERER_DECLARE_ZERO(D3D12_RESOURCE_DESC, desc);
    desc.Dimension                  = res_dim;
    desc.Alignment                  = 0;
    desc.Width                      = p_buffer->size;
    desc.Height                     = 1;
    desc.DepthOrArraySize           = 1;
    desc.MipLevels                  = 1;
    desc.Format                     = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count           = 1;
    desc.SampleDesc.Quality         = 0;
    desc.Layout                     = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags                      = D3D12_RESOURCE_FLAG_NONE;
    if ((p_buffer->usage & tr_buffer_usage_storage_uav) || (p_buffer->usage & tr_buffer_usage_counter_uav)) {
        desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }

    // Adjust for padding
    UINT64 padded_size = 0;
    p_renderer->dx_device->GetCopyableFootprints( &desc, 0, 1, 0, NULL, NULL, NULL, &padded_size );
    p_buffer->size = (uint64_t)padded_size;
    desc.Width = padded_size;

    D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_COPY_DEST;
    switch (p_buffer->usage) {
        case tr_buffer_usage_uniform_cbv: {
            res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        break;

        case tr_buffer_usage_index: {
            res_states = D3D12_RESOURCE_STATE_INDEX_BUFFER;
        }
        break;

        case tr_buffer_usage_vertex: {
            res_states = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
        }
        break;

        case tr_buffer_usage_storage_uav: {
            res_states = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
        }
        break;

        case tr_buffer_usage_counter_uav: {
            res_states = D3D12_RESOURCE_STATE_COMMON;
        }
        break;
    }

    if (p_buffer->host_visible) {
        // D3D12_HEAP_TYPE_UPLOAD requires D3D12_RESOURCE_STATE_GENERIC_READ
        heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
        res_states = D3D12_RESOURCE_STATE_GENERIC_READ;
    }

    HRESULT hres = p_renderer->dx_device->CreateCommittedResource(
        &heap_props, heap_flags, &desc, res_states, NULL,
        __uuidof(p_buffer->dx_resource), (void**)&(p_buffer->dx_resource));
    assert(SUCCEEDED(hres));

    if (p_buffer->host_visible) {
        D3D12_RANGE read_range = {0, 0};
        hres = p_buffer->dx_resource->Map(0, &read_range, (void**)&(p_buffer->cpu_mapped_address));
        assert( SUCCEEDED(hres) );
    }

    switch (p_buffer->usage) {
        case tr_buffer_usage_index: {
            p_buffer->dx_index_buffer_view.BufferLocation = p_buffer->dx_resource->GetGPUVirtualAddress();
            p_buffer->dx_index_buffer_view.SizeInBytes    = (UINT)p_buffer->size;
            // Format is filled out by tr_create_index_buffer
        }
        break;

        case tr_buffer_usage_vertex: {
            p_buffer->dx_vertex_buffer_view.BufferLocation = p_buffer->dx_resource->GetGPUVirtualAddress();
            p_buffer->dx_vertex_buffer_view.SizeInBytes    = (UINT)p_buffer->size;
            // StrideInBytes is filled out by tr_create_vertex_buffer
        }
        break;

        case tr_buffer_usage_uniform_cbv: {
            p_buffer->dx_cbv_view_desc.BufferLocation = p_buffer->dx_resource->GetGPUVirtualAddress();
            p_buffer->dx_cbv_view_desc.SizeInBytes    = (UINT)p_buffer->size;
        }
        break;

        case tr_buffer_usage_storage_srv: {
            D3D12_SHADER_RESOURCE_VIEW_DESC* desc = &(p_buffer->dx_srv_view_desc);
            desc->Format                       = DXGI_FORMAT_UNKNOWN;
            desc->ViewDimension                = D3D12_SRV_DIMENSION_BUFFER;
            desc->Shader4ComponentMapping      = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc->Buffer.FirstElement          = p_buffer->first_element;
            desc->Buffer.NumElements           = (UINT)(p_buffer->element_count);
            desc->Buffer.StructureByteStride   = (UINT)(p_buffer->struct_stride);
            desc->Buffer.Flags                 = D3D12_BUFFER_SRV_FLAG_NONE;
            if (p_buffer->raw) {
                desc->Format = DXGI_FORMAT_R32_TYPELESS;
                desc->Buffer.Flags |= D3D12_BUFFER_SRV_FLAG_RAW;
            }
        }
        break;

        case tr_buffer_usage_storage_uav: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
            desc->Format                       = DXGI_FORMAT_UNKNOWN;
            desc->ViewDimension                = D3D12_UAV_DIMENSION_BUFFER;
            desc->Buffer.FirstElement          = p_buffer->first_element;
            desc->Buffer.NumElements           = (UINT)(p_buffer->element_count);
            desc->Buffer.StructureByteStride   = (UINT)(p_buffer->struct_stride);
            desc->Buffer.CounterOffsetInBytes  = 0;
            desc->Buffer.Flags                 = D3D12_BUFFER_UAV_FLAG_NONE;
            if (p_buffer->raw) {
                desc->Format = DXGI_FORMAT_R32_TYPELESS;
                desc->Buffer.Flags |= D3D12_BUFFER_UAV_FLAG_RAW;
            }
        }
        break;

        case tr_buffer_usage_uniform_texel_srv: {
            D3D12_SHADER_RESOURCE_VIEW_DESC* desc = &(p_buffer->dx_srv_view_desc);
            desc->Format                     = tr_util_to_dx_format(p_buffer->format);
            desc->ViewDimension              = D3D12_SRV_DIMENSION_BUFFER;
            desc->Shader4ComponentMapping    = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            desc->Buffer.FirstElement        = p_buffer->first_element;
            desc->Buffer.NumElements         = (UINT)(p_buffer->element_count);
            desc->Buffer.StructureByteStride = (UINT)(p_buffer->struct_stride);
            desc->Buffer.Flags               = D3D12_BUFFER_SRV_FLAG_NONE;
        }
        break;

        case tr_buffer_usage_storage_texel_uav: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
            desc->Format                       = DXGI_FORMAT_UNKNOWN;
            desc->ViewDimension                = D3D12_UAV_DIMENSION_BUFFER;
            desc->Buffer.FirstElement          = p_buffer->first_element;
            desc->Buffer.NumElements           = (UINT)(p_buffer->element_count);
            desc->Buffer.StructureByteStride   = (UINT)(p_buffer->struct_stride);
            desc->Buffer.CounterOffsetInBytes  = 0;
            desc->Buffer.Flags                 = D3D12_BUFFER_UAV_FLAG_NONE;
        }
        break;

        case tr_buffer_usage_counter_uav: {
            D3D12_UNORDERED_ACCESS_VIEW_DESC* desc = &(p_buffer->dx_uav_view_desc);
            desc->Format                       = DXGI_FORMAT_R32_TYPELESS;
            desc->ViewDimension                = D3D12_UAV_DIMENSION_BUFFER;
            desc->Buffer.FirstElement          = p_buffer->first_element;
            desc->Buffer.NumElements           = (UINT)(p_buffer->element_count);
            desc->Buffer.StructureByteStride   = (UINT)(p_buffer->struct_stride);
            desc->Buffer.CounterOffsetInBytes  = 0;
            desc->Buffer.Flags                 = D3D12_BUFFER_UAV_FLAG_RAW;
        }
        break;
    }
}

void tr_internal_dx_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer)
{
    TINY_RENDERER_SAFE_RELEASE(p_buffer->dx_resource);
}

void tr_internal_dx_create_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    assert(NULL != p_renderer->dx_device);
    assert(DXGI_FORMAT_UNKNOWN != tr_util_to_dx_format(p_texture->format));

    p_texture->renderer = p_renderer;

    if (NULL == p_texture->dx_resource) {
        D3D12_RESOURCE_DIMENSION res_dim = D3D12_RESOURCE_DIMENSION_UNKNOWN;
        switch (p_texture->type) {
            case tr_texture_type_1d   : res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE1D; break;
            case tr_texture_type_2d   : res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
            case tr_texture_type_3d   : res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE3D; break;
            case tr_texture_type_cube : res_dim = D3D12_RESOURCE_DIMENSION_TEXTURE2D; break;
        }
        assert(D3D12_RESOURCE_DIMENSION_UNKNOWN != res_dim);

        TINY_RENDERER_DECLARE_ZERO(D3D12_HEAP_PROPERTIES, heap_props);
        heap_props.Type                 = D3D12_HEAP_TYPE_DEFAULT;
        heap_props.CPUPageProperty      = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        heap_props.CreationNodeMask     = 1;
        heap_props.VisibleNodeMask      = 1;

        D3D12_HEAP_FLAGS heap_flags     = D3D12_HEAP_FLAG_NONE;

        TINY_RENDERER_DECLARE_ZERO(D3D12_RESOURCE_DESC, desc);
        desc.Dimension                  = res_dim;
        desc.Alignment                  = 0;
        desc.Width                      = p_texture->width;
        desc.Height                     = p_texture->height;
        desc.DepthOrArraySize           = p_texture->depth;
        desc.MipLevels                  = (UINT16)p_texture->mip_levels;
        desc.Format                     = tr_util_to_dx_format(p_texture->format);
        desc.SampleDesc.Count           = (UINT)p_texture->sample_count;
        desc.SampleDesc.Quality         = (UINT)p_texture->sample_quality;
        desc.Layout                     = D3D12_TEXTURE_LAYOUT_UNKNOWN;
        desc.Flags                      = D3D12_RESOURCE_FLAG_NONE;
        if (p_texture->usage & tr_texture_usage_color_attachment) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        }
        if (p_texture->usage & tr_texture_usage_depth_stencil_attachment) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        }
        if (p_texture->usage & tr_texture_usage_storage_image) {
            desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
        }

        D3D12_RESOURCE_STATES res_states = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
        if (p_texture->usage & tr_texture_usage_color_attachment) {
          res_states = D3D12_RESOURCE_STATE_RENDER_TARGET;
        }

        TINY_RENDERER_DECLARE_ZERO(D3D12_CLEAR_VALUE, clear_value);
        clear_value.Format = tr_util_to_dx_format(p_texture->format);
        if (tr_texture_usage_depth_stencil_attachment == (p_texture->usage & tr_texture_usage_depth_stencil_attachment)) {
            clear_value.DepthStencil.Depth = p_texture->clear_value.depth;
            clear_value.DepthStencil.Stencil = p_texture->clear_value.stencil;
        }
        else {
            clear_value.Color[0] = p_texture->clear_value.r;
            clear_value.Color[1] = p_texture->clear_value.g;
            clear_value.Color[2] = p_texture->clear_value.b;
            clear_value.Color[3] = p_texture->clear_value.a;
        }

        D3D12_CLEAR_VALUE* p_clear_value = NULL;
        if ((desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET) || (desc.Flags & D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)) {
            p_clear_value = &clear_value;
        }

        HRESULT hres = p_renderer->dx_device->CreateCommittedResource(
            &heap_props, heap_flags, &desc, res_states, p_clear_value,
            __uuidof(p_texture->dx_resource), (void**)&(p_texture->dx_resource));
        assert(SUCCEEDED(hres));
        
        p_texture->owns_image = true;
    }

    if (p_texture->usage & tr_texture_usage_sampled_image) {
      D3D12_SRV_DIMENSION view_dim = D3D12_SRV_DIMENSION_UNKNOWN;
      switch (p_texture->type) {
          case tr_texture_type_1d   : view_dim = D3D12_SRV_DIMENSION_TEXTURE1D; break;
          case tr_texture_type_2d   : view_dim = D3D12_SRV_DIMENSION_TEXTURE2D; break;
          case tr_texture_type_3d   : view_dim = D3D12_SRV_DIMENSION_TEXTURE3D; break;
          case tr_texture_type_cube : view_dim = D3D12_SRV_DIMENSION_TEXTURE2D; break;
      }
      assert(D3D12_SRV_DIMENSION_UNKNOWN != view_dim);

      p_texture->dx_srv_view_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      p_texture->dx_srv_view_desc.Format                  = tr_util_to_dx_format(p_texture->format);
      p_texture->dx_srv_view_desc.ViewDimension           = view_dim;
      p_texture->dx_srv_view_desc.Texture2D.MipLevels     = (UINT)p_texture->mip_levels;
    }

    if (p_texture->usage & tr_texture_usage_storage_image) {
      p_texture->dx_uav_view_desc.Format                = tr_util_to_dx_format(p_texture->format);
      p_texture->dx_uav_view_desc.ViewDimension         = D3D12_UAV_DIMENSION_TEXTURE2D;
      p_texture->dx_uav_view_desc.Texture2D.MipSlice    = 0;
      p_texture->dx_uav_view_desc.Texture2D.PlaneSlice  = 0;
  }   
}

void tr_internal_dx_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
    TINY_RENDERER_SAFE_RELEASE(p_texture->dx_resource);
}

void tr_internal_dx_create_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(NULL != p_renderer->dx_device);

	p_sampler->dx_sampler_desc.Filter         = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	p_sampler->dx_sampler_desc.AddressU       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	p_sampler->dx_sampler_desc.AddressV       = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	p_sampler->dx_sampler_desc.AddressW	      = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	p_sampler->dx_sampler_desc.MipLODBias     = 0;
	p_sampler->dx_sampler_desc.MaxAnisotropy  = 0;
	p_sampler->dx_sampler_desc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	p_sampler->dx_sampler_desc.BorderColor[0] = 0.0f;
	p_sampler->dx_sampler_desc.BorderColor[1] = 0.0f;
	p_sampler->dx_sampler_desc.BorderColor[2] = 0.0f;
	p_sampler->dx_sampler_desc.BorderColor[3] = 0.0f;
	p_sampler->dx_sampler_desc.MinLOD         = 0.0f;
	p_sampler->dx_sampler_desc.MaxLOD         = D3D12_FLOAT32_MAX;    
}

void tr_internal_dx_destroy_sampler(tr_renderer* p_renderer, tr_sampler* p_sampler)
{
    assert(NULL != p_renderer->dx_device);

    // NO-OP for now
}

void tr_internal_dx_create_shader_program(tr_renderer* p_renderer, uint32_t vert_size, const void* vert_code, const char* vert_enpt, uint32_t hull_size, const void* hull_code, const char* hull_enpt, uint32_t domn_size, const void* domn_code, const char* domn_enpt, uint32_t geom_size, const void* geom_code, const char* geom_enpt, uint32_t frag_size, const void* frag_code, const char* frag_enpt, uint32_t comp_size, const void* comp_code, const char* comp_enpt, tr_shader_program* p_shader_program)
{
    assert(NULL != p_renderer->dx_device);

#if defined(_DEBUG)
    // Enable better shader debugging with the graphics debugging tools.
    UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    UINT compile_flags = 0;
#endif

    int major = 5;
    int minor = 0;
    switch(p_renderer->settings.dx_shader_target) {
        case tr_dx_shader_target_5_1: { major = 5; minor = 1; } break;
        case tr_dx_shader_target_6_0: { major = 6; minor = 0; } break;
    }

     TINY_RENDERER_DECLARE_ZERO(char, vs_target[16]);
     TINY_RENDERER_DECLARE_ZERO(char, hs_target[16]);
     TINY_RENDERER_DECLARE_ZERO(char, ds_target[16]);
     TINY_RENDERER_DECLARE_ZERO(char, gs_target[16]);
     TINY_RENDERER_DECLARE_ZERO(char, ps_target[16]);
     TINY_RENDERER_DECLARE_ZERO(char, cs_target[16]);

    sprintf_s(vs_target, "vs_%d_%d", major, minor);
    sprintf_s(hs_target, "hs_%d_%d", major, minor);
    sprintf_s(ds_target, "ds_%d_%d", major, minor);
    sprintf_s(gs_target, "gs_%d_%d", major, minor);
    sprintf_s(ps_target, "ps_%d_%d", major, minor);
    sprintf_s(cs_target, "cs_%d_%d", major, minor);

    const char* vs_name = "VERTEX";
    const char* hs_name = "HULL";
    const char* ds_name = "DOMAIN";
    const char* gs_name = "GEOMETRY";
    const char* ps_name = "PIXEL";
    const char* cs_name = "COMPUTE";

    for (uint32_t i = 0; i < tr_shader_stage_count; ++i) {
        tr_shader_stage stage_mask = (tr_shader_stage)(1 << i);
        if (stage_mask == (p_shader_program->shader_stages & stage_mask)) {
            const void* source        = NULL;
            SIZE_T      source_len    = 0;
            const char* source_name   = NULL;
            const char* target        = NULL;
            const char* entry_point   = NULL;
            ID3DBlob**  compiled_code = NULL;
            switch(stage_mask) {
                case tr_shader_stage_vert: {
                    source        = vert_code;
                    source_len    = vert_size;
                    source_name   = vs_name;
                    target        = vs_target;
                    entry_point   = vert_enpt;
                    compiled_code = &(p_shader_program->dx_vert);
                } break;
                case tr_shader_stage_hull: {
                    source        = hull_code;
                    source_len    = hull_size;
                    source_name   = hs_name;
                    target        = hs_target;
                    entry_point   = hull_enpt;
                    compiled_code = &(p_shader_program->dx_hull);
                } break;
                case tr_shader_stage_domn: {
                    source        = domn_code;
                    source_len    = domn_size;
                    source_name   = ds_name;
                    target        = ds_target;
                    entry_point   = domn_enpt;
                    compiled_code = &(p_shader_program->dx_domn);
                } break;
                case tr_shader_stage_geom: {
                    source        = geom_code;
                    source_len    = geom_size;
                    source_name   = gs_name;
                    target        = gs_target;
                    entry_point   = geom_enpt;
                    compiled_code = &(p_shader_program->dx_geom);
                } break;
                case tr_shader_stage_frag: {
                    source        = frag_code;
                    source_len    = frag_size;
                    source_name   = ps_name;
                    target        = ps_target;
                    entry_point   = frag_enpt;
                    compiled_code = &(p_shader_program->dx_frag);
                } break;
                case tr_shader_stage_comp: {
                    source        = comp_code;
                    source_len    = comp_size;
                    source_name   = cs_name;
                    target        = cs_target;
                    entry_point   = comp_enpt;
                    compiled_code = &(p_shader_program->dx_comp);
                } break;
            }

            D3D_SHADER_MACRO macros[] = { "D3D12", "1", 
                                          NULL, NULL };
            ID3DBlob* error_msgs = NULL;
            HRESULT hres = D3DCompile2(source, source_len, source_name, 
                                       macros, NULL, 
                                       entry_point, target, compile_flags, 
                                       0, 0, NULL, 0, 
                                       compiled_code, &error_msgs);
            if (FAILED(hres)) {
                char* msg = (char*)calloc(error_msgs->GetBufferSize() + 1, sizeof(*msg));
                assert(NULL != msg);
                memcpy(msg, error_msgs->GetBufferPointer(), error_msgs->GetBufferSize());
                tr_internal_log(tr_log_type_error, msg, "tr_internal_dx_create_shader_program");
                TINY_RENDERER_SAFE_FREE(msg);
            }
            assert(SUCCEEDED(hres));
        }
    }
}

void tr_internal_dx_destroy_shader_program(tr_renderer* p_renderer, tr_shader_program* p_shader_program)
{
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_vert);
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_hull);
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_domn);
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_geom);
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_frag);
    TINY_RENDERER_SAFE_RELEASE(p_shader_program->dx_comp);
}

void tr_internal_dx_create_root_signature(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_DECLARE_ZERO(D3D12_FEATURE_DATA_ROOT_SIGNATURE, feature_data);
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
    HRESULT hres = p_renderer->dx_device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &feature_data, sizeof(feature_data));
    if (FAILED(hres )) { 
        feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
    }

    uint32_t range_count = 0;
    D3D12_DESCRIPTOR_RANGE1* ranges_11 = NULL;
    D3D12_DESCRIPTOR_RANGE*  ranges_10 = NULL;

    uint32_t parameter_count = 0;
    D3D12_ROOT_PARAMETER1* parameters_11 = NULL;
    D3D12_ROOT_PARAMETER*  parameters_10 = NULL;

    if (NULL != p_descriptor_set) {
        const uint32_t descriptor_count = p_descriptor_set->descriptor_count;

        uint32_t cbvsrvuav_count = 0;
        uint32_t sampler_count = 0;
        for (uint32_t i = 0; i < descriptor_count; ++i) {
            uint32_t count = p_descriptor_set->descriptors[i].count;
            switch (p_descriptor_set->descriptors[i].type) {
                case tr_descriptor_type_sampler                 : sampler_count   += count; break;
                case tr_descriptor_type_uniform_buffer_cbv       : cbvsrvuav_count += count; break;
                case tr_descriptor_type_storage_buffer_srv       : cbvsrvuav_count += count; break;
                case tr_descriptor_type_storage_buffer_uav       : cbvsrvuav_count += count; break;
                case tr_descriptor_type_uniform_texel_buffer_srv : cbvsrvuav_count += count; break;
                case tr_descriptor_type_storage_texel_buffer_uav : cbvsrvuav_count += count; break;
                case tr_descriptor_type_texture_srv              : cbvsrvuav_count += count; break;
                case tr_descriptor_type_texture_uav              : cbvsrvuav_count += count; break;
            }
        }

        // Allocate everything with an upper bound of descriptor counts
        ranges_11 = (D3D12_DESCRIPTOR_RANGE1*)calloc(descriptor_count, sizeof(*ranges_11));
        ranges_10 = (D3D12_DESCRIPTOR_RANGE*)calloc(descriptor_count, sizeof(*ranges_10));

        parameters_11 = (D3D12_ROOT_PARAMETER1*)calloc(descriptor_count, sizeof(*parameters_11));
        assert(NULL != parameters_11);
        parameters_10 = (D3D12_ROOT_PARAMETER*)calloc(descriptor_count, sizeof(*parameters_11));
        assert(NULL != parameters_10);

        // Build ranges
        for (uint32_t descriptor_index = 0; descriptor_index < descriptor_count; ++descriptor_index) {
            tr_descriptor* descriptor = &(p_descriptor_set->descriptors[descriptor_index]);
            D3D12_DESCRIPTOR_RANGE1* range_11 = &ranges_11[range_count];
            D3D12_DESCRIPTOR_RANGE*  range_10 = &ranges_10[range_count];
            D3D12_ROOT_PARAMETER1* param_11 = &parameters_11[parameter_count];
            D3D12_ROOT_PARAMETER*  param_10 = &parameters_10[parameter_count];
            param_11->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            param_10->ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;

            // Start out with visibility on all shader stages
            param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
            uint32_t shader_stage_count = 0;
            // Select one if there is only one
            if (descriptor->shader_stages & tr_shader_stage_vert) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_hull) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_HULL;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_domn) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_DOMAIN;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_geom) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_frag) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
                ++shader_stage_count;
            }
            if (descriptor->shader_stages & tr_shader_stage_comp) {
                // Keep D3D12_SHADER_VISIBILITY_ALL for compute shaders
                ++shader_stage_count;
            }
            // Go back to all shader stages if there's more than one stage
            if (shader_stage_count > 1) {
                param_11->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
                param_10->ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;                
            }

            bool assign_range = false;
            switch (descriptor->type) {
                case tr_descriptor_type_sampler: {
                    range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
                    assign_range = true;
                }
                break;
                case tr_descriptor_type_storage_buffer_srv:
                case tr_descriptor_type_uniform_texel_buffer_srv: 
                case tr_descriptor_type_texture_srv: {
                    range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
                    assign_range = true;
                }
                break;      
                case tr_descriptor_type_uniform_buffer_cbv: {
                    range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
                    assign_range = true;
                }
                break;
                case tr_descriptor_type_storage_buffer_uav: 
                case tr_descriptor_type_storage_texel_buffer_uav: 
                case tr_descriptor_type_texture_uav: {
                    range_11->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    range_10->RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
                    assign_range = true;
                }
                break;
            }

            if (assign_range) {
                range_11->NumDescriptors                    = descriptor->count;
                range_11->BaseShaderRegister                = descriptor->binding;
                range_11->RegisterSpace                     = 0;
                range_11->Flags                             = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
                range_11->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 

                range_10->NumDescriptors                    = descriptor->count;
                range_10->BaseShaderRegister                = descriptor->binding;
                range_10->RegisterSpace                     = 0;
                range_10->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND; 

                param_11->DescriptorTable.pDescriptorRanges   = range_11;
                param_11->DescriptorTable.NumDescriptorRanges = 1;

                param_10->DescriptorTable.pDescriptorRanges   = range_10;
                param_10->DescriptorTable.NumDescriptorRanges = 1;

                descriptor->dx_root_parameter_index = parameter_count;

                ++range_count;
                ++parameter_count;
            }
        }
    }
    
    TINY_RENDERER_DECLARE_ZERO(D3D12_VERSIONED_ROOT_SIGNATURE_DESC, desc);
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == feature_data.HighestVersion) {
        desc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_1;
        desc.Desc_1_1.NumParameters     = parameter_count;
        desc.Desc_1_1.pParameters       = parameters_11;
        desc.Desc_1_1.NumStaticSamplers = 0;
        desc.Desc_1_1.pStaticSamplers   = NULL;
        desc.Desc_1_1.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }
    else if (D3D_ROOT_SIGNATURE_VERSION_1_0 == feature_data.HighestVersion) {
        desc.Version                    = D3D_ROOT_SIGNATURE_VERSION_1_0;
        desc.Desc_1_0.NumParameters     = parameter_count;
        desc.Desc_1_0.pParameters       = parameters_10;
        desc.Desc_1_0.NumStaticSamplers = 0;
        desc.Desc_1_0.pStaticSamplers   = NULL;
        desc.Desc_1_0.Flags             = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
    }

    ID3DBlob* sig_blob = NULL;
    ID3DBlob* error_msgs = NULL;
    if (D3D_ROOT_SIGNATURE_VERSION_1_1 == feature_data.HighestVersion) {
      hres = fnD3D12SerializeVersionedRootSignature(&desc, &sig_blob, &error_msgs);
    }
    else {
      hres = D3D12SerializeRootSignature(&(desc.Desc_1_0), D3D_ROOT_SIGNATURE_VERSION_1_0, &sig_blob, &error_msgs);
    }
    assert(SUCCEEDED(hres));

    hres = p_renderer->dx_device->CreateRootSignature(0, sig_blob->GetBufferPointer(), sig_blob->GetBufferSize(),
                                                        __uuidof(p_pipeline->dx_root_signature), (void**)&(p_pipeline->dx_root_signature));
    assert(SUCCEEDED(hres));

    TINY_RENDERER_SAFE_RELEASE(sig_blob);
    TINY_RENDERER_SAFE_RELEASE(error_msgs);
    
    TINY_RENDERER_SAFE_FREE(ranges_11);
    TINY_RENDERER_SAFE_FREE(ranges_10);
    TINY_RENDERER_SAFE_FREE(parameters_11);
    TINY_RENDERER_SAFE_FREE(parameters_10);
}

void tr_internal_dx_create_pipeline_state(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, VS);
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, PS);
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, DS);
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, HS);
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, GS);
    if (NULL != p_shader_program->dx_vert) {
        VS.BytecodeLength  = p_shader_program->dx_vert->GetBufferSize();
        VS.pShaderBytecode = p_shader_program->dx_vert->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_frag) {
        PS.BytecodeLength  = p_shader_program->dx_frag->GetBufferSize();
        PS.pShaderBytecode = p_shader_program->dx_frag->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_domn) {
        DS.BytecodeLength  = p_shader_program->dx_domn->GetBufferSize();
        DS.pShaderBytecode = p_shader_program->dx_domn->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_hull) {
        HS.BytecodeLength  = p_shader_program->dx_hull->GetBufferSize();
        HS.pShaderBytecode = p_shader_program->dx_hull->GetBufferPointer();
    }
    if (NULL != p_shader_program->dx_geom) {
        GS.BytecodeLength  = p_shader_program->dx_geom->GetBufferSize();
        GS.pShaderBytecode = p_shader_program->dx_geom->GetBufferPointer();
    }

    TINY_RENDERER_DECLARE_ZERO(D3D12_STREAM_OUTPUT_DESC, stream_output_desc);
    stream_output_desc.pSODeclaration       = NULL;
    stream_output_desc.NumEntries           = 0;
    stream_output_desc.pBufferStrides       = NULL;
    stream_output_desc.NumStrides           = 0;
    stream_output_desc.RasterizedStream     = 0;

    TINY_RENDERER_DECLARE_ZERO(D3D12_BLEND_DESC, blend_desc);
    blend_desc.AlphaToCoverageEnable        = FALSE;
    blend_desc.IndependentBlendEnable       = FALSE;
    for( UINT attrib_index = 0; attrib_index < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++attrib_index ) { 
        blend_desc.RenderTarget[attrib_index].BlendEnable           = FALSE;
        blend_desc.RenderTarget[attrib_index].LogicOpEnable         = FALSE;
        blend_desc.RenderTarget[attrib_index].SrcBlend              = D3D12_BLEND_ONE;
        blend_desc.RenderTarget[attrib_index].DestBlend             = D3D12_BLEND_ZERO;
        blend_desc.RenderTarget[attrib_index].BlendOp               = D3D12_BLEND_OP_ADD;
        blend_desc.RenderTarget[attrib_index].SrcBlendAlpha         = D3D12_BLEND_ONE;
        blend_desc.RenderTarget[attrib_index].DestBlendAlpha        = D3D12_BLEND_ZERO;
        blend_desc.RenderTarget[attrib_index].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
        blend_desc.RenderTarget[attrib_index].LogicOp               = D3D12_LOGIC_OP_NOOP;
        blend_desc.RenderTarget[attrib_index].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    D3D12_CULL_MODE cull_mode = D3D12_CULL_MODE_NONE;
    switch(p_pipeline_settings->cull_mode) {
        case tr_cull_mode_back  : cull_mode = D3D12_CULL_MODE_BACK; break;
        case tr_cull_mode_front : cull_mode = D3D12_CULL_MODE_FRONT; break;
    }
    BOOL front_face_ccw = (tr_front_face_ccw == p_pipeline_settings->front_face) ? TRUE : FALSE;
    TINY_RENDERER_DECLARE_ZERO(D3D12_RASTERIZER_DESC, rasterizer_desc);
    rasterizer_desc.FillMode                = D3D12_FILL_MODE_SOLID;
    rasterizer_desc.CullMode                = cull_mode;
    rasterizer_desc.FrontCounterClockwise   = front_face_ccw;
    rasterizer_desc.DepthBias               = D3D12_DEFAULT_DEPTH_BIAS;
    rasterizer_desc.DepthBiasClamp          = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
    rasterizer_desc.SlopeScaledDepthBias    = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
    rasterizer_desc.DepthClipEnable         = TRUE;
    rasterizer_desc.MultisampleEnable       = FALSE;
    rasterizer_desc.AntialiasedLineEnable   = FALSE;
    rasterizer_desc.ForcedSampleCount       = 0;
    rasterizer_desc.ConservativeRaster      = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

    TINY_RENDERER_DECLARE_ZERO(D3D12_DEPTH_STENCILOP_DESC, depth_stencilop_desc);
    depth_stencilop_desc.StencilFailOp      = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilPassOp      = D3D12_STENCIL_OP_KEEP;
    depth_stencilop_desc.StencilFunc        = D3D12_COMPARISON_FUNC_ALWAYS;

    TINY_RENDERER_DECLARE_ZERO(D3D12_DEPTH_STENCIL_DESC, depth_stencil_desc);
    depth_stencil_desc.DepthEnable          = p_pipeline_settings->depth ? TRUE : FALSE;
    depth_stencil_desc.DepthWriteMask       = D3D12_DEPTH_WRITE_MASK_ALL;
    depth_stencil_desc.DepthFunc            = D3D12_COMPARISON_FUNC_LESS;
    depth_stencil_desc.StencilEnable        = FALSE;
    depth_stencil_desc.StencilReadMask      = D3D12_DEFAULT_STENCIL_READ_MASK;
    depth_stencil_desc.StencilWriteMask     = D3D12_DEFAULT_STENCIL_WRITE_MASK;
    depth_stencil_desc.FrontFace            = depth_stencilop_desc;
    depth_stencil_desc.BackFace             = depth_stencilop_desc;

    uint32_t input_element_count = 0;
    TINY_RENDERER_DECLARE_ZERO(D3D12_INPUT_ELEMENT_DESC, input_elements[tr_max_vertex_attribs]);
    TINY_RENDERER_DECLARE_ZERO(char, semantic_names[tr_max_vertex_attribs][tr_max_semantic_name_length]);

    uint32_t attrib_count = tr_min(p_vertex_layout->attrib_count, tr_max_vertex_attribs);
    for (uint32_t attrib_index = 0; attrib_index < p_vertex_layout->attrib_count; ++attrib_index) {
        const tr_vertex_attrib* attrib = &(p_vertex_layout->attribs[attrib_index]);
        assert(tr_semantic_undefined != attrib->semantic);

        if (attrib->semantic_name_length > 0) {
            uint32_t name_length = tr_min(tr_max_semantic_name_length, attrib->semantic_name_length);
            strncpy_s(semantic_names[attrib_index], attrib->semantic_name, name_length);
        }
        else {
            TINY_RENDERER_DECLARE_ZERO(char, name[tr_max_semantic_name_length]);
            switch (attrib->semantic) {
                case tr_semantic_position  : sprintf_s(name, "POSITION"); break;
                case tr_semantic_normal    : sprintf_s(name, "NORMAL"); break;
                case tr_semantic_color     : sprintf_s(name, "COLOR"); break;
                case tr_semantic_tangent   : sprintf_s(name, "TANGENT"); break;
                case tr_semantic_bitangent : sprintf_s(name, "BITANGENT"); break;
                case tr_semantic_texcoord0 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord1 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord2 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord3 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord4 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord5 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord6 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord7 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord8 : sprintf_s(name, "TEXCOORD"); break;
                case tr_semantic_texcoord9 : sprintf_s(name, "TEXCOORD"); break;
                default: break;
            }
            assert(0 != strlen(name));
            strncpy_s(semantic_names[attrib_index], name, strlen(name));
        }

        UINT semantic_index = 0;
        switch (attrib->semantic) {
            case tr_semantic_texcoord0 : semantic_index = 0; break;
            case tr_semantic_texcoord1 : semantic_index = 1; break;
            case tr_semantic_texcoord2 : semantic_index = 2; break;
            case tr_semantic_texcoord3 : semantic_index = 3; break;
            case tr_semantic_texcoord4 : semantic_index = 4; break;
            case tr_semantic_texcoord5 : semantic_index = 5; break;
            case tr_semantic_texcoord6 : semantic_index = 6; break;
            case tr_semantic_texcoord7 : semantic_index = 7; break;
            case tr_semantic_texcoord8 : semantic_index = 8; break;
            case tr_semantic_texcoord9 : semantic_index = 9; break;
            default: break;
        }

        TINY_RENDERER_DECLARE_ZERO(D3D12_INPUT_ELEMENT_DESC, input_element_desc);
        input_elements[input_element_count].SemanticName            = semantic_names[attrib_index];
        input_elements[input_element_count].SemanticIndex           = semantic_index;
        input_elements[input_element_count].Format                  = tr_util_to_dx_format(attrib->format);
        input_elements[input_element_count].InputSlot               = 0;
        input_elements[input_element_count].AlignedByteOffset       = attrib->offset;
        input_elements[input_element_count].InputSlotClass          = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
        input_elements[input_element_count].InstanceDataStepRate    = 0;
        ++input_element_count;
    }

    TINY_RENDERER_DECLARE_ZERO(D3D12_INPUT_LAYOUT_DESC, input_layout_desc);
    input_layout_desc.pInputElementDescs                    = input_elements;
    input_layout_desc.NumElements                           = input_element_count;

    uint32_t render_target_count = tr_min(p_render_target->color_attachment_count, tr_max_render_target_attachments);
    render_target_count = tr_min(render_target_count, D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT);

    TINY_RENDERER_DECLARE_ZERO(DXGI_SAMPLE_DESC, sample_desc);
    sample_desc.Count                                       = (UINT)p_render_target->sample_count;
    sample_desc.Quality                                     = 0;

    TINY_RENDERER_DECLARE_ZERO(D3D12_CACHED_PIPELINE_STATE, cached_pso_desc);
    cached_pso_desc.pCachedBlob                             = NULL;
    cached_pso_desc.CachedBlobSizeInBytes                   = 0;

    D3D12_PRIMITIVE_TOPOLOGY_TYPE topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
    switch(p_pipeline->settings.primitive_topo) {
        case tr_primitive_topo_point_list     : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; break;
        case tr_primitive_topo_line_list      : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
        case tr_primitive_topo_line_strip     : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; break;
        case tr_primitive_topo_tri_list       : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
        case tr_primitive_topo_tri_strip      : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE; break;
        case tr_primitive_topo_1_point_patch  : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; break;
        case tr_primitive_topo_2_point_patch  : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; break;
        case tr_primitive_topo_3_point_patch  : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; break;
        case tr_primitive_topo_4_point_patch  : topology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH; break;
        default: break;
    }
    assert(topology != D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

    TINY_RENDERER_DECLARE_ZERO(D3D12_GRAPHICS_PIPELINE_STATE_DESC, pipeline_state_desc);
    pipeline_state_desc.pRootSignature                      = p_pipeline->dx_root_signature;
    pipeline_state_desc.VS                                  = VS;
    pipeline_state_desc.PS                                  = PS;
    pipeline_state_desc.DS                                  = DS;
    pipeline_state_desc.HS                                  = HS;
    pipeline_state_desc.GS                                  = GS;
    pipeline_state_desc.StreamOutput                        = stream_output_desc;
    pipeline_state_desc.BlendState                          = blend_desc;
    pipeline_state_desc.SampleMask                          = UINT_MAX;
    pipeline_state_desc.RasterizerState                     = rasterizer_desc;
    pipeline_state_desc.DepthStencilState                   = depth_stencil_desc;
    pipeline_state_desc.InputLayout                         = input_layout_desc;
    pipeline_state_desc.IBStripCutValue                     = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    pipeline_state_desc.PrimitiveTopologyType               = topology;
    pipeline_state_desc.NumRenderTargets                    = render_target_count;
    pipeline_state_desc.DSVFormat                           = (p_render_target->depth_stencil_attachment != NULL) ? tr_util_to_dx_format(p_render_target->depth_stencil_attachment->format) : DXGI_FORMAT_UNKNOWN;
    pipeline_state_desc.SampleDesc                          = sample_desc;
    pipeline_state_desc.NodeMask                            = 0;
    pipeline_state_desc.CachedPSO                           = cached_pso_desc;
    pipeline_state_desc.Flags                               = D3D12_PIPELINE_STATE_FLAG_NONE;

    for (uint32_t attrib_index = 0; attrib_index < render_target_count; ++attrib_index) {
        pipeline_state_desc.RTVFormats[attrib_index] = tr_util_to_dx_format(p_render_target->color_attachments[attrib_index]->format);
    }

    HRESULT hres = p_renderer->dx_device->CreateGraphicsPipelineState(
                       &pipeline_state_desc,
                       __uuidof(p_pipeline->dx_pipeline_state), (void**)&(p_pipeline->dx_pipeline_state));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_vertex_layout* p_vertex_layout, tr_descriptor_set* p_descriptor_set, tr_render_target* p_render_target, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    assert(NULL != p_renderer->dx_device);
    assert((NULL != p_shader_program->dx_vert) || (NULL != p_shader_program->dx_hull) || (NULL != p_shader_program->dx_domn) || (NULL != p_shader_program->dx_geom) || (NULL != p_shader_program->dx_frag));
    assert((NULL != p_render_target->dx_rtv_heap) || (NULL != p_render_target->dx_dsv_heap));

    tr_internal_dx_create_root_signature(p_renderer, p_descriptor_set, p_pipeline);
    tr_internal_dx_create_pipeline_state(p_renderer, p_shader_program, p_vertex_layout, p_render_target, p_pipeline_settings, p_pipeline);
}

void tr_internal_dx_create_compute_pipeline_state(tr_renderer* p_renderer, tr_shader_program* p_shader_program, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_DECLARE_ZERO(D3D12_SHADER_BYTECODE, CS);
    if (NULL != p_shader_program->dx_comp) {
        CS.BytecodeLength  = p_shader_program->dx_comp->GetBufferSize();
        CS.pShaderBytecode = p_shader_program->dx_comp->GetBufferPointer();
    }

    TINY_RENDERER_DECLARE_ZERO(D3D12_CACHED_PIPELINE_STATE, cached_pso_desc);
    cached_pso_desc.pCachedBlob                             = NULL;
    cached_pso_desc.CachedBlobSizeInBytes                   = 0;

    TINY_RENDERER_DECLARE_ZERO(D3D12_COMPUTE_PIPELINE_STATE_DESC, pipeline_state_desc);
    pipeline_state_desc.pRootSignature  = p_pipeline->dx_root_signature;
    pipeline_state_desc.CS              = CS;
    pipeline_state_desc.NodeMask        = 0;
    pipeline_state_desc.CachedPSO       = cached_pso_desc;
    pipeline_state_desc.Flags           = D3D12_PIPELINE_STATE_FLAG_NONE;

    HRESULT hres = p_renderer->dx_device->CreateComputePipelineState(
                       &pipeline_state_desc,
                       __uuidof(p_pipeline->dx_pipeline_state), (void**)&(p_pipeline->dx_pipeline_state));
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_create_compute_pipeline(tr_renderer* p_renderer, tr_shader_program* p_shader_program, tr_descriptor_set* p_descriptor_set, const tr_pipeline_settings* p_pipeline_settings, tr_pipeline* p_pipeline)
{
    assert(NULL != p_renderer->dx_device);
    assert(NULL != p_shader_program->dx_comp);

    tr_internal_dx_create_root_signature(p_renderer, p_descriptor_set, p_pipeline);
    tr_internal_dx_create_compute_pipeline_state(p_renderer, p_shader_program, p_pipeline_settings, p_pipeline);
}

void tr_internal_dx_destroy_pipeline(tr_renderer* p_renderer, tr_pipeline* p_pipeline)
{
    TINY_RENDERER_SAFE_RELEASE(p_pipeline->dx_root_signature);
    TINY_RENDERER_SAFE_RELEASE(p_pipeline->dx_pipeline_state);
}

void tr_internal_dx_create_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    assert(NULL != p_renderer->dx_device);

    if (p_render_target->color_attachment_count > 0) {
        if (p_render_target->sample_count > tr_sample_count_1) {
            assert(NULL != p_render_target->color_attachments_multisample);
        }
        else {
            assert(NULL != p_render_target->color_attachments);
        }

        TINY_RENDERER_DECLARE_ZERO(D3D12_DESCRIPTOR_HEAP_DESC, desc);
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        desc.NumDescriptors = (UINT)p_render_target->color_attachment_count;
        desc.NodeMask       = 0;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(&desc,
            __uuidof(p_render_target->dx_rtv_heap), (void**)&(p_render_target->dx_rtv_heap));
        assert(SUCCEEDED(hres));

        D3D12_CPU_DESCRIPTOR_HANDLE handle = p_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        const UINT inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
            if (p_render_target->sample_count > tr_sample_count_1) {
                assert(NULL != p_render_target->color_attachments_multisample[i]);
                assert(NULL != p_render_target->color_attachments_multisample[i]->dx_resource);

                p_renderer->dx_device->CreateRenderTargetView(
                    p_render_target->color_attachments_multisample[i]->dx_resource, NULL, handle);
            }
            else {
                assert(NULL != p_render_target->color_attachments[i]);
                assert(NULL != p_render_target->color_attachments[i]->dx_resource);

                p_renderer->dx_device->CreateRenderTargetView(
                    p_render_target->color_attachments[i]->dx_resource, NULL, handle);
            }
            handle.ptr += inc_size;
        }
    }

    if (tr_format_undefined != p_render_target->depth_stencil_format) {

        TINY_RENDERER_DECLARE_ZERO(D3D12_DESCRIPTOR_HEAP_DESC, desc);
        desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        desc.NumDescriptors = 1;
        desc.NodeMask       = 0;
        desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        HRESULT hres = p_renderer->dx_device->CreateDescriptorHeap(&desc,
            __uuidof(p_render_target->dx_dsv_heap), (void**)&(p_render_target->dx_dsv_heap));
        assert(SUCCEEDED(hres));

        D3D12_CPU_DESCRIPTOR_HANDLE handle = p_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        const UINT inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

        if (p_render_target->sample_count > tr_sample_count_1) {
          assert(NULL != p_render_target->depth_stencil_attachment_multisample);
          assert(NULL != p_render_target->depth_stencil_attachment_multisample->dx_resource);

          p_renderer->dx_device->CreateDepthStencilView(
            p_render_target->depth_stencil_attachment_multisample->dx_resource, NULL, handle);
        }
        else {
          assert(NULL != p_render_target->depth_stencil_attachment);
          assert(NULL != p_render_target->depth_stencil_attachment->dx_resource);

          p_renderer->dx_device->CreateDepthStencilView(
            p_render_target->depth_stencil_attachment->dx_resource, NULL, handle);
        }
    }
}

void tr_internal_dx_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
    TINY_RENDERER_SAFE_RELEASE(p_render_target->dx_rtv_heap);
    TINY_RENDERER_SAFE_RELEASE(p_render_target->dx_dsv_heap);
}

// -------------------------------------------------------------------------------------------------
// Internal descriptor set functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_update_descriptor_set(tr_renderer* p_renderer, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_renderer->dx_device);
    assert((NULL != p_descriptor_set->dx_cbvsrvuav_heap) || (NULL != p_descriptor_set->dx_sampler_heap));

    // Not really efficient, just write less frequently ;)
    uint32_t write_count = 0;
    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if ((NULL != descriptor->samplers) || (NULL != descriptor->textures) || (NULL != descriptor->uniform_buffers)) {
            ++write_count;
        }
    }
    // Bail if there's nothing to write
    if (0 == write_count) {
        return;
    }

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if ((NULL == descriptor->samplers) && (NULL == descriptor->textures) && (NULL == descriptor->uniform_buffers)) {
            continue;
        }

        switch (descriptor->type) {
            case tr_descriptor_type_sampler: {
                assert(NULL != descriptor->samplers);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_sampler_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->samplers[i]);

                    D3D12_SAMPLER_DESC* sampler_desc = &(descriptor->samplers[i]->dx_sampler_desc);
                    p_renderer->dx_device->CreateSampler(sampler_desc, handle);
                    handle.ptr += handle_inc_size;
                }
            }
            break;

            case tr_descriptor_type_uniform_buffer_cbv: {
                assert(NULL != descriptor->uniform_buffers);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->uniform_buffers[i]);

                    ID3D12Resource* resource = descriptor->uniform_buffers[i]->dx_resource;
                    D3D12_CONSTANT_BUFFER_VIEW_DESC* view_desc = &(descriptor->uniform_buffers[i]->dx_cbv_view_desc);
                    p_renderer->dx_device->CreateConstantBufferView(view_desc, handle);
                    handle.ptr += handle_inc_size;
                }
            }
            break;
           
            case tr_descriptor_type_storage_buffer_srv:
            case tr_descriptor_type_uniform_texel_buffer_srv: {
                assert(NULL != descriptor->buffers);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->buffers[i]);

                    ID3D12Resource* resource = descriptor->buffers[i]->dx_resource;
                    D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(descriptor->buffers[i]->dx_srv_view_desc);
                    p_renderer->dx_device->CreateShaderResourceView(resource, view_desc, handle);
                    handle.ptr += handle_inc_size;
                }
            }
            break;

            case tr_descriptor_type_storage_buffer_uav:
            case tr_descriptor_type_storage_texel_buffer_uav: {
                assert(NULL != descriptor->buffers);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->buffers[i]);

                    ID3D12Resource* resource = descriptor->buffers[i]->dx_resource;
                    D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(descriptor->buffers[i]->dx_uav_view_desc);
                    if (descriptor->buffers[i]->counter_buffer != NULL) {
                      ID3D12Resource* counter_resource = descriptor->buffers[i]->counter_buffer->dx_resource;
                      p_renderer->dx_device->CreateUnorderedAccessView(resource, counter_resource, view_desc, handle);
                    }
                    else {
                      p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc, handle);
                    }
                    handle.ptr += handle_inc_size;
                }
            }
            break;


            case tr_descriptor_type_texture_srv: {
                assert(NULL != descriptor->textures);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->textures[i]);

                    ID3D12Resource* resource = descriptor->textures[i]->dx_resource;
                    D3D12_SHADER_RESOURCE_VIEW_DESC* view_desc = &(descriptor->textures[i]->dx_srv_view_desc);
                    p_renderer->dx_device->CreateShaderResourceView(resource, view_desc, handle);
                    handle.ptr += handle_inc_size;
                }
            }
            break;

            case tr_descriptor_type_texture_uav: {
                assert(NULL != descriptor->textures);

                D3D12_CPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetCPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                for (uint32_t i = 0; i < descriptor->count; ++i) {
                    assert(NULL != descriptor->textures[i]);

                    ID3D12Resource* resource = descriptor->textures[i]->dx_resource;
                    D3D12_UNORDERED_ACCESS_VIEW_DESC* view_desc = &(descriptor->textures[i]->dx_uav_view_desc);
                    p_renderer->dx_device->CreateUnorderedAccessView(resource, NULL, view_desc, handle);
                    handle.ptr += handle_inc_size;
                }
            }
            break;
        }
    }
}

// -------------------------------------------------------------------------------------------------
// Internal command buffer functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_begin_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_cmd->cmd_pool->dx_cmd_alloc);

    HRESULT hres = p_cmd->cmd_pool->dx_cmd_alloc->Reset();
    assert(SUCCEEDED(hres));

    hres = p_cmd->dx_cmd_list->Reset(p_cmd->cmd_pool->dx_cmd_alloc, NULL);
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_end_cmd(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);

    HRESULT hres = p_cmd->dx_cmd_list->Close();
    assert(SUCCEEDED(hres));
}

void tr_internal_dx_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(s_tr_internal->bound_render_target == p_render_target);

    TINY_RENDERER_DECLARE_ZERO(D3D12_CPU_DESCRIPTOR_HANDLE, rtv_handle);
    TINY_RENDERER_DECLARE_ZERO(D3D12_CPU_DESCRIPTOR_HANDLE, dsv_handle);
    D3D12_CPU_DESCRIPTOR_HANDLE* p_rtv_handle = NULL;
    D3D12_CPU_DESCRIPTOR_HANDLE* p_dsv_handle = NULL;

    if (p_render_target->color_attachment_count > 0) {
        rtv_handle = p_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        p_rtv_handle = &rtv_handle;
    }

    if (tr_format_undefined != p_render_target->depth_stencil_format) {
        dsv_handle = p_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();
        p_dsv_handle = &dsv_handle;
    }

    p_cmd->dx_cmd_list->OMSetRenderTargets(p_render_target->color_attachment_count, p_rtv_handle, TRUE, p_dsv_handle);
}

void tr_internal_dx_cmd_end_render(tr_cmd* p_cmd)
{
    assert(NULL != p_cmd->dx_cmd_list);

    
    if ((NULL != s_tr_internal->bound_render_target) && (s_tr_internal->bound_render_target->sample_count > tr_sample_count_1)) {
        tr_render_target* render_target = s_tr_internal->bound_render_target;
        tr_texture** ss_attachments = render_target->color_attachments;
        tr_texture** ms_attachments = render_target->color_attachments_multisample;
        uint32_t color_attachment_count = render_target->color_attachment_count;
        bool is_present = (tr_texture_usage_present == (ss_attachments[0]->usage & tr_texture_usage_present));

        // This means we're dealing with a multisample swapchain
        if ((1 == color_attachment_count) && is_present) {
            tr_texture* ss_attachment = ss_attachments[0];
            tr_texture* ms_attachment = ms_attachments[0];
            if (tr_texture_usage_present == (ss_attachment->usage & tr_texture_usage_present)) {
                // If the render targets have transitioned correctly, we can expect them to be in the require states
                tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_color_attachment, tr_texture_usage_resolve_dst);
                tr_internal_dx_cmd_image_transition(p_cmd, ms_attachment, tr_texture_usage_color_attachment, tr_texture_usage_resolve_src);
                // Resolve from multisample to single sample
                p_cmd->dx_cmd_list->ResolveSubresource(ss_attachment->dx_resource, 0,
                                                       ms_attachment->dx_resource, 0,
                                                       tr_util_to_dx_format(render_target->color_format));
                // Put it back the way we found it
                tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_resolve_dst, tr_texture_usage_color_attachment);
                tr_internal_dx_cmd_image_transition(p_cmd, ms_attachment, tr_texture_usage_resolve_src, tr_texture_usage_color_attachment);
            }
        }
    }
    
}

void tr_internal_dx_cmd_set_viewport(tr_cmd* p_cmd, float x, float y, float width, float height, float min_depth, float max_depth)
{
    assert(NULL != p_cmd->dx_cmd_list);

    TINY_RENDERER_DECLARE_ZERO(D3D12_VIEWPORT, viewport);
    viewport.TopLeftX = x;
    viewport.TopLeftY = y;
    viewport.Width = width;
    viewport.Height = height;
    viewport.MinDepth = min_depth;
    viewport.MaxDepth = max_depth;

    p_cmd->dx_cmd_list->RSSetViewports(1, &viewport);
}

void tr_internal_dx_cmd_set_scissor(tr_cmd* p_cmd, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
{
    assert(NULL != p_cmd->dx_cmd_list);

    TINY_RENDERER_DECLARE_ZERO(D3D12_RECT, scissor);
    scissor.left = x;
    scissor.top = y;
    scissor.right = x + width;
    scissor.bottom = y + height;

    p_cmd->dx_cmd_list->RSSetScissorRects(1, &scissor);
}

void tr_cmd_internal_dx_cmd_clear_color_attachment(tr_cmd* p_cmd, uint32_t attachment_index, const tr_clear_value* clear_value)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != s_tr_internal->bound_render_target);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = s_tr_internal->bound_render_target->dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
    UINT inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    handle.ptr += attachment_index * inc_size;

    TINY_RENDERER_DECLARE_ZERO(FLOAT, color_rgba[4]);
    color_rgba[0] = clear_value->r;
    color_rgba[1] = clear_value->g;
    color_rgba[2] = clear_value->b;
    color_rgba[3] = clear_value->a;

    p_cmd->dx_cmd_list->ClearRenderTargetView(handle, color_rgba, 0, NULL);
}

void tr_cmd_internal_dx_cmd_clear_depth_stencil_attachment(tr_cmd* p_cmd, const tr_clear_value* clear_value)
{
  assert(NULL != p_cmd->dx_cmd_list);
  assert(NULL != s_tr_internal->bound_render_target);

  D3D12_CPU_DESCRIPTOR_HANDLE handle = s_tr_internal->bound_render_target->dx_dsv_heap->GetCPUDescriptorHandleForHeapStart();

  p_cmd->dx_cmd_list->ClearDepthStencilView(handle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, clear_value->depth, (uint8_t)clear_value->stencil, 0, nullptr);
}

void tr_internal_dx_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_pipeline->dx_pipeline_state);
    assert(NULL != p_pipeline->dx_root_signature);  

    p_cmd->dx_cmd_list->SetPipelineState(p_pipeline->dx_pipeline_state);
    
    if (p_pipeline->type == tr_pipeline_type_graphics) {
      D3D_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
      switch(p_pipeline->settings.primitive_topo) {
          case tr_primitive_topo_point_list     : topology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST; break;
          case tr_primitive_topo_line_list      : topology = D3D_PRIMITIVE_TOPOLOGY_LINELIST; break;
          case tr_primitive_topo_line_strip     : topology = D3D_PRIMITIVE_TOPOLOGY_LINESTRIP; break;
          case tr_primitive_topo_tri_list       : topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST; break;
          case tr_primitive_topo_tri_strip      : topology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP; break;
          case tr_primitive_topo_1_point_patch  : topology = D3D_PRIMITIVE_TOPOLOGY_1_CONTROL_POINT_PATCHLIST; break;
          case tr_primitive_topo_2_point_patch  : topology = D3D_PRIMITIVE_TOPOLOGY_2_CONTROL_POINT_PATCHLIST; break;
          case tr_primitive_topo_3_point_patch  : topology = D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST; break;
          case tr_primitive_topo_4_point_patch  : topology = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; break;
          default: break;
      }
      assert(D3D_PRIMITIVE_TOPOLOGY_UNDEFINED != topology);

      p_cmd->dx_cmd_list->SetGraphicsRootSignature(p_pipeline->dx_root_signature);
      p_cmd->dx_cmd_list->IASetPrimitiveTopology(topology);
    }
    else if (p_pipeline->type == tr_pipeline_type_compute) {
      p_cmd->dx_cmd_list->SetComputeRootSignature(p_pipeline->dx_root_signature);
    }
}

void tr_internal_dx_cmd_bind_descriptor_sets(tr_cmd* p_cmd, tr_pipeline* p_pipeline, tr_descriptor_set* p_descriptor_set)
{
    assert(NULL != p_cmd->dx_cmd_list);

    uint32_t descriptor_heap_count = 0;
    ID3D12DescriptorHeap* descriptor_heaps[2];
    if (NULL != p_descriptor_set->dx_cbvsrvuav_heap) {
        descriptor_heaps[descriptor_heap_count] = p_descriptor_set->dx_cbvsrvuav_heap;
        ++descriptor_heap_count;
    }
    if (NULL != p_descriptor_set->dx_sampler_heap) {
        descriptor_heaps[descriptor_heap_count] = p_descriptor_set->dx_sampler_heap;
        ++descriptor_heap_count;
    }

    if (descriptor_heap_count > 0) {
        p_cmd->dx_cmd_list->SetDescriptorHeaps(descriptor_heap_count, descriptor_heaps);
    }

    for (uint32_t i = 0; i < p_descriptor_set->descriptor_count; ++i) {
        tr_descriptor* descriptor = &(p_descriptor_set->descriptors[i]);
        if (UINT32_MAX == descriptor->dx_root_parameter_index) {
            continue;
        }

        switch (p_descriptor_set->descriptors[i].type) {
            case tr_descriptor_type_sampler: {
                D3D12_GPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_sampler_heap->GetGPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
            }
            break;

            case tr_descriptor_type_uniform_buffer_cbv:
            case tr_descriptor_type_storage_buffer_srv:
            case tr_descriptor_type_storage_buffer_uav:
            case tr_descriptor_type_texture_srv:
            case tr_descriptor_type_texture_uav:
            case tr_descriptor_type_uniform_texel_buffer_srv:
            case tr_descriptor_type_storage_texel_buffer_uav: {
                D3D12_GPU_DESCRIPTOR_HANDLE handle = p_descriptor_set->dx_cbvsrvuav_heap->GetGPUDescriptorHandleForHeapStart();
                UINT handle_inc_size = p_cmd->cmd_pool->renderer->dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
                handle.ptr += descriptor->dx_heap_offset * handle_inc_size;
                if (p_pipeline->type == tr_pipeline_type_graphics) {
                  p_cmd->dx_cmd_list->SetGraphicsRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
                }
                else if (p_pipeline->type == tr_pipeline_type_compute) {
                  p_cmd->dx_cmd_list->SetComputeRootDescriptorTable(descriptor->dx_root_parameter_index, handle);
                }
            }
            break;
        }       
    }
}

void tr_internal_dx_cmd_bind_index_buffer(tr_cmd* p_cmd, tr_buffer* p_buffer)
{
    assert(NULL != p_cmd->dx_cmd_list);

    assert(NULL != p_buffer->dx_index_buffer_view.BufferLocation);

    p_cmd->dx_cmd_list->IASetIndexBuffer(&(p_buffer->dx_index_buffer_view));
}

void tr_internal_dx_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t buffer_count, tr_buffer** pp_buffers)
{
    assert(NULL != p_cmd->dx_cmd_list);

    TINY_RENDERER_DECLARE_ZERO(D3D12_VERTEX_BUFFER_VIEW, views[tr_max_vertex_attribs]);
    for (uint32_t i = 0; i < buffer_count; ++i) {
        assert(NULL != pp_buffers[i]->dx_vertex_buffer_view.BufferLocation);

        views[i] = pp_buffers[i]->dx_vertex_buffer_view;
    }

    p_cmd->dx_cmd_list->IASetVertexBuffers(0, buffer_count, views);
}

void tr_internal_dx_cmd_draw(tr_cmd* p_cmd, uint32_t vertex_count, uint32_t first_vertex)
{
    assert(NULL != p_cmd->dx_cmd_list);

    p_cmd->dx_cmd_list->DrawInstanced(
        (UINT)vertex_count,
        (UINT)1,
        (UINT)first_vertex,
        (UINT)0
    );
}

void tr_internal_dx_cmd_draw_indexed(tr_cmd* p_cmd, uint32_t index_count, uint32_t first_index)
{
    assert(NULL != p_cmd->dx_cmd_list);

    p_cmd->dx_cmd_list->DrawIndexedInstanced(
        (UINT)index_count,
        (UINT)1,
        (UINT)first_index,
        (UINT)0,
        (UINT)0
    );
}

void tr_internal_dx_cmd_buffer_transition(tr_cmd* p_cmd, tr_buffer* p_buffer, tr_buffer_usage old_usage, tr_buffer_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_buffer->dx_resource);

    TINY_RENDERER_DECLARE_ZERO(D3D12_RESOURCE_BARRIER, barrier);
    barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource    = p_buffer->dx_resource;
    barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore  = tr_util_to_dx_resource_state_buffer(old_usage);
    barrier.Transition.StateAfter   = tr_util_to_dx_resource_state_buffer(new_usage);

    p_cmd->dx_cmd_list->ResourceBarrier(1, &barrier);
}

void tr_internal_dx_cmd_image_transition(tr_cmd* p_cmd, tr_texture* p_texture, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);
    assert(NULL != p_texture->dx_resource);

    TINY_RENDERER_DECLARE_ZERO(D3D12_RESOURCE_BARRIER, barrier);
    barrier.Type                    = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags                   = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition.pResource    = p_texture->dx_resource;
    barrier.Transition.Subresource  = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    barrier.Transition.StateBefore  = tr_util_to_dx_resource_state_texture(old_usage);
    barrier.Transition.StateAfter   = tr_util_to_dx_resource_state_texture(new_usage);

    p_cmd->dx_cmd_list->ResourceBarrier(1, &barrier);
}

void tr_internal_dx_cmd_render_target_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
    assert(NULL != p_cmd->dx_cmd_list);
    
    if (p_render_target->sample_count > tr_sample_count_1) {
        if (1 == p_render_target->color_attachment_count) {
            tr_texture* ss_attachment = p_render_target->color_attachments[0];
            tr_texture* ms_attachment = p_render_target->color_attachments_multisample[0];

            // This means we're dealing with a multisample swapchain
            if (tr_texture_usage_present == (ss_attachment->usage & tr_texture_usage_present)) {
                if ((tr_texture_usage_present == old_usage) && (tr_texture_usage_color_attachment == new_usage)) {
                    tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_present, tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) && (tr_texture_usage_present == new_usage)) {
                    tr_internal_dx_cmd_image_transition(p_cmd, ss_attachment, tr_texture_usage_color_attachment, tr_texture_usage_present);
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
                    tr_internal_dx_cmd_image_transition(p_cmd, attachment, tr_texture_usage_present, tr_texture_usage_color_attachment);
                }

                if ((tr_texture_usage_color_attachment == old_usage) && (tr_texture_usage_present == new_usage)) {
                    tr_internal_dx_cmd_image_transition(p_cmd, attachment, tr_texture_usage_color_attachment, tr_texture_usage_present);
                }
            }
        }
    }
}

void tr_internal_dx_cmd_depth_stencil_transition(tr_cmd* p_cmd, tr_render_target* p_render_target, tr_texture_usage old_usage, tr_texture_usage new_usage)
{
  assert(NULL != p_cmd->dx_cmd_list);

  if (p_render_target->sample_count > tr_sample_count_1) {
    tr_internal_dx_cmd_image_transition(p_cmd, p_render_target->depth_stencil_attachment_multisample, old_usage, new_usage);
  }
  else {
    tr_internal_dx_cmd_image_transition(p_cmd, p_render_target->depth_stencil_attachment, old_usage, new_usage);
  }
}

void tr_internal_dx_cmd_dispatch(tr_cmd* p_cmd, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z)
{
    assert(p_cmd->dx_cmd_list != NULL);

    p_cmd->dx_cmd_list->Dispatch(group_count_x, group_count_y, group_count_z);
}

void tr_internal_dx_cmd_copy_buffer_to_texture2d(tr_cmd* p_cmd, uint32_t width, uint32_t height, uint32_t row_pitch, uint64_t buffer_offset, uint32_t mip_level, tr_buffer* p_buffer, tr_texture* p_texture)
{
    assert(p_cmd->dx_cmd_list != NULL);

    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    layout.Offset = buffer_offset;
    layout.Footprint.Format   = DXGI_FORMAT_R8G8B8A8_UNORM;
    layout.Footprint.Width    = width;
    layout.Footprint.Height   = height;
    layout.Footprint.Depth    = 1;
    layout.Footprint.RowPitch = row_pitch;

    D3D12_TEXTURE_COPY_LOCATION src = {};
    src.pResource       = p_buffer->dx_resource;
    src.Type            = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
    src.PlacedFootprint = layout;
    D3D12_TEXTURE_COPY_LOCATION dst = {};
    dst.pResource        = p_texture->dx_resource;
    dst.Type             = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
     dst.SubresourceIndex = mip_level;

    p_cmd->dx_cmd_list->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);
}

// -------------------------------------------------------------------------------------------------
// Internal queue functions
// -------------------------------------------------------------------------------------------------
void tr_internal_dx_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
    assert(NULL != p_renderer->dx_swapchain);

    p_renderer->swapchain_image_index = p_renderer->dx_swapchain->GetCurrentBackBufferIndex();
}

void tr_internal_dx_queue_submit(
    tr_queue*      p_queue, 
    uint32_t       cmd_count,
    tr_cmd**       pp_cmds,
    uint32_t       wait_semaphore_count,
    tr_semaphore** pp_wait_semaphores,
    uint32_t       signal_semaphore_count,
    tr_semaphore** pp_signal_semaphores
)
{
    assert(NULL != p_queue->dx_queue);

    ID3D12CommandList* cmds[tr_max_submit_cmds];
    uint32_t count = cmd_count > tr_max_submit_cmds ? tr_max_submit_cmds : cmd_count;
    for (uint32_t i = 0; i < count; ++i) {
        cmds[i] = pp_cmds[i]->dx_cmd_list;
    }

    p_queue->dx_queue->ExecuteCommandLists(count, cmds);
}

void tr_internal_dx_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
    assert(NULL != p_queue->renderer->dx_swapchain);

    UINT sync_interval = 1;
    UINT flags = 0;
    p_queue->renderer->dx_swapchain->Present(sync_interval, flags);
}

void tr_internal_dx_queue_wait_idle(tr_queue* p_queue)
{
    assert(NULL != p_queue->dx_queue);
    assert(NULL != p_queue->dx_wait_idle_fence);
    assert(NULL != p_queue->dx_wait_idle_fence_event);

    // Signal and increment the fence value
    const UINT64 fence_value = p_queue->dx_wait_idle_fence_value;
    p_queue->dx_queue->Signal(p_queue->dx_wait_idle_fence, fence_value);
    ++p_queue->dx_wait_idle_fence_value;

    // Wait until the previous frame is finished.
    const UINT64 complted_value = p_queue->dx_wait_idle_fence->GetCompletedValue();
    if( complted_value < fence_value ) {
        p_queue->dx_wait_idle_fence->SetEventOnCompletion(fence_value, p_queue->dx_wait_idle_fence_event);
        WaitForSingleObject(p_queue->dx_wait_idle_fence_event, INFINITE);
    }
}

#endif // TINY_RENDERER_IMPLEMENTATION

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
} // namespace TINY_RENDERER_CPP_NAMESPACE
#endif