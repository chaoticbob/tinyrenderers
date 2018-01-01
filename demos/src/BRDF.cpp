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
#include "mesh.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TR_IMGUI_GLFW_IMPLEMENTATION
#include "imgui_glfw.h"

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

tr_renderer*        g_renderer = nullptr;
tr_cmd_pool*        g_cmd_pool = nullptr;
tr_cmd**            g_cmds = nullptr;

tr::BRDFEntity      g_brdf_entity;
tr_shader_program*  g_brdf_shader = nullptr;

struct Geometry {
  tr_buffer*        vertex_buffer;
  uint32_t          vertex_count;
};

Geometry            g_shader_widget_geo = {};
Geometry            g_torus_geo = {};
Geometry            g_cylinder_geo = {};

uint32_t            g_window_width;
uint32_t            g_window_height;
uint64_t            g_frame_count = 0;

tr::Camera          g_camera;
tr::ViewParams      g_cpu_view_params;
tr_buffer*          g_gpu_view_params;

#define MAX_POINT_LIGHTS        16
#define MAX_SPOT_LIGHTS         1
#define MAX_DIRECTIONAL_LIGHTS  1
using LightingParams = tr::LightingParams<MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS, MAX_DIRECTIONAL_LIGHTS>;
LightingParams      g_cpu_lighting_params;
tr_buffer*          g_gpu_lighting_params;

tr_clear_value      g_color_clear_value = {};
tr_clear_value      g_depth_stencil_clear_value = {};

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
#elif defined(TINY_RENDERER_DX)
    settings.dx_shader_target               = tr_dx_shader_target_5_1;
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
    tr::fs::path brdf_vs_file_path       = k_asset_dir / "BRDF/shaders/brdf_shader.hlsl";
    tr::fs::path brdf_ps_file_path       = k_asset_dir / "BRDF/shaders/brdf_shader.hlsl";
#endif
    // Create Lambert shader
    g_brdf_shader = tr::CreateShaderProgram(g_renderer,
                                            brdf_vs_file_path, "vsmain",
                                            brdf_ps_file_path, "psmain");
    assert(g_brdf_shader != nullptr);
  }

  // View Params
  {
    tr_create_buffer(g_renderer, 
                     tr_buffer_usage_uniform_cbv,
                     g_cpu_view_params.GetDataSize(),
                     true,
                     &g_gpu_view_params);
    assert(g_gpu_view_params != nullptr);
    
    // Make writes go directly to the GPU buffer
    g_cpu_view_params.SetTarget(g_gpu_view_params->cpu_mapped_address);
  }

  // Lighting Params
  {
    tr_create_buffer(g_renderer, 
                     tr_buffer_usage_uniform_cbv,
                     g_cpu_lighting_params.GetDataSize(),
                     true,
                     &g_gpu_lighting_params);
    assert(g_gpu_lighting_params != nullptr);

    // Make writes go directly to the GPU buffer
    g_cpu_lighting_params.SetTarget(g_gpu_lighting_params->cpu_mapped_address);
  }

  // Entity
  {
    tr::EntityCreateInfo entity_create_info = {};
    entity_create_info.shader_program                   = g_brdf_shader;
    entity_create_info.gpu_view_params                  = g_gpu_view_params;
    entity_create_info.gpu_lighting_params              = g_gpu_lighting_params;
    entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
    entity_create_info.render_target                    = g_renderer->swapchain_render_targets[0];
    entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_tri_list;
    entity_create_info.pipeline_settings.depth          = true;
    entity_create_info.pipeline_settings.cull_mode      = tr_cull_mode_back;

    g_brdf_entity.Create(g_renderer, entity_create_info);
  }

  // Load geometry
  {
    // Shader widget
    tr::fs::path file_path = k_asset_dir / "Shaders/models/ShaderWidget.obj";
    bool result = tr::Mesh::Load(file_path, 
                                 g_renderer, 
                                 &g_shader_widget_geo.vertex_buffer, 
                                 &g_shader_widget_geo.vertex_count);
    assert(result == true);

    // Torus
    file_path = k_asset_dir / "Shaders/models/Torus.obj";
    result = tr::Mesh::Load(file_path, 
                            g_renderer, 
                            &g_torus_geo.vertex_buffer, 
                            &g_torus_geo.vertex_count);
    assert(result == true);

    // Cylinder
    file_path = k_asset_dir / "Shaders/models/Cylinder.obj";
    result = tr::Mesh::Load(file_path, 
                            g_renderer, 
                            &g_cylinder_geo.vertex_buffer, 
                            &g_cylinder_geo.vertex_count);
    assert(result == true);

    // Set default geo
    g_brdf_entity.SetVertexBuffers(g_shader_widget_geo.vertex_buffer,
                               g_shader_widget_geo.vertex_count);
  }

  // Update descriptors
  {
    g_brdf_entity.UpdateGpuDescriptorSets();
  }
}

void destroy_tiny_renderer()
{
    tr_destroy_renderer(g_renderer);
}

void draw_frame(GLFWwindow* p_window)
{
  int window_width = 0;
  int window_height = 0;
  glfwGetWindowSize(p_window, &window_width, &window_height);
  int framebuffer_width = 0;
  int framebuffer_height = 0;
  glfwGetFramebufferSize(p_window, &framebuffer_width, &framebuffer_height);

  static float prevTime = 0;
  float curTime = (float)glfwGetTime();
  float dt = curTime - prevTime;
  prevTime = curTime;

  float content_scale = 1.5f;
  ImGuiIO& io = ImGui::GetIO();
  io.DisplaySize = ImVec2((float)window_width, (float)window_height);
  io.DisplayFramebufferScale = ImVec2((float)framebuffer_width / (float)window_width,
                                      (float)framebuffer_height / (float)window_height);
  io.FontGlobalScale = content_scale;

  io.DeltaTime = (float)dt;
  ImGui::NewFrame();

  uint32_t frameIdx = g_frame_count % g_renderer->settings.swapchain.image_count;

  tr_fence* image_acquired_fence = g_renderer->image_acquired_fences[frameIdx];
  tr_semaphore* image_acquired_semaphore = g_renderer->image_acquired_semaphores[frameIdx];
  tr_semaphore* render_complete_semaphores = g_renderer->render_complete_semaphores[frameIdx];

  tr_acquire_next_image(g_renderer, image_acquired_semaphore, image_acquired_fence);

  uint32_t swapchain_image_index = g_renderer->swapchain_image_index;
  tr_render_target* render_target = g_renderer->swapchain_render_targets[swapchain_image_index];

  // Time
  float t = curTime;

  ImGui::Begin("Params", nullptr, ImVec2(300 * content_scale, 400 * content_scale));
  // Constant buffers
  {
    ImGui::SameLine();
    if (ImGui::Button("Widget")) {
      g_brdf_entity.SetVertexBuffers(g_shader_widget_geo.vertex_buffer,
                                     g_shader_widget_geo.vertex_count);
    }
    ImGui::SameLine();
    if (ImGui::Button("Torus")) {
      g_brdf_entity.SetVertexBuffers(g_torus_geo.vertex_buffer,
                                     g_torus_geo.vertex_count);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cylinder")) {
      g_brdf_entity.SetVertexBuffers(g_cylinder_geo.vertex_buffer,
                                     g_cylinder_geo.vertex_count);
    }

    // Camera
    {
      float3 eye = float3(0, 2, 5);
      float3 look_at = float3(0, 1, 0);
      g_camera.LookAt(eye, look_at);
      g_camera.Perspective(45.0f, (float)g_window_width / (float)g_window_height);

      g_cpu_view_params.SetView(g_camera);
      g_brdf_entity.ApplyView(g_camera);
    }

    // Transforms
    {
      tr::Transform transform;

      static float rot[3] = { 0, 1, 0 };
      ImGui::SliderFloat3("Rotation", rot, 0.0f, 2.0f * 3.141592f);

      // Lambert
      transform.Clear();
      transform.Rotate(rot[0], rot[1], rot[2]);
      transform.Translate(0, 0, 0);
      g_brdf_entity.SetTransform(transform);
    }

    // Material
    if (ImGui::CollapsingHeader("BRDF", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(&g_brdf_shader);
      ImGui::ColorEdit3("BaseColor", (float*)g_brdf_entity.GetMaterialParams().GetData().BaseColor.value_ptr());
      ImGui::SliderFloat("Metallic", g_brdf_entity.GetMaterialParams().GetData().Metallic.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("Subsurface", g_brdf_entity.GetMaterialParams().GetData().Subsurface.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("Specular", g_brdf_entity.GetMaterialParams().GetData().Specular.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("Roughness", g_brdf_entity.GetMaterialParams().GetData().Roughness.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("SpecularTint", g_brdf_entity.GetMaterialParams().GetData().SpecularTint.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("Anisotropic", g_brdf_entity.GetMaterialParams().GetData().Anisotropic.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("Sheen", g_brdf_entity.GetMaterialParams().GetData().Sheen.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("SheenTint", g_brdf_entity.GetMaterialParams().GetData().SheenTint.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("ClearCoat", g_brdf_entity.GetMaterialParams().GetData().ClearCoat.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("ClearCoatGloss", g_brdf_entity.GetMaterialParams().GetData().ClearCoatGloss.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("kA", g_brdf_entity.GetMaterialParams().GetData().kA.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("kD", g_brdf_entity.GetMaterialParams().GetData().kD.value_ptr(), 0.0, 1.0);
      ImGui::SliderFloat("kS", g_brdf_entity.GetMaterialParams().GetData().kS.value_ptr(), 0.0, 1.0);

      ImGui::PopID();
    }

    // Lights
    {
    }

    // Update GPU buffers
    {
      g_brdf_entity.UpdateGpuBuffers();
    }
  }
  ImGui::End();

  tr_cmd* cmd = g_cmds[frameIdx];
  tr_begin_cmd(cmd);
  tr_cmd_render_target_transition(cmd, render_target, tr_texture_usage_present, tr_texture_usage_color_attachment); 
  tr_cmd_depth_stencil_transition(cmd, render_target, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
  tr_cmd_set_viewport(cmd, 0, 0, (float)g_window_width, (float)g_window_height, 0.0f, 1.0f);
  tr_cmd_set_scissor(cmd, 0, 0, g_window_width, g_window_height);
  tr_cmd_begin_render(cmd, render_target);
  tr_cmd_clear_color_attachment(cmd, 0, &g_color_clear_value);
  tr_cmd_clear_depth_stencil_attachment(cmd, &g_depth_stencil_clear_value);
  // Draw Blinn-Phong
  {
    g_brdf_entity.Draw(cmd);
  }

  // Draw IMGUI
  {
    tr::imgui_glfw_set_draw_cmd(cmd);
    ImGui::Render();
    tr::imgui_glfw_clear_draw_cmd();
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
    GLFWwindow* window = glfwCreateWindow(1024, 1024, k_app_name, NULL, NULL);
    
    glfwSetKeyCallback(window, tr::imgui_glfw_keyboard);
    glfwSetMouseButtonCallback(window, tr::imgui_glfw_mouse_button);
    glfwSetCursorPosCallback(window, tr::imgui_glfw_mouse_cursor);
    glfwSetScrollCallback(window, tr::imgui_glfw_scroll);
    
    init_tiny_renderer(window);
    tr::imgui_glfw_init(window, g_renderer);

    while (! glfwWindowShouldClose(window)) {
        draw_frame(window);
        glfwPollEvents();
    }
    
    destroy_tiny_renderer();

    glfwDestroyWindow(window);
    glfwTerminate();
    return EXIT_SUCCESS;
}
