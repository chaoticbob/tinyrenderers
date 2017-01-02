
#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

enum {
	tr_max_instance_extensions       = 256,
	tr_max_device_extensions         = 256,
	tr_max_gpus                      = 4,
	tr_max_descriptors               = 32,
	tr_max_descriptor_sets           = 8,
	tr_max_render_target_attachments = 7,
	tr_max_submit_cmds               = 8,
	tr_max_submit_wait_semaphores    = 8,
	tr_max_submit_signal_semaphores  = 8,
	tr_max_present_wait_semaphores   = 8,
};

typedef enum tr_result {
	tr_result_ok = 0,
	tr_result_mem_alloc_failed,
	tr_result_bad_ptr,
	tr_result_only_one_renderer_allowed,
	tr_result_invalid_renderer,
	tr_result_environment_failed,
	tr_result_enumerate_gpu_failed,
	tr_result_renderer_failed,
	tr_result_device_failed,
	tr_result_surface_failed,
	tr_result_swapchain_failed,
	tr_result_no_queue_families,
	tr_result_render_target_failed,
	tr_result_texture_failed,
	tr_result_cmd_failed,
	tr_result_queue_failed,
	tr_result_fence_failed,
	tr_result_semaphore_failed,
} tr_result;

typedef enum tr_buffer_type {
	tr_buffer_type_index,
	tr_buffer_type_uniform,
	tr_buffer_type_vertex,
} tr_buffer_type;

typedef enum tr_texture_type {
	tr_texture_type_1d,
	tr_texture_type_2d,
	tr_texture_type_3d,
	tr_texture_type_cube,
} tr_texture_type;

typedef enum tr_texture_usage {
	tr_texture_usage_transfer_src             = 0x00000001,
	tr_texture_usage_transfer_dst             = 0x00000002,
	tr_texture_usage_sampled_image            = 0x00000004,
	tr_texture_usage_storage                  = 0x00000008,
	tr_texture_usage_color_attachment         = 0x00000010,
	tr_texture_usage_depth_stencil_attachment = 0x00000020,
	tr_texture_usage_present                  = 0x00000040,
} tr_texture_usage;

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
	tr_descriptor_type_image,
	tr_descriptor_type_sampler,
	tr_descriptor_type_uniform_buffer,
} tr_descriptor_type;

typedef enum tr_sample_count {
	tr_sample_count_1  =  1,
	tr_sample_count_2  =  2,
	tr_sample_count_4  =  4,
	tr_sample_count_8  =  8,
	tr_sample_count_16 = 16,
} tr_sample_count;

typedef enum tr_shader_stage {
	tr_shader_stage_vert      = 0x00000001,
	tr_shader_stage_tess_ctrl = 0x00000002,
	tr_shader_stage_tess_eval = 0x00000004,
	tr_shader_stage_geom      = 0x00000008,
	tr_shader_stage_frag      = 0x00000010,
	tr_shader_stage_comp      = 0x00000020,
} tr_shader_stage;

// Forward declarations
typedef struct tr_renderer tr_renderer;
typedef struct tr_render_target tr_render_target;

typedef struct tr_platform_handle {
#if defined(TINY_RENDERER_MSW)
	HINSTANCE						hinstance;
	HWND							hwnd;
#endif
} tr_platform_handle;

typedef struct tr_swapchain_settings {
	uint32_t						image_count;
	uint32_t						sample_count;
	tr_format						color_format;
	tr_format						depth_stencil_format;
} tr_swapchain_settings;

typedef struct tr_string_list {
	uint32_t		count;
	const char**	names;
} tr_string_list;

typedef struct tr_renderer_settings {
	tr_platform_handle				handle;
	uint32_t						width;
	uint32_t						height;
	tr_swapchain_settings			swapchain;
	// Vulkan specific options
	tr_string_list					instance_layers;
	tr_string_list					instance_extensions;
	tr_string_list					device_layers;
	tr_string_list					device_extensions;
	PFN_vkDebugReportCallbackEXT	vk_debug_fn;
} tr_renderer_settings;

typedef struct tr_fence {
	VkFence							vk_fence;
} tr_fence;

typedef struct tr_semaphore {
	VkSemaphore						vk_semaphore;
} tr_semaphore;

typedef struct tr_queue {
	tr_renderer*					renderer;
	VkQueue							vk_queue;
	uint32_t						vk_queue_family_index;
} tr_queue;

typedef struct tr_renderer {	
	tr_renderer_settings			settings;
	tr_render_target**				swapchain_render_targets;
	uint32_t						swapchain_image_index;
	tr_queue*						graphics_queue;
	tr_queue*						present_queue;
	tr_fence**						image_acquired_fences;
	tr_semaphore**					image_acquired_semaphores;
	tr_semaphore**					render_complete_semaphores;
	VkInstance						vk_instance;
	uint32_t						vk_gpu_count;
	VkPhysicalDevice				vk_gpus[tr_max_gpus];
	VkPhysicalDevice				vk_active_gpu;
	uint32_t						vk_active_gpu_index;
	VkDevice						vk_device;
	VkSurfaceKHR					vk_surface;
	VkSwapchainKHR					vk_swapchain;
	VkDebugReportCallbackEXT		vk_debug_report;
} tr_renderer;

typedef struct tr_descriptor {
	tr_descriptor_type				type;
	uint32_t						binding;
	uint32_t						count;
} tr_descriptor;

typedef struct tr_descriptor_set {
	uint32_t						descriptor_count;
	tr_descriptor					descriptors[tr_max_descriptors];
	VkDescriptorSet					vk_descriptor_set;
} tr_descriptor_set;

typedef struct tr_descriptor_batch {
	uint32_t						set_count;
	tr_descriptor_set				sets[tr_max_descriptor_sets];
	VkDescriptorPool				vk_pool;
} tr_descriptor_batch;

typedef struct tr_pipeline {
	tr_renderer*					renderer;
	VkPipelineLayout				vk_pipeline_layout;
	VkPipeline						vk_pipeline;
} tr_pipeline;

typedef struct tr_cmd {
	tr_renderer*					renderer;
	VkCommandPool					vk_cmd_pool;
	VkCommandBuffer					vk_cmd_buf;
} tr_cmd;

typedef struct tr_buffer {
	tr_renderer*					renderer;
	uint64_t						size;
	bool							host_visible;
	VkBuffer						vk_buffer;
} tr_buffer;

typedef struct tr_texture {
	tr_renderer*					renderer;
	tr_texture_type					type;
	tr_texture_usage				usage;
	uint32_t						width;
	uint32_t						height;
	uint32_t						depth;
	tr_format						format;
	uint32_t						mip_levels;
	tr_sample_count					sample_count;
	bool							host_visible;
	uint32_t						owns_image;
	VkImage							vk_image;
	VkImageView						vk_image_view;
} tr_texture;

typedef struct tr_shader_program {
	tr_renderer*					renderer;
	uint32_t						shader_stages;
	VkShaderModule					vk_vert;
	VkShaderModule					vk_tess_ctrl;
	VkShaderModule					vk_tess_eval;
	VkShaderModule					vk_geom;
	VkShaderModule					vk_frag;
	VkShaderModule					vk_comp;
} tr_shader_program;

typedef struct tr_color_value {
	float							r;
	float							g;
	float							b;
	float							a;
} tr_color_value;

typedef struct tr_depth_stencil_value {
	float							depth;
	uint32_t						stencil;
} tr_depth_stencil_value;

typedef struct tr_render_target {
	tr_renderer*					renderer;
	uint32_t						width;
	uint32_t						height;
	tr_sample_count					sample_count;
	tr_format						color_format;
	uint32_t						color_attachment_count;
	tr_texture*						color_attachments[tr_max_render_target_attachments];
	tr_texture*						color_attachments_multisample[tr_max_render_target_attachments];
	tr_color_value					color_clear_values[tr_max_render_target_attachments];
	tr_format						depth_stencil_format;
	tr_texture*						depth_stencil_attachment;
	tr_depth_stencil_value			depth_stencil_clear_value;
	VkRenderPass					vk_render_pass;
	VkFramebuffer					vk_framebuffer;
} tr_render_target;

typedef struct tr_mesh {
	tr_renderer*					renderer;
	tr_buffer*						uniform_buffer;
	tr_buffer*						index_buffer;
	tr_buffer*						vertex_buffer;
	tr_shader_program*				shader_program;
	tr_pipeline*					pipeline;
} tr_mesh;

// API functions
tr_api_export VkFormat  tr_to_vkformat(tr_format format);
tr_api_export tr_format tr_from_vk_format(VkFormat fomat);

tr_api_export tr_renderer* tr_get_renderer();

tr_api_export tr_result tr_create_renderer(const char*                 app_name, 
                                           const tr_renderer_settings* settings, 
                                           tr_renderer**               pp_renderer);
tr_api_export tr_result tr_destroy_renderer(tr_renderer* p_renderer);

tr_api_export tr_result tr_create_render_target(
    tr_renderer*       p_renderer, 
    uint32_t           width, 
    uint32_t           height, 
    tr_sample_count    sample_count, 
    tr_format          color_format, 
    uint32_t           color_attachment_count, 
    tr_format          depth_stencil_format, 
    tr_render_target** pp_render_target);
tr_api_export tr_result tr_destroy_render_target(
	tr_renderer*      p_renderer,
	tr_render_target* p_render_target);

tr_api_export tr_result tr_create_buffer(tr_renderer* p_renderer, tr_buffer_type type, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export tr_result tr_create_index_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export tr_result tr_create_uniform_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export tr_result tr_create_vertex_buffer(tr_renderer* p_renderer, uint64_t size, bool host_visible, tr_buffer** pp_buffer);
tr_api_export tr_result tr_destroy_buffer(tr_renderer* p_renderer, tr_buffer* p_buffer);

tr_api_export tr_result tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
tr_api_export tr_result tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
tr_api_export tr_result tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
tr_api_export tr_result tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture);
tr_api_export tr_result tr_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture);

tr_api_export tr_result tr_create_cmd(tr_renderer *p_renderer, uint32_t queue_family_index, bool secondary, tr_cmd** pp_cmd);
tr_api_export tr_result tr_destroy_cmd(tr_renderer *p_renderer, tr_cmd* p_cmd);

tr_api_export tr_result tr_create_fence(tr_renderer *p_renderer, tr_fence** pp_fence);
tr_api_export tr_result tr_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence);
tr_api_export tr_result tr_create_semaphore(tr_renderer *p_renderer, tr_semaphore** pp_semaphore);
tr_api_export tr_result tr_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);

tr_api_export tr_result tr_begin_cmd(tr_cmd* p_cmd);
tr_api_export tr_result tr_end_cmd(tr_cmd* p_cmd);
tr_api_export void      tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);
tr_api_export void      tr_cmd_end_render(tr_cmd* p_cmd);
tr_api_export void      tr_cmd_bind_pipeline(tr_cmd* p_cmd, tr_pipeline* p_pipeline);
tr_api_export void      tr_cmd_bind_descriptors(tr_cmd* p_cmd, const tr_descriptor_batch* p_descriptor_batch);
tr_api_export void      tr_cmd_bind_input_buffer(tr_cmd* p_cmd, const tr_buffer* p_buffer);
tr_api_export void      tr_cmd_bind_vertex_buffers(tr_cmd* p_cmd, uint32_t count, const tr_buffer* p_buffers);
tr_api_export void      tr_cmd_draw(uint32_t vertex_count, uint32_t first_vertex);
tr_api_export void      tr_cmd_draw_indexed(uint32_t index_count, uint32_t first_index);
tr_api_export void      tr_cmd_draw_mesh(tr_cmd* p_cmd, const tr_mesh* p_mesh);

tr_api_export tr_result tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
tr_api_export tr_result tr_queue_submit(tr_queue*      p_queue, 
                                        uint32_t       cmd_count,
                                        tr_cmd**       pp_cmds,
                                        uint32_t       wait_semaphore_count,
                                        tr_semaphore** pp_wait_semaphores,
                                        uint32_t       signal_semaphore_count,
                                        tr_semaphore** pp_signal_semaphores);
tr_api_export tr_result tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);
tr_api_export tr_result tr_queue_wait_idle(tr_queue* p_queue);

// =================================================================================================
// IMPLEMENTATION
// =================================================================================================

#if defined(TINY_RENDERER_IMPLEMENTATION)

#define TINY_RENDERER_ASSERT(cond) \
	assert(cond);

#define TINY_RENDERER_ALLOC_CHECK(p_var, ret_val) \
    if (NULL == p_var) {                          \
        return ret_val;                           \
    }

#define TINY_RENDERER_SAFE_FREE(p_var) \
    if (NULL != p_var) {               \
       free(p_var);                    \
       p_var = (void*)NULL;            \
    }

#define TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, ret_val) \
	if ((NULL == s_tr_internal) ||                            \
        (NULL == s_tr_internal->renderer) ||                  \
        (NULL == p_renderer) ||                               \
        (s_tr_internal->renderer != p_renderer)) {            \
		return ret_val;                                       \
	}                                                                 

#define TINY_RENDERER_OBJECT_PTR_CHECK(p_object, ret_val) \
	if (NULL == p_object) {                               \
		return ret_val;                                   \
	}

#define TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_object, renderer_ret_val, object_ret_val) \
	TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, renderer_ret_val);                                         \
	TINY_RENDERER_OBJECT_PTR_CHECK(p_object, object_ret_val);	

#define TINY_RENDERER_VK_RES_CHECK(vk_res, ret_val) \
    if (VK_SUCCESS != vk_res) {                     \
        return ret_val;                             \
    }

// Utility functions (may become external one day)
VkFormat              tr_to_vkformat(tr_format format);
tr_format             tr_from_vk_format(VkFormat format);
VkSampleCountFlagBits tr_to_vk_sample_count(tr_sample_count sample_count);
VkImageUsageFlags     tr_to_vk_image_usage(tr_texture_usage usage);
VkImageAspectFlags    tr_vk_determine_aspect_mask(VkFormat format);

// Internal init functions
tr_result tr_internal_vk_init_instance(const char* app_name, tr_renderer* p_renderer);
tr_result tr_internal_vk_init_surface(tr_renderer* p_renderer);
tr_result tr_internal_vk_init_device(tr_renderer* p_renderer);
tr_result tr_internal_vk_init_swapchain(tr_renderer* p_renderer);
tr_result tr_internal_init_swapchain_renderpass(tr_renderer* p_renderer);
tr_result tr_internal_vk_init_swapchain_renderpass(tr_renderer* p_renderer);

// Internal create functions
tr_result tr_internal_vk_create_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);
tr_result tr_internal_vk_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target);
tr_result tr_internal_vk_create_texture(tr_renderer* p_renderer, tr_texture* p_texture);
tr_result tr_internal_vk_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture);
tr_result tr_internal_vk_create_cmd(tr_renderer* p_renderer, uint32_t queue_family_index, bool secondary, tr_cmd* p_cmd);
tr_result tr_internal_vk_destroy_cmd(tr_renderer* p_renderer, tr_cmd* p_cmd);
tr_result tr_internal_vk_create_fence(tr_renderer *p_renderer, tr_fence* p_fence);
tr_result tr_internal_vk_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence);
tr_result tr_internal_vk_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);
tr_result tr_internal_vk_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore);

// Internal command buffer functions
tr_result tr_internal_vk_begin_cmd(tr_cmd* p_cmd);
void      tr_internal_vk_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target);

// Internal queue/swapchain functions
tr_result tr_internal_vk_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence);
tr_result tr_internal_vk_queue_submit(tr_queue*      p_queue, 
                                      uint32_t       cmd_count,
                                      tr_cmd**       pp_cmds,
                                      uint32_t       wait_semaphore_count,
                                      tr_semaphore** pp_wait_semaphores,
                                      uint32_t       signal_semaphore_count,
                                      tr_semaphore** pp_signal_semaphores);
tr_result tr_internal_vk_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores);

// Internal singleton 
typedef struct tr_internal_data {
	tr_renderer*	renderer;
	char*			error_msg_buf[4096];
} tr_internal_data;

static tr_internal_data* s_tr_internal = NULL;

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
tr_renderer* tr_get_renderer()
{
	tr_renderer* result = NULL;
	if (NULL != s_tr_internal) {
		result = s_tr_internal->renderer;
	}
	return result;
}

tr_result tr_create_renderer(const char *app_name, const tr_renderer_settings* settings, tr_renderer** pp_renderer)
{
	if (NULL == s_tr_internal) {
		s_tr_internal = (tr_internal_data*)calloc(1, sizeof(*s_tr_internal));
		s_tr_internal->renderer = (tr_renderer*)calloc(1, sizeof(*(s_tr_internal->renderer)));

		// Shorter way to get to the object
		tr_renderer* p_renderer = s_tr_internal->renderer;

		// Copy settings
		memcpy(&(p_renderer->settings), settings, sizeof(*settings));

		// Allocate storage for queues
		p_renderer->graphics_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->graphics_queue));
		p_renderer->present_queue = (tr_queue*)calloc(1, sizeof(*p_renderer->present_queue));
		if ((NULL == p_renderer->graphics_queue) || (NULL == p_renderer->present_queue)) {
			TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer);
			TINY_RENDERER_SAFE_FREE(s_tr_internal);
			return tr_result_mem_alloc_failed;
		}

		// Initialize the Vulkan bits
		{
			tr_result result = tr_internal_vk_init_instance(app_name, p_renderer);
			if (tr_result_ok != result) {
				return result;
			}

			result = tr_internal_vk_init_surface(p_renderer);
			if (tr_result_ok != result) {
				return result;
			}

			result = tr_internal_vk_init_device(p_renderer);
			if (tr_result_ok != result) {
				return result;
			}

			result = tr_internal_vk_init_swapchain(p_renderer);
			if (tr_result_ok != result) {
				return result;
			}
		}

		// Allocate and configure render target objects
		tr_result result = tr_internal_init_swapchain_renderpass(p_renderer);
		if (tr_result_ok != result) {
			return result;
		}

		// Initialize the Vulkan bits of the render targets
		result = tr_internal_vk_init_swapchain_renderpass(p_renderer);
		if (tr_result_ok != result) {
			return result;
		}

		// Initialize render sync objects
		p_renderer->image_acquired_fences = (tr_fence**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*(p_renderer->image_acquired_fences)));
		p_renderer->image_acquired_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*(p_renderer->image_acquired_semaphores)));
		p_renderer->render_complete_semaphores = (tr_semaphore**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*(p_renderer->render_complete_semaphores)));
		TINY_RENDERER_ALLOC_CHECK(p_renderer->image_acquired_fences, tr_result_mem_alloc_failed);
		TINY_RENDERER_ALLOC_CHECK(p_renderer->image_acquired_semaphores, tr_result_mem_alloc_failed);
		TINY_RENDERER_ALLOC_CHECK(p_renderer->render_complete_semaphores, tr_result_mem_alloc_failed);
		for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i ) {
			result = tr_create_fence(p_renderer, &(p_renderer->image_acquired_fences[i]));
			if (tr_result_ok != result) {
				return result;
			}

			result = tr_create_semaphore(p_renderer, &(p_renderer->image_acquired_semaphores[i]));
			if (tr_result_ok != result) {
				return result;
			}

			result = tr_create_semaphore(p_renderer, &(p_renderer->render_complete_semaphores[i]));
			if (tr_result_ok != result) {
				return result;
			}
		}

		// Renderer is good! Assign it to result!
		*(pp_renderer) = p_renderer;
	}
	else {
		return tr_result_only_one_renderer_allowed;
	}

	return tr_result_ok;
}

tr_result tr_destroy_renderer(tr_renderer* p_renderer)
{
	TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, tr_result_renderer_failed);

	if ((NULL != s_tr_internal) && (s_tr_internal->renderer == p_renderer)) {
		if (p_renderer != s_tr_internal->renderer) {
			return tr_result_invalid_renderer;
		}

		// Destroy the swapchain render targets
		if (NULL != p_renderer->swapchain_render_targets) {
			for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
				tr_result result = tr_destroy_render_target(p_renderer, p_renderer->swapchain_render_targets[i]);
				if (tr_result_ok != result) {
					return result;
				}
			}
		
			free(p_renderer->swapchain_render_targets);
		}

		// Destroy render sync objects
		if (NULL != p_renderer->image_acquired_fences) {
			for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
				tr_result result = tr_destroy_fence(p_renderer, p_renderer->image_acquired_fences[i]);
				if (tr_result_ok != result) {
					return result;
				}
			}
		}
		if (NULL != p_renderer->image_acquired_semaphores) {
			for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
				tr_result result = tr_destroy_semaphore(p_renderer, p_renderer->image_acquired_semaphores[i]);
				if (tr_result_ok != result) {
					return result;
				}
			}
		}
		if (NULL != p_renderer->render_complete_semaphores) {
			for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
				tr_result result = tr_destroy_semaphore(p_renderer, p_renderer->render_complete_semaphores[i]);
				if (tr_result_ok != result) {
					return result;
				}
			}
		}

		// Destroy the Vulkan bits
		{
			vkDestroySwapchainKHR(p_renderer->vk_device, p_renderer->vk_swapchain, NULL);
			vkDestroyDevice(p_renderer->vk_device, NULL);
			vkDestroySurfaceKHR(p_renderer->vk_instance, p_renderer->vk_surface, NULL);
			if ((NULL != trVkDestroyDebugReportCallbackEXT) && (VK_NULL_HANDLE !=  p_renderer->vk_debug_report)) {
				trVkDestroyDebugReportCallbackEXT(p_renderer->vk_instance, p_renderer->vk_debug_report, NULL);
			}
			vkDestroyInstance(p_renderer->vk_instance, NULL);
		}

		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_fences);
		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->image_acquired_semaphores);
		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->render_complete_semaphores);
		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->present_queue);
		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer->graphics_queue);
		TINY_RENDERER_SAFE_FREE(s_tr_internal->renderer);
		TINY_RENDERER_SAFE_FREE(s_tr_internal);
	}
	return tr_result_ok;
}


tr_result tr_create_render_target(
    tr_renderer*       p_renderer, 
    uint32_t           width, 
    uint32_t           height, 
    tr_sample_count    sample_count, 
    tr_format          color_format, 
    uint32_t           color_attachment_count, 
    tr_format          depth_stencil_format, 
    tr_render_target** pp_render_target
)
{
	TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, tr_result_invalid_renderer);

	tr_render_target render_target = {0};
	render_target.renderer               = p_renderer;
	render_target.width                  = width;
	render_target.height                 = height;
	render_target.sample_count           = sample_count;
	render_target.color_format           = color_format;
	render_target.color_attachment_count = color_attachment_count;
	render_target.depth_stencil_format   = depth_stencil_format;
	
	// Create attachments
	{
		// Color
		for (uint32_t i = 0; i < render_target.color_attachment_count; ++i) {
			tr_result result = tr_create_texture_2d(p_renderer, 
                                                    render_target.width, 
                                                    render_target.height, 
                                                    tr_sample_count_1,
                                                    render_target.color_format, 
                                                    false,
                                                    tr_texture_usage_color_attachment | tr_texture_usage_sampled_image,
                                                    &(render_target.color_attachments[i]));
			if (tr_result_ok != result) {
				return tr_result_render_target_failed;
			}

			if (render_target.sample_count > tr_sample_count_1) {
				result = tr_create_texture_2d(p_renderer, 
                                              render_target.width, 
                                              render_target.height, 
                                              render_target.sample_count,
                                              render_target.color_format, 
                                              false,
                                              tr_texture_usage_color_attachment | tr_texture_usage_sampled_image,
                                              &(render_target.color_attachments_multisample[i]));
				if (tr_result_ok != result) {
					return tr_result_render_target_failed;
				}
			}
		}

		// Depth/stencil
		if (tr_format_undefined != render_target.depth_stencil_format) {
			tr_result result = tr_create_texture_2d(p_renderer, 
                                                    render_target.width, 
                                                    render_target.height, 
                                                    render_target.sample_count,
                                                    render_target.color_format, 
                                                    false,
                                                    tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image,
                                                    &(render_target.depth_stencil_attachment));
			if (tr_result_ok != result) {
				return tr_result_render_target_failed;
			}
		}
	}

	// Create Vulkan specific objects for the render target
	tr_result result = tr_internal_vk_create_render_target(p_renderer, &render_target);
	if (tr_result_ok != result) {
		// Free attachments so we don't leak
		for (uint32_t i = 0; i < render_target.color_attachment_count; ++i) {
			TINY_RENDERER_SAFE_FREE(render_target.color_attachments[i]);
			TINY_RENDERER_SAFE_FREE(render_target.color_attachments_multisample[i]);
		}
		TINY_RENDERER_SAFE_FREE(render_target.depth_stencil_attachment);
		return result;
	}

	*pp_render_target = (tr_render_target*)calloc(1, sizeof(**pp_render_target));
	if (NULL == *pp_render_target) {
		// Free attachments so we don't leak
		for (uint32_t i = 0; i < render_target.color_attachment_count; ++i) {
			TINY_RENDERER_SAFE_FREE(render_target.color_attachments[i]);
			TINY_RENDERER_SAFE_FREE(render_target.color_attachments_multisample[i]);
		}
		TINY_RENDERER_SAFE_FREE(render_target.depth_stencil_attachment);
		return tr_result_render_target_failed;
	}

	memcpy(*pp_render_target, &render_target, sizeof(render_target));

	return tr_result_ok;
}

tr_result tr_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_render_target, tr_result_invalid_renderer, tr_result_bad_ptr);

	if ((s_tr_internal->renderer == p_renderer) && (NULL != p_render_target)) {
		// Destroy color attachments
		for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
			tr_destroy_texture(p_renderer, p_render_target->color_attachments[i]);
			tr_destroy_texture(p_renderer, p_render_target->color_attachments_multisample[i]);
		}

		// Destroy depth attachment
		tr_destroy_texture(p_renderer, p_render_target->depth_stencil_attachment);

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
	return tr_result_ok;
}

tr_result tr_create_texture(tr_renderer* p_renderer, tr_texture_type type, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture)
{
	return tr_result_ok;
}

tr_result tr_create_texture_1d(tr_renderer* p_renderer, uint32_t width, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture)
{
	return tr_create_texture(p_renderer, tr_texture_type_1d, width, 1, 1, sample_count, format, host_visible, usage, pp_texture);
}

tr_result tr_create_texture_2d(tr_renderer* p_renderer, uint32_t width, uint32_t height, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture)
{
	return tr_create_texture(p_renderer, tr_texture_type_2d, width, height, 1, sample_count, format, host_visible, usage, pp_texture);
}

tr_result tr_create_texture_3d(tr_renderer* p_renderer, uint32_t width, uint32_t height, uint32_t depth, tr_sample_count sample_count, tr_format format, bool host_visible, tr_texture_usage usage, tr_texture** pp_texture)
{
	return tr_create_texture(p_renderer, tr_texture_type_3d, width, height, depth, sample_count, format, host_visible, usage, pp_texture);
}

tr_result tr_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_texture, tr_result_invalid_renderer, tr_result_bad_ptr);

	tr_internal_vk_destroy_texture(p_renderer, p_texture);

	TINY_RENDERER_SAFE_FREE(p_texture);
	return tr_result_ok;
}

tr_result tr_create_fence(tr_renderer *p_renderer, tr_fence** pp_fence)
{
	tr_fence* p_fence = (tr_fence*)calloc(1, sizeof(*p_fence));
	TINY_RENDERER_ALLOC_CHECK(p_fence, tr_result_mem_alloc_failed);

	tr_result result = tr_internal_vk_create_fence(p_renderer, p_fence);
	if (tr_result_ok != result) {
		TINY_RENDERER_SAFE_FREE(p_fence);
		return result;
	}

	*pp_fence = p_fence;
	return tr_result_ok;
}

tr_result tr_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{
	tr_internal_vk_destroy_fence(p_renderer, p_fence);
	TINY_RENDERER_SAFE_FREE(p_fence);
	return tr_result_ok;
}

tr_result tr_create_semaphore(tr_renderer *p_renderer, tr_semaphore** pp_semaphore)
{
	tr_semaphore* p_semaphore = (tr_semaphore*)calloc(1, sizeof(*p_semaphore));
	TINY_RENDERER_ALLOC_CHECK(p_semaphore, tr_result_mem_alloc_failed);

	tr_result result = tr_internal_vk_create_semaphore(p_renderer, p_semaphore);
	if (tr_result_ok != result) {
		TINY_RENDERER_SAFE_FREE(p_semaphore);
		return result;
	}

	*pp_semaphore = p_semaphore;
	return tr_result_ok;
}

tr_result tr_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{
	tr_internal_vk_destroy_semaphore(p_renderer, p_semaphore);
	TINY_RENDERER_SAFE_FREE(p_semaphore);
	return tr_result_ok;
}

tr_result tr_create_cmd(tr_renderer *p_renderer, uint32_t queue_family_index, bool secondary, tr_cmd** pp_cmd)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, pp_cmd, tr_result_invalid_renderer, tr_result_bad_ptr);

	tr_cmd* p_cmd = (tr_cmd*)calloc(1, sizeof(*p_cmd));
	TINY_RENDERER_ALLOC_CHECK(p_cmd, tr_result_mem_alloc_failed);

	tr_result result = tr_internal_vk_create_cmd(p_renderer, queue_family_index, secondary, p_cmd);
	if (tr_result_ok != result) {
		TINY_RENDERER_SAFE_FREE(p_cmd);
		return tr_result_cmd_failed;
	}

	*pp_cmd = p_cmd;

	return tr_result_ok;
}

tr_result tr_destroy_cmd(tr_renderer *p_renderer, tr_cmd* p_cmd)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_cmd, tr_result_invalid_renderer, tr_result_bad_ptr);

	tr_internal_vk_destroy_cmd(p_renderer, p_cmd);
	
	TINY_RENDERER_SAFE_FREE(p_cmd);
	return tr_result_ok;
}


tr_result tr_begin_cmd(tr_cmd* p_cmd)
{
	tr_result result = tr_internal_vk_begin_cmd(p_cmd);
	if (tr_result_ok != result) {
		return result;
	}

	return tr_result_ok;
}

tr_result tr_end_cmd(tr_cmd* p_cmd)
{
	TINY_RENDERER_OBJECT_PTR_CHECK(p_cmd, tr_result_bad_ptr);

	VkResult res = vkEndCommandBuffer(p_cmd->vk_cmd_buf);
	TINY_RENDERER_VK_RES_CHECK(res, tr_result_cmd_failed);

	return tr_result_ok;
}

void tr_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
	tr_internal_vk_cmd_begin_render(p_cmd, p_render_target);
}

void tr_cmd_end_render(tr_cmd* p_cmd)
{
	TINY_RENDERER_ASSERT(NULL != p_cmd);
	vkCmdEndRenderPass(p_cmd->vk_cmd_buf);
}

tr_result tr_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
	TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, tr_result_invalid_renderer);

	tr_result result = tr_internal_vk_acquire_next_image(p_renderer, p_signal_semaphore, p_fence);
	if (tr_result_ok != result) {
		return result;
	}

	return tr_result_ok;
}

tr_result tr_queue_submit(
	tr_queue*      p_queue, 
	uint32_t       cmd_count,
	tr_cmd**       pp_cmds,
	uint32_t       wait_semaphore_count,
	tr_semaphore** pp_wait_semaphores,
	uint32_t       signal_semaphore_count,
	tr_semaphore** pp_signal_semaphores
)
{
	tr_result result = tr_internal_vk_queue_submit(p_queue, 
                                                   cmd_count, 
                                                   pp_cmds, 
                                                   wait_semaphore_count, 
                                                   pp_wait_semaphores, 
                                                   signal_semaphore_count, 
                                                   pp_signal_semaphores);
	if (tr_result_ok != result) {
		return result;
	}

	return tr_result_ok;
}

tr_result tr_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
	return tr_internal_vk_queue_present(p_queue, wait_semaphore_count, pp_wait_semaphores);
}

tr_result tr_queue_wait_idle(tr_queue* p_queue)
{
	VkResult vk_res = vkQueueWaitIdle(p_queue->vk_queue);
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_queue_failed);

	return tr_result_ok;
}

// -------------------------------------------------------------------------------------------------
// Utility functions
// -------------------------------------------------------------------------------------------------
tr_api_export VkFormat tr_to_vkformat(tr_format format)
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

tr_api_export tr_format tr_from_vk_format(VkFormat format)
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

VkSampleCountFlagBits tr_to_vk_sample_count(tr_sample_count sample_count) 
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

VkImageUsageFlags tr_to_vk_image_usage(tr_texture_usage usage)
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
	if (tr_texture_usage_storage == (usage & tr_texture_usage_storage)) {
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

VkImageAspectFlags tr_vk_determine_aspect_mask(VkFormat format)
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

// -------------------------------------------------------------------------------------------------
// Internal init functions
// -------------------------------------------------------------------------------------------------
tr_result tr_internal_vk_find_queue_family(VkPhysicalDevice gpu, const VkQueueFlags queue_flags, uint32_t* p_queue_family_index, VkQueueFamilyProperties* p_queue_family_properties )
{
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
	if (0 == count) {
		return tr_result_no_queue_families;
	}

	VkQueueFamilyProperties* properties = (VkQueueFamilyProperties*)calloc(count, sizeof(*properties));
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

	free(properties);

	return (VK_TRUE == found) ?  tr_result_ok : tr_result_no_queue_families;
}

tr_result tr_internal_vk_find_present_queue_family(VkPhysicalDevice gpu, VkSurfaceKHR surface, uint32_t* p_queue_family_index)
{
	uint32_t count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
	if (0 == count) {
		return tr_result_no_queue_families;
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

	return (VK_TRUE == found) ?  tr_result_ok : tr_result_no_queue_families;
}

tr_result tr_internal_vk_init_instance(const char* app_name, tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	VkApplicationInfo app_info = {0};
	app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pNext              = NULL;
	app_info.pApplicationName   = app_name;
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName        = "tinyvk";
	app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion         = VK_MAKE_VERSION(1, 0, 3);

	// Instance
	{
		uint32_t extension_count = 0;
		const char** extensions = (const char**)calloc(tr_max_instance_extensions, sizeof(*extensions));
		if (extension_count < tr_max_instance_extensions) {
			++extension_count;
			uint32_t n = extension_count - 1;
			extensions[n] = (const char*)calloc(1, strlen(VK_KHR_SURFACE_EXTENSION_NAME) + 1);
			memcpy((void*)extensions[n], VK_KHR_SURFACE_EXTENSION_NAME, strlen(VK_KHR_SURFACE_EXTENSION_NAME) + 1);
		}

#if defined(TINY_RENDERER_MSW)
		if (extension_count < tr_max_instance_extensions) {
			++extension_count;
			uint32_t n = extension_count - 1;
			extensions[n] = (const char*)calloc(1, strlen(VK_KHR_WIN32_SURFACE_EXTENSION_NAME) + 1);
			memcpy((void*)extensions[n], VK_KHR_WIN32_SURFACE_EXTENSION_NAME, strlen(VK_KHR_WIN32_SURFACE_EXTENSION_NAME) + 1);
		}
#endif
		// Add more extensions here
		VkInstanceCreateInfo create_info = {0};
		create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pNext                   = NULL;
		create_info.flags                   = 0;
		create_info.pApplicationInfo        = &app_info;
		create_info.enabledLayerCount       = 0;
		create_info.ppEnabledLayerNames     = NULL;
		create_info.enabledExtensionCount   = extension_count;
		create_info.ppEnabledExtensionNames = extensions;
		VkResult res = vkCreateInstance(&create_info, NULL, &(p_renderer->vk_instance));
		// Free extensions names first so we don't leak
		for (uint32_t i = 0; i < extension_count; ++i) {
			free((void*)extensions[i]);
		}
		free((void*)extensions);
		if (VK_SUCCESS != res) {
			return tr_result_environment_failed;
		}
	}

	// Debug
	{
		trVkCreateDebugReportCallbackEXT  = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkCreateDebugReportCallbackEXT");
		trVkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkDestroyDebugReportCallbackEXT");
		trVkDebugReportMessageEXT         = (PFN_vkDebugReportMessageEXT)vkGetInstanceProcAddr(p_renderer->vk_instance, "vkDebugReportMessageEXT");

		if ((NULL != trVkCreateDebugReportCallbackEXT) && (NULL != trVkDestroyDebugReportCallbackEXT) && (NULL != trVkDebugReportMessageEXT)) {
			VkDebugReportCallbackCreateInfoEXT create_info = {0};
			create_info.sType       = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
			create_info.pNext       = NULL;
			create_info.pfnCallback	= tr_internal_debug_report_callback;
			create_info.pUserData   = NULL;
			create_info.flags       = VK_DEBUG_REPORT_INFORMATION_BIT_EXT | 
									  VK_DEBUG_REPORT_WARNING_BIT_EXT | 
									  VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | 
									  VK_DEBUG_REPORT_ERROR_BIT_EXT |
									  VK_DEBUG_REPORT_DEBUG_BIT_EXT;
			VkResult res = trVkCreateDebugReportCallbackEXT(p_renderer->vk_instance, &create_info, NULL, &(p_renderer->vk_debug_report));
			if (VK_SUCCESS != res) {
				return tr_result_environment_failed;
			}
		}
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_init_surface(tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	// Surface
	{
#if defined(TINY_RENDERER_MSW)
		VkWin32SurfaceCreateInfoKHR create_info = {0};
		create_info.sType     = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		create_info.pNext     = NULL;
		create_info.flags     = 0;
		create_info.hinstance = p_renderer->settings.handle.hinstance;
		create_info.hwnd      = p_renderer->settings.handle.hwnd;
		VkResult res = vkCreateWin32SurfaceKHR(p_renderer->vk_instance, &create_info, NULL, &(p_renderer->vk_surface));
		if (VK_SUCCESS != res) {
			return tr_result_surface_failed;
		}
#endif
	}
	return tr_result_ok;
}

tr_result tr_internal_vk_init_device(tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	VkResult res = vkEnumeratePhysicalDevices(p_renderer->vk_instance, &(p_renderer->vk_gpu_count), NULL);
	if ((VK_SUCCESS != res) || (0 == p_renderer->vk_gpu_count)) {
		return tr_result_enumerate_gpu_failed;
	}

	if (p_renderer->vk_gpu_count > tr_max_gpus) {
		p_renderer->vk_gpu_count = tr_max_gpus;
	}

	res = vkEnumeratePhysicalDevices(p_renderer->vk_instance, &(p_renderer->vk_gpu_count), p_renderer->vk_gpus);
	if (VK_SUCCESS != res) {
		return tr_result_enumerate_gpu_failed;
	}

	// Find gpu that supports both graphics and present
	p_renderer->vk_active_gpu_index = UINT32_MAX;
	for (uint32_t gpu_index = 0; gpu_index < p_renderer->vk_gpu_count; ++gpu_index) {
		VkPhysicalDevice gpu = p_renderer->vk_gpus[gpu_index];

		// Make sure GPU supports graphics queue
		uint32_t graphics_queue_family_index = UINT32_MAX;
		if (tr_result_ok != tr_internal_vk_find_queue_family(gpu, VK_QUEUE_GRAPHICS_BIT, &graphics_queue_family_index, NULL)) {
			continue;
		}

		// Make sure GPU supports present
		uint32_t present_queue_family_index = UINT32_MAX;
		if (tr_result_ok != tr_internal_vk_find_present_queue_family(gpu, p_renderer->vk_surface, &present_queue_family_index)) {
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

	float queue_priorites[1] = {1.0f};

	VkDeviceQueueCreateInfo queue_create_infos[2] = {0};
	queue_create_infos[0].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_infos[0].pNext            = NULL;
	queue_create_infos[0].flags            = 0;
	queue_create_infos[0].queueFamilyIndex = p_renderer->graphics_queue->vk_queue_family_index;
	queue_create_infos[0].queueCount       = 1;
	queue_create_infos[0].pQueuePriorities = queue_priorites;
	queue_create_infos[1].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_infos[1].pNext            = NULL;
	queue_create_infos[1].flags            = 0;
	queue_create_infos[1].queueFamilyIndex = p_renderer->present_queue->vk_queue_family_index;
	queue_create_infos[1].queueCount       = 1;
	queue_create_infos[1].pQueuePriorities = queue_priorites;

	uint32_t extension_count = 0;
	char** extensions = (char**)calloc(tr_max_instance_extensions, sizeof(*extensions));
	if (extension_count < tr_max_instance_extensions) {
		++extension_count;
		uint32_t n = extension_count - 1;
		extensions[n] = calloc(1, strlen(VK_KHR_SWAPCHAIN_EXTENSION_NAME) + 1);
		memcpy(extensions[n], VK_KHR_SWAPCHAIN_EXTENSION_NAME, strlen(VK_KHR_SWAPCHAIN_EXTENSION_NAME) + 1);
	}
	// Add more extensions here
		
	VkDeviceCreateInfo create_info = {0};
	create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	create_info.pNext                   = NULL;
	create_info.flags                   = 0;
	create_info.queueCreateInfoCount    = 2;
	create_info.pQueueCreateInfos       = queue_create_infos;
	create_info.enabledLayerCount       = 0;
	create_info.ppEnabledLayerNames     = NULL;
	create_info.enabledExtensionCount   = extension_count;
	create_info.ppEnabledExtensionNames = extensions;
	create_info.pEnabledFeatures        = NULL;
	res = vkCreateDevice(p_renderer->vk_active_gpu, &create_info, NULL, &(p_renderer->vk_device));
	// Free extensions names first so we don't leak
	for (uint32_t i = 0; i < extension_count; ++i) {
		free(extensions[i]);
	}
	free(extensions);
	// Now check result
	if (VK_SUCCESS != res) {
		return tr_result_device_failed;
	}

	vkGetDeviceQueue(p_renderer->vk_device, p_renderer->graphics_queue->vk_queue_family_index, 0, &(p_renderer->graphics_queue->vk_queue));
	vkGetDeviceQueue(p_renderer->vk_device, p_renderer->present_queue->vk_queue_family_index, 0, &(p_renderer->present_queue->vk_queue));
	p_renderer->graphics_queue->renderer = p_renderer;
	p_renderer->present_queue->renderer = p_renderer;

	return tr_result_ok;
}

tr_result tr_internal_vk_init_swapchain(tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	// Image count
	{
		if (0 == p_renderer->settings.swapchain.image_count) {
			p_renderer->settings.swapchain.image_count = 2;
		}

		VkSurfaceCapabilitiesKHR caps = {0};
		VkResult res = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &caps);
		if (VK_SUCCESS != res) { 
			return tr_result_swapchain_failed; 
		}

		if ((caps.maxImageCount > 0) && (p_renderer->settings.swapchain.image_count > caps.maxImageCount)) {
			p_renderer->settings.swapchain.image_count = caps.maxImageCount;
		}
	}

	// Surface format
	VkSurfaceFormatKHR surface_format = {0};
	surface_format.format = VK_FORMAT_UNDEFINED;
	{
		uint32_t count = 0;
		VkSurfaceFormatKHR* formats = NULL;
	
		// Get surface formats count
		VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, NULL);
		if (VK_SUCCESS != res) { 
			return tr_result_swapchain_failed; 
		}
		// Allocate and get surface formats
		formats = (VkSurfaceFormatKHR*)calloc(count, sizeof(*formats));
		res = vkGetPhysicalDeviceSurfaceFormatsKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, formats);
		// Now error check
		if (VK_SUCCESS != res) { 
			// Free first so we don't leak
			if (NULL != formats) {
				free(formats);
			}
			return tr_result_swapchain_failed; 
		}

		if ((1 == count) && (VK_FORMAT_UNDEFINED == formats[0].format)) {
			surface_format.format = VK_FORMAT_B8G8R8A8_UNORM;
			surface_format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		}
		else {
			VkFormat requested_format = tr_to_vkformat(p_renderer->settings.swapchain.color_format);
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

		if (VK_FORMAT_UNDEFINED == surface_format.format) {
			return tr_result_swapchain_failed;
		}

		// Free formats now that we're done
		if (NULL != formats) {
			free(formats);
		}
	}

	// Present modes
	VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
	{
		uint32_t count = 0;
		VkPresentModeKHR* modes = NULL;
		// Get present mode count
		VkResult res = vkGetPhysicalDeviceSurfacePresentModesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, NULL);
		if (VK_SUCCESS != res) {
			return tr_result_swapchain_failed;
		}
		// Allocate and get present modes
		modes = (VkPresentModeKHR*)calloc(count, sizeof(*modes));
		res = vkGetPhysicalDeviceSurfacePresentModesKHR(p_renderer->vk_active_gpu, p_renderer->vk_surface, &count, modes);
		// Now error check
		if (VK_SUCCESS != res) { 
			// Free first so we don't leak
			if (NULL != modes) {
				free(modes);
			}
			return tr_result_swapchain_failed; 
		}

		for (uint32_t i = 0; i < count; ++i) {
			if (VK_PRESENT_MODE_IMMEDIATE_KHR == modes[i]) {
				present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
				break;
			}
		}

		// Free modes now that we're done
		if (NULL != modes) {
			free(modes);
		}
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

		VkSwapchainCreateInfoKHR create_info = {0};
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
		VkResult res = vkCreateSwapchainKHR(p_renderer->vk_device, &create_info, NULL, &(p_renderer->vk_swapchain));
		if (VK_SUCCESS != res) {
			return tr_result_swapchain_failed;
		}

		p_renderer->settings.swapchain.color_format = tr_from_vk_format(surface_format.format);

		// Make sure depth/stencil format is supported - fall back to VK_FORMAT_D16_UNORM if not
		VkFormat vk_depth_stencil_format = tr_to_vkformat(p_renderer->settings.swapchain.depth_stencil_format);
		if (VK_FORMAT_UNDEFINED != vk_depth_stencil_format) {
			VkImageFormatProperties properties = {0};
            res = vkGetPhysicalDeviceImageFormatProperties(p_renderer->vk_active_gpu, 
                                                           vk_depth_stencil_format,
                                                           VK_IMAGE_TYPE_2D, VK_IMAGE_TILING_OPTIMAL,
                                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
                                                           0, 
                                                           &properties);
			if (VK_SUCCESS != res) {
				p_renderer->settings.swapchain.depth_stencil_format = tr_format_d16_unorm;
			}
		}
	}

	return tr_result_ok;
}

tr_result tr_internal_init_swapchain_renderpass(tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	p_renderer->swapchain_render_targets = (tr_render_target**)calloc(p_renderer->settings.swapchain.image_count, sizeof(*p_renderer->swapchain_render_targets));
	for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
		p_renderer->swapchain_render_targets[i] = (tr_render_target*)calloc(1, sizeof(*(p_renderer->swapchain_render_targets[i])));
		tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
		render_target->renderer               = p_renderer;
		render_target->width                  = p_renderer->settings.width;
		render_target->height                 = p_renderer->settings.height;
		render_target->sample_count           = p_renderer->settings.swapchain.sample_count;
		render_target->color_format           = p_renderer->settings.swapchain.color_format;
		render_target->color_attachment_count = 1;
		render_target->depth_stencil_format   = p_renderer->settings.swapchain.depth_stencil_format;

		render_target->color_attachments[0] = (tr_texture*)calloc(1, sizeof(*render_target->color_attachments[0]));
		if (NULL == render_target->color_attachments[0]){
			goto label_attachment_allocation_failed;
		}

		if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
			render_target->color_attachments_multisample[0] = (tr_texture*)calloc(1, sizeof(*render_target->color_attachments_multisample[0]));
			if (NULL == render_target->color_attachments_multisample[0]){
				goto label_attachment_allocation_failed;
			}
		}

		if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
			render_target->depth_stencil_attachment = (tr_texture*)calloc(1, sizeof(*render_target->depth_stencil_attachment));
			if (NULL == render_target->depth_stencil_attachment) {
				goto label_attachment_allocation_failed;
			}
		}
	}

	goto label_attachment_allocation_goood;

label_attachment_allocation_failed:
	for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
		tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
			
		if (NULL != render_target->color_attachments[0]) {
			free(render_target->color_attachments[0]);
		}

		if (NULL != render_target->color_attachments[0]) {
			free(render_target->color_attachments[0]);
		}

		if (NULL != render_target->depth_stencil_attachment) {
			free(render_target->depth_stencil_attachment);
		}
	}
	return tr_result_swapchain_failed;

label_attachment_allocation_goood:
	for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
		tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
		render_target->color_attachments[0]->type         = tr_texture_type_2d;
		render_target->color_attachments[0]->usage        = tr_texture_usage_sampled_image | tr_texture_usage_color_attachment;
		render_target->color_attachments[0]->width        = p_renderer->settings.width;
		render_target->color_attachments[0]->height       = p_renderer->settings.height;
		render_target->color_attachments[0]->depth        = 1;
		render_target->color_attachments[0]->format       = p_renderer->settings.swapchain.color_format;
		render_target->color_attachments[0]->mip_levels   = 1;
		render_target->color_attachments[0]->sample_count = tr_sample_count_1;

		if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
			render_target->color_attachments_multisample[0]->type         = tr_texture_type_2d;
			render_target->color_attachments_multisample[0]->usage        = tr_texture_usage_sampled_image | tr_texture_usage_color_attachment;
			render_target->color_attachments_multisample[0]->width        = p_renderer->settings.width;
			render_target->color_attachments_multisample[0]->height       = p_renderer->settings.height;
			render_target->color_attachments_multisample[0]->depth        = 1;
			render_target->color_attachments_multisample[0]->format       = p_renderer->settings.swapchain.color_format;
			render_target->color_attachments_multisample[0]->mip_levels   = 1;
			render_target->color_attachments_multisample[0]->sample_count = render_target->sample_count;
		}

		if (tr_format_undefined != p_renderer->settings.swapchain.depth_stencil_format) {
			render_target->depth_stencil_attachment->type         = tr_texture_type_2d;
			render_target->depth_stencil_attachment->usage        = tr_texture_usage_sampled_image | tr_texture_usage_depth_stencil_attachment;
			render_target->depth_stencil_attachment->width        = p_renderer->settings.width;
			render_target->depth_stencil_attachment->height       = p_renderer->settings.height;
			render_target->depth_stencil_attachment->depth        = 1;
			render_target->depth_stencil_attachment->format       = p_renderer->settings.swapchain.depth_stencil_format;
			render_target->depth_stencil_attachment->mip_levels   = 1;
			render_target->depth_stencil_attachment->sample_count = render_target->sample_count;
		}
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_init_swapchain_renderpass(tr_renderer* p_renderer)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}

	uint32_t image_count = 0;
	VkResult res = vkGetSwapchainImagesKHR(p_renderer->vk_device, p_renderer->vk_swapchain, &image_count, NULL);
	if (VK_SUCCESS != res) {
		return tr_result_swapchain_failed;
	}

	if (image_count != p_renderer->settings.swapchain.image_count) {
		return tr_result_swapchain_failed;
	}

	VkImage* swapchain_images = (VkImage*)calloc(image_count, sizeof(*swapchain_images));
	if (NULL == swapchain_images) {
		return tr_result_swapchain_failed;
	}
	res = vkGetSwapchainImagesKHR(p_renderer->vk_device, p_renderer->vk_swapchain, &image_count, swapchain_images);
	if (VK_SUCCESS != res) {
		return tr_result_swapchain_failed;
	}

	// Populate the vk_image field and create the Vulkan texture objects
	for (size_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
		tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
		render_target->color_attachments[0]->vk_image = swapchain_images[i];
		tr_result result = tr_internal_vk_create_texture(p_renderer, render_target->color_attachments[0]);
		if (tr_result_ok != result) {
			return tr_result_swapchain_failed;
		}

		if (p_renderer->settings.swapchain.sample_count > tr_sample_count_1) {
			result = tr_internal_vk_create_texture(p_renderer, render_target->color_attachments_multisample[0]);
			if (tr_result_ok != result) {
				return tr_result_swapchain_failed;
			}
		}

		if (NULL != render_target->depth_stencil_attachment) {
			result = tr_internal_vk_create_texture(p_renderer, render_target->depth_stencil_attachment);
			if (tr_result_ok != result) {
				return tr_result_swapchain_failed;
			}
		}
	}

	// Initialize Vulkan render target objects
	for (uint32_t i = 0; i < p_renderer->settings.swapchain.image_count; ++i) {
		tr_render_target* render_target = p_renderer->swapchain_render_targets[i];
		tr_result result = tr_internal_vk_create_render_target(p_renderer, render_target);
		if (tr_result_ok  != result) {
			return tr_result_swapchain_failed;
		}
	}

	return tr_result_ok;
}

// -------------------------------------------------------------------------------------------------
// Internal create functions
// -------------------------------------------------------------------------------------------------
tr_result tr_internal_vk_create_render_pass(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_render_target, tr_result_invalid_renderer, tr_result_bad_ptr);

	uint32_t color_attachment_count = p_render_target->color_attachment_count;
	uint32_t depth_stencil_attachment_count = (tr_format_undefined != p_render_target->depth_stencil_format) ? 1 : 0;

	VkAttachmentDescription* attachments = NULL;
	VkAttachmentReference* color_attachment_refs = NULL;
	VkAttachmentReference* resolve_attachment_refs = NULL;
	VkAttachmentReference* depth_stencil_attachment_ref = NULL;

	// Fill out attachment descriptions and references
	if (p_render_target->sample_count > tr_sample_count_1) {
		attachments = (VkAttachmentDescription*)calloc((2 * color_attachment_count) + depth_stencil_attachment_count, sizeof(*attachments));
		TINY_RENDERER_ALLOC_CHECK(attachments, tr_result_render_target_failed);
		if (color_attachment_count > 0) {
			color_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*color_attachment_refs));
			TINY_RENDERER_ALLOC_CHECK(color_attachment_refs, tr_result_render_target_failed);
			resolve_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*resolve_attachment_refs));
			TINY_RENDERER_ALLOC_CHECK(resolve_attachment_refs, tr_result_render_target_failed);
		}
		if (depth_stencil_attachment_count > 0) {
			depth_stencil_attachment_ref = (VkAttachmentReference*)calloc(1, sizeof(*depth_stencil_attachment_ref));
			TINY_RENDERER_ALLOC_CHECK(depth_stencil_attachment_ref, tr_result_render_target_failed);
		}

		// Color
		for (uint32_t i = 0; i < color_attachment_count; ++i) {
			const uint32_t ssidx = 2*i;
			const uint32_t msidx = ssidx + 1;

			// descriptions
			attachments[ssidx].flags          = 0;
			attachments[ssidx].format         = tr_to_vkformat(p_render_target->color_format);
			attachments[ssidx].samples        = tr_to_vk_sample_count(tr_sample_count_1);
			attachments[ssidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[ssidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[ssidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[ssidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[ssidx].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[ssidx].finalLayout    = (p_render_target->color_attachments[i]->usage & tr_texture_usage_present) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			attachments[msidx].flags          = 0;
			attachments[msidx].format         = tr_to_vkformat(p_render_target->color_format);
			attachments[msidx].samples        = tr_to_vk_sample_count(p_render_target->sample_count);
			attachments[msidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[msidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[msidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[msidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[msidx].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[msidx].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// references
			color_attachment_refs[i].attachment   = msidx;
			color_attachment_refs[i].layout       = attachments[msidx].initialLayout;
			resolve_attachment_refs[i].attachment = ssidx;
			resolve_attachment_refs[i].layout     = attachments[ssidx].initialLayout;
		}

		// Depth stencil
		if (depth_stencil_attachment_count > 0) {
			uint32_t idx =  (2 * color_attachment_count);
			attachments[idx].flags          = 0;
			attachments[idx].format         = tr_to_vkformat(p_render_target->color_format);
			attachments[idx].samples        = tr_to_vk_sample_count(p_render_target->sample_count);
			attachments[idx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[idx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[idx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[idx].initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[idx].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depth_stencil_attachment_ref[0].attachment = idx;
			depth_stencil_attachment_ref[0].layout     = attachments[idx].initialLayout;
		}
	}
	else {
		attachments = (VkAttachmentDescription*)calloc(color_attachment_count + depth_stencil_attachment_count, sizeof(*attachments));
		TINY_RENDERER_ALLOC_CHECK(attachments, tr_result_render_target_failed);
		if (color_attachment_count > 0) {
			color_attachment_refs = (VkAttachmentReference*)calloc(color_attachment_count, sizeof(*color_attachment_refs));
			TINY_RENDERER_ALLOC_CHECK(color_attachment_refs, tr_result_render_target_failed);
		}
		if (depth_stencil_attachment_count > 0) {
			depth_stencil_attachment_ref = (VkAttachmentReference*)calloc(1, sizeof(*depth_stencil_attachment_ref));
			TINY_RENDERER_ALLOC_CHECK(depth_stencil_attachment_ref, tr_result_render_target_failed);
		}

		// Color
		for (uint32_t i = 0; i < color_attachment_count; ++i) {
			const uint32_t ssidx = i;

			// descriptions
			attachments[ssidx].flags          = 0;
			attachments[ssidx].format         = tr_to_vkformat(p_render_target->color_format);
			attachments[ssidx].samples        = tr_to_vk_sample_count(tr_sample_count_1);
			attachments[ssidx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[ssidx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[ssidx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[ssidx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[ssidx].initialLayout  = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachments[ssidx].finalLayout    = (p_render_target->color_attachments[i]->usage & tr_texture_usage_present) ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			// references
			color_attachment_refs[i].attachment = ssidx;
			color_attachment_refs[i].layout     = attachments[ssidx].initialLayout;
		}

		// Depth stencil
		if (depth_stencil_attachment_count > 0) {
			uint32_t idx =  color_attachment_count;
			attachments[idx].flags          = 0;
			attachments[idx].format         = tr_to_vkformat(p_render_target->color_format);
			attachments[idx].samples        = tr_to_vk_sample_count(p_render_target->sample_count);
			attachments[idx].loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[idx].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[idx].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;
			attachments[idx].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
			attachments[idx].initialLayout  = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			attachments[idx].finalLayout    = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			depth_stencil_attachment_ref[0].attachment = idx;
			depth_stencil_attachment_ref[0].layout     = attachments[idx].initialLayout;
		}
	}
			
	VkSubpassDescription subpass = {0};
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
	VkSubpassDependency subpass_dependency = {0};
	subpass_dependency.srcSubpass      = 0;
	subpass_dependency.dstSubpass      = 0;
	subpass_dependency.srcStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.dstStageMask    = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	subpass_dependency.srcAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dstAccessMask   = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	subpass_dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo create_info = {0};
	create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	create_info.pNext           = NULL;
	create_info.flags           = 0;
	create_info.attachmentCount = color_attachment_count + depth_stencil_attachment_count;
	create_info.pAttachments    = attachments;
	create_info.subpassCount    = 1;
	create_info.pSubpasses      = &subpass;
	create_info.dependencyCount = 1;
	create_info.pDependencies   = &subpass_dependency;
	
	VkResult vk_res = vkCreateRenderPass(p_renderer->vk_device, &create_info, NULL, &(p_render_target->vk_render_pass));
	if (VK_SUCCESS != vk_res) {
		// Free first so we don't leak
		TINY_RENDERER_SAFE_FREE(attachments);
		TINY_RENDERER_SAFE_FREE(color_attachment_refs);
		TINY_RENDERER_SAFE_FREE(resolve_attachment_refs);
		TINY_RENDERER_SAFE_FREE(depth_stencil_attachment_ref);
		return tr_result_render_target_failed;
	}
	
	TINY_RENDERER_SAFE_FREE(attachments);
	TINY_RENDERER_SAFE_FREE(color_attachment_refs);
	TINY_RENDERER_SAFE_FREE(resolve_attachment_refs);
	TINY_RENDERER_SAFE_FREE(depth_stencil_attachment_ref);
	return tr_result_ok;
}

tr_result tr_internal_vk_create_framebuffer(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}
	if (NULL == p_render_target) {
		return tr_result_bad_ptr;
	}

	uint32_t attachment_count = p_render_target->color_attachment_count;
	if (p_render_target->sample_count > tr_sample_count_1) {
		attachment_count *= 2;
	}
	if (tr_format_undefined != p_render_target->depth_stencil_format) {
		attachment_count += 1;
	}

	VkImageView* attachments = (VkImageView*)calloc(attachment_count, sizeof(*attachments));
	TINY_RENDERER_ALLOC_CHECK(attachments, tr_result_render_target_failed);

	VkImageView* iter_attachments = attachments;
	// Color
	for (uint32_t i = 0; i < p_render_target->color_attachment_count; ++i) {
		*iter_attachments = p_render_target->color_attachments[i]->vk_image_view;
		++iter_attachments;
		if (p_render_target->sample_count > tr_sample_count_1) {
			*iter_attachments = p_render_target->color_attachments_multisample[i]->vk_image_view;
			++iter_attachments;
		}
	}
	// Depth/stencil
	if (tr_format_undefined != p_render_target->depth_stencil_format) {
		*iter_attachments = p_render_target->depth_stencil_attachment->vk_image_view;
		++iter_attachments;
	}

	VkFramebufferCreateInfo create_info = {0};
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
	if (VK_SUCCESS != vk_res) {
		TINY_RENDERER_SAFE_FREE(attachments);
		return tr_result_render_target_failed;
	}

	TINY_RENDERER_SAFE_FREE(attachments);
	return tr_result_ok;
}

tr_result tr_internal_vk_create_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
	tr_result result = tr_internal_vk_create_render_pass(p_renderer, p_render_target);
	if (tr_result_ok != result) {
		return tr_result_render_target_failed;
	}

	result = tr_internal_vk_create_framebuffer(p_renderer, p_render_target);
	if (tr_result_ok != result) {
		return tr_result_render_target_failed;
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_destroy_render_target(tr_renderer* p_renderer, tr_render_target* p_render_target)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}
	if (NULL == p_render_target) {
		return tr_result_bad_ptr;
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_create_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
	if (s_tr_internal->renderer != p_renderer) {
		return tr_result_invalid_renderer;
	}
	if (NULL == p_texture) {
		return tr_result_bad_ptr;
	}

	p_texture->renderer = p_renderer;

	if (VK_NULL_HANDLE == p_texture->vk_image) {
		VkImageType image_type = VK_IMAGE_TYPE_2D;
		switch (p_texture->type) {
			case tr_texture_type_1d   : image_type = VK_IMAGE_TYPE_1D;   break;
			case tr_texture_type_2d   : image_type = VK_IMAGE_TYPE_2D;   break;
			case tr_texture_type_3d   : image_type = VK_IMAGE_TYPE_3D;   break;
			case tr_texture_type_cube : image_type = VK_IMAGE_TYPE_2D; break;
		}

		VkImageCreateInfo create_info = {0};
		create_info.sType                 = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		create_info.pNext                 = NULL;
		create_info.flags                 = 0;
		create_info.imageType             = image_type;
		create_info.format                = tr_to_vkformat(p_texture->format);
		create_info.extent.width          = p_texture->width;
		create_info.extent.height         = p_texture->height;
		create_info.mipLevels             = (0 != p_texture->host_visible) ? 1 : 1;
		create_info.arrayLayers           = (0 != p_texture->host_visible) ? 1 : 1;
		create_info.samples               = tr_to_vk_sample_count(p_texture->sample_count);
		create_info.tiling                = (0 != p_texture->host_visible) ? VK_IMAGE_TILING_LINEAR : VK_IMAGE_TILING_OPTIMAL;
		create_info.usage                 = tr_to_vk_image_usage(p_texture->usage);
		create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
		create_info.queueFamilyIndexCount = 0;
		create_info.pQueueFamilyIndices   = NULL;
		create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;
		VkResult res = vkCreateImage(p_renderer->vk_device, &create_info, NULL, &(p_texture->vk_image));
		if (VK_SUCCESS != res) {
			return tr_result_texture_failed;
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

		VkImageViewCreateInfo create_info = {0};
		create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		create_info.pNext                           = NULL;
		create_info.flags                           = 0;
		create_info.image                           = p_texture->vk_image;
		create_info.viewType                        = view_type;
		create_info.format                          = tr_to_vkformat(p_texture->format);
		create_info.components.r                    = VK_COMPONENT_SWIZZLE_R;
		create_info.components.g                    = VK_COMPONENT_SWIZZLE_G;
		create_info.components.b                    = VK_COMPONENT_SWIZZLE_B;
		create_info.components.a                    = VK_COMPONENT_SWIZZLE_A;
		create_info.subresourceRange.aspectMask     = tr_vk_determine_aspect_mask(tr_to_vkformat(p_texture->format));
		create_info.subresourceRange.baseMipLevel   = 0;
		create_info.subresourceRange.levelCount     = 1;
		create_info.subresourceRange.baseArrayLayer = 0;
		create_info.subresourceRange.layerCount     = 1;
		VkResult res = vkCreateImageView(p_renderer->vk_device, &create_info, NULL, &(p_texture->vk_image_view));
		if (VK_SUCCESS != res) {
			return tr_result_texture_failed;
		}
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_destroy_texture(tr_renderer* p_renderer, tr_texture* p_texture)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_texture, tr_result_invalid_renderer, tr_result_bad_ptr);

	if ((VK_NULL_HANDLE != p_texture->vk_image) && (p_texture->owns_image)) {
		vkDestroyImage(p_renderer->vk_device, p_texture->vk_image, NULL);
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_create_cmd(tr_renderer* p_renderer, uint32_t queue_family_index, bool secondary, tr_cmd* p_cmd)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_cmd, tr_result_invalid_renderer, tr_result_bad_ptr);

	if ((queue_family_index != p_renderer->graphics_queue->vk_queue_family_index) ||
		(queue_family_index != p_renderer->present_queue->vk_queue_family_index))
	{
		return tr_result_cmd_failed;
	}

	VkCommandPoolCreateInfo create_info = {0};
	create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	create_info.pNext            = NULL;
	create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	create_info.queueFamilyIndex = queue_family_index;
	VkResult vk_res = vkCreateCommandPool(p_renderer->vk_device, &create_info, NULL, &(p_cmd->vk_cmd_pool));
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_cmd_failed);

	VkCommandBufferAllocateInfo alloc_info = {0};
	alloc_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.pNext              = NULL;
	alloc_info.commandPool        = p_cmd->vk_cmd_pool;
	alloc_info.level              = secondary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	alloc_info.commandBufferCount = 1;
	vk_res = vkAllocateCommandBuffers(p_renderer->vk_device, &alloc_info, &(p_cmd->vk_cmd_buf));
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_cmd_failed);

	return tr_result_ok;
}

tr_result tr_internal_vk_destroy_cmd(tr_renderer* p_renderer, tr_cmd* p_cmd)
{
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_cmd, tr_result_invalid_renderer, tr_result_bad_ptr);

	if (VK_NULL_HANDLE != p_cmd->vk_cmd_pool) {
		if (VK_NULL_HANDLE != p_cmd->vk_cmd_buf) {
			vkFreeCommandBuffers(p_renderer->vk_device, p_cmd->vk_cmd_pool, 1, &(p_cmd->vk_cmd_buf));
		}

		vkDestroyCommandPool(p_renderer->vk_device, p_cmd->vk_cmd_pool, NULL);
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_create_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{	
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_fence, tr_result_invalid_renderer, tr_result_bad_ptr);

	VkFenceCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	VkResult vk_res = vkCreateFence(p_renderer->vk_device,  &create_info, NULL, &(p_fence->vk_fence));
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_renderer_failed);

	return tr_result_ok;
}

tr_result tr_internal_vk_destroy_fence(tr_renderer *p_renderer, tr_fence* p_fence)
{	
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_fence, tr_result_invalid_renderer, tr_result_bad_ptr);

	if (VK_NULL_HANDLE != p_fence->vk_fence) {
		vkDestroyFence(p_renderer->vk_device, p_fence->vk_fence, NULL);
	}

	return tr_result_ok;
}

tr_result tr_internal_vk_create_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{	
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_semaphore, tr_result_invalid_renderer, tr_result_bad_ptr);

	VkSemaphoreCreateInfo create_info = {0};
	create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	create_info.pNext = NULL;
	create_info.flags = 0;
	VkResult vk_res = vkCreateSemaphore(p_renderer->vk_device,  &create_info, NULL, &(p_semaphore->vk_semaphore));
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_semaphore_failed);

	return tr_result_ok;
}

tr_result tr_internal_vk_destroy_semaphore(tr_renderer *p_renderer, tr_semaphore* p_semaphore)
{	
	TINY_RENDERER_RENDERER_PTR_OBJECT_PTR_CHECK(p_renderer, p_semaphore, tr_result_invalid_renderer, tr_result_bad_ptr);

	if (VK_NULL_HANDLE != p_semaphore->vk_semaphore) {
		vkDestroySemaphore(p_renderer->vk_device, p_semaphore->vk_semaphore, NULL);
	}
	
	return tr_result_ok;
}

// -------------------------------------------------------------------------------------------------
// Internal command buffer functions
// -------------------------------------------------------------------------------------------------
tr_result tr_internal_vk_begin_cmd(tr_cmd* p_cmd)
{
	if (NULL == p_cmd) {
		return tr_result_cmd_failed;
	}

	VkCommandBufferBeginInfo begin_info = {0};
	begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;;
	begin_info.pNext            = NULL;
	begin_info.flags            = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	begin_info.pInheritanceInfo = NULL;
	VkResult vk_res = vkBeginCommandBuffer(p_cmd->vk_cmd_buf, &begin_info);
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_cmd_failed);

	return tr_result_ok;
}

void tr_internal_vk_cmd_begin_render(tr_cmd* p_cmd, tr_render_target* p_render_target)
{
	TINY_RENDERER_ASSERT(NULL != p_cmd);
	TINY_RENDERER_ASSERT(NULL != p_render_target);

	VkRect2D render_area = {0};
	render_area.offset.x = 0;
	render_area.offset.y = 0;
	render_area.extent.width = p_render_target->width;
	render_area.extent.height = p_render_target->height;

	VkClearValue clear_values[tr_max_render_target_attachments] = {0};
	
	uint32_t clear_value_count = p_render_target->color_attachment_count;
	for (uint32_t i = 0; i < clear_value_count; ++i) {
		clear_values[i].color.float32[0] = p_render_target->color_clear_values[i].r;
		clear_values[i].color.float32[1] = p_render_target->color_clear_values[i].g;
		clear_values[i].color.float32[2] = p_render_target->color_clear_values[i].b;
		clear_values[i].color.float32[3] = p_render_target->color_clear_values[i].a;
	}

	VkRenderPassBeginInfo begin_info = {0};
	begin_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	begin_info.pNext           = NULL;
	begin_info.renderPass      = p_render_target->vk_render_pass;
	begin_info.framebuffer     = p_render_target->vk_framebuffer;
	begin_info.renderArea      = render_area;
	begin_info.clearValueCount = clear_value_count;
	begin_info.pClearValues    = clear_values;

	vkCmdBeginRenderPass(p_cmd->vk_cmd_buf, &begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

// -------------------------------------------------------------------------------------------------
// Internal queue functions
// -------------------------------------------------------------------------------------------------
tr_result tr_internal_vk_acquire_next_image(tr_renderer* p_renderer, tr_semaphore* p_signal_semaphore, tr_fence* p_fence)
{
	TINY_RENDERER_RENDERER_PTR_CHECK(p_renderer, tr_result_invalid_renderer);

	VkSemaphore semaphore = p_signal_semaphore->vk_semaphore;
	VkFence fence = p_fence->vk_fence;

	VkResult vk_res = vkAcquireNextImageKHR(p_renderer->vk_device, 
                                            p_renderer->vk_swapchain, 
                                            UINT64_MAX, 
                                            semaphore, 
                                            fence, 
                                            &(p_renderer->swapchain_image_index));
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_swapchain_failed);

	if (VK_SUCCESS == vk_res) {
		vk_res = vkWaitForFences(p_renderer->vk_device, 1, &fence, VK_TRUE, UINT64_MAX);
		if (VK_SUCCESS == vk_res) {
			vkResetFences(p_renderer->vk_device, 1, &fence);
		}
	}


	return tr_result_ok;
}

tr_result tr_internal_vk_queue_submit(
	tr_queue*      p_queue, 
	uint32_t       cmd_count,
	tr_cmd**       pp_cmds,
	uint32_t       wait_semaphore_count,
	tr_semaphore** pp_wait_semaphores,
	uint32_t       signal_semaphore_count,
	tr_semaphore** pp_signal_semaphores
)
{
	TINY_RENDERER_OBJECT_PTR_CHECK(p_queue, tr_result_bad_ptr);
	TINY_RENDERER_RENDERER_PTR_CHECK(p_queue->renderer, tr_result_invalid_renderer);

	VkCommandBuffer cmds[tr_max_submit_cmds] = {0};
	VkSemaphore wait_semaphores[tr_max_submit_wait_semaphores] = {0};
	VkPipelineStageFlags wait_masks[tr_max_submit_wait_semaphores] = {0};
	VkSemaphore signal_semaphores[tr_max_submit_signal_semaphores] = {0};


	cmd_count = cmd_count > tr_max_submit_cmds ? tr_max_submit_cmds : cmd_count;
	for (uint32_t i = 0; i < cmd_count; ++i) {
		cmds[i] = pp_cmds[i]->vk_cmd_buf;
	}

	wait_semaphore_count = wait_semaphore_count > tr_max_submit_wait_semaphores ? tr_max_submit_wait_semaphores : wait_semaphore_count;
	for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
		wait_semaphores[i] = pp_wait_semaphores[i]->vk_semaphore;
		wait_masks[i] = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
	}

	signal_semaphore_count = signal_semaphore_count > tr_max_submit_signal_semaphores ? tr_max_submit_signal_semaphores : signal_semaphore_count;
	for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
		signal_semaphores[i] = pp_signal_semaphores[i]->vk_semaphore;
	}

	VkSubmitInfo submit_info = {0}; 
	submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pNext                = NULL;
	submit_info.waitSemaphoreCount   = wait_semaphore_count;
	submit_info.pWaitSemaphores      = wait_semaphores;
	submit_info.pWaitDstStageMask    = wait_masks;
	submit_info.commandBufferCount   = cmd_count;
	submit_info.pCommandBuffers      = cmds;
	submit_info.signalSemaphoreCount = signal_semaphore_count;
	submit_info.pSignalSemaphores    = signal_semaphores;

	return tr_result_ok;
}

tr_result tr_internal_vk_queue_present(tr_queue* p_queue, uint32_t wait_semaphore_count, tr_semaphore** pp_wait_semaphores)
{
	TINY_RENDERER_OBJECT_PTR_CHECK(p_queue, tr_result_bad_ptr);
	TINY_RENDERER_RENDERER_PTR_CHECK(p_queue->renderer, tr_result_invalid_renderer);

	tr_renderer* renderer = p_queue->renderer;

	VkSemaphore wait_semaphores[tr_max_present_wait_semaphores] = {0};
	wait_semaphore_count = wait_semaphore_count > tr_max_present_wait_semaphores ? wait_semaphore_count : wait_semaphore_count;
	for (uint32_t i = 0; i < wait_semaphore_count; ++i) {
		wait_semaphores[i] = pp_wait_semaphores[i]->vk_semaphore;
	}

	VkPresentInfoKHR present_info = {0};
	present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present_info.pNext              = NULL;
	present_info.waitSemaphoreCount = wait_semaphore_count;
	present_info.pWaitSemaphores    = wait_semaphores;
	present_info.swapchainCount     = 1;
	present_info.pSwapchains        = &(renderer->vk_swapchain);
	present_info.pImageIndices      = &(renderer->swapchain_image_index);
	present_info.pResults           = NULL;

	VkResult vk_res = vkQueuePresentKHR(s_tr_internal->renderer->present_queue->vk_queue, &present_info);
	TINY_RENDERER_VK_RES_CHECK(vk_res, tr_result_queue_failed);

	return tr_result_ok;
}

#endif // TINY_RENDERER_IMPLEMENTATION

#if defined(__cplusplus) && defined(TINY_RENDERER_CPP_NAMESPACE)
} // namespace TINY_RENDERER_CPP_NAMESPACE
#endif