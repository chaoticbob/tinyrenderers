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

const char*         k_app_name = "01_Color";
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
tr_cmd_pool*        m_cmd_pool = nullptr;
tr_cmd**            m_cmds = nullptr;
tr_shader_program*  m_shader = nullptr;
tr_buffer*          m_tri_vertex_buffer = nullptr;
tr_buffer*          m_rect_index_buffer = nullptr;
tr_buffer*          m_rect_vertex_buffer = nullptr;
tr_pipeline*        m_pipeline = nullptr;

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
    settings.swapchain.depth_stencil_format = tr_format_undefined;
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
    // Uses HLSL source
    auto vert = load_file(k_asset_dir + "color.vs.spv");
    auto frag = load_file(k_asset_dir + "color.ps.spv");
    tr_create_shader_program(m_renderer, 
                             (uint32_t)vert.size(), (uint32_t*)(vert.data()), "VSMain", 
                             (uint32_t)frag.size(), (uint32_t*)(frag.data()), "PSMain", &m_shader);
#elif defined(TINY_RENDERER_DX)
    auto hlsl = load_file(k_asset_dir + "color.hlsl");
    tr_create_shader_program(m_renderer, 
                             (uint32_t)hlsl.size(), hlsl.data(), "VSMain", 
                             (uint32_t)hlsl.size(), hlsl.data(), "PSMain", &m_shader);
#endif

    tr_vertex_layout vertex_layout = {};
    vertex_layout.attrib_count = 2;
    vertex_layout.attribs[0].semantic = tr_semantic_position;
    vertex_layout.attribs[0].format   = tr_format_r32g32b32a32_float;
    vertex_layout.attribs[0].binding  = 0;
    vertex_layout.attribs[0].location = 0;
    vertex_layout.attribs[0].offset   = 0;
    vertex_layout.attribs[1].semantic = tr_semantic_color;
    vertex_layout.attribs[1].format   = tr_format_r32g32b32_float;
    vertex_layout.attribs[1].binding  = 0;
    vertex_layout.attribs[1].location = 1;
    vertex_layout.attribs[1].offset   = tr_util_format_stride(tr_format_r32g32b32a32_float);
    tr_pipeline_settings pipeline_settings = {tr_primitive_topo_tri_list};
    tr_create_pipeline(m_renderer, m_shader, &vertex_layout, nullptr, m_renderer->swapchain_render_targets[0], &pipeline_settings, &m_pipeline);

    // tri
    {
        std::vector<float> vertexData = {
             0.00f,  0.25f, 0.0f,   1.0f, 1.0f, 0.0f, 0.0f,
            -0.25f, -0.25f, 0.0f,   1.0f, 0.0f, 1.0f, 0.0f,
             0.25f, -0.25f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,
        };

        vertexData[7*0 + 0] += -0.5f;
        vertexData[7*1 + 0] += -0.5f;
        vertexData[7*2 + 0] += -0.5f;

        uint64_t vertexDataSize = sizeof(float) * vertexData.size();
        uint32_t vertexStride = sizeof(float) * 7;
        tr_create_vertex_buffer(m_renderer, vertexDataSize, true, vertexStride, &m_tri_vertex_buffer);
        memcpy(m_tri_vertex_buffer->cpu_mapped_address, vertexData.data(), vertexDataSize);
    }

    // quad
    {
        std::vector<float> vertexData = {
            -0.25f,  0.25f, 0.0f,   1.0f, 1.0f, 0.0f, 0.0f,
            -0.25f, -0.25f, 0.0f,   1.0f, 0.0f, 1.0f, 0.0f,
             0.25f, -0.25f, 0.0f,   1.0f, 0.0f, 0.0f, 1.0f,
             0.25f,  0.25f, 0.0f,   1.0f, 1.0f, 1.0f, 1.0f,
        };

        vertexData[7*0 + 0] += 0.5f;
        vertexData[7*1 + 0] += 0.5f;
        vertexData[7*2 + 0] += 0.5f;
        vertexData[7*3 + 0] += 0.5f;

        uint64_t vertexDataSize = sizeof(float) * vertexData.size();
        uint32_t vertexStride = sizeof(float) * 7;
        tr_create_vertex_buffer(m_renderer, vertexDataSize, true, vertexStride, &m_rect_vertex_buffer);
        memcpy(m_rect_vertex_buffer->cpu_mapped_address, vertexData.data(), vertexDataSize);
        
        std::vector<uint16_t> indexData = {
            0, 1, 2,
            0, 2, 3
        };
        
        uint64_t indexDataSize = sizeof(uint16_t) * indexData.size();
        tr_create_index_buffer(m_renderer, indexDataSize, true, tr_index_type_uint16, &m_rect_index_buffer);
        memcpy(m_rect_index_buffer->cpu_mapped_address, indexData.data(), indexDataSize);
    }
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

    tr_cmd* cmd = m_cmds[frameIdx];

    tr_begin_cmd(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
    tr_cmd_set_viewport(cmd, 0, 0, (float)s_window_width, (float)s_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, s_window_width, s_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_clear_value clear_value = {0.0f, 0.0f, 0.0f, 0.0f};
    tr_cmd_clear_color_attachment(cmd, 0, &clear_value);
    tr_cmd_bind_pipeline(cmd, m_pipeline);
    tr_cmd_bind_vertex_buffers(cmd, 1, &m_tri_vertex_buffer);
    tr_cmd_draw(cmd, 3, 0);
    tr_cmd_bind_index_buffer(cmd, m_rect_index_buffer);
    tr_cmd_bind_vertex_buffers(cmd, 1, &m_rect_vertex_buffer);
    tr_cmd_draw_indexed(cmd, 6, 0);
    tr_cmd_end_render(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment, tr_texture_usage_present); 
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
