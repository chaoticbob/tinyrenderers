#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef TINY_RENDERER_ENTITY_H
#define TINY_RENDERER_ENTITY_H

#define TINY_RENDERER_IMPLEMENTATION
#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
#endif

#include "cbuffer.h"
#include "filesystem.h"
#include "mesh.h"
#include "transform.h"

#include <algorithm>

namespace tr {

enum EntityDescriptorBinding {
  ENTITY_DESCRIPTOR_BINDING_VIEW_TRANSFORM  = 0,
  ENTITY_DESCRIPTOR_BINDING_LIGHTING_PARAMS = 1,
  ENTITY_DESCRIPTOR_BINDING_TESS_PARAMS     = 2,
  ENTITY_DESCRIPTOR_BINDING_COUNT,
};

struct ShaderCreateInfo {
  std::vector<uint8_t>  code;
  std::string           entry_point;
};

struct EntityCreateInfo {
  tr_shader_program*    shader_program;
  tr_vertex_layout      vertex_layout;
  // Bindings should start at ENTITY_DESCRIPTOR_BINDING_COUNT. 
  std::vector<uint32_t> texture_bindings;
  std::vector<uint32_t> buffer_bindings;
  tr_render_target*     render_target;
  tr_pipeline_settings  pipeline_settings;
};

/*! @class EntityT

*/
template <typename LightingParamsT, typename TessParamsT>
class EntityT {
public:
  EntityT();

  bool Create(tr_renderer* p_renderer, const EntityCreateInfo& create_info);
  void Destroy();

  bool SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count);
  bool SetVertexBuffers(const tr::Mesh& mesh);
  bool LoadVertexBuffers(const tr::fs::path& file_path);
  bool SetTexture(uint32_t binding, tr_texture* p_texture);

  tr::Transform&          GetTransform();
  const tr::Transform&    GetTransform() const;
  void                    SetTransform(const tr::Transform& transform);
  void                    SetView(const tr::Camera& camera);
  void                    SetColor(const float3& color);

  LightingParamsT&        GetLightingParams();
  const LightingParamsT&  GetLightingParams() const;

  TessParamsT&            GetTessParams();
  const TessParamsT&      GetTessParams() const;

  void UpdateGpuDescriptorSets();
  void UpdateGpuBuffers();

  void Draw(tr_cmd* p_cmd, uint32_t vertex_count = UINT32_MAX);
  void DrawIndexed(tr_cmd* p_cmd, uint32_t index_count = UINT32_MAX);

private:
  void SetViewDirty(bool value);
  void SetTranformDirty(bool value);

private:
  // Renderer
  tr_renderer*                m_renderer = nullptr;
  // Pipeline
  EntityCreateInfo            m_create_info = {};
  tr_descriptor_set*          m_descriptor_set = nullptr;
  tr_pipeline*                m_pipeline = nullptr;
  // Textures
  struct TextureBinding {
    uint32_t    binding;
    tr_texture* texture;
  };
  std::vector<TextureBinding> m_texture_bindings;
  // Geometry
  tr_buffer*                  m_index_buffer = nullptr;
  uint32_t                    m_index_count = UINT32_MAX;
  std::vector<tr_buffer*>     m_vertex_buffers;
  uint32_t                    m_vertex_count = UINT32_MAX;
  // Transform
  tr::Transform               m_transform;
  bool                        m_view_dirty = false;
  bool                        m_transform_dirty = false;
  ViewTransformBuffer         m_cpu_view_transform;
  tr_buffer*                  m_gpu_view_transform = nullptr;
  // Lighting
  LightingParamsT             m_cpu_lighting_params;
  tr_buffer*                  m_gpu_lighting_params = nullptr;
  // Tessellation
  TessParamsT                 m_cpu_tess_params;
  tr_buffer*                  m_gpu_tess_params = nullptr;
};

/*! @fn EntityT<CpuLightingBufferT>::EntityT */
template <typename LightingParamsT, typename TessParamsT>
EntityT<LightingParamsT, TessParamsT>::EntityT() 
{
  auto fn = std::bind(&EntityT::SetTranformDirty, this, std::placeholders::_1);
  m_transform.SetModelChangedCallback(fn);
}

/*! @fn EntityT<CpuLightingBufferT>::SetViewDirty */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::SetViewDirty(bool value) 
{
  m_view_dirty = value;
}

/*! @fn EntityT<CpuLightingBufferT>::SetTranformDirty */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::SetTranformDirty(bool value) 
{
  m_transform_dirty = value;
}

/*! @fn EntityT<CpuLightingBufferT>::Create */
template <typename LightingParamsT, typename TessParamsT>
bool EntityT<LightingParamsT, TessParamsT>::Create(tr_renderer* p_renderer, const EntityCreateInfo& create_info) {
  // Copy renderer
  g_renderer = p_renderer;

  // Copy create info
  m_create_info = create_info;

  // Minimum size must be 4 bytes. Just note that empty C++ structs
  // are always 1 byte in size.
  bool has_view_transform = m_cpu_view_transform.GetDataSize()  >= 4;
  bool has_lighting       = m_cpu_lighting_params.GetDataSize() >= 4;
  bool has_tess           = m_cpu_tess_params.GetDataSize()     >= 4;

  // Descriptor set
  {
    uint32_t const_buffer_count = 0;
    const_buffer_count += has_view_transform ? 1 : 0;
    const_buffer_count += has_lighting       ? 1 : 0;
    const_buffer_count += has_tess           ? 1 : 0;

    uint32_t texture_count = (uint32_t)m_create_info.texture_bindings.size();
    uint32_t buffer_count = (uint32_t)m_create_info.buffer_bindings.size();

    uint32_t total_descriptor_count = const_buffer_count + texture_count + buffer_count;
    std::vector<tr_descriptor> descriptors(total_descriptor_count);

    uint32_t index = 0;
    // Constant buffers descriptors
    {
      // View transform
      if (has_view_transform) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = ENTITY_DESCRIPTOR_BINDING_VIEW_TRANSFORM;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Lighting
      if (has_lighting) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = ENTITY_DESCRIPTOR_BINDING_LIGHTING_PARAMS;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Tessellation
      if (has_tess) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = ENTITY_DESCRIPTOR_BINDING_TESS_PARAMS;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
    }
    // Textures descriptors
    for (uint32_t i = 0; i < texture_count; ++i) {
      uint32_t index = i + ENTITY_DESCRIPTOR_BINDING_COUNT;
      uint32_t binding = m_create_info.texture_bindings[i];
      descriptors[index].type           = tr_descriptor_type_texture_srv;
      descriptors[index].count          = 1;
      descriptors[index].binding        = binding;
      descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
      ++index;
    }
    // Buffers descriptors
    for (uint32_t i = 0; i < buffer_count; ++i) {
      uint32_t index = i + ENTITY_DESCRIPTOR_BINDING_COUNT;
      uint32_t binding = m_create_info.texture_bindings[i];
      descriptors[index].type           = tr_descriptor_type_storage_buffer_srv;
      descriptors[index].count          = 1;
      descriptors[index].binding        = binding;
      descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
      ++index;
    }

    tr_create_descriptor_set(p_renderer,
                              (uint32_t)descriptors.size(),
                              descriptors.data(),
                              &m_descriptor_set);
    assert(m_descriptor_set != nullptr);
  }

  // Pipeline
  {
    tr_create_pipeline(g_renderer,
                        m_create_info.shader_program,
                        &m_create_info.vertex_layout,
                        m_descriptor_set,
                        m_create_info.render_target,
                        &m_create_info.pipeline_settings,
                        &m_pipeline);
    assert(m_pipeline != nullptr);
  }

  // Constant buffers
  {
    if (has_view_transform) {
      uint32_t buffer_size = m_cpu_view_transform.GetDataSize();
      tr_create_uniform_buffer(g_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_view_transform);
      assert(m_gpu_view_transform != nullptr);
    }

    if (has_lighting) {
      uint32_t buffer_size = m_cpu_lighting_params.GetDataSize();
      tr_create_uniform_buffer(g_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_lighting_params);
      assert(m_gpu_lighting_params != nullptr);
    }

    if (has_tess) {
      uint32_t buffer_size = m_cpu_tess_params.GetDataSize();
      tr_create_uniform_buffer(g_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_tess_params);
      assert(m_gpu_tess_params != nullptr);
    }
  }

  return true;
}

/*! @fn EntityT<CpuLightingBufferT>::Destroy */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::Destroy()
{
}

/*! @fn EntityT<CpuLightingBufferT>::SetVertexBuffers */
template <typename LightingParamsT, typename TessParamsT>
bool EntityT<LightingParamsT, TessParamsT>::SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count)
{
  m_vertex_buffers = { p_buffer };
  m_vertex_count = vertex_count;
  return true;
}

/*! @fn EntityT<CpuLightingBufferT>::SetVertexBuffers */
template <typename LightingParamsT, typename TessParamsT>
bool EntityT<LightingParamsT, TessParamsT>::SetVertexBuffers(const tr::Mesh& mesh)
{
  tr_buffer* p_buffer = nullptr;
  tr_create_vertex_buffer(g_renderer, mesh.GetVertexDataSize(), true, mesh.GetVertexStride(), &p_buffer);
  assert(p_buffer != nullptr);

  memcpy(p_buffer->cpu_mapped_address, mesh.GetVertexData(), mesh.GetVertexDataSize());

  m_vertex_buffers = { p_buffer };
  m_vertex_count = mesh.GetVertexCount();
  return true;
}

/*! @fn EntityT<CpuLightingBufferT>::LoadVertexBuffers */
template <typename LightingParamsT, typename TessParamsT>
bool EntityT<LightingParamsT, TessParamsT>::LoadVertexBuffers(const tr::fs::path& file_path)
{
  tr::Mesh mesh;
  bool mesh_load_res = tr::Mesh::Load(file_path, &mesh);
  if (!mesh_load_res) {
    return false;
  }
  bool set_res = SetVertexBuffers(mesh);
  return set_res;
}

/*! @fn EntityT<CpuLightingBufferT>::SetTexture */
template <typename LightingParamsT, typename TessParamsT>
bool EntityT<LightingParamsT, TessParamsT>::SetTexture(uint32_t binding, tr_texture* p_texture) 
{
  auto it = std::find_if(std::begin(m_texture_bindings),
                         std::end(m_texture_bindings),
                         [binding](const EntityT::TextureBinding& elem) -> bool
                             { return elem.binding == binding; });
  if (it != std::end(m_texture_bindings)) {
    return false;
  }

  it->texture = p_texture;

  return true;
}

/*! @fn EntityT<CpuLightingBufferT>::GetTransform */
template <typename LightingParamsT, typename TessParamsT>
Transform& EntityT<LightingParamsT, TessParamsT>::GetTransform() 
{ 
  return m_transform; 
}

/*! @fn EntityT<CpuLightingBufferT>::GetTransform */
template <typename LightingParamsT, typename TessParamsT>
const Transform& EntityT<LightingParamsT, TessParamsT>::GetTransform() const
{ 
  return m_transform; 
}

/*! @fn EntityT<CpuLightingBufferT>::GetLightingParams */
template <typename LightingParamsT, typename TessParamsT>
LightingParamsT& EntityT<LightingParamsT, TessParamsT>::GetLightingParams()
{
  return m_cpu_lighting_params;
}

/*! @fn EntityT<CpuLightingBufferT>::GetLightingParams */
template <typename LightingParamsT, typename TessParamsT>
const LightingParamsT& EntityT<LightingParamsT, TessParamsT>::GetLightingParams() const
{
  return m_cpu_lighting_params;
}

/*! @fn EntityT<CpuLightingBufferT>::GetTessParams */
template <typename LightingParamsT, typename TessParamsT>
TessParamsT& EntityT<LightingParamsT, TessParamsT>::GetTessParams()
{
  return m_cpu_tess_params;
}

/*! @fn EntityT<CpuLightingBufferT>::GetTessParams */
template <typename LightingParamsT, typename TessParamsT>
const TessParamsT& EntityT<LightingParamsT, TessParamsT>::GetTessParams() const
{
  return m_cpu_tess_params;
}

/*! @fn EntityT<CpuLightingBufferT>::SetView */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::SetView(const tr::Camera& camera)
{
  m_cpu_view_transform.SetView(camera);
  SetViewDirty(true);
}

/*! @fn EntityT<CpuLightingBufferT>::SetTransform */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::SetTransform(const tr::Transform& transform)
{
  m_transform = transform;
  SetTranformDirty(true);
}

/*! @fn EntityT<CpuLightingBufferT>::SetColor */
template <typename LightingParamsT, typename TessParamsT>
void tr::EntityT<LightingParamsT, TessParamsT>::SetColor(const float3& color)
{
  m_cpu_view_transform.SetColor(color);
  SetTranformDirty(true);
}

/*! @fn EntityT<CpuLightingBufferT>::UpdateGpuDescriptorSets */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::UpdateGpuDescriptorSets()
{
  uint32_t index = 0;
  if (m_gpu_view_transform != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_view_transform;
    ++index;
  }

  if (m_gpu_lighting_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_lighting_params;
    ++index;
  }

  if (m_gpu_tess_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_tess_params;
    ++index;
  }

  for (const auto& texture_binding : m_texture_bindings) {
    for (uint32_t i = 0; i < m_descriptor_set->descriptor_count; ++i) {
      auto& descriptor = m_descriptor_set->descriptors[index];
      if  (descriptor.binding == texture_binding.binding) {
        assert(descriptor.type == tr_descriptor_type_texture_srv);
        descriptor.textures[0] = texture_binding.texture;
      }
    }
  }
  tr_update_descriptor_set(g_renderer, m_descriptor_set);
}

/*! @fn EntityT<CpuLightingBufferT>::UpdateGpuBuffers */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::UpdateGpuBuffers()
{
  // View/transform constant buffer
  if ((m_gpu_view_transform != nullptr) && (m_view_dirty || m_transform_dirty)) {
    if (m_view_dirty) {
      // Nothing for now
      m_view_dirty = false;
    }

    if (m_transform_dirty) {
      m_cpu_view_transform.SetTransform(m_transform);
      m_transform_dirty = false;
    }

    m_cpu_view_transform.Write(m_gpu_view_transform->cpu_mapped_address);
  }

  // Lighting constant buffer
  if (m_gpu_lighting_params != nullptr) {
    m_cpu_lighting_params.Write(m_gpu_lighting_params->cpu_mapped_address);
  }

  // Tess constant buffer
  if (m_gpu_tess_params != nullptr) {
    m_cpu_tess_params.Write(m_gpu_tess_params->cpu_mapped_address);
  }
}

/*! @fn EntityT<CpuLightingBufferT>::Draw */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::Draw(tr_cmd* p_cmd, uint32_t vertex_count) 
{
  tr_cmd_bind_pipeline(p_cmd, m_pipeline);
  
  tr_cmd_bind_descriptor_sets(p_cmd, m_pipeline, m_descriptor_set);
  
  tr_cmd_bind_vertex_buffers(p_cmd, 
                             (uint32_t)m_vertex_buffers.size(),
                             m_vertex_buffers.data());

  vertex_count = (vertex_count == UINT32_MAX) ? m_vertex_count : vertex_count;
  tr_cmd_draw(p_cmd, vertex_count, 0);
}

/*! @fn EntityT<CpuLightingBufferT>::DrawIndexed */
template <typename LightingParamsT, typename TessParamsT>
void EntityT<LightingParamsT, TessParamsT>::DrawIndexed(tr_cmd* p_cmd, uint32_t index_count) {
}

// =================================================================================================

/*! @class BasicEntity

*/
class BasicEntity : public EntityT<NullBuffer, NullBuffer> {
public:
};

// =================================================================================================

/*! @class BlinnPhongEntity

*/
class BlinnPhongEntity : public EntityT<BlinnPhongBuffer, NullBuffer> {
public:
};

// =================================================================================================

/*! @class GGXEntity

*/
class GGXEntity : public EntityT<GGXBuffer, NullBuffer> {
public:
};

// =================================================================================================
// LoadShaderModule
// =================================================================================================

/*! @fn LoadShaderModule

*/
inline std::vector<uint8_t> LoadShaderModule(const fs::path& file_path)
{
  std::vector<uint8_t> byte_code;
  if (fs::exists(file_path)) {
    std::ifstream is;
    is.open(file_path.c_str(), std::ios::in | std::ios::binary);
    assert(is.is_open());

    is.seekg(0, std::ios::end);
    byte_code.resize(is.tellg());
    assert(0 != byte_code.size());

    is.seekg(0, std::ios::beg);
    is.read((char*)byte_code.data(), byte_code.size());
  }
  return byte_code;
}

// =================================================================================================
// CreateShaderProgram
// =================================================================================================

/*! @fn CreateShaderProgram - VS/PS

*/
inline tr_shader_program* CreateShaderProgram(
  tr_renderer*        p_renderer,
  const fs::path&     vs_file_path,
  const std::string&  vs_entry_point,
  const fs::path&     ps_file_path,
  const std::string&  ps_entry_point
)
{
  tr_shader_program* p_shader_program = nullptr;

  auto vs = LoadShaderModule(vs_file_path);
  auto ps = LoadShaderModule(ps_file_path);

  bool has_byte_code    = !(vs.empty() && ps.empty());
  bool has_entry_point  = !(vs_entry_point.empty() && ps_entry_point.empty());
  if (has_byte_code && has_entry_point) {
    tr_create_shader_program(p_renderer, 
                             (uint32_t)vs.size(), (uint32_t*)vs.data(), vs_entry_point.c_str(),
                             (uint32_t)ps.size(), (uint32_t*)ps.data(), ps_entry_point.c_str(),
                             &p_shader_program);
  }

  return p_shader_program;
}

/*! @fn CreateShaderProgram - VS/GS/PS

*/
inline tr_shader_program* CreateShaderProgram(
  tr_renderer*        p_renderer,
  const fs::path&     vs_file_path,
  const std::string&  vs_entry_point,
  const fs::path&     gs_file_path,
  const std::string&  gs_entry_point,
  const fs::path&     ps_file_path,
  const std::string&  ps_entry_point
)
{
  tr_shader_program* p_shader_program = nullptr;

  auto vs = LoadShaderModule(vs_file_path);
  auto gs = LoadShaderModule(gs_file_path);
  auto ps = LoadShaderModule(ps_file_path);

  bool has_byte_code    = !(vs.empty() && gs.empty() && ps.empty());
  bool has_entry_point  = !(vs_entry_point.empty() &&
                            gs_entry_point.empty() &&
                            ps_entry_point.empty());
  if (has_byte_code && has_entry_point) {
    tr_create_shader_program_n(p_renderer, 
                               (uint32_t)vs.size(), (uint32_t*)vs.data(), vs_entry_point.c_str(),
                                                 0,              nullptr,                nullptr,
                                                 0,              nullptr,                nullptr,
                               (uint32_t)gs.size(), (uint32_t*)gs.data(), gs_entry_point.c_str(),
                               (uint32_t)ps.size(), (uint32_t*)ps.data(), ps_entry_point.c_str(),
                                                 0,              nullptr,                nullptr,
                               &p_shader_program);
  }
  return p_shader_program;
}

/*! @fn CreateShaderProgram - VS/HS/DS/PS

*/
inline tr_shader_program* CreateShaderProgram(
  tr_renderer*        p_renderer,
  const fs::path&     vs_file_path,
  const std::string&  vs_entry_point,
  const fs::path&     hs_file_path,
  const std::string&  hs_entry_point,
  const fs::path&     ds_file_path,
  const std::string&  ds_entry_point,
  const fs::path&     ps_file_path,
  const std::string&  ps_entry_point
)
{
  tr_shader_program* p_shader_program = nullptr;

  auto vs = LoadShaderModule(vs_file_path);
  auto hs = LoadShaderModule(hs_file_path);
  auto ds = LoadShaderModule(ds_file_path);
  auto ps = LoadShaderModule(ps_file_path);

  bool has_byte_code    = !(vs.empty() && hs.empty() && ds.empty() && ps.empty());
  bool has_entry_point  = !(vs_entry_point.empty() &&
                            hs_entry_point.empty() &&
                            ds_entry_point.empty() &&
                            ps_entry_point.empty());
  if (has_byte_code && has_entry_point) {
    tr_create_shader_program_n(p_renderer, 
                               (uint32_t)vs.size(), (uint32_t*)vs.data(), vs_entry_point.c_str(),
                               (uint32_t)hs.size(), (uint32_t*)hs.data(), hs_entry_point.c_str(),
                               (uint32_t)ds.size(), (uint32_t*)ds.data(), ds_entry_point.c_str(),
                                                 0,              nullptr,                nullptr,
                               (uint32_t)ps.size(), (uint32_t*)ps.data(), ps_entry_point.c_str(),
                                                 0,              nullptr,                nullptr,
                               &p_shader_program);
  }
  return p_shader_program;
}

/*! @fn CreateShaderProgram - VS/HS/DS/GS/PS

*/
inline tr_shader_program* CreateShaderProgram(
  tr_renderer*        p_renderer,
  const fs::path&     vs_file_path,
  const std::string&  vs_entry_point,
  const fs::path&     hs_file_path,
  const std::string&  hs_entry_point,
  const fs::path&     ds_file_path,
  const std::string&  ds_entry_point,
  const fs::path&     gs_file_path,
  const std::string&  gs_entry_point,
  const fs::path&     ps_file_path,
  const std::string&  ps_entry_point
)
{
  tr_shader_program* p_shader_program = nullptr;

  auto vs = LoadShaderModule(vs_file_path);
  auto hs = LoadShaderModule(hs_file_path);
  auto ds = LoadShaderModule(ds_file_path);
  auto gs = LoadShaderModule(gs_file_path);
  auto ps = LoadShaderModule(ps_file_path);

  bool has_byte_code    = !(vs.empty() && hs.empty() && ds.empty() && gs.empty() && ps.empty());
  bool has_entry_point  = !(vs_entry_point.empty() &&
                            hs_entry_point.empty() &&
                            ds_entry_point.empty() &&
                            gs_entry_point.empty() &&
                            ps_entry_point.empty());
  if (has_byte_code && has_entry_point) {
    tr_create_shader_program_n(p_renderer, 
                               (uint32_t)vs.size(), (uint32_t*)vs.data(), vs_entry_point.c_str(),
                               (uint32_t)hs.size(), (uint32_t*)hs.data(), hs_entry_point.c_str(),
                               (uint32_t)ds.size(), (uint32_t*)ds.data(), ds_entry_point.c_str(),
                               (uint32_t)gs.size(), (uint32_t*)gs.data(), gs_entry_point.c_str(),
                               (uint32_t)ps.size(), (uint32_t*)ps.data(), ps_entry_point.c_str(),
                                                 0,              nullptr,                nullptr,
                               &p_shader_program);
  }
  return p_shader_program;
}

} // namespace tr

#endif // TINY_RENDERER_ENTITY_H