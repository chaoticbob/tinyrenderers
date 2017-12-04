#include "GLFW/glfw3.h"
#if defined(__linux__)
  #define GLFW_EXPOSE_NATIVE_X11
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
#include "camera.h"
#include "cbuffer.h"
#include "entity.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "mesh.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>
using float2   = glm::vec2;
using float3   = glm::vec3;
using float4   = glm::vec4;
using float3x3 = glm::mat3;
using float4x4 = glm::mat4;
using float3x4 = glm::mat3x4;
using float4x3 = glm::mat4x3;

const char*           k_app_name = "ChessSet";
const uint32_t        k_image_count = 1;
#if defined(__linux__)
const tr::fs::path    k_asset_dir = "../demos/assets/";
#elif defined(_WIN32)
const tr::fs::path    k_asset_dir = "../../demos/assets/";
#endif

tr_renderer*          g_renderer = nullptr;
tr_clear_value        g_color_clear_value = {};
tr_clear_value        g_depth_stencil_clear_value = {};

tr_cmd_pool*          g_cmd_pool = nullptr;
tr_cmd**              g_cmds = nullptr;

tr::BlinnPhongEntity  g_chess_board_1_solid;
tr::BlinnPhongEntity  g_chess_board_2_solid;
tr::BlinnPhongEntity  g_chess_pieces_1_solid;
tr::BlinnPhongEntity  g_chess_pieces_2_solid;
tr::BasicEntity       g_chess_pieces_1_wireframe;
tr::BasicEntity       g_chess_pieces_2_wireframe;

tr_shader_program*    g_phong_shader = nullptr;
tr_shader_program*    g_normal_wireframe_shader = nullptr;

uint32_t              g_window_width;
uint32_t              g_window_height;
uint64_t              g_frame_count = 0;

tr::Camera            g_camera;

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

void init_tiny_renderer(GLFWwindow* window)
{
  // Renderer
  {
    std::vector<const char*> instance_layers = {
#if defined(_DEBUG)
      // "VK_LAYER_LUNARG_standard_validation",
#endif
    };

    std::vector<const char*> device_layers;

    int width = 0;
    int height = 0;
    glfwGetWindowSize(window, &width, &height);
    g_window_width = (uint32_t)width;
    g_window_height = (uint32_t)height;

    g_color_clear_value                 = { 0.1f, 0.1f, 0.1f, 0.1f };
    g_depth_stencil_clear_value.depth   = 1.0f;
    g_depth_stencil_clear_value.stencil = 255;

    tr_renderer_settings settings = {0};
#if defined(__linux__)
    settings.handle.connection                    = XGetXCBConnection(glfwGetX11Display());
    settings.handle.window                        = glfwGetX11Window(window);
#elif defined(_WIN32)
    settings.handle.hinstance                     = ::GetModuleHandle(NULL);
    settings.handle.hwnd                          = glfwGetWin32Window(window);
#endif
    settings.width                                = g_window_width;
    settings.height                               = g_window_height;
    settings.swapchain.image_count                = k_image_count;
    settings.swapchain.sample_count               = tr_sample_count_8;
    settings.swapchain.color_format               = tr_format_b8g8r8a8_unorm;
    settings.swapchain.depth_stencil_format       = tr_format_d32_float;
    settings.swapchain.color_clear_value          = g_color_clear_value;
    settings.swapchain.depth_stencil_clear_value  = g_depth_stencil_clear_value;
    settings.log_fn                               = renderer_log;
#if defined(TINY_RENDERER_VK)
    settings.vk_debug_fn                          = vulkan_debug;
    settings.instance_layers.count                = (uint32_t)instance_layers.size();
    settings.instance_layers.names                = instance_layers.empty() ? nullptr : instance_layers.data();
#endif
    tr_create_renderer(k_app_name, &settings, &g_renderer);

    // Command buffers
    {
      tr_create_cmd_pool(g_renderer, g_renderer->graphics_queue, false, &g_cmd_pool);
      tr_create_cmd_n(g_cmd_pool, false, k_image_count, &g_cmds);
    }
  }
    
  // Shaders
  {
    // Shader file paths
#if defined(TINY_RENDERER_VK)
    tr::fs::path phong_shader_vs_file_path     = k_asset_dir / "ChessSet/shaders/phong.vs.spv";
    tr::fs::path phong_shader_ps_file_path     = k_asset_dir / "ChessSet/shaders/phong.ps.spv";
    tr::fs::path wireframe_shader_vs_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.vs.spv";
    tr::fs::path wireframe_shader_gs_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.gs.spv";
    tr::fs::path wireframe_shader_ps_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.ps.spv";
#elif defined(TINY_RENDERER_DX)
    tr::fs::path phong_shader_vs_file_path     = k_asset_dir / "ChessSet/shaders/phong.hlsl";
    tr::fs::path phong_shader_ps_file_path     = k_asset_dir / "ChessSet/shaders/phong.hlsl";
    tr::fs::path wireframe_shader_vs_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.hlsl";
    tr::fs::path wireframe_shader_gs_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.hlsl";
    tr::fs::path wireframe_shader_ps_file_path = k_asset_dir / "ChessSet/shaders/normal_wireframe.hlsl";
#endif
    // Create Blinn-Phong shader
    g_phong_shader = tr::CreateShaderProgram(g_renderer,
                                              phong_shader_vs_file_path, "VSMain",
                                              phong_shader_ps_file_path, "PSMain");
    assert(g_phong_shader != nullptr);
    // Create wireframe shader
    g_normal_wireframe_shader = tr::CreateShaderProgram(g_renderer,
                                                        wireframe_shader_vs_file_path, "VSMain",
                                                        wireframe_shader_gs_file_path, "GSMain",
                                                        wireframe_shader_ps_file_path, "PSMain");
    assert(g_phong_shader != nullptr);
  }

  // Entities
  {
    tr::EntityCreateInfo entity_create_info = {};
    entity_create_info.shader_program           = g_phong_shader;
    entity_create_info.vertex_layout            = tr::Mesh::DefaultVertexLayout();
    entity_create_info.render_target            = g_renderer->swapchain_render_targets[0];
    entity_create_info.pipeline_settings.depth  = true;
    entity_create_info.pipeline_settings.cull_mode  = tr_cull_mode_back;

    // Create solids
    g_chess_board_1_solid.Create(g_renderer, entity_create_info);
    g_chess_board_2_solid.Create(g_renderer, entity_create_info);
    g_chess_pieces_1_solid.Create(g_renderer, entity_create_info);
    g_chess_pieces_2_solid.Create(g_renderer, entity_create_info);

    entity_create_info = {};
    entity_create_info.shader_program           = g_normal_wireframe_shader;
    entity_create_info.vertex_layout            = tr::Mesh::DefaultVertexLayout();
    entity_create_info.render_target            = g_renderer->swapchain_render_targets[0];
    entity_create_info.pipeline_settings.depth  = true;

    // Create wireframes
    g_chess_pieces_1_wireframe.Create(g_renderer, entity_create_info);
    g_chess_pieces_2_wireframe.Create(g_renderer, entity_create_info);
  }

  // Vertex data
  {
    // Chess board 1
    tr::fs::path file_path = k_asset_dir / "ChessSet/models/board1.obj";
    g_chess_board_1_solid.LoadVertexBuffers(file_path);     
    // Chess board 2
    file_path = k_asset_dir / "ChessSet/models/board2.obj";
    g_chess_board_2_solid.LoadVertexBuffers(file_path);
    // Chest pieces 1
    file_path = k_asset_dir / "ChessSet/models/pieces1.obj";
    g_chess_pieces_1_solid.LoadVertexBuffers(file_path);
    g_chess_pieces_1_wireframe.LoadVertexBuffers(file_path);   
    // Chest pieces 2
    file_path = k_asset_dir / "ChessSet/models/pieces2.obj";
    g_chess_pieces_2_solid.LoadVertexBuffers(file_path);
    g_chess_pieces_2_wireframe.LoadVertexBuffers(file_path);
  }

  // Update descriptors
  {
    g_chess_board_1_solid.UpdateGpuDescriptorSets();
    g_chess_board_2_solid.UpdateGpuDescriptorSets();
    g_chess_pieces_1_solid.UpdateGpuDescriptorSets();
    g_chess_pieces_2_solid.UpdateGpuDescriptorSets();
    g_chess_pieces_1_wireframe.UpdateGpuDescriptorSets();
    g_chess_pieces_2_wireframe.UpdateGpuDescriptorSets();
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

    float3 eye = float3(0, 7, 12);
    float3 look_at = float3(0, 0, 0);
    g_camera.LookAt(eye, look_at);
    g_camera.Perspective(65.0f, (float)g_window_width / (float)g_window_height);

    // Model
    float t = (float)glfwGetTime();
    float ry = t / 2.0f;

    g_chess_board_1_solid.SetColor(float3(0.23f));
    g_chess_board_2_solid.SetColor(float3(0.88f));
    g_chess_pieces_1_solid.SetColor(float3(0.85f, 0.3f, 0.3f));
    g_chess_pieces_2_solid.SetColor(float3(0.4f, 0.4f, 0.8f));

    g_chess_board_1_solid.SetView(g_camera);
    g_chess_board_2_solid.SetView(g_camera);
    g_chess_pieces_1_solid.SetView(g_camera);
    g_chess_pieces_2_solid.SetView(g_camera);
    g_chess_pieces_1_wireframe.SetView(g_camera);
    g_chess_pieces_2_wireframe.SetView(g_camera);

    g_chess_board_1_solid.GetTransform().SetRotate(0, ry, 0);
    g_chess_board_2_solid.GetTransform().SetRotate(0, ry, 0);
    g_chess_pieces_1_solid.GetTransform().SetRotate(0, ry, 0);
    g_chess_pieces_2_solid.GetTransform().SetRotate(0, ry, 0);
    g_chess_pieces_1_wireframe.GetTransform().SetRotate(0, ry, 0);
    g_chess_pieces_2_wireframe.GetTransform().SetRotate(0, ry, 0);

    g_chess_board_1_solid.UpdateGpuBuffers();
    g_chess_board_2_solid.UpdateGpuBuffers();
    g_chess_pieces_1_solid.UpdateGpuBuffers();
    g_chess_pieces_2_solid.UpdateGpuBuffers();
    g_chess_pieces_1_wireframe.UpdateGpuBuffers();
    g_chess_pieces_2_wireframe.UpdateGpuBuffers();

    tr_cmd* cmd = g_cmds[frameIdx];
    tr_begin_cmd(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
    tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
    tr_cmd_set_viewport(cmd, 0, 0, (float)g_window_width, (float)g_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, g_window_width, g_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_cmd_clear_color_attachment(cmd, 0, &g_color_clear_value);
    tr_cmd_clear_depth_stencil_attachment(cmd, &g_depth_stencil_clear_value);
    // Draw phong
    {
      g_chess_board_1_solid.Draw(cmd);
      g_chess_board_2_solid.Draw(cmd);
      g_chess_pieces_1_solid.Draw(cmd);
      g_chess_pieces_2_solid.Draw(cmd);
    }
    // Draw normal wireframe 
    {
      g_chess_pieces_1_wireframe.Draw(cmd);
      g_chess_pieces_2_wireframe.Draw(cmd);
    }
    tr_cmd_end_render(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_color_attachment, tr_texture_usage_present); 
    tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_depth_stencil_attachment, tr_texture_usage_sampled_image);
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
    GLFWwindow* window = glfwCreateWindow(1920, 1080, k_app_name, NULL, NULL);
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
