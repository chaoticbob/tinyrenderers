
#ifndef TR_IMGUI_GLFW_H
#define TR_IMGUI_GLFW_H

#ifndef __cplusplus
  #error "C++ is required"
#endif

#include "GLFW/glfw3.h"
#if defined(__linux__)
  #define GLFW_EXPOSE_NATIVE_X11
#elif defined(_WIN32)
  #define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3native.h"

#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
#endif

#include <imgui.h>

namespace tr {

void imgui_glfw_init(GLFWwindow* p_window, tr_renderer* p_renderer);
void imgui_glfw_keyboard(GLFWwindow* p_window, int key, int scancode, int action, int mods);
void imgui_glfw_mouse_button(GLFWwindow* p_window, int button, int action, int mods);
void imgui_glfw_mouse_cursor(GLFWwindow* window, double x_pos, double y_pos);
void imgui_glfw_scroll(GLFWwindow* p_window, double x_offset, double y_offset);
void imgui_glfw_set_draw_cmd(tr_cmd* p_cmd);
void imgui_glfw_clear_draw_cmd();

#if defined(TR_IMGUI_GLFW_IMPLEMENTATION)

const char* g_vs_shader_hlsl = R"hlsl(
  cbuffer ShaderParams : register(b0) {
    float4x4 ProjectionMatrix;
  };

  struct VSOutput {
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR;
  };

  VSOutput vsmain(float2 Position : POSITION, float2 TexCoord : TEXCOORD, float4 Color : COLOR) {
    VSOutput output;
    output.Position = mul(ProjectionMatrix, float4(Position, 0, 1));
    output.TexCoord = TexCoord; 
    output.Color    = Color;
    return output;
  }
)hlsl";

const char* g_ps_shader_hlsl = R"hlsl(
  Texture2D     Texture0 : register(t1);
  SamplerState  Sampler0 : register(s2);

  struct PSInput {
    float4 Position : SV_Position;
    float2 TexCoord : TEXCOORD0;
    float4 Color    : COLOR;
  };

  float4 psmain(PSInput input) : SV_Target {
    float4 result = input.Color * Texture0.Sample(Sampler0, input.TexCoord);
    return result;
  }
)hlsl";

struct ImGlfwApp {
  tr_renderer*        renderer;

  tr_descriptor_set*  descriptor_set;
  tr_shader_program*  shader_program;
  tr_pipeline*        pipeline;

  tr_buffer*          constant_buffer;
  tr_texture*         font_texture;
  tr_sampler*         sampler;

  size_t              index_data_size;
  tr_buffer*          index_buffer;
  size_t              vertex_data_size;
  tr_buffer*          vertex_buffer;

  tr_cmd*             draw_cmd;
};

static ImGlfwApp g_app = {};

static void imgui_glfw_render_draw_lists(ImDrawData* p_draw_data);
static const char* imgui_glfw_get_clipboard_text(void* p_user_data);
static void imgui_glfw_set_clipboard_text(void* p_user_data, const char* p_text);

void imgui_glfw_init(GLFWwindow* p_window, tr_renderer* p_renderer)
{
  assert(p_window != nullptr);
  assert(p_renderer != nullptr);

  g_app.renderer = p_renderer;

  ImGuiIO& io = ImGui::GetIO();
  
  // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array.
  {    
    io.KeyMap[ImGuiKey_Tab]         = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]   = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow]  = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]     = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow]   = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp]      = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown]    = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home]        = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End]         = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Delete]      = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace]   = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Enter]       = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape]      = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_A]           = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C]           = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V]           = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X]           = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y]           = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z]           = GLFW_KEY_Z;
  }

  // Callbacks
  {
    io.RenderDrawListsFn = imgui_glfw_render_draw_lists;
    io.GetClipboardTextFn = imgui_glfw_get_clipboard_text;
    io.SetClipboardTextFn = imgui_glfw_set_clipboard_text;
    io.ClipboardUserData = p_window;
  }

  // Window handle
  {
#ifdef _WIN32
    io.ImeWindowHandle = glfwGetWin32Window(p_window);
#endif
  }

  // Shader
  {
#if defined(TINY_RENDERER_VK)
    tr_create_shader_program(p_renderer,
                             sizeof(g_vert_shader_spirv), g_vert_shader_spirv, "main",
                             sizeof(g_frag_shader_spirv), g_frag_shader_spirv, "main",
                             &g_app.shader_program);
#elif defined(TINY_RENDERER_DX)
    tr_create_shader_program(g_app.renderer,
                             (uint32_t)strlen(g_vs_shader_hlsl), g_vs_shader_hlsl, "vsmain",
                             (uint32_t)strlen(g_ps_shader_hlsl), g_ps_shader_hlsl, "psmain",
                             &g_app.shader_program);
#endif
    assert(g_app.shader_program != nullptr);
  }

  // Descriptors
  {
    std::vector<tr_descriptor> descriptors(3);
    descriptors[0].type          = tr_descriptor_type_uniform_buffer_cbv;
    descriptors[0].count         = 1;
    descriptors[0].binding       = 0;
    descriptors[0].shader_stages = tr_shader_stage_vert;
    descriptors[1].type          = tr_descriptor_type_texture_srv;
    descriptors[1].count         = 1;
    descriptors[1].binding       = 1;
    descriptors[1].shader_stages = tr_shader_stage_frag;
    descriptors[2].type          = tr_descriptor_type_sampler;
    descriptors[2].count         = 1;
    descriptors[2].binding       = 2;
    descriptors[2].shader_stages = tr_shader_stage_frag;
    tr_create_descriptor_set(g_app.renderer, (uint32_t)descriptors.size(), descriptors.data(), &g_app.descriptor_set);
  }

  // Pipeline
  {
    tr_vertex_layout vertex_layout = {};
    vertex_layout.attrib_count = 3;
    vertex_layout.attribs[0].semantic = tr_semantic_position;
    vertex_layout.attribs[0].format   = tr_format_r32g32_float;
    vertex_layout.attribs[0].binding  = 0;
    vertex_layout.attribs[0].location = 0;
    vertex_layout.attribs[0].offset   = 0;
    vertex_layout.attribs[1].semantic = tr_semantic_texcoord0;
    vertex_layout.attribs[1].format   = tr_format_r32g32_float;
    vertex_layout.attribs[1].binding  = 0;
    vertex_layout.attribs[1].location = 1;
    vertex_layout.attribs[1].offset   = tr_util_format_stride(tr_format_r32g32_float);
    vertex_layout.attribs[2].semantic = tr_semantic_color;
    vertex_layout.attribs[2].format   = tr_format_r8g8b8a8_unorm;
    vertex_layout.attribs[2].binding  = 0;
    vertex_layout.attribs[2].location = 2;
    vertex_layout.attribs[2].offset   = tr_util_format_stride(tr_format_r32g32_float) + tr_util_format_stride(tr_format_r32g32_float);

    tr_pipeline_settings pipeline_settings = { tr_primitive_topo_tri_list };
    pipeline_settings.color_blend_modes[0] = tr_blend_mode_alpha;

    tr_create_pipeline(g_app.renderer, 
                       g_app.shader_program, 
                       &vertex_layout,
                       g_app.descriptor_set, 
                       g_app.renderer->swapchain_render_passes[0], 
                       &pipeline_settings, 
                       &g_app.pipeline);
  }

  // Constant buffer
  {
    tr_create_uniform_buffer(p_renderer,
                             16 * sizeof(float),
                             true,
                             &g_app.constant_buffer);
    assert(g_app.constant_buffer != nullptr);
  }

  // Font texture
  {
    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    int row_stride = 4 * width;
    
    tr_create_texture_2d(g_app.renderer, 
                         width, 
                         height, 
                         tr_sample_count_1, 
                         tr_format_r8g8b8a8_unorm, 
                         1, 
                         nullptr, 
                         false, 
                         tr_texture_usage_sampled_image,  // usage
                         tr_texture_usage_sampled_image,  // initial state
                         &g_app.font_texture);
    
    tr_util_update_texture_uint8(g_app.renderer->graphics_queue, 
                                 width, 
                                 height, 
                                 row_stride, 
                                 pixels, 
                                 4, 
                                 g_app.font_texture, 
                                 nullptr, 
                                 nullptr);
  }

  // Sampler
  {
    tr_sampler_settings sampler_settings = {};
    sampler_settings.address_mode_u = tr_texture_address_mode_repeat;
    sampler_settings.address_mode_v = tr_texture_address_mode_repeat;
    sampler_settings.address_mode_w = tr_texture_address_mode_repeat;

    tr_create_sampler(p_renderer, &sampler_settings, &g_app.sampler);
    assert(g_app.sampler != nullptr);
  }

  // Update descriptors
  {
    g_app.descriptor_set->descriptors[0].uniform_buffers[0]  = g_app.constant_buffer;
    g_app.descriptor_set->descriptors[1].textures[0] = g_app.font_texture;
    g_app.descriptor_set->descriptors[2].samplers[0] = g_app.sampler;
    tr_update_descriptor_set(p_renderer, g_app.descriptor_set);
  }
}

void imgui_glfw_keyboard(GLFWwindow* p_window, int key, int scancode, int action, int mods)
{
  ImGuiIO& io = ImGui::GetIO();

  if (action == GLFW_PRESS) {
    io.KeysDown[key] = true;
  }
  if (action == GLFW_RELEASE) {
      io.KeysDown[key] = false;
  }

  io.KeyCtrl  = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
  io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT]   || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
  io.KeyAlt   = io.KeysDown[GLFW_KEY_LEFT_ALT]     || io.KeysDown[GLFW_KEY_RIGHT_ALT];
  io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER]   || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void imgui_glfw_mouse_button(GLFWwindow* p_window, int button, int action, int mods)
{
  ImGuiIO& io = ImGui::GetIO();

  if ((button >= 0) && (button < 3)) {
    io.MouseDown[button] = (action == GLFW_PRESS ? true : false);
  }
}


void imgui_glfw_mouse_cursor(GLFWwindow* window, double x_pos, double y_pos)
{
  ImGuiIO& io = ImGui::GetIO();

  io.MousePos = ImVec2((float)x_pos, (float)y_pos);
}

void imgui_glfw_scroll(GLFWwindow* p_window, double x_offset, double y_offset)
{
  ImGuiIO& io = ImGui::GetIO();

  io.MouseWheel += (float)y_offset;
}

void imgui_glfw_set_draw_cmd(tr_cmd* p_cmd)
{
  g_app.draw_cmd = p_cmd;
}

void imgui_glfw_clear_draw_cmd()
{
  g_app.draw_cmd = nullptr;
}

static void imgui_glfw_render_draw_lists(ImDrawData* p_draw_data)
{
  // Constant buffer
  {
    float L = 0.0f;
    float R = ImGui::GetIO().DisplaySize.x;
    float B = ImGui::GetIO().DisplaySize.y;
    float T = 0.0f;
    float mvp[4][4] =
    {
        { 2.0f/(R-L),   0.0f,           0.0f,       0.0f },
        { 0.0f,         2.0f/(T-B),     0.0f,       0.0f },
        { 0.0f,         0.0f,           0.5f,       0.0f },
        { (R+L)/(L-R),  (T+B)/(B-T),    0.5f,       1.0f },
    };
    memcpy(g_app.constant_buffer->cpu_mapped_address, mvp, 16 * sizeof(float));
  }

  // Index buffer
  {
    size_t index_data_size = p_draw_data->TotalIdxCount * sizeof(ImDrawIdx);
    if ((g_app.index_buffer == nullptr) || (index_data_size > g_app.index_data_size)) {
      if (g_app.index_buffer != nullptr) {
        tr_destroy_buffer(g_app.renderer, g_app.index_buffer);
      }

      tr_create_index_buffer(g_app.renderer,
                             index_data_size,
                             true,
                             sizeof(ImDrawIdx) == 2 ? tr_index_type_uint16 : tr_index_type_uint32,
                             &g_app.index_buffer);
      assert(g_app.index_buffer != nullptr);

      g_app.index_data_size = index_data_size;
    }
  }

  // Vertex buffer
  {
    size_t vertex_data_size = p_draw_data->TotalVtxCount * sizeof(ImDrawVert);
    if ((g_app.vertex_buffer == nullptr) || (vertex_data_size > g_app.vertex_data_size)) {
      if (g_app.vertex_buffer != nullptr) {
        tr_destroy_buffer(g_app.renderer, g_app.vertex_buffer);
      }
    
      uint32_t stride = tr_util_format_stride(tr_format_r32g32_float) 
                      + tr_util_format_stride(tr_format_r32g32_float)
                      + tr_util_format_stride(tr_format_r8g8b8a8_unorm);
      tr_create_vertex_buffer(g_app.renderer,
                              vertex_data_size,
                              true,
                              stride,
                              &g_app.vertex_buffer);
      assert(g_app.vertex_buffer != nullptr);

      g_app.vertex_data_size = vertex_data_size;
    }
  }

  // Update index and vertex buffers
  {
    ImDrawIdx* idx_dst = (ImDrawIdx*)g_app.index_buffer->cpu_mapped_address;
    ImDrawVert* vtx_dst = (ImDrawVert*)g_app.vertex_buffer->cpu_mapped_address;
    for (int n = 0; n < p_draw_data->CmdListsCount; ++n) {
      const ImDrawList* cmd_list = p_draw_data->CmdLists[n];
      memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
      memcpy(vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert));
      idx_dst += cmd_list->IdxBuffer.Size;
      vtx_dst += cmd_list->VtxBuffer.Size;
    }
  }

  // Draw
  {
    assert(g_app.draw_cmd != nullptr);  
    tr_cmd_bind_pipeline(g_app.draw_cmd, g_app.pipeline);
    tr_cmd_bind_descriptor_sets(g_app.draw_cmd, g_app.pipeline, g_app.descriptor_set);
    tr_cmd_bind_index_buffer(g_app.draw_cmd, g_app.index_buffer);
    tr_cmd_bind_vertex_buffers(g_app.draw_cmd, 1, &g_app.vertex_buffer);

    // Render the command lists:
    int vtx_offset = 0;
    int idx_offset = 0;
    for (int n = 0; n < p_draw_data->CmdListsCount; ++n) {
      const ImDrawList* cmd_list = p_draw_data->CmdLists[n];
      for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; ++cmd_i) {
        const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];

        uint32_t x = (int32_t)(pcmd->ClipRect.x) > 0 ? (int32_t)(pcmd->ClipRect.x) : 0;
        uint32_t y = (int32_t)(pcmd->ClipRect.y) > 0 ? (int32_t)(pcmd->ClipRect.y) : 0;
        uint32_t width = (uint32_t)(pcmd->ClipRect.z - pcmd->ClipRect.x);
        uint32_t height = (uint32_t)(pcmd->ClipRect.w - pcmd->ClipRect.y + 1); // FIXME: Why +1 here?
        tr_cmd_set_scissor(g_app.draw_cmd, x, y, width, height);

        //vkCmdDrawIndexed(g_CommandBuffer, pcmd->ElemCount, 1, idx_offset, vtx_offset, 0);
        tr_cmd_draw_indexed(g_app.draw_cmd, pcmd->ElemCount, idx_offset, vtx_offset); 

        idx_offset += pcmd->ElemCount;
      }
      vtx_offset += cmd_list->VtxBuffer.Size;
    }
  }
}

static const char* imgui_glfw_get_clipboard_text(void* p_user_data)
{
  return glfwGetClipboardString((GLFWwindow*)p_user_data);
}

static void imgui_glfw_set_clipboard_text(void* p_user_data, const char* p_text)
{
  glfwSetClipboardString((GLFWwindow*)p_user_data, p_text);
}

#endif // defined(TR_IMGUI_GLFW_IMPLEMENTATION)

} // namespace tr

#endif // TR_IMGUI_GLFW_H