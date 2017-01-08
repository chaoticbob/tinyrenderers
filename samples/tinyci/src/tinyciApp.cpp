#include "cinder/app/App.h"
#include "cinder/Log.h"
using namespace ci;
using namespace ci::app;
using namespace std;

#define TINY_RENDERER_IMPLEMENTATION

#include "tinyci.h"
#if defined(TINY_RENDERER_DX)
	#include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
	#include "tinyvk.h"
#endif

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

const uint32_t kImageCount = 3;

class TinyCiApp : public App {
public:
	void setup() override;
	void cleanup() override;
	void mouseDown( MouseEvent event ) override;
	void update() override;
	void draw() override;

private:
	tr_renderer*		m_renderer = nullptr;
	tr_descriptor_set*	m_desc_set = nullptr;
	tr_cmd_pool*		m_cmd_pool;
	tr_cmd**			m_cmds;
	tr_shader_program*	m_shader;
	tr_buffer*			m_vertex_buffer;
	tr_pipeline*		m_pipeline;
};

void renderer_log(tr_log_type type, const char* msg, const char* component)
{
	switch(type) {
		case tr_log_type_info  : {CI_LOG_I("[" << component << "] : " << msg);} break;
		case tr_log_type_warn  : {CI_LOG_W("[" << component << "] : " << msg);} break;
		case tr_log_type_debug : {CI_LOG_E("[" << component << "] : " << msg);} break;
		case tr_log_type_error : {CI_LOG_D("[" << component << "] : " << msg);} break;
	}
}

#if defined(TINY_RENDERER_VK)
VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug(
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
	if( flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
		//CI_LOG_W( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
		//CI_LOG_I( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
		CI_LOG_E( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
		CI_LOG_D( "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")" );
	}
	return VK_FALSE;
}
#endif

void TinyCiApp::setup()
{
	std::vector<const char*> instance_layers = {
		"VK_LAYER_LUNARG_api_dump",
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_swapchain",
		"VK_LAYER_LUNARG_image",
		"VK_LAYER_LUNARG_parameter_validation"
	};

	std::vector<const char*> device_layers = {
		"VK_LAYER_LUNARG_api_dump",
		"VK_LAYER_LUNARG_core_validation",
		"VK_LAYER_LUNARG_swapchain",
		"VK_LAYER_LUNARG_image",
		"VK_LAYER_LUNARG_parameter_validation"
	};

	tr_renderer_settings settings = {0};
	settings.handle.hinstance               = static_cast<TinyRenderer*>(getRenderer().get())->getHinstance();
	settings.handle.hwnd                    = static_cast<TinyRenderer*>(getRenderer().get())->getHwnd();
	settings.width                          = getWindowWidth();
	settings.height                         = getWindowHeight();
	settings.swapchain.image_count          = kImageCount;
	settings.swapchain.sample_count         = tr_sample_count_1;
	settings.swapchain.color_format         = tr_format_b8g8r8a8_unorm;
	settings.swapchain.depth_stencil_format = tr_format_d16_unorm;
	settings.log_fn							= renderer_log;
#if defined(TINY_RENDERER_VK)
	settings.vk_debug_fn                    = vulkan_debug;
	settings.instance_layers.count			= static_cast<uint32_t>(instance_layers.size());
	settings.instance_layers.names			= instance_layers.empty() ? nullptr : instance_layers.data();
	settings.device_layers.count			= static_cast<uint32_t>(device_layers.size());
	settings.device_layers.names			= device_layers.data();
#endif
	tr_create_renderer("myappTinyCiApp", &settings, &m_renderer);


	std::vector<tr_descriptor> descriptors(3);
	descriptors[0].type  = tr_descriptor_type_image;
	descriptors[0].count = 4;
	descriptors[0].binding = 0;
	descriptors[1].type  = tr_descriptor_type_sampler;
	descriptors[1].count = 4;
	descriptors[1].binding = 1;
	descriptors[2].type  = tr_descriptor_type_uniform_buffer;
	descriptors[2].count = 4;
	descriptors[2].binding = 2;
	tr_create_descriptor_set(m_renderer, descriptors.size(), descriptors.data(), &m_desc_set);

	tr_create_cmd_pool(m_renderer, m_renderer->graphics_queue, false, &m_cmd_pool);
	tr_create_cmd_n(m_cmd_pool, false, kImageCount, &m_cmds);
	
#if defined(TINY_RENDERER_VK)
	auto vert = loadFile(getAssetPath("basic_vert.spv"))->getBuffer();
	auto frag = loadFile(getAssetPath("basic_frag.spv"))->getBuffer();
	tr_create_shader_program(m_renderer, 
		                     vert->getSize(), (uint32_t*)(vert->getData()), "main", 
		                     frag->getSize(), (uint32_t*)(frag->getData()), "main", &m_shader);
#elif defined(TINY_RENDERER_DX)
	auto hlsl = loadFile(getAssetPath("basic.hlsl"))->getBuffer();	
	tr_create_shader_program(m_renderer, 
                             hlsl->getSize(), (uint32_t*)(hlsl->getData()), "VSMain", 
		                     hlsl->getSize(), (uint32_t*)(hlsl->getData()), "PSMain", &m_shader);
#endif


	tr_vertex_layout vertex_layout = {};
	vertex_layout.attrib_count = 2;
	vertex_layout.attribs[0].semantic = tr_semantic_position;
	vertex_layout.attribs[0].format   = tr_format_r32g32b32a32_float;
	vertex_layout.attribs[0].binding  = 0;
	vertex_layout.attribs[0].location = 0;
	vertex_layout.attribs[0].offset   = 0;
	vertex_layout.attribs[1].semantic = tr_semantic_texcoord0;
	vertex_layout.attribs[1].format   = tr_format_r32g32_float;
	vertex_layout.attribs[1].binding  = 0;
	vertex_layout.attribs[1].location = 1;
	vertex_layout.attribs[1].offset   = tr_util_format_stride(tr_format_r32g32b32a32_float);
	tr_pipeline_settings pipeline_settings = {tr_primitive_topo_tri_strip};
	tr_create_pipeline(m_renderer, m_shader, &vertex_layout, m_desc_set, m_renderer->swapchain_render_targets[0], &pipeline_settings, &m_pipeline);
	tr_create_pipeline(m_renderer, m_shader, &vertex_layout, NULL, m_renderer->swapchain_render_targets[0], &pipeline_settings, &m_pipeline);

	std::vector<float> data = {
		-0.5f, -0.5f, 0.0f,	1.0f, 0.0f, 0.0f,
		-0.5f,  0.5f, 0.0f,	1.0f, 0.0f, 1.0f,
		 0.5f, -0.5f, 0.0f,	1.0f, 1.0f, 0.0f,
		 0.5f,  0.5f, 0.0f,	1.0f, 1.0f, 1.0f,
	};

	uint64_t dataSize = sizeof(float) * data.size();
	uint64_t strideSize = sizeof(float) * 6;
	tr_create_vertex_buffer(m_renderer, dataSize, true, strideSize, &m_vertex_buffer);
	memcpy(m_vertex_buffer->cpu_mapped_address, data.data(), dataSize);
}

void TinyCiApp::cleanup()
{
	if (nullptr != m_renderer) {
		//tr_destroy_pipeline(m_renderer, m_pipeline);
		//tr_destroy_descriptor_set(m_renderer, m_desc_set);
		//tr_destroy_buffer(m_renderer, m_vertex_buffer);	
		tr_destroy_renderer(m_renderer);
	}
}

void TinyCiApp::mouseDown( MouseEvent event )
{
}

void TinyCiApp::update()
{
}

void TinyCiApp::draw()
{
	uint32_t frameIdx = getElapsedFrames() % m_renderer->settings.swapchain.image_count;

	tr_fence* image_acquired_fence = m_renderer->image_acquired_fences[frameIdx];
	tr_semaphore* image_acquired_semaphore = m_renderer->image_acquired_semaphores[frameIdx];
	tr_semaphore* render_complete_semaphores = m_renderer->render_complete_semaphores[frameIdx];

	tr_acquire_next_image(m_renderer, image_acquired_semaphore, image_acquired_fence);

	float t = static_cast<float>(getElapsedSeconds());
	float t0 = fmod(t, 1.0f);

	uint32_t swapchain_image_index = m_renderer->swapchain_image_index;
	tr_render_target* render_target = m_renderer->swapchain_render_targets[swapchain_image_index];
	tr_render_target_set_color_clear_value(render_target, 0, 0.0f, 0.0f, 0.0f, 0.0f );

	tr_cmd* cmd = m_cmds[frameIdx];

	tr_begin_cmd(cmd);
	tr_cmd_image_transition(cmd, render_target->color_attachments[0], tr_texture_usage_present, tr_texture_usage_color_attachment);
	tr_cmd_set_viewport(cmd, 0, 0, getWindowWidth(), getWindowHeight(), 0.0f, 1.0f);
	tr_cmd_set_scissor(cmd, 0, 0, getWindowWidth(), getWindowHeight());
	tr_cmd_begin_render(cmd, render_target);
	tr_clear_value clear_value = {1.0f, 0.0f, 0.0f, 1.0f};
	tr_cmd_clear_color_attachment(cmd, 0, &clear_value);
	tr_cmd_bind_pipeline(cmd, m_pipeline);
	tr_cmd_bind_vertex_buffers(cmd, 1, &m_vertex_buffer);
	tr_cmd_bind_descriptor_sets(cmd, m_pipeline, 1, &m_desc_set);
	tr_cmd_draw(cmd, 4, 0);
	tr_cmd_end_render(cmd);
	tr_cmd_image_transition(cmd, render_target->color_attachments[0], tr_texture_usage_color_attachment, tr_texture_usage_present);
	tr_end_cmd(cmd);

	tr_queue_submit(m_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1, &render_complete_semaphores);
	tr_queue_present(m_renderer->present_queue, 1, &render_complete_semaphores);

	tr_queue_wait_idle(m_renderer->graphics_queue);
}

CINDER_APP(TinyCiApp, TinyRenderer)
