#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef TR_TRANSFORM_H
#define TR_TRANSFORM_H

#define GLM_FORCE_RADIANS 
#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>

#include <algorithm>
#include <cctype>
#include <functional>
#include <string>

namespace tr {

using int2      = glm::ivec2;
using int3      = glm::ivec3;
using int4      = glm::ivec4;
using uint      = glm::uint;
using uint2     = glm::uvec2;
using uint3     = glm::uvec3;
using uint4     = glm::uvec4;
using float2    = glm::vec2;
using float3    = glm::vec3;
using float4    = glm::vec4;
using float2x2  = glm::mat2x2;
using float2x3  = glm::mat2x3;
using float2x4  = glm::mat2x4;
using float3x2  = glm::mat3x2;
using float3x3  = glm::mat3x3;
using float3x4  = glm::mat3x4;
using float4x2  = glm::mat4x2;
using float4x3  = glm::mat4x3;
using float4x4  = glm::mat4x4;

/*! @class Transform

 Angles are always in radians unless otherwise stated.

*/
class Transform {
public:
  enum RotationMode {
    ROTATION_MODE_UNKNOWN,

    ROTATION_MODE_ZYX, // Default
    ROTATION_MODE_ZXY,
    ROTATION_MODE_YZX,
    ROTATION_MODE_YXZ,
    ROTATION_MODE_XZY,
    ROTATION_MODE_XYZ,
    ROTATION_MODE_BLENDER_XYZ,

    ROTATION_MODE_DEFAULT = ROTATION_MODE_ZYX,
  };

  Transform() {}
  ~Transform() {}

  void Clear() {
    m_model_matrix = float4x4(1.0f);
  }

  void Translate(const float3& xyz) {
    float4x4 m = glm::translate(xyz);
    m_model_matrix = m * m_model_matrix;
  }

  void Translate(float x, float y, float z) {
    Translate(float3(x, y, z));
  }

  void Rotate(const float3& xyz, RotationMode rotation_order = ROTATION_MODE_DEFAULT) {
    //float4x4 rot_x = glm::rotate(xyz.x, float3(1, 0, 0));
    //float4x4 rot_y = glm::rotate(xyz.y, float3(0, 1, 0));
    //float4x4 rot_z = glm::rotate(xyz.z, float3(0, 0, 1));

    float4x4 rot_x = glm::eulerAngleX(xyz.x);
    float4x4 rot_y = glm::eulerAngleY(xyz.y);
    float4x4 rot_z = glm::eulerAngleZ(xyz.z);
    float4x4 m = float4x4(1);
    switch (rotation_order) {
      default: assert(false && "unsupported rotation mode"); break;
      case ROTATION_MODE_ZYX         : m = rot_x * rot_y * rot_z; break;
      case ROTATION_MODE_ZXY         : m = rot_y * rot_x * rot_z; break;
      case ROTATION_MODE_YZX         : m = rot_x * rot_z * rot_y; break;
      case ROTATION_MODE_YXZ         : m = rot_z * rot_x * rot_y; break;
      case ROTATION_MODE_XZY         : m = rot_y * rot_z * rot_x; break;
      case ROTATION_MODE_XYZ         : m = rot_z * rot_y * rot_x; break;
      case ROTATION_MODE_BLENDER_XYZ : m = rot_y * rot_z * rot_x; break;
    }    
    m_model_matrix = m * m_model_matrix;
  }

  void Rotate(float x, float y, float z, RotationMode rotation_order = ROTATION_MODE_DEFAULT) {
    Rotate(float3(x, y, z), rotation_order);
  }

  void RotateX(float angle) {
    float4x4 m = glm::rotate(angle, float3(1, 0, 0));
    m_model_matrix = m * m_model_matrix;
  }

  void RotateY(float angle) {
    float4x4 m = glm::rotate(angle, float3(0, 1, 0));
    m_model_matrix = m * m_model_matrix;
  }

  void RotateZ(float angle) {
    float4x4 m = glm::rotate(angle, float3(0, 0, 1));
    m_model_matrix = m * m_model_matrix;
  }

  void Scale(const float3& xyz) {
    float4x4 m = glm::scale(xyz);
    m_model_matrix = m * m_model_matrix;
  }

  void Scale(float x, float y, float z) {
    Scale(float3(x, y, z));
  }

  const float4x4& GetModelMatrix() const {   
    return m_model_matrix;
  }

  static Transform::RotationMode RotationModeFromString(const std::string& mode_str) {
    Transform::RotationMode rotation_mode = Transform::ROTATION_MODE_UNKNOWN;
    std::string uc_mode_str = mode_str;
    std::transform(std::begin(uc_mode_str), std::end(uc_mode_str), std::begin(uc_mode_str), std::toupper);    
    if (uc_mode_str == "ZYX") rotation_mode = ROTATION_MODE_ZYX;
    else if (uc_mode_str == "ZXY") rotation_mode = ROTATION_MODE_ZXY;
    else if (uc_mode_str == "YZX") rotation_mode = ROTATION_MODE_YZX;
    else if (uc_mode_str == "YXZ") rotation_mode = ROTATION_MODE_YXZ;
    else if (uc_mode_str == "XZY") rotation_mode = ROTATION_MODE_XZY;
    else if (uc_mode_str == "XYZ") rotation_mode = ROTATION_MODE_XYZ;
    else if (uc_mode_str == "BLENDER_XYZ") rotation_mode = ROTATION_MODE_BLENDER_XYZ;
    return rotation_mode;
  }

private:
  float4x4  m_model_matrix = float4x4(1.0f);
};

} // namespace tr

#endif // TR_TRANSFORM_H