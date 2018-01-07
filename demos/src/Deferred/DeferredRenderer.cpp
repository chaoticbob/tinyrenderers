#include "DeferredRenderer.h"

#define COMPOSITE_NUM_THREADS_X   8
#define COMPOSITE_NUM_THREADS_Y   8
#define COMPOSITE_NUM_THREADS_Z   1

DeferredRenderer::DeferredRenderer()
{
}

DeferredRenderer::DeferredRenderer(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, uint32_t frame_count, bool enable_debug_clear)
{
  Initialize(p_renderer, asset_dir, width, height, rtv_count, frame_count);
}

DeferredRenderer::~DeferredRenderer()
{
}

void DeferredRenderer::Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, const tr_format* p_rtv_formats, const tr_clear_value* p_rtv_clears, tr_format dsv_format, const tr_clear_value* p_dsv_clear, uint32_t frame_count)
{
  assert(rtv_count <= tr_max_rtv_count);
  assert(frame_count > 0);

  if (m_initialized) {
    return;
  }

  const tr_sample_count         k_sample_count        = tr_sample_count_1;
  const uint32_t                k_mip_levels          = 1;
  const bool                    k_host_visible        = false;
  const tr_texture_usage_flags  k_rtv_usage           = tr_texture_usage_color_attachment | tr_texture_usage_sampled_image;
  const tr_texture_usage_flags  k_dsv_usage           = tr_texture_usage_depth_stencil_attachment | tr_texture_usage_sampled_image;
  const tr_texture_usage        k_rtv_initial_state   = tr_texture_usage_sampled_image;
  const tr_texture_usage        k_dsv_initial_state   = tr_texture_usage_sampled_image;
  const tr_format               k_lighting_rtv_format = tr_format_r16g16b16a16_float;
  const tr_clear_value          k_lighting_clear      = {};

  m_renderer = p_renderer;
  m_width = width;
  m_height = height;
  m_frame_count = frame_count;

  m_gbuffer_rtv_count = rtv_count;
  memcpy(m_gbuffer_rtv_formats, p_rtv_formats, m_gbuffer_rtv_count * sizeof(*p_rtv_formats));
  memcpy(m_gbuffer_rtv_clears, p_rtv_clears, m_gbuffer_rtv_count * sizeof(*p_rtv_clears));
  m_gbuffer_dsv_format = dsv_format;
  memcpy(&m_gbuffer_dsv_clear, p_dsv_clear, sizeof(*p_dsv_clear));
  
  // Create per frame resources
  for (uint32_t frame_num = 0; frame_num < m_frame_count; ++frame_num) {
    // Backface
    {
      // DSV
      tr_texture* p_dsv = nullptr;
      tr_create_texture_2d(p_renderer, 
                           width, 
                           height, 
                           k_sample_count, 
                           dsv_format, 
                           k_mip_levels, 
                           &m_gbuffer_dsv_clear, 
                           k_host_visible, 
                           k_dsv_usage,
                           k_dsv_initial_state,
                           &p_dsv);
      assert(p_dsv != nullptr);
      m_backface_dsv.push_back(p_dsv);

      // Render pass
      tr_render_pass* p_render_pass = nullptr;
      tr_create_render_pass(p_renderer, 
                            width, 
                            height, 
                            k_sample_count, 
                            0, 
                            nullptr, 
                            nullptr, 
                            m_gbuffer_dsv_format, 
                            &m_gbuffer_dsv_clear, 
                            &p_render_pass);
      assert(p_render_pass != nullptr);
      m_backface_render_pass.push_back(p_render_pass);
    }

    // GBuffer
    {
      // RTVs
      rtv_array per_frame_rtvs;
      for (uint32_t rtv_index = 0; rtv_index < m_gbuffer_rtv_count; ++rtv_index) {
        tr_format format = m_gbuffer_rtv_formats[rtv_index];
        tr_clear_value* p_clear = &m_gbuffer_rtv_clears[rtv_index];
        tr_texture* p_rtv = nullptr;
        tr_create_texture_2d(p_renderer, 
                             width, 
                             height, 
                             k_sample_count, 
                             format, 
                             k_mip_levels, 
                             p_clear, 
                             k_host_visible, 
                             k_rtv_usage,
                             k_rtv_initial_state,
                             &p_rtv);
        assert(p_rtv != nullptr);
        per_frame_rtvs.push_back(p_rtv);
      }
      m_gbuffer_rtvs.push_back(per_frame_rtvs);

      // DSV
      tr_texture* p_dsv = nullptr;
      tr_create_texture_2d(p_renderer, 
                           width, 
                           height, 
                           k_sample_count, 
                           dsv_format, 
                           k_mip_levels, 
                           &m_gbuffer_dsv_clear, 
                           k_host_visible, 
                           k_dsv_usage,
                           k_dsv_initial_state,
                           &p_dsv);
      assert(p_dsv != nullptr);
      m_gbuffer_dsv.push_back(p_dsv);

      // Render pass
      tr_render_pass* p_render_pass = nullptr;
      tr_create_render_pass_existing(p_renderer, 
                                     width, 
                                     height, 
                                     k_sample_count, 
                                     m_gbuffer_rtv_count, 
                                     m_gbuffer_rtvs.back().data(), 
                                     nullptr, 
                                     m_gbuffer_dsv.back(), 
                                     nullptr, 
                                     &p_render_pass);
      assert(p_render_pass != nullptr);
      m_gbuffer_render_pass.push_back(p_render_pass);
    }

    // Lighting
    {
      // RTV
      tr_texture* p_rtv = nullptr;
      tr_create_texture_2d(p_renderer, 
                            width, 
                            height, 
                            k_sample_count, 
                            k_lighting_rtv_format, 
                            k_mip_levels, 
                            &k_lighting_clear, 
                            k_host_visible, 
                            tr_texture_usage_storage_image | tr_texture_usage_sampled_image,
                            tr_texture_usage_sampled_image,
                            &p_rtv);
      assert(p_rtv != nullptr);
      m_lighting_rtv.push_back(p_rtv);
    }

    // Composite
    {
      // RTV - should match swapchain and sample count must always be 1
      assert(p_renderer->settings.swapchain.sample_count == tr_sample_count_1);
      tr_texture* p_rtv = nullptr;
      tr_create_texture_2d(p_renderer, 
                           p_renderer->settings.width,
                           p_renderer->settings.height, 
                           p_renderer->settings.swapchain.sample_count,
                           p_renderer->settings.swapchain.rtv_format,
                           k_mip_levels, 
                           &p_renderer->settings.swapchain.rtv_clear_value, 
                           k_host_visible, 
                           tr_texture_usage_storage_image | tr_texture_usage_transfer_src,
                           tr_texture_usage_transfer_src,
                           &p_rtv);
      assert(p_rtv != nullptr);
      m_composite_rtv.push_back(p_rtv);
    }
  }

  CreateShaders(p_renderer, asset_dir);
  CreateConstantBuffers(p_renderer);
  CreateSamplers(p_renderer);
  CreateDescriptorSets(p_renderer);
  CreatePipelines(p_renderer);
  UpdateDescriptorSets(p_renderer);
  
  m_initialized = true;
}

void DeferredRenderer::Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, uint32_t width, uint32_t height, uint32_t rtv_count, uint32_t frame_count, bool enable_debug_clear)
{
  // RTV formats
  tr_format rtv_formats[tr_max_rtv_count] = {};
  for (uint32_t rtv_index = 0; rtv_index < rtv_count; ++rtv_index) {
    rtv_formats[rtv_index] = tr_format_r16g16b16a16_float;
  }
  // RTV clear values
  tr_clear_value rtv_clears[tr_max_rtv_count] = {};
  if (enable_debug_clear) {
    const tr_clear_value k_debug_clears[tr_max_rtv_count] = {
      { 1.0f, 0.0f, 0.0f, 1.0f },
      { 0.0f, 1.0f, 0.0f, 1.0f },
      { 0.0f, 0.0f, 1.0f, 1.0f },
      { 1.0f, 1.0f, 0.0f, 1.0f },
      { 1.0f, 0.0f, 1.0f, 1.0f },
      { 0.0f, 1.0f, 1.0f, 1.0f },
      { 1.0f, 0.5f, 0.0f, 1.0f },
      { 0.0f, 1.0f, 0.5f, 1.0f },
    };

    for (uint32_t rtv_index = 0; rtv_index < rtv_count; ++rtv_index) {
      memcpy(&rtv_clears[rtv_index], &k_debug_clears[rtv_index], sizeof(tr_clear_value));
    }
  }
  // DSV format
  tr_format dsv_format = tr_format_d32_float;
  // DSV clear value
  tr_clear_value dsv_clear = {};
  dsv_clear.depth = 1.0f;
  dsv_clear.stencil = 255;

  Initialize(p_renderer, asset_dir, width, height, rtv_count, rtv_formats, rtv_clears, dsv_format, &dsv_clear, frame_count);
}


void DeferredRenderer::Destroy()
{
}

void DeferredRenderer::CreateShaders(tr_renderer* p_renderer, const tr::fs::path& asset_dir)
{
  // Lighting
  {
#if defined(TINY_RENDERER_VK)
    tr::fs::path cs_file_path = asset_dir / "shaders/lighting.cs.spv";
#elif defined(TINY_RENDERER_DX)
    tr::fs::path cs_file_path = asset_dir / "shaders/lighting.cs.hlsl";
#endif

    m_lighting_shader = tr::CreateShaderProgram(p_renderer, cs_file_path, "csmain");
    assert(m_lighting_shader != nullptr);
  }

  // Composite
  {
#if defined(TINY_RENDERER_VK)
    tr::fs::path cs_file_path = asset_dir / "shaders/composite.cs.spv";
#elif defined(TINY_RENDERER_DX)
    tr::fs::path cs_file_path = asset_dir / "shaders/composite.cs.hlsl";
#endif

    m_composite_shader = tr::CreateShaderProgram(p_renderer, cs_file_path, "csmain");
    assert(m_composite_shader != nullptr);
  }

  // Debug
  {
#if defined(TINY_RENDERER_VK)
    tr::fs::path cs_file_path = asset_dir / "shaders/resample.cs.spv";
#elif defined(TINY_RENDERER_DX)
    tr::fs::path cs_file_path = asset_dir / "shaders/resample.cs.hlsl";
#endif

    m_debug_shader = tr::CreateShaderProgram(p_renderer, cs_file_path, "csmain");
    assert(m_debug_shader != nullptr);
  }
}

void DeferredRenderer::CreateConstantBuffers(tr_renderer* p_renderer)
{
  for (uint32_t frame_num = 0; frame_num < m_frame_count; ++frame_num) {
    // Lighting
    {
      tr_buffer* p_buffer = nullptr;
      tr_create_buffer(p_renderer, 
                       tr_buffer_usage_uniform_cbv,
                       m_lighting_cpu_lighting_params.GetDataSize(),
                       true,
                       &p_buffer);
      assert(p_buffer != nullptr);
      m_lighting_gpu_lighting_params.push_back(p_buffer);
    }
  }
}

void DeferredRenderer::CreateSamplers(tr_renderer* p_renderer)
{
  // Lighting
  {
    tr_sampler_settings sampler_settings = {};
    tr_create_sampler(p_renderer, &sampler_settings, &m_lighting_sampler);
  }

  // Composite
  {
    tr_sampler_settings sampler_settings = {};
    tr_create_sampler(p_renderer, &sampler_settings, &m_composite_sampler);
  }

  // Debug
  {
    tr_sampler_settings sampler_settings = {};
    tr_create_sampler(p_renderer, &sampler_settings, &m_debug_sampler);
  }
}

void DeferredRenderer::CreateDescriptorSets(tr_renderer* p_renderer)
{
  for (uint32_t frame_num = 0; frame_num < m_frame_count; ++frame_num) {
    // Lighting
    {
      std::vector<tr_descriptor> descriptors;
      // Lighting params
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_uniform_buffer_cbv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_LIGHTING_PARAMS;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Input texture 0
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER0_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Input texture 1
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER1_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Input texture 2
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER2_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Input texture 3
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER3_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Sampler
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_sampler;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_SAMPLER;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Output texture
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_uav;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_LIGHTING_OUTPUT_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }

      tr_descriptor_set* p_descriptor_set = nullptr;
      tr_create_descriptor_set(p_renderer, (uint32_t)descriptors.size(), descriptors.data(), &p_descriptor_set);
      assert(p_descriptor_set != nullptr);
      m_lighting_descriptor_sets.push_back(p_descriptor_set);
    }

    // Composite
    {
      std::vector<tr_descriptor> descriptors;
      // Input texture
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Sampler
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_sampler;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Output texture
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_uav;
        descriptor.count         = 1;
        descriptor.binding       = DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }

      tr_descriptor_set* p_descriptor_set = nullptr;
      tr_create_descriptor_set(p_renderer, (uint32_t)descriptors.size(), descriptors.data(), &p_descriptor_set);
      assert(p_descriptor_set != nullptr);
      m_composite_descriptor_sets.push_back(p_descriptor_set);
    }

    // Debug
    {
      std::vector<tr_descriptor> descriptors;
      // Input texture
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_srv;
        descriptor.count         = 1;
        descriptor.binding       = 0;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Sampler
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_sampler;
        descriptor.count         = 1;
        descriptor.binding       = 1;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }
      // Output texture
      {
        tr_descriptor descriptor = {};
        descriptor.type          = tr_descriptor_type_texture_uav;
        descriptor.count         = 1;
        descriptor.binding       = 2;
        descriptor.shader_stages = tr_shader_stage_comp;
        descriptors.push_back(descriptor);
      }

      tr_descriptor_set* p_descriptor_set = nullptr;
      tr_create_descriptor_set(p_renderer, (uint32_t)descriptors.size(), descriptors.data(), &p_descriptor_set);
      assert(p_descriptor_set != nullptr);
      m_debug_descriptor_sets.push_back(p_descriptor_set);
    }
  }
}

void DeferredRenderer::CreatePipelines(tr_renderer* p_renderer)
{    
  // Lighting
  {
    tr_pipeline_settings pipeline_settings = {};
    tr_create_compute_pipeline(p_renderer, m_lighting_shader, m_lighting_descriptor_sets[0], &pipeline_settings, &m_lighting_pipeline);
    assert(m_lighting_pipeline != nullptr);
  }

  // Composite
  {
    tr_pipeline_settings pipeline_settings = {};
    tr_create_compute_pipeline(p_renderer, m_composite_shader, m_composite_descriptor_sets[0], &pipeline_settings, &m_composite_pipeline);
    assert(m_composite_pipeline != nullptr);
  }

  // Debug
  {
    tr_pipeline_settings pipeline_settings = {};
    tr_create_compute_pipeline(p_renderer, m_debug_shader, m_debug_descriptor_sets[0], &pipeline_settings, &m_debug_pipeline);
    assert(m_debug_pipeline != nullptr);
  }
}

void DeferredRenderer::UpdateDescriptorSets(tr_renderer* p_renderer)
{
  for (uint32_t frame_num = 0; frame_num < m_frame_count; ++frame_num) {
    // Lighting
    {
      // Get the descriptor set for the frame number
      tr_descriptor_set* p_descriptor_set = m_lighting_descriptor_sets[frame_num];
      // Update the descriptor set
      p_descriptor_set->descriptors[0].uniform_buffers[0] = m_lighting_gpu_lighting_params[frame_num];
      p_descriptor_set->descriptors[1].textures[0]        = m_gbuffer_rtvs[frame_num][0];
      p_descriptor_set->descriptors[2].textures[0]        = m_gbuffer_rtvs[frame_num][1];
      p_descriptor_set->descriptors[3].textures[0]        = m_gbuffer_rtvs[frame_num][2];
      p_descriptor_set->descriptors[4].textures[0]        = m_gbuffer_rtvs[frame_num][3];
      p_descriptor_set->descriptors[5].samplers[0]        = m_lighting_sampler;
      p_descriptor_set->descriptors[6].textures[0]        = m_lighting_rtv[frame_num];
      tr_update_descriptor_set(m_renderer, p_descriptor_set);
    }

    // Composite
    {
      // Get the descriptor set for the frame number
      tr_descriptor_set* p_descriptor_set = m_composite_descriptor_sets[frame_num];
      // Update the descriptor set
      p_descriptor_set->descriptors[0].textures[0]  = m_lighting_rtv[frame_num];
      p_descriptor_set->descriptors[1].samplers[0]  = m_composite_sampler;
      p_descriptor_set->descriptors[2].textures[0]  = m_composite_rtv[frame_num];
      tr_update_descriptor_set(m_renderer, p_descriptor_set);
    }
  }
}

void DeferredRenderer::BeginBackFacePass(tr_cmd* p_cmd, uint32_t frame_num)
{ 
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  tr_render_pass* p_render_pass = GetBackFaceRenderPass(frame_num);
  assert(p_render_pass != nullptr);
 
  tr_cmd_render_pass_dsv_transition(p_cmd, p_render_pass, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
  tr_cmd_begin_render(p_cmd, p_render_pass);

  tr_cmd_set_viewport(p_cmd, 0, 0, (float)m_width, (float)m_height, 0.0f, 1.0f);
  tr_cmd_set_scissor(p_cmd, 0, 0, m_width, m_height);

  tr_cmd_clear_depth_stencil_attachment(p_cmd, &m_gbuffer_dsv_clear);
}

void DeferredRenderer::EndBackFacePass(tr_cmd* p_cmd, uint32_t frame_num)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  tr_render_pass* p_render_pass = GetBackFaceRenderPass(frame_num);
  assert(p_render_pass != nullptr);

  tr_cmd_end_render(p_cmd);
  tr_cmd_render_pass_dsv_transition(p_cmd, p_render_pass, tr_texture_usage_depth_stencil_attachment, tr_texture_usage_sampled_image);
}

void DeferredRenderer::BeginGBufferPass(tr_cmd* p_cmd, uint32_t frame_num)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  tr_render_pass* p_render_pass = GetGBufferRenderPass(frame_num);
  assert(p_render_pass != nullptr);

  tr_cmd_render_pass_rtv_transition(p_cmd, p_render_pass, tr_texture_usage_sampled_image, tr_texture_usage_color_attachment);
  tr_cmd_render_pass_dsv_transition(p_cmd, p_render_pass, tr_texture_usage_sampled_image, tr_texture_usage_depth_stencil_attachment);
  tr_cmd_begin_render(p_cmd, p_render_pass);

  tr_cmd_set_viewport(p_cmd, 0, 0, (float)m_width, (float)m_height, 0.0f, 1.0f);
  tr_cmd_set_scissor(p_cmd, 0, 0, m_width, m_height);

  for (uint32_t rtv_index = 0; rtv_index < m_gbuffer_rtv_count; ++rtv_index) {
    const tr_clear_value* p_clear_value = &m_gbuffer_rtv_clears[rtv_index];
    tr_cmd_clear_color_attachment(p_cmd, rtv_index, p_clear_value);
  }
  tr_cmd_clear_depth_stencil_attachment(p_cmd, &m_gbuffer_dsv_clear);
}

void DeferredRenderer::EndGBufferPass(tr_cmd* p_cmd, uint32_t frame_num)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  tr_render_pass* p_render_pass = GetGBufferRenderPass(frame_num);
  assert(p_render_pass != nullptr);

  tr_cmd_end_render(p_cmd);
  tr_cmd_render_pass_rtv_transition(p_cmd, p_render_pass, tr_texture_usage_color_attachment, tr_texture_usage_sampled_image);
  tr_cmd_render_pass_dsv_transition(p_cmd, p_render_pass, tr_texture_usage_depth_stencil_attachment, tr_texture_usage_sampled_image);
}

void DeferredRenderer::UpdateGpuBuffers(uint32_t frame_num)
{
  assert(frame_num < m_frame_count);

  // Lighting
  {
    tr_buffer* p_buffer = m_lighting_gpu_lighting_params[frame_num];
    m_lighting_cpu_lighting_params.Write(p_buffer->cpu_mapped_address);
  }
}

void DeferredRenderer::Lighting(tr_cmd* p_cmd, uint32_t frame_num)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  uint32_t num_groups_x = m_width  / COMPOSITE_NUM_THREADS_X;
  uint32_t num_groups_y = m_height / COMPOSITE_NUM_THREADS_Y;
  uint32_t num_groups_z = COMPOSITE_NUM_THREADS_Z;
  
  tr_texture* p_rtv = m_lighting_rtv[frame_num];
  tr_descriptor_set* p_descriptor_set = m_lighting_descriptor_sets[frame_num];

  tr_cmd_image_transition(p_cmd, p_rtv, tr_texture_usage_sampled_image, tr_texture_usage_storage_image);
  tr_cmd_bind_pipeline(p_cmd, m_lighting_pipeline);
  tr_cmd_bind_descriptor_sets(p_cmd, m_lighting_pipeline, p_descriptor_set);
  tr_cmd_dispatch(p_cmd, num_groups_x, num_groups_y, num_groups_z);
  tr_cmd_image_transition(p_cmd, p_rtv, tr_texture_usage_storage_image, tr_texture_usage_sampled_image);
}
  

void DeferredRenderer::Composite(tr_cmd* p_cmd, uint32_t frame_num, tr_texture* p_swapchain_image)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  uint32_t num_groups_x = p_swapchain_image->width  / COMPOSITE_NUM_THREADS_X;
  uint32_t num_groups_y = p_swapchain_image->height / COMPOSITE_NUM_THREADS_Y;
  uint32_t num_groups_z = COMPOSITE_NUM_THREADS_Z;
  
  tr_texture* p_rtv = m_composite_rtv[frame_num];
  tr_descriptor_set* p_descriptor_set = m_composite_descriptor_sets[frame_num];

  tr_cmd_image_transition(p_cmd, p_rtv, tr_texture_usage_transfer_src, tr_texture_usage_storage_image);
  tr_cmd_bind_pipeline(p_cmd, m_composite_pipeline);
  tr_cmd_bind_descriptor_sets(p_cmd, m_composite_pipeline, p_descriptor_set);
  tr_cmd_dispatch(p_cmd, num_groups_x, num_groups_y, num_groups_z);
  tr_cmd_image_transition(p_cmd, p_rtv, tr_texture_usage_storage_image, tr_texture_usage_transfer_src);

  tr_cmd_copy_texture2d_exact(p_cmd, 0, p_rtv, p_swapchain_image);
}

void DeferredRenderer::DebugShowDebuffer(tr_cmd* p_cmd, uint32_t frame_num, uint32_t gbuffer_index, tr_texture* p_swapchain_image)
{
  assert(p_cmd != nullptr);
  assert(frame_num < m_frame_count);

  uint32_t num_groups_x = p_swapchain_image->width  / COMPOSITE_NUM_THREADS_X;
  uint32_t num_groups_y = p_swapchain_image->height / COMPOSITE_NUM_THREADS_Y;
  uint32_t num_groups_z = COMPOSITE_NUM_THREADS_Z;
  
  tr_texture* p_gbuffer_rtv = nullptr;
  switch (gbuffer_index) {
    default: assert(false && "invalid gbuffer_index"); break;
    case 0:
    case 1:
    case 2:
    case 3: {
      p_gbuffer_rtv = m_gbuffer_rtvs[frame_num][gbuffer_index];
    }
    break;

    case 4: {
      p_gbuffer_rtv = m_gbuffer_dsv[frame_num];
    }
    break;
  }
  tr_texture* p_composite_rtv = m_composite_rtv[frame_num];
  tr_descriptor_set* p_descriptor_set = m_debug_descriptor_sets[frame_num];

  if (p_descriptor_set->descriptors[0].textures[0] != p_gbuffer_rtv) {
    // Get the descriptor set for the frame number
    tr_descriptor_set* p_descriptor_set = m_debug_descriptor_sets[frame_num];
    // Update the descriptor set
    p_descriptor_set->descriptors[0].textures[0]  = p_gbuffer_rtv;
    p_descriptor_set->descriptors[1].samplers[0]  = m_debug_sampler;
    p_descriptor_set->descriptors[2].textures[0]  = m_composite_rtv[frame_num];
    tr_update_descriptor_set(m_renderer, p_descriptor_set);
  }

  tr_cmd_image_transition(p_cmd, p_composite_rtv, tr_texture_usage_transfer_src, tr_texture_usage_storage_image);
  tr_cmd_bind_pipeline(p_cmd, m_debug_pipeline);
  tr_cmd_bind_descriptor_sets(p_cmd, m_debug_pipeline, p_descriptor_set);
  tr_cmd_dispatch(p_cmd, num_groups_x, num_groups_y, num_groups_z);
  tr_cmd_image_transition(p_cmd, p_composite_rtv, tr_texture_usage_storage_image, tr_texture_usage_transfer_src);

  tr_cmd_copy_texture2d_exact(p_cmd, 0, p_composite_rtv, p_swapchain_image);
}

