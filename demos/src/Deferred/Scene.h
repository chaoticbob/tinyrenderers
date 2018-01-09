#ifndef DEMO_SCENE_H
#define DEMO_SCENE_H

#include "DeferredRenderer.h"
#include "filesystem.h"

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

  void ApplyView(const tr::Camera& camera) {
    for (auto& entity : m_entities) {
      entity->ApplyView(camera);
    }
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
  std::vector<std::unique_ptr<SceneEntityT>>  m_entities;
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