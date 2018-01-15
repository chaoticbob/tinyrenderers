#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef TINY_RENDERER_UTIL_H
#define TINY_RENDERER_UTIL_H

#if defined(TINY_RENDERER_DX)
  #include "tinydx.h"
#elif defined(TINY_RENDERER_VK)
  #include "tinyvk.h"
#endif

#include <array>
#include <string>
#include <vector>

namespace tr {

/*! @struct Box3

*/
struct AABB {
  float3 min;
  float3 max;
  
  void Set(const AABB& b) {
    min = glm::min(b.min, b.max);
    max = glm::max(b.min, b.max);
  }

  void Fit(const float3& P) {
    if (initialized) {
      min = glm::min(min, P);
      max = glm::max(max, P);
    }
    else {
      min = P;
      max = P;
      initialized = true;
    }
  }

  void Transform(const float4x4& matrix) {
    float4 obb_points[8] = {
      { min.x, min.y, min.z, 1 },
      { max.x, min.y, min.z, 1 },
      { max.x, max.y, min.z, 1 },
      { min.x, max.y, min.z, 1 },
      { min.x, min.y, max.z, 1 },
      { max.x, min.y, max.z, 1 },
      { max.x, max.y, max.z, 1 },
      { min.x, max.y, max.z, 1 },
    };

    float4 min4 = matrix * obb_points[0];
    float4 max4 = min4;
    for (uint32_t i = 1; i < 7; ++i) {
      float4 p = matrix * obb_points[i];
      min4 = glm::min(min4, p);
      max4 = glm::max(max4, p);
    }

    min = float3(min4);
    max = float3(max4);
  }

  AABB GetTransformed(const float4x4& matrix) const {
    AABB transformed = *this;
    transformed.Transform(matrix);
    return transformed;
  }

private:
  bool initialized = false;
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

/*! @fn CreateShaderProgram - CS

*/
inline tr_shader_program* CreateShaderProgram(
  tr_renderer*        p_renderer,
  const fs::path&     cs_file_path,
  const std::string&  cs_entry_point
)
{
  tr_shader_program* p_shader_program = nullptr;

  auto cs = LoadShaderModule(cs_file_path);

  bool has_byte_code    = !cs.empty();
  bool has_entry_point  = !cs_entry_point.empty();

  if (has_byte_code && has_entry_point) {
    tr_create_shader_program_compute(p_renderer, 
                                     (uint32_t)cs.size(), (uint32_t*)cs.data(), cs_entry_point.c_str(),
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

#endif // TINY_RENDERER_UTIL_H