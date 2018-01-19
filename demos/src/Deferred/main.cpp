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
#include <random>
#include <sstream>
#include <vector>

#define TINY_RENDERER_IMPLEMENTATION
#if defined(TINY_RENDERER_DX)
    #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
    #include "tinyvk.h"
#endif

#define TINYOBJLOADER_IMPLEMENTATION
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

#include "DeferredRenderer.h"
#include "Scene.h"

#define ENABLE_UI

const char*           k_app_name = "Deferred Rendering";
const uint32_t        k_image_count = 1;
#if defined(__linux__)
const tr::fs::path    k_asset_dir = "../demos/assets/";
#elif defined(_WIN32)
const tr::fs::path    k_asset_dir = "../../demos/assets/";
#endif

tr_renderer*        g_renderer = nullptr;
tr_cmd_pool*        g_cmd_pool = nullptr;
tr_cmd**            g_cmds = nullptr;

uint32_t            g_window_width;
uint32_t            g_window_height;
uint64_t            g_frame_count = 0;

tr::Camera          g_camera;
tr::ViewParams      g_cpu_view_params;
tr_buffer*          g_gpu_view_params;

tr_clear_value      g_rtv_clear_value = {};
tr_clear_value      g_dsv_clear_value = {};

DeferredRenderer        g_deferred_renderer;
DeferredTubeWorldScene  g_tube_world;

bool                g_mouse_captured = true;
bool                g_mouse_move_camera = false;
float2              g_prev_mouse_pos;
bool                g_ui_active = true;

bool                g_key_down[GLFW_KEY_LAST + 1] = {};

//int32_t             g_debug_gbuffer_index = -1;

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

float Rand() 
{
  static std::mt19937_64                        s_rng;
  static std::uniform_real_distribution<float>  s_distrib(0, 1);

  float value = s_distrib(s_rng);
  return value;
}

float3 Rand3()
{
  float3 value(Rand(), Rand(), Rand());
  return value;
}

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

    g_rtv_clear_value = { 0.1f, 0.1f, 0.1f, 0.1f };
    g_dsv_clear_value.depth = 1.0f;
    g_dsv_clear_value.stencil = 255;

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
    settings.swapchain.sample_count         = tr_sample_count_1;
    settings.swapchain.rtv_format           = tr_format_r8g8b8a8_unorm;
    settings.swapchain.dsv_format           = tr_format_undefined;
    settings.swapchain.rtv_clear_value      = g_rtv_clear_value;
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

  // Deferred renderer
  {
    const uint32_t k_rtv_count        = 4;
    const uint32_t k_frame_count      = 1;
    const bool k_enable_debug_colors  = false;
    g_deferred_renderer.Initialize(g_renderer, k_asset_dir / "Deferred", g_window_width, g_window_height, k_rtv_count, k_frame_count, k_enable_debug_colors);
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

  // Initial view 
  {
    float3 eye = float3(-13, 5, 14);
    float3 look_at = float3(-13, 5, 0);
    g_camera.LookAt(eye, look_at);
    g_camera.Perspective(75.0f, (float)g_window_width / (float)g_window_height, 1.0f, 100000.0f);
  }

  // Scene
  {
    g_tube_world.Initialize(g_renderer, k_asset_dir / "Deferred", g_gpu_view_params, g_deferred_renderer.GetGBufferRenderPass(0));
    g_tube_world.UpdateDescriptorSets();
  }

  // Initial lighting values
  {
    auto& params = g_deferred_renderer.GetLightingParams();
    params.ApplyView(g_camera);

    auto& data = params.GetData();    
    data.AmbientLight.Intensity           = 0.1f;

    {
      float radius = 54;
      const uint32_t k_num_lights = 20;
      float dt = 1.0f / (float)k_num_lights;
      for (uint32_t i = 0; i < k_num_lights; ++i) {
        float t = i * dt * 2.0f * 3.141592f;
        float x = cos(t) * radius;
        float y = 30.0f;
        float z = sin(t) * radius;
        data.PointLights[i].Position  = float3(x, y, z);
        data.PointLights[i].Color     = float3(0.4f, 0.4f, 0.4f) + 0.8f * Rand3();
        data.PointLights[i].Intensity = 1.0;
        data.PointLights[i].FallOff   = 50.0f;
      }

      radius = 175;
      for (uint32_t i = 0; i < k_num_lights; ++i) {
        float t = i * dt * 2.0f * 3.141592f;
        float x = cos(t) * radius;
        float y = 40.0f;
        float z = sin(t) * radius;
        data.PointLights[i + (1 * k_num_lights)].Position  = float3(x, y, z);
        data.PointLights[i + (1 * k_num_lights)].Color     = float3(0.6f, 0.6f, 0.6f) + 0.7f * Rand3();
        data.PointLights[i + (1 * k_num_lights)].Intensity = 1.0f;
        data.PointLights[i + (1 * k_num_lights)].FallOff   = 80;
      }
    }

    // -Y
    data.DirectionalLights[0].Direction   = float3(-0.1f, -1, 0.2);
    data.DirectionalLights[0].Intensity   = 0.1f;
    // +Y
    data.DirectionalLights[1].Direction   = float3(0, 1, 0);
    data.DirectionalLights[1].Intensity   = 0.0f;
    // -X
    data.DirectionalLights[2].Direction   = float3(-1, 0, 0);
    data.DirectionalLights[2].Intensity   = 0.0f;
    // +X
    data.DirectionalLights[3].Direction   = float3(1, 0, 0);
    data.DirectionalLights[3].Intensity   = 0.0f;
    // -Z
    data.DirectionalLights[4].Direction   = float3(0, 0, -1);
    data.DirectionalLights[4].Intensity   = 0.0f;
    // +Z
    data.DirectionalLights[5].Direction   = float3(0, 0, 1);
    data.DirectionalLights[5].Intensity   = 0.0f;
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

  static float prevTime = (float)glfwGetTime();
  float curTime = (float)glfwGetTime();
  float dt = curTime - prevTime;
  prevTime = curTime;
  
  uint32_t frameIdx = g_frame_count % g_renderer->settings.swapchain.image_count;
  tr_fence* image_acquired_fence = g_renderer->image_acquired_fences[frameIdx];
  tr_semaphore* image_acquired_semaphore = g_renderer->image_acquired_semaphores[frameIdx];
  tr_semaphore* render_complete_semaphores = g_renderer->render_complete_semaphores[frameIdx];

  tr_acquire_next_image(g_renderer, image_acquired_semaphore, image_acquired_fence);

  uint32_t swapchain_image_index = g_renderer->swapchain_image_index;
  tr_render_pass* render_pass = g_renderer->swapchain_render_passes[swapchain_image_index];
   
#if defined(ENABLE_UI)
  // UI Start
  {
    float content_scale_x = 1.0f;
    float content_scale_y = 1.0f;
    glfwGetWindowContentScale(p_window, &content_scale_x, &content_scale_y);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)window_width, (float)window_height);
    io.DisplayFramebufferScale = ImVec2((float)framebuffer_width / (float)window_width,
                                        (float)framebuffer_height / (float)window_height);
    io.FontGlobalScale = std::max(content_scale_x, content_scale_y);

    io.DeltaTime = dt;
    ImGui::NewFrame();

    ImGui::Begin("Params", &g_ui_active, ImVec2(300 * content_scale_x, 400 * content_scale_y));
  }

  // Build UI
  {
    // FPS
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::Text("Visible objects: %d/%d", g_tube_world.GetVisibleObjectCount(), g_tube_world.GetTotalObjectCount());
    ImGui::Text("Delta: %f, %f", g_camera.GetForwardDelta(), g_camera.GetRightDelta());

    g_deferred_renderer.BuildDebugUI();
    //g_tube_world.BuildDebugUi();
  }
#endif // defined(ENABLE_UI)

  // Update camera
  {
    g_camera.MoveClearAccel();
    if (g_key_down[GLFW_KEY_W]) {
      g_camera.MoveForward();
    }
    if (g_key_down[GLFW_KEY_A]) {
      g_camera.MoveLeft();
    }
    if (g_key_down[GLFW_KEY_S]) {
      g_camera.MoveBackward();
    }
    if (g_key_down[GLFW_KEY_D]) {
      g_camera.MoveRight();
    }
    g_camera.MoveUpdate(dt);
  }

  // Update view params
  {
    g_cpu_view_params.SetView(g_camera);
    g_deferred_renderer.ApplyView(g_camera);
  }

  // Update scene objects
  {
    g_tube_world.SetViewTransform(g_camera);
    g_tube_world.FinalizeTransforms();
  }

  // Cull objects
  {
    float4x4 model_view_matrix = g_camera.GetViewMatrix();
    float4 p1 = float4(g_camera.GetEyePosition(), 1);
    float4 p2 = model_view_matrix* p1;

    g_tube_world.Cull(g_camera);
  }

  // Update lighting
  {
    auto& lighting_params = g_deferred_renderer.GetLightingParams();
    lighting_params.ApplyView(g_camera);

#if defined(ENABLE_UI)
    auto& data = lighting_params.GetData();
    if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(&data.AmbientLight);
      ImGui::SliderFloat("Intensity", data.AmbientLight.Intensity.value_ptr(), 0.0f, 1.0f);
      ImGui::PopID();
    }

    for (uint32_t i = 0; i < DEFERRED_MAX_DIRECTIONAL_LIGHTS; ++i) {
      char name[256] = {};
      sprintf(name, "Directional %d", i);
      if (ImGui::CollapsingHeader(name, ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::PushID(&data.DirectionalLights[i]);
        ImGui::SliderFloat("Intensity", data.DirectionalLights[i].Intensity.value_ptr(), 0.0f, 1.0f);
        ImGui::PopID();
      }
    }
#endif // defined(ENABLE_UI)
  }

  
  tr_cmd* cmd = g_cmds[frameIdx];
  tr_begin_cmd(cmd);
  // Constant buffers
  {
    //g_deferred_entity.UpdateGpuBuffers();
    g_deferred_renderer.UpdateGpuBuffers(0, cmd);
    g_tube_world.UpdateGpuBuffers(cmd);
  }

  // GBuffer
  {
    g_deferred_renderer.BeginGBufferPass(cmd, 0);
    {
      //g_deferred_entity.Draw(cmd);
      g_tube_world.Draw(cmd);
    }
    g_deferred_renderer.EndGBufferPass(cmd, 0);
  }

  // Backface
  {
  }

  // Lighting/Composite or Debug
  if (g_deferred_renderer.GetDebugParams().GetData().GBufferElement == 0) {
    // Lighting
    {
      g_deferred_renderer.Lighting(cmd, 0);
    }
    // Composite
    {
      tr_cmd_render_pass_rtv_transition(cmd, render_pass, tr_texture_usage_present, tr_texture_usage_transfer_dst);
      g_deferred_renderer.Composite(cmd, 0, render_pass->rtv[0]);
      tr_cmd_render_pass_rtv_transition(cmd, render_pass, tr_texture_usage_transfer_dst, tr_texture_usage_color_attachment); 
    }
  }
  else {
    tr_cmd_render_pass_rtv_transition(cmd, render_pass, tr_texture_usage_present, tr_texture_usage_transfer_dst);
    g_deferred_renderer.DebugShowGBufferElement(cmd, 0, render_pass->rtv[0]);
    tr_cmd_render_pass_rtv_transition(cmd, render_pass, tr_texture_usage_transfer_dst, tr_texture_usage_color_attachment); 
  }

#if defined(ENABLE_UI)
  // UI End
  {
    ImGui::End();

    tr_cmd_begin_render(cmd, render_pass);
    {
      tr::imgui_glfw_set_draw_cmd(cmd);
      ImGui::Render();
      tr::imgui_glfw_clear_draw_cmd();
    }
    tr_cmd_end_render(cmd);

    tr_cmd_render_pass_rtv_transition(cmd, render_pass, tr_texture_usage_color_attachment, tr_texture_usage_present); 
  }
#endif // defined(ENABLE_UI)

  tr_end_cmd(cmd);

  tr_queue_submit(g_renderer->graphics_queue, 1, &cmd, 1, &image_acquired_semaphore, 1, &render_complete_semaphores);
  tr_queue_present(g_renderer->present_queue, 1, &render_complete_semaphores);

  tr_queue_wait_idle(g_renderer->graphics_queue);
}

void keyboard(GLFWwindow* p_window, int key, int scancode, int action, int mods)
{
  auto& gbuffer_element = g_deferred_renderer.GetDebugParams().GetData().GBufferElement;

  if (action == GLFW_PRESS) {
    switch (key) {
      default: break;

      case GLFW_KEY_1: {
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION;
      }
      break;

      case GLFW_KEY_2:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_NORMAL) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_NORMAL;   
      }
      break;

      case GLFW_KEY_3:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_ALBEDO) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_ALBEDO;    
      }
      break;

      case GLFW_KEY_4:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_ROUGHNESS) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_ROUGHNESS;      
      }
      break;

      case GLFW_KEY_5:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_METALLIC) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_METALLIC;
      }
      break;

      case GLFW_KEY_6:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_SPECULAR) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_SPECULAR;
      }
      break;

      case GLFW_KEY_7:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL;
      }
      break;

      case GLFW_KEY_8:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL_POWER) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL_POWER;
      }
      break;

      case GLFW_KEY_9:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_DEPTH) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_DEPTH;
      }
      break;

      case GLFW_KEY_0:{
        gbuffer_element = (gbuffer_element == DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION_FROM_DEPTH) ? 0 : DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION_FROM_DEPTH;
      }
      break;

      case GLFW_KEY_TAB:{
        g_mouse_captured = !g_mouse_captured;
        if (g_mouse_captured) {
          glfwSetInputMode(p_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
          g_mouse_move_camera = false;
        }
        else {
          glfwSetInputMode(p_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        g_ui_active = !g_mouse_captured;
      }
      break;

      case GLFW_KEY_ESCAPE:{
        glfwSetWindowShouldClose(p_window, 1);
      }
      break;
    }
  }

  if (g_mouse_captured) {
    switch (key) {
      case GLFW_KEY_W:
      case GLFW_KEY_A:
      case GLFW_KEY_S:
      case GLFW_KEY_D: {
        if (action == GLFW_PRESS) {
          g_key_down[key] = true;
        }
        else if (action == GLFW_RELEASE) {
          g_key_down[key] = false;
        }
      }
      break;
    }
  }
  
  if (!g_mouse_captured) {
    tr::imgui_glfw_keyboard(p_window, key, scancode, action, mods); 
  }
}

void mouse_button(GLFWwindow* p_window, int button, int action, int mods)
{
  if (g_mouse_captured) {    
  }
  else {
    tr::imgui_glfw_mouse_button(p_window, button, action, mods);
  }
}

void mouse_cursor(GLFWwindow* window, double x_pos, double y_pos)
{
  if (g_mouse_captured) {
    if (g_mouse_move_camera) {
      float dx = (float)x_pos - g_prev_mouse_pos.x;
      float dy = (float)y_pos - g_prev_mouse_pos.y;
      g_camera.MouseMove(dx, dy);

      g_prev_mouse_pos = float2(x_pos, y_pos);
    }
    else {
      g_prev_mouse_pos = float2(x_pos, y_pos);
      g_mouse_move_camera = true;
    }
  }
  else {
    tr::imgui_glfw_mouse_cursor(window, x_pos, y_pos);
  }
}

int main(int argc, char **argv)
{
  glfwSetErrorCallback(app_glfw_error);
  if (! glfwInit()) {
      exit(EXIT_FAILURE);
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window = glfwCreateWindow(1920, 1080, k_app_name, NULL, NULL);
 
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  glfwSetKeyCallback(window, keyboard);
  glfwSetMouseButtonCallback(window, mouse_button);
  glfwSetCursorPosCallback(window, mouse_cursor);
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
