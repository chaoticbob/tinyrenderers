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
#include "util.h"

#include <algorithm>
#include <set>

namespace tr {

struct ShaderCreateInfo {
  std::vector<uint8_t>  code;
  std::string           entry_point;
};

// =================================================================================================
// EntityT
// =================================================================================================

/*! @enum EntityDescriptorBinding

*/
enum EntityDescriptorBinding {
  ENTITY_DESCRIPTOR_BINDING_DISABLED          = -1,

  ENTITY_DESCRIPTOR_BINDING_VIEW_PARAMS       = 0,
  ENTITY_DESCRIPTOR_BINDING_TRANSFORM_PARAMS  = 1,
  ENTITY_DESCRIPTOR_BINDING_MATERIAL_PARAMS   = 2,
  ENTITY_DESCRIPTOR_BINDING_LIGHTING_PARAMS   = 3,
  ENTITY_DESCRIPTOR_BINDING_TESS_PARAMS       = 4,
  ENTITY_DESCRIPTOR_BINDING_COUNT,
};

/*! @struct EntityCreateInfo

*/
struct EntityCreateInfo {
  tr_shader_program*    shader_program;
  tr_vertex_layout      vertex_layout;
  tr_buffer*            gpu_view_params;
  tr_buffer*            gpu_lighting_params;
  int32_t               view_params_binding       = ENTITY_DESCRIPTOR_BINDING_DISABLED;
  int32_t               transform_params_binding  = ENTITY_DESCRIPTOR_BINDING_DISABLED;
  int32_t               material_params_binding   = ENTITY_DESCRIPTOR_BINDING_DISABLED;
  int32_t               lighting_params_binding   = ENTITY_DESCRIPTOR_BINDING_DISABLED;
  int32_t               tess_params_binding       = ENTITY_DESCRIPTOR_BINDING_DISABLED;
  std::vector<uint32_t> texture_bindings;
  std::vector<uint32_t> buffer_bindings;
  tr_render_pass*       render_pass;
  tr_pipeline_settings  pipeline_settings;
};

/*! @class EntityT

*/
template <typename MaterialParamsT>
class EntityT {
public:
  EntityT();

  bool Create(tr_renderer* p_renderer, const EntityCreateInfo& create_info);
  void Destroy();

  const std::string&  GetName() const { return m_name; }
  void SetName(const std::string& name) { m_name = name; }

  bool SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count);
  bool SetVertexBuffers(const tr::Mesh& mesh);
  bool LoadVertexBuffers(const tr::fs::path& file_path);

  tr::Transform&          GetModelTransform();
  const tr::Transform&    GetModelTransform() const;
  void                    SetModelTransform(const tr::Transform& transform);
  void                    SetViewTransform(const tr::Camera& camera);
  void                    SetDebugColor(const float3& debug_color);

  MaterialParamsT&        GetMaterialParams();
  const MaterialParamsT&  GetMaterialParams() const;

  TessParams&             GetTessParams();
  const TessParams&       GetTessParams() const;

  bool SetTexture(uint32_t binding, tr_texture* p_texture);

  const AABB& GetObjectSpaceBounds() const;
  const AABB& GetWorldSpaceBounds() const;
  const AABB& GetViewSpaceBounds() const;
  void SetObjectSpaceBounds(const AABB& bounds);

  bool GetCameraVisible() const;
  void SetCameraVisible(bool value);

  void UpdateGpuDescriptorSets();

  void FinalizeTransforms();

  void UpdateGpuBuffers(tr_cmd* p_cmd);

  void Draw(tr_cmd* p_cmd, uint32_t vertex_count = UINT32_MAX);
  void DrawIndexed(tr_cmd* p_cmd, uint32_t index_count = UINT32_MAX);

private:
  std::string                 m_name;
  // Renderer
  tr_renderer*                m_renderer = nullptr;
  // Pipeline
  EntityCreateInfo            m_create_info = {};
  tr_descriptor_set*          m_descriptor_set = nullptr;
  tr_pipeline*                m_pipeline = nullptr;
  // Geometry
  tr_buffer*                  m_index_buffer = nullptr;
  uint32_t                    m_index_count = UINT32_MAX;
  std::vector<tr_buffer*>     m_vertex_buffers;
  uint32_t                    m_vertex_count = UINT32_MAX;
  // Transform
  tr::Transform               m_model_transform;
  bool                        m_model_transform_dirty = false;
  TransformParams             m_cpu_transform_params;
  tr_buffer*                  m_gpu_transform_params = nullptr;
  tr_buffer*                  m_gpu_transform_params_staging = nullptr;
  // Materials
  MaterialParamsT             m_cpu_material_params;
  tr_buffer*                  m_gpu_material_params = nullptr;
  tr_buffer*                  m_gpu_material_params_staging = nullptr;
  // Tessellation
  TessParams                  m_cpu_tess_params;
  tr_buffer*                  m_gpu_tess_params = nullptr;
  tr_buffer*                  m_gpu_tess_params_staging = nullptr;
  // Textures
  struct TextureBinding {
    uint32_t    binding;
    tr_texture* texture;
  };
  std::vector<TextureBinding> m_texture_bindings;
  // Bounds
  AABB                        m_object_space_bounds = {};
  AABB                        m_world_space_bounds = {};
  AABB                        m_view_space_bounds = {};
  // Visibility
  bool                        m_camera_visible = false;
};

/*! @fn EntityT<MaterialParamsT>::EntityT */
template <typename MaterialParamsT>
inline EntityT<MaterialParamsT>::EntityT() 
{
}

/*! @fn EntityT<MaterialParamsT>::Create */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::Create(tr_renderer* p_renderer, const EntityCreateInfo& create_info) 
{
  // Copy renderer
  m_renderer = p_renderer;

  // Copy create info
  m_create_info = create_info;

  bool has_view       = (m_create_info.view_params_binding != ENTITY_DESCRIPTOR_BINDING_DISABLED);
  bool has_transform  = (m_create_info.transform_params_binding != ENTITY_DESCRIPTOR_BINDING_DISABLED);
  bool has_material   = (m_create_info.material_params_binding != ENTITY_DESCRIPTOR_BINDING_DISABLED);
  bool has_lighting   = (m_create_info.lighting_params_binding != ENTITY_DESCRIPTOR_BINDING_DISABLED);
  bool has_tess       = (m_create_info.tess_params_binding != ENTITY_DESCRIPTOR_BINDING_DISABLED);

  uint32_t total_descriptor_count = 0;
  // Descriptor set
  {
    uint32_t const_buffer_count = 0;
    const_buffer_count += has_transform ? 1 : 0;
    const_buffer_count += has_material  ? 1 : 0;
    const_buffer_count += has_tess      ? 1 : 0;
    const_buffer_count += has_view      ? 1 : 0;
    const_buffer_count += has_lighting  ? 1 : 0;

    uint32_t texture_count = (uint32_t)m_create_info.texture_bindings.size();
    uint32_t buffer_count = (uint32_t)m_create_info.buffer_bindings.size();

    total_descriptor_count = const_buffer_count + texture_count + buffer_count;
    std::vector<tr_descriptor> descriptors(total_descriptor_count);

    uint32_t index = 0;
    // Constant buffers descriptors
    {
      // View
      if (has_view) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = m_create_info.view_params_binding;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Transform
      if (has_transform) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = m_create_info.transform_params_binding;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Material
      if (has_material) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = m_create_info.material_params_binding;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Lighting
      if (has_lighting) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        = m_create_info.lighting_params_binding;
        descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
        ++index;
      }
      // Tessellation
      if (has_tess) {
        descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
        descriptors[index].count          = 1;
        descriptors[index].binding        =  m_create_info.tess_params_binding;
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
      uint32_t binding = m_create_info.buffer_bindings[i];
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
    // Check to make sure all descriptor bindings are unique
    {
      std::set<uint32_t> unique_bindings;
      // Constant buffer bindings
      if (has_view) {
       unique_bindings.insert(m_create_info.view_params_binding);
      }
      if (has_transform) {
       unique_bindings.insert(m_create_info.transform_params_binding);
      }
      if (has_material) {
       unique_bindings.insert(m_create_info.material_params_binding);
      }
      if (has_lighting) {
       unique_bindings.insert(m_create_info.lighting_params_binding);
      }
      if (has_tess) {
       unique_bindings.insert(m_create_info.tess_params_binding);
      }
      // Textures descriptors
      for (uint32_t i = 0; i < (uint32_t)m_create_info.texture_bindings.size(); ++i) {
        uint32_t binding = m_create_info.texture_bindings[i];
        if (binding != ENTITY_DESCRIPTOR_BINDING_DISABLED) {
          unique_bindings.insert(binding);
        }
      }
      // Buffers descriptors
      for (uint32_t i = 0; i < (uint32_t)m_create_info.buffer_bindings.size(); ++i) {
        uint32_t binding = m_create_info.buffer_bindings[i];
        if (binding != ENTITY_DESCRIPTOR_BINDING_DISABLED) {
          unique_bindings.insert(binding);
        }
      }

      // If this assert gets hit...there's a duplicate binding number. Vulkan doesn't allow for this
      // so it has to be enforced on both renderers.
      uint32_t unique_descriptor_count = (uint32_t)unique_bindings.size();
      assert(total_descriptor_count == unique_descriptor_count);
    }

    tr_create_pipeline(m_renderer,
                       m_create_info.shader_program,
                       &m_create_info.vertex_layout,
                       m_descriptor_set,
                       m_create_info.render_pass,
                       &m_create_info.pipeline_settings,
                       &m_pipeline);
    assert(m_pipeline != nullptr);
  }

  // Constant buffers
  {
    if (has_transform) {
      uint32_t buffer_size = m_cpu_transform_params.GetDataSize();
      assert(buffer_size >= 4); // Minimum size must be 4 bytes. Just note that empty C++ structs are always 1 byte in size.

      // GPU device local buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               false, 
                               &m_gpu_transform_params);
      assert(m_gpu_transform_params != nullptr);

      // GPU staging buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_transform_params_staging);
      assert(m_gpu_transform_params_staging != nullptr);

      //// Make writes go directly to the GPU buffer
      //m_cpu_transform_params.SetTarget(m_gpu_transform_params->cpu_mapped_address);
    }

    if (has_material) {
      uint32_t buffer_size = m_cpu_material_params.GetDataSize();
      assert(buffer_size >= 4); // Minimum size must be 4 bytes. Just note that empty C++ structs are always 1 byte in size.

      // GPU device local buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               false, 
                               &m_gpu_material_params);
      assert(m_gpu_material_params != nullptr);

      // GPU staging buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_material_params_staging);
      assert(m_gpu_material_params_staging != nullptr);

      //// Make writes go directly to the GPU buffer
      //m_cpu_material_params.SetTarget(m_gpu_material_params->cpu_mapped_address);
    }

    if (has_tess) {
      uint32_t buffer_size = m_cpu_tess_params.GetDataSize();
      assert(buffer_size >= 4); // Minimum size must be 4 bytes. Just note that empty C++ structs are always 1 byte in size.

      // GPU device local buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               false, 
                               &m_gpu_tess_params);
      assert(m_gpu_tess_params != nullptr);

      // GPU staging buffer
      tr_create_uniform_buffer(m_renderer, 
                               buffer_size, 
                               true, 
                               &m_gpu_tess_params_staging);
      assert(m_gpu_tess_params_staging != nullptr);

      //// Make writes go directly to the GPU buffer
      //m_cpu_tess_params.SetTarget(m_gpu_tess_params->cpu_mapped_address);
    }
  }

  return true;
}

/*! @fn EntityT<MaterialParamsT>::Destroy */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::Destroy()
{
}

/*! @fn EntityT<MaterialParamsT>::SetVertexBuffers */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count)
{
  m_vertex_buffers = { p_buffer };
  m_vertex_count = vertex_count;
  return true;
}

/*! @fn EntityT<MaterialParamsT>::SetVertexBuffers */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::SetVertexBuffers(const tr::Mesh& mesh)
{
  tr_buffer* p_buffer = nullptr;
  tr_create_vertex_buffer(m_renderer, mesh.GetVertexDataSize(), true, mesh.GetVertexStride(), &p_buffer);
  assert(p_buffer != nullptr);

  memcpy(p_buffer->cpu_mapped_address, mesh.GetVertexData(), mesh.GetVertexDataSize());

  m_vertex_buffers = { p_buffer };
  m_vertex_count = mesh.GetVertexCount();
  return true;
}

/*! @fn EntityT<MaterialParamsT>::LoadVertexBuffers */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::LoadVertexBuffers(const tr::fs::path& file_path)
{
  tr::Mesh mesh;
  bool mesh_load_res = tr::Mesh::Load(file_path, &mesh);
  if (!mesh_load_res) {
    return false;
  }
  bool set_res = SetVertexBuffers(mesh);
  return set_res;
}

/*! @fn EntityT<MaterialParamsT>::GetTransform */
template <typename MaterialParamsT>
inline Transform& EntityT<MaterialParamsT>::GetModelTransform() 
{ 
  // Assume that if this version of the function gets called, changes are made.
  m_model_transform_dirty = true;
  return m_model_transform; 
}

/*! @fn EntityT<MaterialParamsT>::GetTransform */
template <typename MaterialParamsT>
inline const Transform& EntityT<MaterialParamsT>::GetModelTransform() const
{ 
  return m_model_transform; 
}

/*! @fn EntityT<MaterialParamsT>::SetTransform */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::SetModelTransform(const tr::Transform& transform)
{
  m_model_transform = transform;
  m_model_transform_dirty = true;
}

/*! @fn EntityT<MaterialParamsT>::SetView */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::SetViewTransform(const tr::Camera& camera)
{
  m_cpu_transform_params.SetViewTransform(camera);
}

/*! @fn EntityT<MaterialParamsT>::SetColor */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::SetDebugColor(const float3& color)
{
  m_cpu_transform_params.SetDebugColor(color);
}

/*! @fn EntityT<MaterialParamsT>::GetLightingParams */
template <typename MaterialParamsT>
inline MaterialParamsT& EntityT<MaterialParamsT>::GetMaterialParams()
{
  return m_cpu_material_params;
}

/*! @fn EntityT<MaterialParamsT>::GetLightingParams */
template <typename MaterialParamsT>
inline const MaterialParamsT& EntityT<MaterialParamsT>::GetMaterialParams() const
{
  return m_cpu_material_params;
}

/*! @fn EntityT<MaterialParamsT>::GetTessParams */
template <typename MaterialParamsT>
inline TessParams& EntityT<MaterialParamsT>::GetTessParams()
{
  return m_cpu_tess_params;
}

/*! @fn EntityT<MaterialParamsT>::GetTessParams */
template <typename MaterialParamsT>
inline const TessParams& EntityT<MaterialParamsT>::GetTessParams() const
{
  return m_cpu_tess_params;
}

/*! @fn EntityT<MaterialParamsT>::SetTexture */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::SetTexture(uint32_t binding, tr_texture* p_texture) 
{
  auto it = std::find_if(std::begin(m_texture_bindings),
                         std::end(m_texture_bindings),
                         [binding](const EntityT<MaterialParamsT>::TextureBinding& elem) -> bool
                             { return elem.binding == binding; });
  if (it != std::end(m_texture_bindings)) {
    return false;
  }

  it->texture = p_texture;

  return true;
}

/*! @fn EntityT<MaterialParamsT>::GetObjectSpaceBounds */
template <typename MaterialParamsT>
inline const AABB& EntityT<MaterialParamsT>::GetObjectSpaceBounds() const
{
  return m_world_space_bounds;
}

/*! @fn EntityT<MaterialParamsT>::GetWorldSpaceBounds */
template <typename MaterialParamsT>
inline const AABB& EntityT<MaterialParamsT>::GetWorldSpaceBounds() const
{
  return m_world_space_bounds;
}

/*! @fn EntityT<MaterialParamsT>::GetViewSpaceBounds */
template <typename MaterialParamsT>
inline const AABB& EntityT<MaterialParamsT>::GetViewSpaceBounds() const
{
  return m_view_space_bounds;
}

/*! @fn EntityT<MaterialParamsT>::SetObjectSpaceBounds */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::SetObjectSpaceBounds(const AABB& bounds)
{
  m_object_space_bounds = bounds;
}

/*! @fn EntityT<MaterialParamsT>::GetCameraVisible */
template <typename MaterialParamsT>
inline bool EntityT<MaterialParamsT>::GetCameraVisible() const
{
  return m_camera_visible;
}

/*! @fn EntityT<MaterialParamsT>::SetCameraVisible */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::SetCameraVisible(bool value)
{
  m_camera_visible = value;
}

/*! @fn EntityT<MaterialParamsT>::UpdateGpuDescriptorSets */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::UpdateGpuDescriptorSets()
{
  uint32_t index = 0;

  // View
  if (m_create_info.gpu_view_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_create_info.gpu_view_params;
    ++index;
  }

   // Transform
  if (m_gpu_transform_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_transform_params;
    ++index;
  }

  // Material
  if (m_gpu_material_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_material_params;
    ++index;
  }

  // Lighting
  if (m_create_info.gpu_lighting_params != nullptr) {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_create_info.gpu_lighting_params;
    ++index;
  }

  // Tessellation
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
  tr_update_descriptor_set(m_renderer, m_descriptor_set);
}

/*! @fn EntityT<MaterialParamsT>::UpdateGpuBuffers */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::FinalizeTransforms()
{
  if (m_model_transform_dirty) {
    m_cpu_transform_params.SetModelTransform(m_model_transform);
    m_model_transform_dirty = false;
  }

  float4x4 model_matrix = m_cpu_transform_params.GetData().ModelMatrix;
  AABB bounds = m_object_space_bounds.GetTransformed(model_matrix);
  m_world_space_bounds.Set(bounds);

  float4x4 model_view_matrix = m_cpu_transform_params.GetData().ModelViewMatrix;
  bounds = m_object_space_bounds.GetTransformed(model_view_matrix);
  m_view_space_bounds.Set(bounds);
}

/*! @fn EntityT<MaterialParamsT>::UpdateGpuBuffers */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::UpdateGpuBuffers(tr_cmd* p_cmd)
{
  if (m_model_transform_dirty) {
    m_cpu_transform_params.SetModelTransform(m_model_transform);
    m_model_transform_dirty = false;
  }

  // View/transform constant buffer
  if (m_gpu_transform_params != nullptr) {
    m_cpu_transform_params.Write(m_gpu_transform_params_staging->cpu_mapped_address);
    // Copy staging to device local
    tr_cmd_copy_buffer_exact(p_cmd, m_gpu_transform_params_staging, m_gpu_transform_params);
  }

  // Lighting constant buffer
  if (m_gpu_material_params != nullptr) {
    m_cpu_material_params.Write(m_gpu_material_params_staging->cpu_mapped_address);
    // Copy staging to device local
    tr_cmd_copy_buffer_exact(p_cmd, m_gpu_material_params_staging, m_gpu_material_params);
  }

  // Tess constant buffer
  if (m_gpu_tess_params != nullptr) {
    m_cpu_tess_params.Write(m_gpu_tess_params_staging->cpu_mapped_address);
    // Copy staging to device local
    tr_cmd_copy_buffer_exact(p_cmd, m_gpu_tess_params_staging, m_gpu_tess_params);
  }
}

/*! @fn EntityT<MaterialParamsT>::Draw */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::Draw(tr_cmd* p_cmd, uint32_t vertex_count) 
{
  if (!m_camera_visible) {
    return;
  }

  tr_cmd_bind_pipeline(p_cmd, m_pipeline);
  
  tr_cmd_bind_descriptor_sets(p_cmd, m_pipeline, m_descriptor_set);
  
  tr_cmd_bind_vertex_buffers(p_cmd, 
                             (uint32_t)m_vertex_buffers.size(),
                             m_vertex_buffers.data());

  vertex_count = (vertex_count == UINT32_MAX) ? m_vertex_count : vertex_count;
  tr_cmd_draw(p_cmd, vertex_count, 0);
}

/*! @fn EntityT<MaterialParamsT>::DrawIndexed */
template <typename MaterialParamsT>
inline void EntityT<MaterialParamsT>::DrawIndexed(tr_cmd* p_cmd, uint32_t index_count) {
}

// =================================================================================================
// BRDFEntity
// =================================================================================================
using BRDFEntity = EntityT<BRDFMaterialParams>;

// =================================================================================================
// SimpleEntity
// =================================================================================================

/*! @enum SimpleEntityDescriptorBinding

*/
enum SimpleEntityDescriptorBinding {
  SIMPLE_ENTITY_DESCRIPTOR_BINDING_SHADER_PARAMS  = 0,
  SIMPLE_ENTITY_DESCRIPTOR_BINDING_COUNT,
};

/*! @struct SimpleEntityCreateInfo

*/
struct SimpleEntityCreateInfo {
  tr_shader_program*    shader_program;
  tr_vertex_layout      vertex_layout;
  // Bindings should start at ENTITY_DESCRIPTOR_BINDING_COUNT. 
  std::vector<uint32_t> texture_bindings;
  std::vector<uint32_t> buffer_bindings;
  tr_render_pass*     render_target;
  tr_pipeline_settings  pipeline_settings;
  SimpleMaterialProfile material_profile;
};

/*! @class SimpleEntity

*/
class SimpleEntity {
public:
  SimpleEntity();

  bool Create(tr_renderer* p_renderer, const SimpleEntityCreateInfo& create_info);
  void Destroy();

  bool SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count);
  bool SetVertexBuffers(const tr::Mesh& mesh);
  bool LoadVertexBuffers(const tr::fs::path& file_path);
  bool SetTexture(uint32_t binding, tr_texture* p_texture);

  tr::Transform&            GetTransform();
  const tr::Transform&      GetTransform() const;
  void                      SetTransform(const tr::Transform& transform);
  void                      ApplyView(const tr::Camera& camera);
  void                      SetDebugColor(const float3& debug_color);

  SimpleShaderParams&       GetShaderParams();
  const SimpleShaderParams& GetShaderParams() const;

  void UpdateGpuDescriptorSets();
  void UpdateGpuBuffers();

  void Draw(tr_cmd* p_cmd, uint32_t vertex_count = UINT32_MAX);
  void DrawIndexed(tr_cmd* p_cmd, uint32_t index_count = UINT32_MAX);

private:
  // Renderer
  tr_renderer*                m_renderer = nullptr;
  // Pipeline
  SimpleEntityCreateInfo      m_create_info = {};
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
  // Shader Params
  tr::Transform               m_transform;
  bool                        m_transform_dirty = true;
  SimpleShaderParams          m_cpu_shader_params = {};
  tr_buffer*                  m_gpu_shader_params = nullptr;
};

/*! @fn SimpleEntity::SimpleEntity */
inline SimpleEntity::SimpleEntity() 
{
}

/*! @fn SimpleEntity::Create */
inline bool SimpleEntity::Create(tr_renderer* p_renderer, const SimpleEntityCreateInfo& create_info) {
  // Copy renderer
  m_renderer = p_renderer;

  // Copy create info
  m_create_info = create_info;

  // Descriptor set
  {
    uint32_t const_buffer_count = 1;
    uint32_t texture_count = (uint32_t)m_create_info.texture_bindings.size();
    uint32_t buffer_count   = (uint32_t)m_create_info.buffer_bindings.size();

    uint32_t total_descriptor_count = const_buffer_count + texture_count + buffer_count;
    std::vector<tr_descriptor> descriptors(total_descriptor_count);

    uint32_t index = 0;
    // Constant buffers descriptors
    {
      descriptors[index].type           = tr_descriptor_type_uniform_buffer_cbv;
      descriptors[index].count          = 1;
      descriptors[index].binding        = SIMPLE_ENTITY_DESCRIPTOR_BINDING_SHADER_PARAMS;
      descriptors[index].shader_stages  = tr_shader_stage_all_graphics;
      ++index;
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
    tr_create_pipeline(m_renderer,
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
    uint32_t buffer_size = m_cpu_shader_params.GetDataSize();
    tr_create_uniform_buffer(m_renderer, 
                             buffer_size, 
                             true, 
                             &m_gpu_shader_params);
    assert(m_gpu_shader_params != nullptr);
  }

  // Material profile
  SimpleMaterialParams::SetProfile(create_info.material_profile, &m_cpu_shader_params.GetData().Material);

  return true;
}

/*! @fn SimpleEntity::Destroy */
inline void SimpleEntity::Destroy()
{
}

/*! @fn SimpleEntity::SetVertexBuffers */
inline bool SimpleEntity::SetVertexBuffers(tr_buffer* p_buffer, uint32_t vertex_count)
{
  m_vertex_buffers = { p_buffer };
  m_vertex_count = vertex_count;
  return true;
}

/*! @fn SimpleEntity::SetVertexBuffers */
inline bool SimpleEntity::SetVertexBuffers(const tr::Mesh& mesh)
{
  tr_buffer* p_buffer = nullptr;
  tr_create_vertex_buffer(m_renderer, mesh.GetVertexDataSize(), true, mesh.GetVertexStride(), &p_buffer);
  assert(p_buffer != nullptr);

  memcpy(p_buffer->cpu_mapped_address, mesh.GetVertexData(), mesh.GetVertexDataSize());

  m_vertex_buffers = { p_buffer };
  m_vertex_count = mesh.GetVertexCount();
  return true;
}

/*! @fn SimpleEntity::LoadVertexBuffers */
inline bool SimpleEntity::LoadVertexBuffers(const tr::fs::path& file_path)
{
  tr::Mesh mesh;
  bool mesh_load_res = tr::Mesh::Load(file_path, &mesh);
  if (!mesh_load_res) {
    return false;
  }
  bool set_res = SetVertexBuffers(mesh);
  return set_res;
}

/*! @fn SimpleEntity::SetTexture */
inline bool SimpleEntity::SetTexture(uint32_t binding, tr_texture* p_texture) 
{
  auto it = std::find_if(std::begin(m_texture_bindings),
                         std::end(m_texture_bindings),
                         [binding](const SimpleEntity::TextureBinding& elem) -> bool
                             { return elem.binding == binding; });
  if (it != std::end(m_texture_bindings)) {
    return false;
  }

  it->texture = p_texture;

  return true;
}

/*! @fn SimpleEntity::GetTransform */
inline Transform& SimpleEntity::GetTransform() 
{ 
  return m_transform; 
}

/*! @fn SimpleEntity::GetTransform */
inline const Transform& SimpleEntity::GetTransform() const
{ 
  return m_transform; 
}

/*! @fn SimpleEntity::GetShaderParams */
inline SimpleShaderParams& SimpleEntity::GetShaderParams()
{
  return m_cpu_shader_params;
}

/*! @fn SimpleEntity::GetShaderParams */
inline const SimpleShaderParams& SimpleEntity::GetShaderParams() const
{
  return m_cpu_shader_params;
}

/*! @fn SimpleEntity::SetTransform */
inline void SimpleEntity::SetTransform(const tr::Transform& transform)
{
  m_transform = transform;
}

/*! @fn SimpleEntity::SetColor */
inline void tr::SimpleEntity::SetDebugColor(const float3& color)
{
  m_cpu_shader_params.SetDebugColor(color);
}

/*! @fn SimpleEntity::UpdateGpuDescriptorSets */
inline void SimpleEntity::UpdateGpuDescriptorSets()
{
  uint32_t index = 0;

  // Shader Params
  {
    m_descriptor_set->descriptors[index].uniform_buffers[0] = m_gpu_shader_params;
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
  tr_update_descriptor_set(m_renderer, m_descriptor_set);
}

/*! @fn SimpleEntity::UpdateGpuBuffers */
inline void SimpleEntity::UpdateGpuBuffers()
{
  if (m_transform_dirty) {
    m_cpu_shader_params.SetTransform(m_transform);
    m_transform_dirty = false;
  }

  m_cpu_shader_params.Write(m_gpu_shader_params->cpu_mapped_address);
}

/*! @fn SimpleEntity::Draw */
inline void SimpleEntity::Draw(tr_cmd* p_cmd, uint32_t vertex_count) 
{
  tr_cmd_bind_pipeline(p_cmd, m_pipeline);
  
  tr_cmd_bind_descriptor_sets(p_cmd, m_pipeline, m_descriptor_set);
  
  tr_cmd_bind_vertex_buffers(p_cmd, 
                             (uint32_t)m_vertex_buffers.size(),
                             m_vertex_buffers.data());

  vertex_count = (vertex_count == UINT32_MAX) ? m_vertex_count : vertex_count;
  tr_cmd_draw(p_cmd, vertex_count, 0);
}

/*! @fn SimpleEntity::DrawIndexed */
inline void SimpleEntity::DrawIndexed(tr_cmd* p_cmd, uint32_t index_count) {
}

} // namespace tr

#endif // TINY_RENDERER_ENTITY_H
