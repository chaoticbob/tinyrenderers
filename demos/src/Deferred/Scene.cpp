#include "Scene.h"
#include "imgui_glfw.h"
#include "json.hpp"

#if defined(_DEBUG)
  #define FAST_DEBUG_LOAD
#endif

#define ENABLE_TUBE_SET_1
#define ENABLE_TUBE_SET_2
#define ENABLE_TUBE_SET_3
#define ENABLE_TUBE_SET_4
#define ENABLE_TUBE_SET_5

using tr::float3;
using json = nlohmann::json;

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
  
  // Load scene
  {
    tr::fs::path scene_file_path = asset_dir / "models/scene.json";
    assert(tr::fs::exists(scene_file_path) == true);

    std::ifstream is(scene_file_path.c_str());
    json scene;
    is >> scene;

    auto objects = scene["objects"];
    for (auto& obj : objects) {
      std::string name = obj["name"];
      std::string geo = obj["geo"];
      // Geometry - load if it's not in the map
      if (m_geometry_data.find(geo) == m_geometry_data.end()) {
        tr::fs::path geo_file_path = asset_dir / "models" / geo;
        GeometryData data = {};
        bool result = tr::Mesh::Load(geo_file_path, p_renderer, &data.vertex_buffer, &data.vertex_count, &data.object_space_bounds);
        assert(result == true);
        assert(data.vertex_buffer != nullptr);
        assert(data.vertex_count > 0);
        m_geometry_data[geo] = data;
      }
      // Geometry data
      auto& geo_data = m_geometry_data[geo];
      // Entity create info
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
      // Create entity
      {
        auto entity = std::make_unique<DeferredEntity>();
        entity->Create(p_renderer, entity_create_info);
        entity->SetName(name);
        entity->SetVertexBuffers(geo_data.vertex_buffer, geo_data.vertex_count);
        entity->SetObjectSpaceBounds(geo_data.object_space_bounds);
        // Initial transform data        
        float tx = obj["translate"]["x"];
        float ty = obj["translate"]["y"];
        float tz = obj["translate"]["z"];
        float rx = obj["rotate"]["x"];
        float ry = obj["rotate"]["y"];
        float rz = obj["rotate"]["z"];
        float sx = obj["scale"]["x"];
        float sy = obj["scale"]["y"];
        float sz = obj["scale"]["z"];
        std::string rotation_mode_str = obj["rotation_mode"];
        tr::Transform::RotationMode rotation_mode = tr::Transform::RotationModeFromString(rotation_mode_str);
        entity->GetModelTransform().Scale(sx, sy, sz);
        entity->GetModelTransform().Rotate(rx, ry, rz, rotation_mode);
        entity->GetModelTransform().Translate(tx, ty, tz);
        // Add entity to entities list
        m_entities.push_back(std::move(entity));
      }
    }
  }

/*
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

#if ! defined(FAST_DEBUG_LOAD)
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

    entity->GetMaterialParams().GetData().Color         = float3(0.9f, 0.9f, 0.9f);
    entity->GetMaterialParams().GetData().Specular      = 1.0f;
    entity->GetMaterialParams().GetData().Roughness     = 0.2f;
    entity->GetMaterialParams().GetData().Fresnel       = 0.3f;
    entity->GetMaterialParams().GetData().FresnelPower  = 4;

    m_entities.push_back(std::move(entity));
  }

  const float3 k_tube_color           = float3(0.8f, 0.8f, 0.97f);
  const float  k_tube_specular        = 3.5f;
  const float  k_tube_roughness       = 0.25f;
  const float  k_tube_fresnel         = 0.1f;
  const float  k_tube_fresnel_power   = 3.2f;

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

    entity->GetMaterialParams().GetData().Color         = k_tube_color;
    entity->GetMaterialParams().GetData().Specular      = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness     = k_tube_roughness;
    entity->GetMaterialParams().GetData().Fresnel       = k_tube_fresnel;
    entity->GetMaterialParams().GetData().FresnelPower  = k_tube_fresnel_power;

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

    entity->GetMaterialParams().GetData().Color         = k_tube_color;
    entity->GetMaterialParams().GetData().Specular      = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness     = k_tube_roughness;
    entity->GetMaterialParams().GetData().Fresnel       = k_tube_fresnel;
    entity->GetMaterialParams().GetData().FresnelPower  = k_tube_fresnel_power;


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

    entity->GetMaterialParams().GetData().Color         = k_tube_color;
    entity->GetMaterialParams().GetData().Specular      = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness     = k_tube_roughness;
    entity->GetMaterialParams().GetData().Fresnel       = k_tube_fresnel;
    entity->GetMaterialParams().GetData().FresnelPower  = k_tube_fresnel_power;


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

    entity->GetMaterialParams().GetData().Color         = k_tube_color;
    entity->GetMaterialParams().GetData().Specular      = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness     = k_tube_roughness;
    entity->GetMaterialParams().GetData().Fresnel       = k_tube_fresnel;
    entity->GetMaterialParams().GetData().FresnelPower  = k_tube_fresnel_power;


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

    entity->GetMaterialParams().GetData().Color         = k_tube_color;
    entity->GetMaterialParams().GetData().Specular      = k_tube_specular;
    entity->GetMaterialParams().GetData().Roughness     = k_tube_roughness;
    entity->GetMaterialParams().GetData().Fresnel       = k_tube_fresnel;
    entity->GetMaterialParams().GetData().FresnelPower  = k_tube_fresnel_power;


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

    entity->GetMaterialParams().GetData().Color         = float3(0.8f, 0.8f, 0.8f);
    entity->GetMaterialParams().GetData().Roughness     = 0.7f;
    entity->GetMaterialParams().GetData().Metallic      = 0.8f;
    entity->GetMaterialParams().GetData().Specular      = 1.5f;    
    entity->GetMaterialParams().GetData().Fresnel       = 0.2f;
    entity->GetMaterialParams().GetData().FresnelPower  = 2.2f;

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

    entity->GetMaterialParams().GetData().Color         = float3(0.8f, 0.8f, 0.8f);
    entity->GetMaterialParams().GetData().Roughness     = 0.7f;
    entity->GetMaterialParams().GetData().Metallic      = 0.8f;
    entity->GetMaterialParams().GetData().Specular      = 1.5f;    
    entity->GetMaterialParams().GetData().Fresnel       = 0.2f;
    entity->GetMaterialParams().GetData().FresnelPower  = 2.2f;

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

    entity->GetMaterialParams().GetData().Color         = float3(0.4f, 0.8f, 0.5f);
    entity->GetMaterialParams().GetData().Roughness     = 0.3f;
    entity->GetMaterialParams().GetData().Metallic      = 0.8f;
    entity->GetMaterialParams().GetData().Specular      = 2.5f;
    entity->GetMaterialParams().GetData().Fresnel       = 0.15f;
    entity->GetMaterialParams().GetData().FresnelPower  = 3.0f;

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

    entity->GetMaterialParams().GetData().Color         = float3(0.4f, 0.4f, 0.9f);
    entity->GetMaterialParams().GetData().Roughness     = 0.3f;
    entity->GetMaterialParams().GetData().Metallic      = 0.8f;
    entity->GetMaterialParams().GetData().Specular      = 2.5f;
    entity->GetMaterialParams().GetData().Fresnel       = 0.15f;
    entity->GetMaterialParams().GetData().FresnelPower  = 3.0f;

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

    entity->GetMaterialParams().GetData().Color         = float3(0.85f, 0.5f, 0.5f);
    entity->GetMaterialParams().GetData().Roughness     = 0.3f;
    entity->GetMaterialParams().GetData().Metallic      = 0.8f;
    entity->GetMaterialParams().GetData().Specular      = 2.5f;
    entity->GetMaterialParams().GetData().Fresnel       = 0.15f;
    entity->GetMaterialParams().GetData().FresnelPower  = 3.0f;

    m_entities.push_back(std::move(entity));
  }
#endif // ! defined(FAST_DEBUG)
*/
}

void DeferredTubeWorldScene::BuildDebugUi()
{
  for (auto& entity : m_entities) {
    if (ImGui::CollapsingHeader(entity->GetName().c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
      ImGui::PushID(entity.get());
      // Bounds
      //if (ImGui::CollapsingHeader("Bounds", ImGuiTreeNodeFlags_DefaultOpen)) 
      {
        auto& bounds = entity->GetViewSpaceBounds();
        ImGui::Text("Visible: %d", entity->GetCameraVisible());
        ImGui::Text("Min: %.6f, %.6f, %.6f", bounds.min.x, bounds.min.y, bounds.min.z);
        ImGui::Text("Max: %.6f, %.6f, %.6f", bounds.max.x, bounds.max.y, bounds.max.z);
      }
      //// Material
      //{
      //  auto& params = entity->GetMaterialParams();
      //  auto& data = params.GetData();
      //  ImGui::ColorEdit3("Color", (float*)data.Color.value_ptr());
      //  ImGui::SliderFloat("Roughness", data.Roughness.value_ptr(),       0.0, 1.0);
      //  ImGui::SliderFloat("Metallic", data.Metallic.value_ptr(),         0.0, 1.0);
      //  ImGui::SliderFloat("Specular", data.Specular.value_ptr(),         0.0, 5.0);
      //  ImGui::SliderFloat("Fresnel", data.Fresnel.value_ptr(),           0.0, 1.0);
      //  ImGui::SliderFloat("FresnelPower", data.FresnelPower.value_ptr(), 1.0, 10.0);
      //}
      ImGui::PopID();
    }
  }
}
