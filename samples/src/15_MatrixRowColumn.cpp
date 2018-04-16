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

const char*         k_app_name = "15_MatrixRowColumn";
const uint32_t      k_image_count = 3;
#if defined(__linux__)
const std::string   k_asset_dir = "../samples/assets/";
#elif defined(_WIN32)
const std::string   k_asset_dir = "../../samples/assets/";
#endif

#define NUM_THREADS_X  16
#define NUM_THREADS_Y  16

tr_renderer*        m_renderer = nullptr;
tr_descriptor_set*  m_desc_set = nullptr;
tr_descriptor_set*  m_compute_desc_set = nullptr;
tr_cmd_pool*        m_cmd_pool = nullptr;
tr_cmd**            m_cmds = nullptr;
tr_shader_program*  m_compute_shader = nullptr;
tr_pipeline*        m_pipeline = nullptr;
tr_pipeline*        m_compute_pipeline = nullptr;
tr_buffer*          m_uniform_buffer = nullptr;
tr_buffer*          m_cpu_buffer = nullptr;
tr_buffer*          m_gpu_buffer = nullptr;

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
        //"VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char*> device_layers;

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    s_window_width = (uint32_t)width;
    s_window_height = (uint32_t)height;

    tr_renderer_settings settings = {0};
#if defined(__linux__)
    settings.handle.connection              = XGetXCBConnection(glfwGetX11Display());
    settings.handle.window                  = glfwGetX11Window(window);
#elif defined(_WIN32)
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
    auto comp = load_file(k_asset_dir + "matrix_row_column.cs.spv");
    tr_create_shader_program_compute(m_renderer, 
                                     (uint32_t)comp.size(), comp.data(), "main", &m_compute_shader);
#elif defined(TINY_RENDERER_DX)
    auto hlsl = load_file(k_asset_dir + "matrix_row_column.hlsl");
    tr_create_shader_program_compute(m_renderer, 
                                     (uint32_t)hlsl.size(), hlsl.data(), "main", &m_compute_shader);
#endif

    std::vector<tr_descriptor> descriptors(2);
    descriptors[0].type          = tr_descriptor_type_uniform_buffer_cbv;
    descriptors[0].count         = 1;
    descriptors[0].binding       = 0;
    descriptors[0].shader_stages = tr_shader_stage_comp;
    descriptors[1].type          = tr_descriptor_type_storage_buffer_uav;
    descriptors[1].count         = 1;
    descriptors[1].binding       = 1;
    descriptors[1].shader_stages = tr_shader_stage_comp;
    tr_create_descriptor_set(m_renderer, (uint32_t)descriptors.size(), descriptors.data(), &m_compute_desc_set);

    tr_pipeline_settings pipeline_settings = {};
    tr_create_compute_pipeline(m_renderer, m_compute_shader, m_compute_desc_set, &pipeline_settings, &m_compute_pipeline);

    tr_create_uniform_buffer(m_renderer, 2 * sizeof(float4x4), true, &m_uniform_buffer);

    uint64_t buffer_size = 256;
    tr_create_buffer(m_renderer, tr_buffer_usage_storage_srv, buffer_size, true, &m_cpu_buffer);
    
    uint64_t element_count = 1;
    uint64_t struct_stride = buffer_size;
    tr_create_rw_structured_buffer(m_renderer, buffer_size, 0, element_count, struct_stride, false, NULL, &m_gpu_buffer);
    tr_util_transition_buffer(m_renderer->graphics_queue, m_gpu_buffer, tr_buffer_usage_storage_uav, tr_buffer_usage_transfer_src);

    m_compute_desc_set->descriptors[0].uniform_buffers[0] = m_uniform_buffer;
    m_compute_desc_set->descriptors[1].buffers[0]         = m_gpu_buffer;
    tr_update_descriptor_set(m_renderer, m_compute_desc_set);

    const float mat[32] = {
       1.0f,  2.0f,  3.0f,  4.0f,
       5.0f,  6.0f,  7.0f,  8.0f,
       9.0f, 10.0f, 11.0f, 12.0f,
      13.0f, 14.0f, 15.0f, 16.0f,

       1.0f,  2.0f,  3.0f,  4.0f,
       5.0f,  6.0f,  7.0f,  8.0f,
       9.0f, 10.0f, 11.0f, 12.0f,
      13.0f, 14.0f, 15.0f, 16.0f,
    };
    size_t size = 32 * sizeof(float);
    memcpy(m_uniform_buffer->cpu_mapped_address, mat, size);
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
    // Use compute write out matrix data
    tr_cmd_buffer_transition(cmd, m_gpu_buffer, tr_buffer_usage_transfer_src, tr_buffer_usage_storage_uav);
    tr_cmd_bind_pipeline(cmd, m_compute_pipeline);
    tr_cmd_bind_descriptor_sets(cmd, m_compute_pipeline, m_compute_desc_set);
    tr_cmd_dispatch(cmd, 1, 1, 1);
    // Copy buffer
    tr_cmd_buffer_transition(cmd, m_gpu_buffer, tr_buffer_usage_storage_uav, tr_buffer_usage_transfer_src);
    tr_cmd_copy_buffer_to_buffer(cmd, 0, 0, 256, m_gpu_buffer, m_cpu_buffer);
    // Clear the screen nice blue
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
    tr_cmd_set_viewport(cmd, 0, 0, (float)s_window_width, (float)s_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, s_window_width, s_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_clear_value clear_value = {0.1f, 0.0f, 0.8f, 0.0f};
    tr_cmd_clear_color_attachment(cmd, 0, &clear_value);
    tr_cmd_end_render(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment, tr_texture_usage_present); 
    tr_end_cmd(cmd);

    tr_queue_submit(m_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1, &render_complete_semaphores);
    tr_queue_present(m_renderer->present_queue, 1, &render_complete_semaphores);

    tr_queue_wait_idle(m_renderer->graphics_queue);

    static bool s_printed = false;
    if (! s_printed) {
      const float* m = (const float*)m_cpu_buffer->cpu_mapped_address;
      assert(m != nullptr);
      std::stringstream ss;
      ss << "OUTPUT:" << "\n";
      size_t offset = 0;
      for (size_t i = 0; i < 16; ++i) {
        ss <<  m[offset + i] << " ";
      }
      ss << "\n";
      offset = 16;
      for (size_t i = 0; i < 16; ++i) {
        ss <<  m[offset + i] << " ";
      }
      ss << "\n";
      offset = 32;
      for (size_t i = 0; i < 4; ++i) {
        ss <<  m[offset + i] << " ";
      }
      ss << "\n";
      offset = 36;
      for (size_t i = 0; i < 4; ++i) {
        ss <<  m[offset + i] << " ";
      }
      ss << "\n";
      platform_log(ss.str().c_str());
      s_printed = true;
    }
}

int main(int argc, char **argv)
{
    glfwSetErrorCallback(app_glfw_error);
    if (! glfwInit()) {
        exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(640, 480, k_app_name, NULL, NULL);
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
