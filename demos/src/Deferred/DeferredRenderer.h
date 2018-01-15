#ifndef DEFERRED_RENDERER_H
#define DEFERRED_RENDERER_H

#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
#endif

#include "cbuffer.h"
#include "entity.h"
#include "filesystem.h"
#include "util.h"

// GBuffer
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_VIEW_PARAMS        0
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TRANSFORM_PARAMS   1
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_MATERIAL_PARAMS    2
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TESS_PARAMS        3

// Lighting
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_LIGHTING_PARAMS   0
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER0_TEX      1
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER1_TEX      2
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER2_TEX      3
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER3_TEX      4
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_SAMPLER           5
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_OUTPUT_TEX        6

// Composite
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX         0
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER           1
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX        2

#define DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION                 1
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_NORMAL                   2
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_ALBEDO                   3
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_ROUGHNESS                4
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_METALLIC                 5
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_SPECULAR                 6
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL                  7
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL_POWER            8
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_DEPTH                    9
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION_FROM_DEPTH      10


#define DEFERRED_MAX_POINT_LIGHTS        512
#define DEFERRED_MAX_SPOT_LIGHTS         1
#define DEFERRED_MAX_DIRECTIONAL_LIGHTS  6
using DeferredLightingParams = tr::LightingParams<DEFERRED_MAX_POINT_LIGHTS, DEFERRED_MAX_SPOT_LIGHTS, DEFERRED_MAX_DIRECTIONAL_LIGHTS>;

/*! @struct DeferredMaterialData

*/
struct DeferredMaterialData {
  tr::hlsl_float3<3>  Color         = { 0.82f, 0.67f, 0.16f };
  tr::hlsl_float<1>   Roughness     = { 0.5f };
  tr::hlsl_float<1>   Metallic      = { 0.0f };
  tr::hlsl_float<1>   Specular      = { 0.5f };
  tr::hlsl_float<1>   Fresnel       = { 0.0f };
  tr::hlsl_float<1>   FresnelPower  = { 1.0f };  
};

/*! @class DeferredMaterialParams

*/
class DeferredMaterialParams : public tr::ConstantBuffer<DeferredMaterialData> {
public:
  DeferredMaterialParams() {}
  ~DeferredMaterialParams() {}
};

/*! @struct DeferredDebugData

*/
struct DeferredDebugData {
  tr::hlsl_int<4>   GBufferElement = { 0 };
  tr::hlsl_float4x4 InverseViewMatrix;
  tr::hlsl_float4x4 InverseProjectionMatrix;
};

/*! @class DeferredDebugParams

*/
class DeferredDebugParams : public tr::ConstantBuffer<DeferredDebugData> {
public:
  DeferredDebugParams() {}
  ~DeferredDebugParams() {}
};


/*!

/*! @class DeferredEntity

*/
using DeferredEntity = tr::EntityT<DeferredMaterialParams>;


/*! @class DeferredRenderer

*/
class DeferredRenderer {
public:

  DeferredRenderer();
  DeferredRenderer(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, uint32_t frame_count, bool enable_debug_clear = false);
  ~DeferredRenderer();

  void Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, const tr_format* p_rtv_formats, const tr_clear_value* p_rtv_clears, tr_format dsv_format, const tr_clear_value* p_dsv_clear, uint32_t frame_count);
  void Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, uint32_t frame_count, bool enable_debug_clear = false);
  void Destroy();

  void ApplyView(const tr::Camera& camera);

  DeferredLightingParams& GetLightingParams() {
    return m_lighting_cpu_lighting_params;
  }

  const DeferredLightingParams& GetLightingParams() const {
    return m_lighting_cpu_lighting_params;
  }

  tr_texture* GetGBufferRtv(uint32_t rtv_index, uint32_t frame_num) const { 
    assert(frame_num < m_frame_count);
    tr_texture* p_rtv = m_gbuffer_rtvs[frame_num][rtv_index];
    return p_rtv;
  }

  tr_texture* GetGBuffertDsv(uint32_t frame_num) const {
    assert(frame_num < m_frame_count);
    tr_texture* p_dsv = m_gbuffer_dsv[frame_num];
    return p_dsv;
  }

  tr_render_pass* GetGBufferRenderPass(uint32_t frame_num) const {
    assert(frame_num < m_frame_count);
    tr_render_pass* p_render_pass = m_gbuffer_render_pass[frame_num];
    return p_render_pass;
  }

  tr_render_pass* GetBackFaceRenderPass(uint32_t frame_num) const {
    assert(frame_num < m_frame_count);
    tr_render_pass* p_render_pass = m_backface_render_pass[frame_num];
    return p_render_pass;
  }

  void BeginBackFacePass(tr_cmd* p_cmd, uint32_t frame_num);
  void EndBackFacePass(tr_cmd* p_cmd, uint32_t frame_num);

  void BeginGBufferPass(tr_cmd* p_cmd, uint32_t frame_num);
  void EndGBufferPass(tr_cmd* p_cmd, uint32_t frame_num);

  void UpdateGpuBuffers(uint32_t frame_num, tr_cmd* p_cmd);

  void Lighting(tr_cmd* p_cmd, uint32_t frame_num);

  void Composite(tr_cmd* p_cmd, uint32_t frame_num, tr_texture* p_swapchain_image);

  DeferredDebugParams& GetDebugParams() {
    return m_debug_cpu_params;
  }

  void DebugShowGBufferElement(tr_cmd* p_cmd, uint32_t frame_num, tr_texture* p_swapchain_image);

  void BuildDebugUI();

private:
  void CreateShaders(tr_renderer* p_renderer, const tr::fs::path& asset_dir);
  void CreateConstantBuffers(tr_renderer* p_renderer);
  void CreateSamplers(tr_renderer* p_renderer);
  void CreateDescriptorSets(tr_renderer* p_renderer);
  void CreatePipelines(tr_renderer* p_renderer);
  void UpdateDescriptorSets(tr_renderer* p_renderer);

private:
  using rtv_array = std::vector<tr_texture*>;

  bool                                m_initialized = false;

  tr_renderer*                        m_renderer = nullptr;
  uint32_t                            m_width = 0;
  uint32_t                            m_height = 0;
  uint32_t                            m_frame_count = 0;

  // Backface
  std::vector<tr_texture*>            m_backface_dsv;
  std::vector<tr_render_pass*>        m_backface_render_pass;
  tr_pipeline*                        m_backface_pipeline = nullptr;

  // GBuffer
  uint32_t                            m_gbuffer_rtv_count = 0;
  tr_format                           m_gbuffer_rtv_formats[tr_max_rtv_count] = {};
  tr_clear_value                      m_gbuffer_rtv_clears[tr_max_rtv_count] = {};
  std::vector<rtv_array>              m_gbuffer_rtvs;
  tr_format                           m_gbuffer_dsv_format = tr_format_undefined;
  tr_clear_value                      m_gbuffer_dsv_clear = {};
  std::vector<tr_texture*>            m_gbuffer_dsv;
  std::vector<tr_render_pass*>        m_gbuffer_render_pass;
  tr_pipeline*                        m_gbuffer_pipeline = nullptr;

  // Composite
  std::vector<tr_texture*>            m_lighting_rtv;
  tr_shader_program*                  m_lighting_shader = nullptr;
  std::vector<tr_descriptor_set*>     m_lighting_descriptor_sets;
  tr_pipeline*                        m_lighting_pipeline = nullptr;
  DeferredLightingParams              m_lighting_cpu_lighting_params;
  std::vector<tr_buffer*>             m_lighting_gpu_lighting_params;
  std::vector<tr_buffer*>             m_lighting_gpu_lighting_params_staging;
  tr_sampler*                         m_lighting_sampler = nullptr;

  // Composite
  std::vector<tr_texture*>            m_composite_rtv;
  tr_shader_program*                  m_composite_shader = nullptr;
  std::vector<tr_descriptor_set*>     m_composite_descriptor_sets;
  tr_pipeline*                        m_composite_pipeline = nullptr;
  tr_sampler*                         m_composite_sampler = nullptr;

  // Debug - will render to composite RTV
  tr_shader_program*                  m_debug_shader = nullptr;
  std::vector<tr_descriptor_set*>     m_debug_descriptor_sets;
  tr_pipeline*                        m_debug_pipeline = nullptr;
  DeferredDebugParams                 m_debug_cpu_params;
  tr_buffer*                          m_debug_gpu_params;
  tr_sampler*                         m_debug_sampler = nullptr;
};

#endif // DEFERRED_RENDERER_H