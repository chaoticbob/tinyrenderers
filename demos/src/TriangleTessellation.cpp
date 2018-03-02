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
using namespace tr;

const char*         k_app_name = "TriangleTessellation";
const uint32_t      k_image_count = 1;
#if defined(__linux__)
const tr::fs::path  k_asset_dir = "../demos/assets/";
#elif defined(_WIN32)
const tr::fs::path  k_asset_dir = "../../demos/assets/";
#endif

struct TessData {
  // Padded C++         // HLSL
  float4  tess_factor;  // float
};

using TessParams            = tr::ConstantBuffer<TessData>;
using TessBlinnPhongEntity  = tr::EntityT<tr::BlinnPhongBuffer, TessParams>;
using TessBasicEntity       = tr::EntityT<NullBuffer,TessParams>;

tr_renderer*          g_renderer = nullptr;
tr_cmd_pool*          g_cmd_pool = nullptr;
tr_cmd**              g_cmds = nullptr;

tr::BlinnPhongEntity  g_chess_pieces_base;
tr::BasicEntity       g_chess_pieces_base_wireframe;
TessBlinnPhongEntity  g_chess_pieces_tess;
TessBasicEntity       g_chess_pieces_tess_wireframe;

tr_shader_program*    g_base_shader = nullptr;
tr_shader_program*    g_base_wireframe_shader = nullptr;
tr_shader_program*    g_tess_shader = nullptr;
tr_shader_program*    g_tess_wireframe_shader = nullptr;

tr_buffer*            g_vertex_buffer = nullptr;
uint32_t              g_vertex_count = 0;

uint32_t              g_window_width;
uint32_t              g_window_height;
uint64_t              g_frame_count = 0;

tr::Camera            g_camera;

tr_clear_value        g_color_clear_value = {};
tr_clear_value        g_depth_stencil_clear_value = {};


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
    g_window_width = (uint32_t)width;
    g_window_height = (uint32_t)height;

    g_color_clear_value = { 0.1f, 0.1f, 0.1f, 0.1f };
    g_depth_stencil_clear_value.depth = 1.0f;
    g_depth_stencil_clear_value.stencil = 255;

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
    settings.swapchain.depth_stencil_format = tr_format_d32_float;
    settings.swapchain.color_clear_value          = g_color_clear_value;
    settings.swapchain.depth_stencil_clear_value  = g_depth_stencil_clear_value;
    settings.log_fn                         = renderer_log;
#if defined(TINY_RENDERER_VK)
    settings.vk_debug_fn                    = vulkan_debug;
    settings.instance_layers.count          = (uint32_t)instance_layers.size();
    settings.instance_layers.names          = instance_layers.empty() ? nullptr : instance_layers.data();
#endif
    tr_create_renderer(k_app_name, &settings, &g_renderer);

    tr_create_cmd_pool(g_renderer, g_renderer->graphics_queue, false, &g_cmd_pool);
    tr_create_cmd_n(g_cmd_pool, false, k_image_count, &g_cmds);
    
#if defined(TINY_RENDERER_VK)
    tr::fs::path base_vs_file_path            = k_asset_dir / "TriangleTessellation/shaders/base.vs.spv"; 
    tr::fs::path base_ps_file_path            = k_asset_dir / "TriangleTessellation/shaders/base.ps.spv"; 
    tr::fs::path base_wireframe_vs_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.vs.spv"; 
    tr::fs::path base_wireframe_gs_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.gs.spv"; 
    tr::fs::path base_wireframe_ps_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.ps.spv"; 
    tr::fs::path tess_vs_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.vs.spv"; 
    tr::fs::path tess_hs_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.hs.spv"; 
    tr::fs::path tess_ds_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.ds.spv"; 
    tr::fs::path tess_ps_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.ps.spv"; 
    tr::fs::path tess_wireframe_vs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.vs.spv"; 
    tr::fs::path tess_wireframe_hs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hs.spv"; 
    tr::fs::path tess_wireframe_ds_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.ds.spv"; 
    tr::fs::path tess_wireframe_gs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.gs.spv"; 
    tr::fs::path tess_wireframe_ps_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.ps.spv"; 
#elif defined(TINY_RENDERER_DX)
    tr::fs::path base_vs_file_path            = k_asset_dir / "TriangleTessellation/shaders/base.hlsl"; 
    tr::fs::path base_ps_file_path            = k_asset_dir / "TriangleTessellation/shaders/base.hlsl"; 
    tr::fs::path base_wireframe_vs_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.hlsl"; 
    tr::fs::path base_wireframe_gs_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.hlsl"; 
    tr::fs::path base_wireframe_ps_file_path  = k_asset_dir / "TriangleTessellation/shaders/base_wireframe.hlsl"; 
    tr::fs::path tess_vs_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.hlsl"; 
    tr::fs::path tess_hs_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.hlsl"; 
    tr::fs::path tess_ds_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.hlsl"; 
    tr::fs::path tess_ps_file_path            = k_asset_dir / "TriangleTessellation/shaders/tess_phong.hlsl"; 
    tr::fs::path tess_wireframe_vs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hlsl";
    tr::fs::path tess_wireframe_hs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hlsl";
    tr::fs::path tess_wireframe_ds_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hlsl";
    tr::fs::path tess_wireframe_gs_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hlsl";
    tr::fs::path tess_wireframe_ps_file_path  = k_asset_dir / "TriangleTessellation/shaders/tess_phong_wireframe.hlsl";
#endif
    g_base_shader = tr::CreateShaderProgram(g_renderer,
                                            base_vs_file_path, "VSMain",
                                            base_ps_file_path, "PSMain");
    g_base_wireframe_shader = tr::CreateShaderProgram(g_renderer,
                                                      base_wireframe_vs_file_path, "VSMain",
                                                      base_wireframe_gs_file_path, "GSMain",
                                                      base_wireframe_ps_file_path, "PSMain");
    g_tess_shader = tr::CreateShaderProgram(g_renderer,
                                            tess_vs_file_path, "VSMain",
                                            tess_hs_file_path, "HSMain",
                                            tess_ds_file_path, "DSMain",
                                            tess_ps_file_path, "PSMain");
    g_tess_wireframe_shader = tr::CreateShaderProgram(g_renderer,
                                                      tess_wireframe_vs_file_path, "VSMain",
                                                      tess_wireframe_hs_file_path, "HSMain",
                                                      tess_wireframe_ds_file_path, "DSMain",
                                                      tess_wireframe_gs_file_path, "GSMain",
                                                      tess_wireframe_ps_file_path, "PSMain");

    // Entities
    {
      tr::EntityCreateInfo entity_create_info = {};

      // Base chess pieces
      entity_create_info.shader_program                   = g_base_shader;
      entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
      entity_create_info.render_target                    = g_renderer->swapchain_render_targets[0];
      entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_tri_list;
      entity_create_info.pipeline_settings.depth          = true;
      entity_create_info.pipeline_settings.cull_mode      = tr_cull_mode_back;
      g_chess_pieces_base.Create(g_renderer, entity_create_info);

      // Base wireframe chess pieces
      entity_create_info.shader_program                   = g_base_wireframe_shader;
      entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
      entity_create_info.render_target                    = g_renderer->swapchain_render_targets[0];
      entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_tri_list;
      entity_create_info.pipeline_settings.depth          = true;      
      g_chess_pieces_base_wireframe.Create(g_renderer, entity_create_info);

      // Tessellated chess pieces
      entity_create_info.shader_program                   = g_tess_shader;
      entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
      entity_create_info.render_target                    = g_renderer->swapchain_render_targets[0];
      entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_3_point_patch;
      entity_create_info.pipeline_settings.depth          = true;      
      entity_create_info.pipeline_settings.cull_mode      = tr_cull_mode_front;
      entity_create_info.pipeline_settings.tessellation_domain_origin = tr_tessellation_domain_origin_upper_left;
      g_chess_pieces_tess.Create(g_renderer, entity_create_info);

      // Tessellated wireframe chess pieces
      entity_create_info.shader_program                   = g_tess_wireframe_shader;
      entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
      entity_create_info.render_target                    = g_renderer->swapchain_render_targets[0];
      entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_3_point_patch;
      entity_create_info.pipeline_settings.depth          = true;
      g_chess_pieces_tess_wireframe.Create(g_renderer, entity_create_info);
    }

    // Vertex data
    {
      tr::fs::path file_path = k_asset_dir / "TriangleTessellation/models/chess_pieces_shared_normals.obj";
      bool result = tr::Mesh::Load(file_path, g_renderer, &g_vertex_buffer, &g_vertex_count);
      assert(result == true);

      // Base chess pieces
      g_chess_pieces_base.SetVertexBuffers(g_vertex_buffer, g_vertex_count);
      // Base wireframe chess pieces
      g_chess_pieces_base_wireframe.SetVertexBuffers(g_vertex_buffer, g_vertex_count);
      // Tessellated chess pieces
      g_chess_pieces_tess.SetVertexBuffers(g_vertex_buffer, g_vertex_count);
      // Tessellated wireframe chess pieces
      g_chess_pieces_tess_wireframe.SetVertexBuffers(g_vertex_buffer, g_vertex_count);
    }

    // Update descriptors
    {
      g_chess_pieces_base.UpdateGpuDescriptorSets();
      g_chess_pieces_base_wireframe.UpdateGpuDescriptorSets();
      g_chess_pieces_tess.UpdateGpuDescriptorSets();
      g_chess_pieces_tess_wireframe.UpdateGpuDescriptorSets();
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

    float3 eye = float3(0, 8, 8);
    float3 look_at = float3(0, 1, 0);
    g_camera.LookAt(eye, look_at);
    g_camera.Perspective(65.0f, (float)g_window_width / (float)g_window_height);

    
    float t = (float)glfwGetTime();
    float ry = t / 3.0f;

    // Update base transform and constant buffers
    {
      // View
      g_chess_pieces_base.SetView(g_camera);
      g_chess_pieces_base_wireframe.SetView(g_camera);
      // Transform
      tr::Transform transform;
      transform.Translate(0, 0, -3);
      transform.Rotate(0, ry, 0);
      g_chess_pieces_base.SetTransform(transform);
      g_chess_pieces_base_wireframe.SetTransform(transform);
      // Color
      g_chess_pieces_base.SetColor(float3(0.45f, 0.4f, 0.8f));
      // Constant buffers
      g_chess_pieces_base.UpdateGpuBuffers();
      g_chess_pieces_base_wireframe.UpdateGpuBuffers();
    }

    // Update tess transform and constant buffers
    {
      // View
      g_chess_pieces_tess.SetView(g_camera);
      g_chess_pieces_tess_wireframe.SetView(g_camera);
      // Transform
      tr::Transform transform;
      transform.Clear();
      transform.Translate(0, 0, 3);
      transform.Rotate(0, ry, 0);
      g_chess_pieces_tess.SetTransform(transform);
      g_chess_pieces_tess_wireframe.SetTransform(transform);
      // Color
      g_chess_pieces_tess.SetColor(float3(0.45f, 0.4f, 0.8f));
      // Tess factor
      g_chess_pieces_tess.GetTessParams().data.tess_factor = float4(float3(3.0f), 0);
      g_chess_pieces_tess_wireframe.GetTessParams().data.tess_factor = float4(float3(3.0f), 0);
      // Constant buffers
      g_chess_pieces_tess.UpdateGpuBuffers();
      g_chess_pieces_tess_wireframe.UpdateGpuBuffers();
    }

    tr_cmd* cmd = g_cmds[frameIdx];
    tr_begin_cmd(cmd);
    tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
    tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
    tr_cmd_set_viewport(cmd, 0, 0, (float)g_window_width, (float)g_window_height, 0.0f, 1.0f);
    tr_cmd_set_scissor(cmd, 0, 0, g_window_width, g_window_height);
    tr_cmd_begin_render(cmd, render_target);
    tr_cmd_clear_color_attachment(cmd, 0, &g_color_clear_value);
    tr_cmd_clear_depth_stencil_attachment(cmd, &g_depth_stencil_clear_value);
    tr_cmd_set_line_width(cmd, 1.0f);
    {
      // Draw base
      g_chess_pieces_base.Draw(cmd);
      // Draw base wireframe
      g_chess_pieces_base_wireframe.Draw(cmd);
      // Draw tess
      g_chess_pieces_tess.Draw(cmd);
      // Draw tess wireframe
      g_chess_pieces_tess_wireframe.Draw(cmd);
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
