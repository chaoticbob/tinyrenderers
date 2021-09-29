#include "GLFW/glfw3.h"
#if defined(__linux__)
  #define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3native.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <vector>

#define TINY_RENDERER_IMPLEMENTATION
#if defined(TINY_RENDERER_DX)
    #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
    #include "tinyvk.h"
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

const char*         k_app_name = "14_ComputeBloom";
const uint32_t      k_image_count = 3;
#if defined(__linux__)
const std::string   k_asset_dir = "../samples/assets/";
#elif defined(_WIN32)
const std::string   k_asset_dir = "../../samples/assets/";
#endif

#define NUM_THREADS_X  1024
#define NUM_THREADS_Y  1
#define NUM_THREADS_Z  1

tr_renderer*        g_renderer = nullptr;
tr_descriptor_set*  g_desc_set = nullptr;
tr_descriptor_set*  g_compute_desc_set_hblur = nullptr;
tr_descriptor_set*  g_compute_desc_set_vblur = nullptr;
tr_cmd_pool*        g_cmd_pool = nullptr;
tr_cmd**            g_cmds = nullptr;
tr_shader_program*  g_compute_shader_hblur = nullptr;
tr_shader_program*  g_compute_shader_vblur = nullptr;
tr_shader_program*  g_texture_shader = nullptr;
tr_buffer*          g_rect_index_buffer = nullptr;
tr_buffer*          g_rect_vertex_buffer = nullptr;
tr_pipeline*        g_pipeline = nullptr;
tr_pipeline*        g_compute_pipeline_hblur = nullptr;
tr_pipeline*        g_compute_pipeline_vblur = nullptr;
tr_texture*         g_texture = nullptr;
tr_texture*         g_texture_compute_output_hblur = nullptr;
tr_texture*         g_texture_compute_output_vblur = nullptr;
tr_sampler*         g_sampler = nullptr;

uint32_t            g_window_width;
uint32_t            g_window_height;
uint64_t            g_frame_count = 0;

#define LOG(STR)  { std::stringstream ss; ss << STR << std::endl; \
                    platform_log(ss.str().c_str()); }

static void platform_log(const char* s)
{
#if defined(_WIN32)
  OutputDebugStringA(s);
#else
  printf("%s", s);
#endif
}

static void app_glfw_error(int error, const char* description)
{
  LOG("Error " << error << ":" << description);
}

void renderer_log(tr_log_type type, const char* msg, const char* component)
{
  switch(type) {
    case tr_log_type_info  : {LOG("[INFO]"  << "[" << component << "] : " << msg);} break;
    case tr_log_type_warn  : {LOG("[WARN]"  << "[" << component << "] : " << msg);} break;
    case tr_log_type_debug : {LOG("[DEBUG]" << "[" << component << "] : " << msg);} break;
    case tr_log_type_error : {LOG("[ERORR]" << "[" << component << "] : " << msg);} break;
    default: break;
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
        //LOG("[INFO]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")");
    }
    else if( flags & VK_DEBUG_REPORT_WARNING_BIT_EXT ) {
        //LOG("[WARN]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")");
    }
    else if( flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT ) {
        //LOG("[PERF]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")");
    }
    else if( flags & VK_DEBUG_REPORT_ERROR_BIT_EXT ) {
        LOG("[ERROR]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")"); 
    }
    else if( flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT ) {
        LOG("[DEBUG]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")");
    }
    return VK_FALSE;
}
#endif

std::vector<uint8_t> load_file(const std::string& path)
{
    std::ifstream is;
    is.open(path.c_str(), std::ios::in | std::ios::binary);
    assert(is.is_open());

    is.seekg(0, std::ios::end);
    std::vector<uint8_t> buffer(is.tellg());
    assert(0 != buffer.size());

    is.seekg(0, std::ios::beg);
    is.read((char*)buffer.data(), buffer.size());

    return buffer;
}

void init_tiny_renderer(GLFWwindow* window)
{
  // Renderer
  {
    std::vector<const char*> instance_layers = {
#if defined(_DEBUG) && defined(TINY_RENDERER_VK)
        VK_KHR_KHRONOS_VALIDATION_LAYER_NAME,
#endif
    };

    std::vector<const char*> device_layers;

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    g_window_width = (uint32_t)width;
    g_window_height = (uint32_t)height;

    tr_renderer_settings settings = {0};
#if defined(__linux__)
    settings.handle.connection              = XGetXCBConnection(glfwGetX11Display());
    settings.handle.window                  = glfwGetX11Window(window);
#elif defined(_WIN32)
    settings.handle.hinstance               = ::GetModuleHandle(NULL);
    settings.handle.hwnd                    = glfwGetWin32Window(window);
#endif
    settings.width                          = g_window_width;
    settings.height                         = g_window_height;
    settings.swapchain.image_count          = k_image_count;
    settings.swapchain.sample_count         = tr_sample_count_8;
    settings.swapchain.color_format         = tr_format_b8g8r8a8_unorm;
    settings.swapchain.depth_stencil_format = tr_format_undefined;
    settings.log_fn                         = renderer_log;
#if defined(TINY_RENDERER_VK)
    settings.vk_debug_fn                    = vulkan_debug;
    settings.instance_layers.count          = (uint32_t)instance_layers.size();
    settings.instance_layers.names          = instance_layers.empty() ? nullptr : instance_layers.data();
#endif
    tr_create_renderer(k_app_name, &settings, &g_renderer);
  }

  // Command buffer
  {
    tr_create_cmd_pool(g_renderer, g_renderer->graphics_queue, false, &g_cmd_pool);
    tr_create_cmd_n(g_cmd_pool, false, k_image_count, &g_cmds);
  }
  
  // Shaders
  {
#if defined(TINY_RENDERER_VK)
    // Uses HLSL source
    auto comp = load_file(k_asset_dir + "compute_blur.hblur_main.cs.spv");
    tr_create_shader_program_compute(g_renderer, 
                                     (uint32_t)comp.size(), comp.data(), "hblur_main", &g_compute_shader_hblur);
    comp = load_file(k_asset_dir + "compute_blur.vblur_main.cs.spv");
    tr_create_shader_program_compute(g_renderer, 
                                     (uint32_t)comp.size(), comp.data(), "vblur_main", &g_compute_shader_vblur);

    auto vert = load_file(k_asset_dir + "texture.vs.spv");
    auto frag = load_file(k_asset_dir + "texture.ps.spv");
    tr_create_shader_program(g_renderer, 
                             (uint32_t)vert.size(), (uint32_t*)(vert.data()), "VSMain", 
                             (uint32_t)frag.size(), (uint32_t*)(frag.data()), "PSMain", &g_texture_shader);
#elif defined(TINY_RENDERER_DX)
    auto hlsl = load_file(k_asset_dir + "compute_blur.hlsl");
    tr_create_shader_program_compute(g_renderer, 
                                     (uint32_t)hlsl.size(), hlsl.data(), "hblur_main", &g_compute_shader_hblur);
    tr_create_shader_program_compute(g_renderer, 
                                     (uint32_t)hlsl.size(), hlsl.data(), "vblur_main", &g_compute_shader_vblur);

    hlsl = load_file(k_asset_dir + "texture.hlsl");
    tr_create_shader_program(g_renderer, 
                             (uint32_t)hlsl.size(), hlsl.data(), "VSMain", 
                             (uint32_t)hlsl.size(), hlsl.data(), "PSMain", &g_texture_shader);
#endif
  }

  // Descriptors
  {
    std::vector<tr_descriptor> descriptors(2);
    descriptors[0].type          = tr_descriptor_type_texture_srv;
    descriptors[0].count         = 1;
    descriptors[0].binding       = 0;
    descriptors[0].shader_stages = tr_shader_stage_frag;
    descriptors[1].type          = tr_descriptor_type_sampler;
    descriptors[1].count         = 1;
    descriptors[1].binding       = 1;
    descriptors[1].shader_stages = tr_shader_stage_frag;
    tr_create_descriptor_set(g_renderer, (uint32_t)descriptors.size(), descriptors.data(), &g_desc_set);

    //
    // In D3D12, the first descriptor can also just be a texture 
    // to use Texture2D, but Vulkan requires that texture reads
    // in compute be done through imageLoad - which requires 
    // a storage image. So we just match it in D3D12.
    //
    descriptors[0].type          = tr_descriptor_type_texture_srv;
    descriptors[0].count         = 1;
    descriptors[0].binding       = 0;
    descriptors[0].shader_stages = tr_shader_stage_comp;
    descriptors[1].type          = tr_descriptor_type_texture_uav;
    descriptors[1].count         = 1;
    descriptors[1].binding       = 1;
    descriptors[1].shader_stages = tr_shader_stage_comp;
    tr_create_descriptor_set(g_renderer, (uint32_t)descriptors.size(), descriptors.data(), &g_compute_desc_set_hblur);
    tr_create_descriptor_set(g_renderer, (uint32_t)descriptors.size(), descriptors.data(), &g_compute_desc_set_vblur);
  }

  // Geometry
  {
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
    tr_pipeline_settings pipeline_settings = {tr_primitive_topo_tri_list};
    tr_create_pipeline(g_renderer, g_texture_shader, &vertex_layout, g_desc_set, g_renderer->swapchain_render_targets[0], &pipeline_settings, &g_pipeline);

    pipeline_settings = {};
    tr_create_compute_pipeline(g_renderer, g_compute_shader_hblur, g_compute_desc_set_hblur, &pipeline_settings, &g_compute_pipeline_hblur);
    tr_create_compute_pipeline(g_renderer, g_compute_shader_vblur, g_compute_desc_set_vblur, &pipeline_settings, &g_compute_pipeline_vblur);

    std::vector<float> vertexData = {
        -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f,
    };

    uint64_t vertexDataSize = sizeof(float) * vertexData.size();
    uint32_t vertexStride = sizeof(float) * 6;
    tr_create_vertex_buffer(g_renderer, vertexDataSize, true, vertexStride, &g_rect_vertex_buffer);
    memcpy(g_rect_vertex_buffer->cpu_mapped_address, vertexData.data(), vertexDataSize);
        
    std::vector<uint16_t> indexData = {
        0, 1, 2,
        0, 2, 3
    };
        
    uint64_t indexDataSize = sizeof(uint16_t) * indexData.size();
    tr_create_index_buffer(g_renderer, indexDataSize, true, tr_index_type_uint16, &g_rect_index_buffer);
    memcpy(g_rect_index_buffer->cpu_mapped_address, indexData.data(), indexDataSize);
  }

  // Textures
  {
    int image_width = 0;
    int image_height = 0;
    int image_channels = 0;
    int required_channels = 4;
    unsigned char* image_data = stbi_load((k_asset_dir + "forced_persp.jpg").c_str(), &image_width, &image_height, &image_channels, required_channels);
    assert(NULL != image_data);
    int image_row_stride = image_width * required_channels;
    tr_create_texture_2d(g_renderer, image_width, image_height, tr_sample_count_1, tr_format_r8g8b8a8_unorm, 1, NULL, false, tr_texture_usage_sampled_image, &g_texture);
    tr_util_update_texture_uint8(g_renderer->graphics_queue, image_width, image_height, image_row_stride, image_data, required_channels, g_texture, NULL, NULL);
    stbi_image_free(image_data);

    // hblur
    tr_create_texture_2d(g_renderer, image_width, image_height, tr_sample_count_1, tr_format_r8g8b8a8_unorm, 1, NULL, false, tr_texture_usage_sampled_image | tr_texture_usage_storage_image, &g_texture_compute_output_hblur); 
    tr_util_transition_image(g_renderer->graphics_queue, g_texture_compute_output_hblur, tr_texture_usage_undefined, tr_texture_usage_sampled_image);
    // vblur
    tr_create_texture_2d(g_renderer, image_width, image_height, tr_sample_count_1, tr_format_r8g8b8a8_unorm, 1, NULL, false, tr_texture_usage_sampled_image | tr_texture_usage_storage_image, &g_texture_compute_output_vblur); 
    tr_util_transition_image(g_renderer->graphics_queue, g_texture_compute_output_vblur, tr_texture_usage_undefined, tr_texture_usage_sampled_image);
  }

  // Samplers
  {
    tr_create_sampler(g_renderer, &g_sampler);
  }

  // Update descriptor sets
  {
    g_desc_set->descriptors[0].textures[0] = g_texture_compute_output_vblur;
    g_desc_set->descriptors[1].samplers[0] = g_sampler;
    tr_update_descriptor_set(g_renderer, g_desc_set);

    // hblur
    g_compute_desc_set_hblur->descriptors[0].textures[0] = g_texture;
    g_compute_desc_set_hblur->descriptors[1].textures[0] = g_texture_compute_output_hblur;
    tr_update_descriptor_set(g_renderer, g_compute_desc_set_hblur);
    // vblur
    g_compute_desc_set_vblur->descriptors[0].textures[0] = g_texture_compute_output_hblur;
    g_compute_desc_set_vblur->descriptors[1].textures[0] = g_texture_compute_output_vblur;
    tr_update_descriptor_set(g_renderer, g_compute_desc_set_vblur);
  }
}

void destroy_tiny_renderer()
{
    tr_destroy_renderer(g_renderer);
}

void draw_frame()
{
  uint32_t frameIdx = g_frame_count % g_renderer->settings.swapchain.image_count;

  tr_fence* image_acquired_fence = g_renderer->image_acquired_fences[frameIdx];
  tr_semaphore* image_acquired_semaphore = g_renderer->image_acquired_semaphores[frameIdx];
  tr_semaphore* render_complete_semaphores = g_renderer->render_complete_semaphores[frameIdx];

  tr_acquire_next_image(g_renderer, image_acquired_semaphore, image_acquired_fence);

  uint32_t swapchain_image_index = g_renderer->swapchain_image_index;
  tr_render_target* render_target = g_renderer->swapchain_render_targets[swapchain_image_index];

  tr_cmd* cmd = g_cmds[frameIdx];

  tr_begin_cmd(cmd);
  // hblur
  {
    tr_cmd_image_transition(cmd, g_texture_compute_output_hblur, tr_texture_usage_sampled_image, tr_texture_usage_storage_image);
    tr_cmd_bind_pipeline(cmd, g_compute_pipeline_hblur);
    tr_cmd_bind_descriptor_sets(cmd, g_compute_pipeline_hblur, g_compute_desc_set_hblur);
    const int num_groups_x = 1;
    const int num_groups_y = g_texture_compute_output_hblur->height;
    const int num_groups_z = 1;
    tr_cmd_dispatch(cmd, num_groups_x, num_groups_y, num_groups_z);
    tr_cmd_image_transition(cmd, g_texture_compute_output_hblur, tr_texture_usage_storage_image, tr_texture_usage_sampled_image);
  }
  // vbblur
  {
    tr_cmd_image_transition(cmd, g_texture_compute_output_vblur, tr_texture_usage_sampled_image, tr_texture_usage_storage_image);
    tr_cmd_bind_pipeline(cmd, g_compute_pipeline_vblur);
    tr_cmd_bind_descriptor_sets(cmd, g_compute_pipeline_vblur, g_compute_desc_set_vblur);
    const int num_groups_x = g_texture_compute_output_vblur->width;
    const int num_groups_y = 1;
    const int num_groups_z = 1;
    tr_cmd_dispatch(cmd, num_groups_x, num_groups_y, num_groups_z);
    tr_cmd_image_transition(cmd, g_texture_compute_output_vblur, tr_texture_usage_storage_image, tr_texture_usage_sampled_image);
  }
  // Draw compute result to screen
  tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
  tr_cmd_set_viewport(cmd, 0, 0, (float)g_window_width, (float)g_window_height, 0.0f, 1.0f);
  tr_cmd_set_scissor(cmd, 0, 0, g_window_width, g_window_height);
  tr_cmd_begin_render(cmd, render_target);
  tr_clear_value clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
  tr_cmd_clear_color_attachment(cmd, 0, &clear_value);
  tr_cmd_bind_pipeline(cmd, g_pipeline);
  tr_cmd_bind_index_buffer(cmd, g_rect_index_buffer);
  tr_cmd_bind_vertex_buffers(cmd, 1, &g_rect_vertex_buffer);
  tr_cmd_bind_descriptor_sets(cmd, g_pipeline, g_desc_set);
  tr_cmd_draw_indexed(cmd, 6, 0);
  tr_cmd_end_render(cmd);
  tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment, tr_texture_usage_present); 
  tr_end_cmd(cmd);

  tr_queue_submit(g_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1, &render_complete_semaphores);
  tr_queue_present(g_renderer->present_queue, 1, &render_complete_semaphores);

  tr_queue_wait_idle(g_renderer->graphics_queue);
}

int main(int argc, char **argv)
{
  glfwSetErrorCallback(app_glfw_error);
  if (! glfwInit()) {
    exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(1024, 1024, k_app_name, NULL, NULL);
  init_tiny_renderer(window);

  while (! glfwWindowShouldClose(window)) {
    draw_frame();
    glfwPollEvents();
  }
    
  destroy_tiny_renderer();

  glfwDestroyWindow(window);
  glfwTerminate();
  return EXIT_SUCCESS;
}
