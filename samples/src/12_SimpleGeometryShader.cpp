#include "GLFW/glfw3.h"
#if defined(__linux__)
  #if defined(__ggp__)
  #else
    #define GLFW_EXPOSE_NATIVE_X11
  #endif
#elif defined(_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3native.h"
#include <cmath>
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

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
using float2   = glm::vec2;
using float3   = glm::vec3;
using float4   = glm::vec4;
using float4x4 = glm::mat4;


const char*         k_app_name = "12_SimpleGeometryShader";
const uint32_t      k_image_count = 3;
#if defined(TINY_RENDERER_GGP)
const std::string   k_asset_dir = "./samples/assets/";
#elif defined(TINY_RENDERER_LINUX)
const std::string   k_asset_dir = "../samples/assets/";
#elif defined(TINY_RENDERER_MSW)
const std::string   k_asset_dir = "../../samples/assets/";
#endif

#if defined(TINY_RENDERER_GGP)
const uint32_t k_window_width  = 1920;
const uint32_t k_window_height = 1080;
#else
const uint32_t k_window_width  = 640;
const uint32_t k_window_height = 480;
#endif

tr_renderer*        m_renderer = nullptr;
tr_descriptor_set*  m_desc_set = nullptr;
tr_cmd_pool*        m_cmd_pool = nullptr;
tr_cmd**            m_cmds = nullptr;
tr_shader_program*  m_shader = nullptr;
tr_buffer*          m_rect_vertex_buffer = nullptr;
tr_pipeline*        m_pipeline = nullptr;
tr_texture*         m_texture = nullptr;
tr_sampler*         m_sampler = nullptr;
tr_buffer*          m_uniform_buffer = nullptr;

uint32_t            s_window_width;
uint32_t            s_window_height;
uint64_t            s_frame_count = 0;

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
    case tr_log_type_info  : {LOG("[INFO]" << "[" << component << "] : " << msg);} break;
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
        LOG("[WARN]" << "[" << pLayerPrefix << "] : " << pMessage << " (" << messageCode << ")");
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
    std::vector<const char*> instance_layers = {
#if defined(_DEBUG)
        "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char*> device_layers;

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    s_window_width = (uint32_t)width;
    s_window_height = (uint32_t)height;

    tr_renderer_settings settings = {};
#if defined(TINY_RENDERER_GGP)
#elif defined(TINY_RENDERER_LINUX)
    settings.handle.connection              = XGetXCBConnection(glfwGetX11Display());
    settings.handle.window                  = glfwGetX11Window(window);
#elif defined(TINY_RENDERER_MSW)
    settings.handle.hinstance               = ::GetModuleHandle(NULL);
    settings.handle.hwnd                    = glfwGetWin32Window(window);
#endif
    settings.width                          = s_window_width;
    settings.height                         = s_window_height;
    settings.swapchain.image_count          = k_image_count;
    settings.swapchain.sample_count         = tr_sample_count_8;
    settings.swapchain.color_format         = tr_format_b8g8r8a8_unorm;
    settings.swapchain.depth_stencil_format = tr_format_d32_float;
    settings.swapchain.depth_stencil_clear_value.depth    = 1.0f;
    settings.swapchain.depth_stencil_clear_value.stencil  = 255;
    settings.log_fn                         = renderer_log;
#if defined(TINY_RENDERER_VK)
    settings.vk_debug_fn                    = vulkan_debug;
    settings.instance_layers.count          = (uint32_t)instance_layers.size();
    settings.instance_layers.names          = instance_layers.empty() ? nullptr : instance_layers.data();
#endif
    tr_create_renderer(k_app_name, &settings, &m_renderer);

    tr_create_cmd_pool(m_renderer, m_renderer->graphics_queue, false, &m_cmd_pool);
    tr_create_cmd_n(m_cmd_pool, false, k_image_count, &m_cmds);
    
#if defined(TINY_RENDERER_VK)
    auto vert = load_file(k_asset_dir + "triangle_wireframe.vs.spv");
    auto geom = load_file(k_asset_dir + "triangle_wireframe.gs.spv");
    auto frag = load_file(k_asset_dir + "triangle_wireframe.ps.spv");
    tr_create_shader_program_n(m_renderer, 
                             (uint32_t)vert.size(), (uint32_t*)(vert.data()), "VSMain",
                             0, nullptr, nullptr,
                             0, nullptr, nullptr,
                             (uint32_t)geom.size(), (uint32_t*)(geom.data()), "GSMain", 
                             (uint32_t)frag.size(), (uint32_t*)(frag.data()), "PSMain",
                             0, nullptr, nullptr,
                             &m_shader);
#elif defined(TINY_RENDERER_DX)
    auto hlsl = load_file(k_asset_dir + "triangle_wireframe.hlsl");
    tr_create_shader_program_n(m_renderer,
                               (uint32_t)hlsl.size(), hlsl.data(), "VSMain", 
                               0, nullptr, nullptr,
                               0, nullptr, nullptr,
                               (uint32_t)hlsl.size(), hlsl.data(), "GSMain", 
                               (uint32_t)hlsl.size(), hlsl.data(), "PSMain", 
                               0, nullptr, nullptr,
                               &m_shader);                               
#endif

    std::vector<tr_descriptor> descriptors(1);
    descriptors[0].type          = tr_descriptor_type_uniform_buffer_cbv;
    descriptors[0].count         = 1;
    descriptors[0].binding       = 0;
    descriptors[0].shader_stages = (tr_shader_stage)(tr_shader_stage_vert | tr_shader_stage_geom);
    //descriptors[1].type          = tr_descriptor_type_texture_srv;
    //descriptors[1].count         = 1;
    //descriptors[1].binding       = 1;
    //descriptors[1].shader_stages = tr_shader_stage_frag;
    //descriptors[2].type          = tr_descriptor_type_sampler;
    //descriptors[2].count         = 1;
    //descriptors[2].binding       = 2;
    //descriptors[2].shader_stages = tr_shader_stage_frag;
    tr_create_descriptor_set(m_renderer, (uint32_t)descriptors.size(), descriptors.data(), &m_desc_set);

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
    //pipeline_settings.depth = true;
    tr_create_pipeline(m_renderer, m_shader, &vertex_layout, m_desc_set, m_renderer->swapchain_render_targets[0], &pipeline_settings, &m_pipeline);

    float4 positions[8] = {
      { -0.5f,  0.5f,  0.5f, 1.0f },  // 0: -X,  Y, +Z
      { -0.5f, -0.5f,  0.5f, 1.0f },  // 1: -X, -Y, +Z
      {  0.5f, -0.5f,  0.5f, 1.0f },  // 2:  X, -Y, +Z
      {  0.5f,  0.5f,  0.5f, 1.0f },  // 3:  X,  Y, +Z
      {  0.5f,  0.5f, -0.5f, 1.0f },  // 4:  X,  Y,  Z
      {  0.5f, -0.5f, -0.5f, 1.0f },  // 5:  X, -Y,  Z
      { -0.5f, -0.5f, -0.5f, 1.0f },  // 6: -X, -Y,  Z
      { -0.5f,  0.5f, -0.5f, 1.0f },  // 7: -X,  Y,  Z
    };

    float2 uvs[4] = {
      { 0.0f, 0.0f },
      { 0.0f, 1.0f },
      { 1.0f, 1.0f },
      { 1.0f, 0.0f },
    };

    struct Vertex {
      float4  position;
      float2  tex_coord;
    };

    std::vector<Vertex> vertex_data = {
      // Front +Z (near to camera)
      { positions[0], uvs[0] },
      { positions[1], uvs[1] },
      { positions[2], uvs[2] },
      { positions[0], uvs[0] },
      { positions[2], uvs[2] },
      { positions[3], uvs[3] },
      // Back -Z (far from camera)
      { positions[4], uvs[0] },
      { positions[5], uvs[1] },
      { positions[6], uvs[2] },
      { positions[4], uvs[0] },
      { positions[6], uvs[2] },
      { positions[7], uvs[3] },
      // Right +X
      { positions[3], uvs[0] },
      { positions[2], uvs[1] },
      { positions[5], uvs[2] },
      { positions[3], uvs[0] },
      { positions[5], uvs[2] },
      { positions[4], uvs[3] },
      // Left -X
      { positions[7], uvs[0] },
      { positions[6], uvs[1] },
      { positions[1], uvs[2] },
      { positions[7], uvs[0] },
      { positions[1], uvs[2] },
      { positions[0], uvs[3] },
      // Top +Y
      { positions[7], uvs[0] },
      { positions[0], uvs[1] },
      { positions[3], uvs[2] },
      { positions[7], uvs[0] },
      { positions[3], uvs[2] },
      { positions[4], uvs[3] },
      // Top -Y
      { positions[1], uvs[0] },
      { positions[6], uvs[1] },
      { positions[5], uvs[2] },
      { positions[1], uvs[0] },
      { positions[5], uvs[2] },
      { positions[2], uvs[3] },
    };

    uint32_t vertex_stride   = sizeof(Vertex);
    uint32_t vertex_count    = (uint32_t)vertex_data.size();
    uint32_t vertex_data_size = vertex_stride * vertex_count;

    tr_create_vertex_buffer(m_renderer, vertex_data_size, true, vertex_stride, &m_rect_vertex_buffer);
    memcpy(m_rect_vertex_buffer->cpu_mapped_address, vertex_data.data(), vertex_data_size);

    int image_width = 0;
    int image_height = 0;
    int image_channels = 0;
    unsigned char* image_data = stbi_load((k_asset_dir + "box_panel.jpg").c_str(), &image_width, &image_height, &image_channels, 0);
    assert(NULL != image_data);
    int image_row_stride = image_width * image_channels;
    tr_create_texture_2d(m_renderer, image_width, image_height, tr_sample_count_1, tr_format_r8g8b8a8_unorm, tr_max_mip_levels, NULL, false, tr_texture_usage_sampled_image, &m_texture);
    //tr_util_update_texture_uint8(m_renderer->graphics_queue, image_width, image_height, image_row_stride, image_data, image_channels, m_texture, NULL, NULL);
    stbi_image_free(image_data);

    tr_create_sampler(m_renderer, &m_sampler);

    tr_create_uniform_buffer(m_renderer, 16 * sizeof(float), true, &m_uniform_buffer);

    m_desc_set->descriptors[0].uniform_buffers[0] = m_uniform_buffer;
    m_desc_set->descriptors[1].textures[0]        = m_texture;
    m_desc_set->descriptors[2].samplers[0]        = m_sampler;
    tr_update_descriptor_set(m_renderer, m_desc_set);
}

void destroy_tiny_renderer()
{
    tr_destroy_renderer(m_renderer);
}

void draw_frame()
{
    uint32_t frameIdx = s_frame_count % m_renderer->settings.swapchain.image_count;

    tr_fence* image_acquired_fence = m_renderer->image_acquired_fences[frameIdx];
    tr_semaphore* image_acquired_semaphore = m_renderer->image_acquired_semaphores[frameIdx];
    tr_semaphore* render_complete_semaphores = m_renderer->render_complete_semaphores[frameIdx];

    tr_acquire_next_image(m_renderer, image_acquired_semaphore, image_acquired_fence);

    uint32_t swapchain_image_index = m_renderer->swapchain_image_index;
    tr_render_target* render_target = m_renderer->swapchain_render_targets[swapchain_image_index];

    //// No projection or view for GLFW since we don't have a math library
    //float t = (float)glfwGetTime();
    //std::vector<float> mvp(16);
    //std::fill(std::begin(mvp), std::end(mvp), 0.0f);
    //mvp[ 0] =  cos(t); 
    //mvp[ 1] =  sin(t);
    //mvp[ 4] = -sin(t);
    //mvp[ 5] =  cos(t);
    //mvp[10] =  1.0f;
    //mvp[15] =  1.0f;
    //memcpy(m_uniform_buffer->cpu_mapped_address, mvp.data(), mvp.size() * sizeof(float));
    float t = (float)glfwGetTime();
    float4x4 view  = glm::lookAt(float3(0, 0, 2),  float3(0, 0, 0), float3(0, 1, 0));                               
    float4x4 proj  = glm::perspective(glm::radians(60.0f), (float)s_window_width / (float)s_window_height, 0.1f, 10000.0f);
    float4x4 rot_x = glm::rotate(t, float3(1, 0, 0));
    float4x4 rot_y = glm::rotate(t / 2.0f, float3(0, 1, 0));
    float4x4 rot_z = glm::rotate(t / 3.0f, float3(0, 0, 1));
    float4x4 model = rot_x * rot_y * rot_z;
    float4x4 mvp = proj * view * model;
    memcpy(m_uniform_buffer->cpu_mapped_address, &mvp, sizeof(mvp));

    tr_cmd* cmd = m_cmds[frameIdx];

    tr_begin_cmd(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
    tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
    tr_cmd_set_viewport(cmd, 0, 0, (float)s_window_width, (float)s_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, s_window_width, s_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_clear_value color_clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
    tr_cmd_clear_color_attachment(cmd, 0, &color_clear_value);
    tr_clear_value depth_stencil_clear_value = { 0 };
    depth_stencil_clear_value.depth = 1.0f;
    depth_stencil_clear_value.stencil = 255;
    tr_cmd_clear_depth_stencil_attachment(cmd, &depth_stencil_clear_value);
	tr_cmd_set_line_width(cmd, 1.0f);
    tr_cmd_bind_pipeline(cmd, m_pipeline);
    tr_cmd_bind_vertex_buffers(cmd, 1, &m_rect_vertex_buffer);
    tr_cmd_bind_descriptor_sets(cmd, m_pipeline, m_desc_set);
    tr_cmd_draw(cmd, 36, 0);
    tr_cmd_end_render(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment, tr_texture_usage_present); 
    tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_depth_stencil_attachment, tr_texture_usage_sampled_image);
    tr_end_cmd(cmd);

    tr_queue_submit(m_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1, &render_complete_semaphores);
    tr_queue_present(m_renderer->present_queue, 1, &render_complete_semaphores);

    tr_queue_wait_idle(m_renderer->graphics_queue);
}

int main(int argc, char **argv)
{
    glfwSetErrorCallback(app_glfw_error);
    if (! glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(k_window_width, k_window_height, k_app_name, NULL, NULL);
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
