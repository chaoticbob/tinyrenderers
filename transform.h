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
#include <glm/gtx/transform.hpp>

namespace tr {

using float2   = glm::vec2;
using float3   = glm::vec3;
using float4   = glm::vec4;
using float2x2 = glm::mat2x2;
using float2x3 = glm::mat2x3;
using float2x4 = glm::mat2x4;
using float3x2 = glm::mat3x2;
using float3x3 = glm::mat3x3;
using float3x4 = glm::mat3x4;
using float4x2 = glm::mat4x2;
using float4x3 = glm::mat4x3;
using float4x4 = glm::mat4x4;

/*! @class Transform

 Angles are always in radians unless otherwise stated.

*/
class Transform {
public:
  enum RotationOrder {
    ROTATION_ORDER_ZYX, // Default
    ROTATION_ORDER_ZXY,
    ROTATION_ORDER_YZX,
    ROTATION_ORDER_YXZ,
    ROTATION_ORDER_XZY,
    ROTATION_ORDER_XYZ,
  };

  Transform() {}
  ~Transform() {}

  void Translate(float3 pos) {
    m_translate = pos;
    m_translate_dirty = true;
    m_model_dirty = true;
  }

  void Rotate(float angle, float3 axis) {
    m_rotate_axis_angle.w = angle;
    m_rotate_axis_angle.x = axis.x;
    m_rotate_axis_angle.y = axis.y;
    m_rotate_axis_angle.z = axis.z;
    m_rotation_mode = ROTATION_MODE_AXIS_ANGLE;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void RotateOrder(RotationOrder rotation_order) {
    m_rotation_order = rotation_order;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void Rotate(float3 angles) {
    m_rotate_euler = angles;
    m_rotation_mode = ROTATION_MODE_EULER;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void Rotate(float x_angle, float y_angle, float z_angle) {
    m_rotate_euler = float3(x_angle, y_angle, z_angle);
    m_rotation_mode = ROTATION_MODE_EULER;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void RotateX(float angle) {
    m_rotate_euler.x = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void RotateY(float angle) {
    m_rotate_euler.y = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void RotateZ(float angle) {
    m_rotate_euler.z = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    m_rotate_dirty = true;
    m_model_dirty = true;
  }

  void Scale(float s) {
    m_scale = float3(s);
    m_scale_dirty = true;
    m_model_dirty = true;
  }

  void Scale(float3 s) {
    m_scale = s;
    m_scale_dirty = true;
    m_model_dirty = true;
  }

  void ScaleX(float sx) {
    m_scale.x = sx;
    m_scale_dirty = true;
    m_model_dirty = true;
  }

  void ScaleY(float sy) {
    m_scale.y = sy;
    m_scale_dirty = true;
    m_model_dirty = true;
  }

  void ScaleZ(float sz) {
    m_scale.z = sz;
    m_scale_dirty = true;
    m_model_dirty = true;
  }

  const float4x4& GetTranslateMatrix() const {
    if (m_translate_dirty) {
      m_translate_matrix = glm::translate(m_translate);
      m_translate_dirty = false;
    }
    return m_translate_matrix;
  }

  const float4x4& GetRotateMatrix() const {
    if (m_rotate_dirty) {
      if (m_rotation_mode == ROTATION_MODE_EULER) {
        float4x4 rot_x = glm::rotate(m_rotate_euler.x, float3(1, 0, 0));
        float4x4 rot_y = glm::rotate(m_rotate_euler.y, float3(0, 1, 0));
        float4x4 rot_z = glm::rotate(m_rotate_euler.z, float3(0, 0, 1));
        m_rotate_matrix = rot_z * rot_y * rot_x;
      }
      else {
        float angle = m_rotate_axis_angle.w;
        float3 axis = float3(m_rotate_axis_angle.x, m_rotate_axis_angle.y, m_rotate_axis_angle.z);
        m_rotate_matrix = glm::rotate(angle, axis);
      }
      m_rotate_dirty = false;
    }
    return m_rotate_matrix;
  }

 const float4x4& GetScaleMatrix() const {
   if (m_scale_dirty) {
     m_scale_matrix = glm::scale(m_scale);
     m_scale_dirty = false;
   }
   return m_scale_matrix;
 }

  const float4x4& GetModelMatrix() const {
    if (m_model_dirty) {
      const float4x4& translate = GetTranslateMatrix();
      const float4x4& rotate = GetRotateMatrix();
      const float4x4& scale = GetScaleMatrix();
      m_model_matrix = translate * rotate * scale;
      m_model_dirty = false;
    }
    return m_model_matrix;
  }

private:
  enum RotationMode {
    ROTATION_MODE_EULER,      // Default
    ROTATION_MODE_AXIS_ANGLE,
  };

  RotationMode      m_rotation_mode     = ROTATION_MODE_EULER;
  RotationOrder     m_rotation_order    = ROTATION_ORDER_ZYX;
  float3            m_translate         = float3(0, 0, 0);
  float3            m_rotate_euler      = float3(0, 0, 0);
  float4            m_rotate_axis_angle = float4(0, 0, 1, 0);
  float3            m_scale             = float3(1, 1, 1);
  mutable bool      m_translate_dirty   = true;
  mutable bool      m_rotate_dirty      = true;
  mutable bool      m_scale_dirty       = true;
  mutable bool      m_model_dirty       = true;
  mutable float4x4  m_translate_matrix;
  mutable float4x4  m_rotate_matrix;
  mutable float4x4  m_scale_matrix;
  mutable float4x4  m_model_matrix;
};

} // namespace tr

#endif // TR_TRANSFORM_H