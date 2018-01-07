#include "Scene.h"
#include "imgui_glfw.h"

#define ENABLE_TUBE_SET_1
#define ENABLE_TUBE_SET_2
#define ENABLE_TUBE_SET_3
#define ENABLE_TUBE_SET_4
#define ENABLE_TUBE_SET_5

using tr::float3;

// =================================================================================================
// DeferredTubeWorldScene
// =================================================================================================
void DeferredTubeWorldScene::Initialize(tr_renderer* p_renderer, const tr::fs::path& asset_dir, tr_buffer* p_gpu_view_params, tr_render_pass* p_gbuffer_render_pass)
{
  // Shaders
  {
    // Create deferred color
    {
#if defined(TINY_RENDERER_VK)
      tr::fs::path vs_file_path = asset_dir / "shaders/deferred_gbuffer_color.vs.spv";
      tr::fs::path ps_file_path = asset_dir / "shaders/deferred_gbuffer_color.vs.spv";
#elif defined(TINY_RENDERER_DX)
      tr::fs::path vs_file_path = asset_dir / "shaders/deferred_gbuffer_color.hlsl";
      tr::fs::path ps_file_path = asset_dir / "shaders/deferred_gbuffer_color.hlsl";
#endif
      g_deferred_color_shader = tr::CreateShaderProgram(p_renderer,
                                                        vs_file_path, "vsmain",
                                                        ps_file_path, "psmain");
      assert(g_deferred_color_shader != nullptr);
    }
  }
  
  tr::EntityCreateInfo entity_create_info = {};
  entity_create_info.shader_program                   = g_deferred_color_shader;
  entity_create_info.gpu_view_params                  = p_gpu_view_params;
  entity_create_info.view_params_binding              = DESCRIPTOR_BINDING_DEFERRED_GBUFFER_VIEW_PARAMS;
  entity_create_info.transform_params_binding         = DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TRANSFORM_PARAMS;
  entity_create_info.material_params_binding          = DESCRIPTOR_BINDING_DEFERRED_GBUFFER_MATERIAL_PARAMS;
  entity_create_info.lighting_params_binding          = tr::ENTITY_DESCRIPTOR_BINDING_DISABLED;
  entity_create_info.tess_params_binding              = tr::ENTITY_DESCRIPTOR_BINDING_DISABLED;;
  entity_create_info.vertex_layout                    = tr::Mesh::DefaultVertexLayout();
  entity_create_info.render_pass                      = p_gbuffer_render_pass;
  entity_create_info.pipeline_settings.primitive_topo = tr_primitive_topo_tri_list;
  entity_create_info.pipeline_settings.depth          = true;
  entity_create_info.pipeline_settings.cull_mode      = tr_cull_mode_back;

  // Main Structure
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/main_structure.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Main Structure");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.7f, 0.7f, 0.7f);
    entity->GetMaterialParams().GetData().Specular  = 0.2f;
    entity->GetMaterialParams().GetData().Roughness = 0.9f;
    
    m_entities.push_back(std::move(entity));
  }

  // Ground
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/ground.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.6f, 0.6f, 0.6f);
    entity->GetMaterialParams().GetData().Specular  = 0.2f;
    entity->GetMaterialParams().GetData().Roughness = 0.9f;

    m_entities.push_back(std::move(entity));
  }

  // Main Road
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/main_road.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Main Road");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.65f, 0.63f, 0.6f);
    entity->GetMaterialParams().GetData().Specular  = 0.2f;
    entity->GetMaterialParams().GetData().Roughness = 0.9f;

    m_entities.push_back(std::move(entity));
  }

  // Tube Connectors
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_connectors.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Connectors");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.9f, 0.9f, 0.9f);
    entity->GetMaterialParams().GetData().Specular  = 1.0f;
    entity->GetMaterialParams().GetData().Roughness = 0.001f;

    m_entities.push_back(std::move(entity));
  }

  const float3 k_tube_color       = float3(0.93f, 0.93f, 0.97f);
  const float  k_tube_specular    = 3.5f;
  const float  k_tube_roughness   = 0.3f;
  const float  k_tube_subsurface  = 0.5f;
  const float  k_tube_clearcoat   = 0.9f;

#if defined(ENABLE_TUBE_SET_1)
  // Tube Set 1
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_set_1.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Set 1");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color       = k_tube_color;
    entity->GetMaterialParams().GetData().Specular    = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness   = k_tube_roughness;
    entity->GetMaterialParams().GetData().Subsurface  = k_tube_subsurface;
    entity->GetMaterialParams().GetData().ClearCoat   = k_tube_clearcoat;

    m_entities.push_back(std::move(entity));
  }
#endif // defined(ENABLE_TUBE_SET_1)

#if defined(ENABLE_TUBE_SET_1)
  // Tube Set 2
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_set_2.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Set 2");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color       = k_tube_color;
    entity->GetMaterialParams().GetData().Specular    = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness   = k_tube_roughness;
    entity->GetMaterialParams().GetData().Subsurface  = k_tube_subsurface;
    entity->GetMaterialParams().GetData().ClearCoat   = k_tube_clearcoat;


    m_entities.push_back(std::move(entity));
  }
#endif // defined(ENABLE_TUBE_SET_2)

#if defined(ENABLE_TUBE_SET_3)
  // Tube Set 3
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_set_3.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Set 3");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color       = k_tube_color;
    entity->GetMaterialParams().GetData().Specular    = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness   = k_tube_roughness;
    entity->GetMaterialParams().GetData().Subsurface  = k_tube_subsurface;
    entity->GetMaterialParams().GetData().ClearCoat   = k_tube_clearcoat;


    m_entities.push_back(std::move(entity));
  }
#endif // defined(ENABLE_TUBE_SET_3)

#if defined(ENABLE_TUBE_SET_4)
  // Tube Set 4
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_set_4.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Set 4");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color       = k_tube_color;
    entity->GetMaterialParams().GetData().Specular    = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness   = k_tube_roughness;
    entity->GetMaterialParams().GetData().Subsurface  = k_tube_subsurface;
    entity->GetMaterialParams().GetData().ClearCoat   = k_tube_clearcoat;


    m_entities.push_back(std::move(entity));
  }
#endif // defined(ENABLE_TUBE_SET_4)

#if defined(ENABLE_TUBE_SET_5)
// Tube Set 5
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tube_set_5.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tube Set 5");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color       = k_tube_color;
    entity->GetMaterialParams().GetData().Specular    = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness   = k_tube_roughness;
    entity->GetMaterialParams().GetData().Subsurface  = k_tube_subsurface;
    entity->GetMaterialParams().GetData().ClearCoat   = k_tube_clearcoat;


    m_entities.push_back(std::move(entity));
  }
#endif // defined(ENABLE_TUBE_SET_5)


  // Tall Stakes Ground
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tall_stakes_ground.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tall Stakes Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.7f, 0.7f, 0.7f);
    entity->GetMaterialParams().GetData().Roughness = 0.2f;
    entity->GetMaterialParams().GetData().Metallic  = 0.5f;
    entity->GetMaterialParams().GetData().Specular  = 0.6f;

    m_entities.push_back(std::move(entity));
  }

  // Tall Stakes Rails Ground
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tall_stakes_rails_ground.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tall Stakes Rails Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.8f, 0.8f, 0.8f);
    entity->GetMaterialParams().GetData().Roughness = 0.7f;
    entity->GetMaterialParams().GetData().Metallic  = 0.8f;
    entity->GetMaterialParams().GetData().Specular  = 1.5f;

    m_entities.push_back(std::move(entity));
  }

    // Tall Stakes Air
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tall_stakes_air.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tall Stakes Air");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.7f, 0.7f, 0.7f);
    entity->GetMaterialParams().GetData().Roughness = 0.2f;
    entity->GetMaterialParams().GetData().Metallic  = 0.5f;
    entity->GetMaterialParams().GetData().Specular  = 0.6f;

    m_entities.push_back(std::move(entity));
  }

  // Tall Stakes Rails Air
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/tall_stakes_rails_air.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Tall Stakes Rails Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.8f, 0.8f, 0.8f);
    entity->GetMaterialParams().GetData().Roughness = 0.7f;
    entity->GetMaterialParams().GetData().Metallic  = 0.8f;
    entity->GetMaterialParams().GetData().Specular  = 1.5f;

    m_entities.push_back(std::move(entity));
  }

  // Clamps Tall Stakes Ground
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/clamps_tall_stakes_ground.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Clamps Tall Stakes Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.7f, 0.7f, 0.8f);
    entity->GetMaterialParams().GetData().Specular  = 2.0f;
    entity->GetMaterialParams().GetData().Roughness = 0.2f;

    m_entities.push_back(std::move(entity));
  }

  // Clamps Ground
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/clamps_ground.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Clamps Ground");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.7f, 0.7f, 0.8f);
    entity->GetMaterialParams().GetData().Roughness = 0.3f;
    entity->GetMaterialParams().GetData().Metallic  = 0.7f;
    entity->GetMaterialParams().GetData().Specular  = 2.5f;

    m_entities.push_back(std::move(entity));
  }

  // Clamps Air
  {
    tr_buffer* p_vertex_buffer = nullptr;
    uint32_t vertex_count = 0;
    tr::fs::path file_path = asset_dir / "models/clamps_air.obj";
    bool result = tr::Mesh::Load(file_path, p_renderer, &p_vertex_buffer, &vertex_count);
    assert(result == true);
    assert(p_vertex_buffer != nullptr);
    assert(vertex_count > 0);

    auto entity = std::make_unique<DeferredEntity>();
    entity->Create(p_renderer, entity_create_info);
    entity->SetName("Clamps Air");
    entity->SetVertexBuffers(p_vertex_buffer, vertex_count);

    entity->GetMaterialParams().GetData().Color     = float3(0.85f, 0.7f, 0.7f);
    entity->GetMaterialParams().GetData().Specular  = 2.0f;
    entity->GetMaterialParams().GetData().Roughness = 0.2f;

    m_entities.push_back(std::move(entity));
  }
}

void DeferredTubeWorldScene::BuildUi()
{
  for (auto& entity : m_entities) {
    if (ImGui::CollapsingHeader(entity->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(entity.get());
      // Material
      {
        auto& params = entity->GetMaterialParams();
        auto& data = params.GetData();
        ImGui::ColorEdit3("Color", (float*)data.Color.value_ptr());
        ImGui::SliderFloat("Roughness", data.Roughness.value_ptr(),   0.0, 1.0);
        ImGui::SliderFloat("Metallic", data.Metallic.value_ptr(),     0.0, 1.0);
        ImGui::SliderFloat("Specular", data.Specular.value_ptr(),     0.0, 5.0);
        ImGui::SliderFloat("Subsurface", data.Subsurface.value_ptr(), 0.0, 1.0);
        ImGui::SliderFloat("ClearCoat", data.ClearCoat.value_ptr(),   0.0, 1.0);
      }
      ImGui::PopID();
    }
  }
}
