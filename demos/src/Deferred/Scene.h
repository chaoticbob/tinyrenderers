#ifndef DEMO_SCENE_H
#define DEMO_SCENE_H

#include "DeferredRenderer.h"
#include "filesystem.h"

#include <map>
#include <memory>

/*! @class Scene

*/
template <typename SceneEntityT>
class Scene {
public:
  Scene() {}
  virtual ~Scene() {}

  virtual void Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, tr_buffer* p_gpu_view_params, tr_render_pass* p_gbuffer_render_pass) = 0;

  void UpdateDescriptorSets() {
    for (auto& entity : m_entities) {
      entity->UpdateGpuDescriptorSets();
    }
  }

  virtual void BuildUi() {};

  void SetViewTransform(const tr::Camera& camera) {
    for (auto& entity : m_entities) {
      entity->SetViewTransform(camera);
    }
  }

  void FinalizeTransforms() {
    for (auto& entity : m_entities) {
      entity->FinalizeTransforms();
    }
  }

  void Cull(const tr::Camera& camera) {
    m_visible_object_count = 0;
    for (auto& entity : m_entities) {
      auto& bounds = entity->GetViewSpaceBounds();
      bool is_out = (bounds.min.z >= 0) && (bounds.max.z >= 0);
      bool visible = !is_out;
      entity->SetCameraVisible(visible);
      m_visible_object_count += visible ? 1 : 0;
    }
  }

  uint32_t GetTotalObjectCount() const {
    uint32_t count = static_cast<uint32_t>(m_entities.size());
    return count;
  }

  uint32_t GetVisibleObjectCount() const {
    return m_visible_object_count;
  }

  void UpdateGpuBuffers(tr_cmd* p_cmd) {
    for (auto& entity : m_entities) {
      entity->UpdateGpuBuffers(p_cmd);
    }
  }

  void Draw(tr_cmd* p_cmd) {
    for (auto& entity : m_entities) {
      entity->Draw(p_cmd);
    }
  }

protected:
  struct GeometryData {
    tr_buffer*  vertex_buffer;
    uint32_t    vertex_count;
    tr::AABB    object_space_bounds;
  };

  std::map<std::string, GeometryData>         m_geometry_data;
  std::vector<std::unique_ptr<SceneEntityT>>  m_entities;
  uint32_t                                    m_visible_object_count = 0;
};

/*! @class TubeWorldScene

*/
class DeferredTubeWorldScene : public Scene<DeferredEntity> {
public:
  DeferredTubeWorldScene() {}
  virtual ~DeferredTubeWorldScene() {}

  virtual void Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, tr_buffer* p_gpu_view_params, tr_render_pass* p_gbuffer_render_pass);

  virtual void BuildDebugUi();

private:
  tr_shader_program*  g_deferred_color_shader = nullptr;
  tr_shader_program*  g_deferred_texture_shader = nullptr;
};

#endif // DEMO_SCENE_H