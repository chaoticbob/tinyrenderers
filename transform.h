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

#include <functional>

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
/*
private:
  enum DirtyMask {
    DIRTY_MASK_TRANSLATE  = 0x01,
    DIRTY_MASK_ROTATE     = 0x02,
    DIRTY_MASK_SCALE      = 0x04,
    DIRTY_MASK_MODEL      = 0x08,
  };
*/

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

  void Clear() {
    m_model_matrix = float4x4(1.0f);
    ClearModelDirty();
  }

  void Translate(const float3& xyz) {
    float4x4 m = glm::translate(xyz);
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void Translate(float x, float y, float z) {
    Translate(float3(x, y, z));
  }

  void Rotate(const float3& xyz, RotationOrder rotation_order = ROTATION_ORDER_ZYX) {
    float4x4 rot_x = glm::rotate(xyz.x, float3(1, 0, 0));
    float4x4 rot_y = glm::rotate(xyz.y, float3(0, 1, 0));
    float4x4 rot_z = glm::rotate(xyz.z, float3(0, 0, 1));
    float4x4 m = float4x4(1.0f);
    switch (rotation_order) {
      case ROTATION_ORDER_ZYX: m = rot_z * rot_y * rot_x; break;
      case ROTATION_ORDER_ZXY: m = rot_z * rot_x * rot_y; break;
      case ROTATION_ORDER_YZX: m = rot_y * rot_z * rot_x; break;
      case ROTATION_ORDER_YXZ: m = rot_y * rot_x * rot_z; break;
      case ROTATION_ORDER_XZY: m = rot_x * rot_z * rot_y; break;
      case ROTATION_ORDER_XYZ: m = rot_x * rot_y * rot_z; break;
    }
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void Rotate(float x, float y, float z, RotationOrder rotation_order = ROTATION_ORDER_ZYX) {
    Rotate(float3(x, y, z), rotation_order);
  }

  void RotateX(float angle) {
    float4x4 m = glm::rotate(angle, float3(1, 0, 0));
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void RotateY(float angle) {
    float4x4 m = glm::rotate(angle, float3(0, 1, 0));
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void RotateZ(float angle) {
    float4x4 m = glm::rotate(angle, float3(0, 0, 1));
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void Scale(const float3& xyz) {
    float4x4 m = glm::translate(xyz);
    m_model_matrix = m * m_model_matrix;
    SetModelDirty();
  }

  void Scale(float x, float y, float z) {
    Scale(float3(x, y, z));
  }

  const float4x4& GetModelMatrix() const {   
    return m_model_matrix;
  }

  void SetModelChangedCallback(std::function<void(bool)> fn) {
    m_model_changed_callback = fn;
  }

private:
  void SetModelDirty() const {
    m_model_dirty = true;
    if (m_model_changed_callback) {
      m_model_changed_callback(true);
    }
  }

  void ClearModelDirty() const {
    m_model_dirty = false;
  }

private:
  mutable bool              m_model_dirty = true;
  mutable float4x4          m_model_matrix = float4x4(1.0f);
  std::function<void(bool)> m_model_changed_callback;

/*
  void Translate(const float3& delta) {
    m_translate += delta;
    SetDirty(DIRTY_MASK_TRANSLATE);
  }

  void SetTranslate(const float3& pos) {
    m_translate = pos;
    SetDirty(DIRTY_MASK_TRANSLATE);
  }

  void SetTranslate(float x, float y, float z) {
    m_translate = float3(x, y, z);
    SetDirty(DIRTY_MASK_TRANSLATE);
  }

  void SetRotate(float angle, const float3& axis) {
    m_rotate_axis_angle.w = angle;
    m_rotate_axis_angle.x = axis.x;
    m_rotate_axis_angle.y = axis.y;
    m_rotate_axis_angle.z = axis.z;
    m_rotation_mode = ROTATION_MODE_AXIS_ANGLE;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotateOrder(RotationOrder rotation_order) {
    m_rotation_order = rotation_order;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotate(const float3& angles) {
    m_rotate_euler = angles;
    m_rotation_mode = ROTATION_MODE_EULER;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotate(float x_angle, float y_angle, float z_angle) {
    m_rotate_euler = float3(x_angle, y_angle, z_angle);
    m_rotation_mode = ROTATION_MODE_EULER;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotateX(float angle) {
    m_rotate_euler.x = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotateY(float angle) {
    m_rotate_euler.y = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetRotateZ(float angle) {
    m_rotate_euler.z = angle;
    m_rotation_mode = ROTATION_MODE_EULER;
    SetDirty(DIRTY_MASK_ROTATE);
  }

  void SetScale(float s) {
    m_scale = float3(s);
    SetDirty(DIRTY_MASK_SCALE);
  }

  void SetScale(const float3& s) {
    m_scale = s;
    SetDirty(DIRTY_MASK_SCALE);
  }

  void SetScaleX(float sx) {
    m_scale.x = sx;
    SetDirty(DIRTY_MASK_SCALE);
  }

  void SetScaleY(float sy) {
    m_scale.y = sy;
    SetDirty(DIRTY_MASK_SCALE);
  }

  void SetScaleZ(float sz) {
    m_scale.z = sz;
    SetDirty(DIRTY_MASK_SCALE);
  }

  const float4x4& GetTranslateMatrix() const {
    if (m_translate_dirty) {
      m_translate_matrix = glm::translate(m_translate);
      ClearDirty(DIRTY_MASK_TRANSLATE);
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
      ClearDirty(DIRTY_MASK_ROTATE);
    }
    return m_rotate_matrix;
  }

 const float4x4& GetScaleMatrix() const {
   if (m_scale_dirty) {
     m_scale_matrix = glm::scale(m_scale);
     ClearDirty(DIRTY_MASK_SCALE);
   }
   return m_scale_matrix;
 }

  const float4x4& GetModelMatrix() const {
    if (m_model_dirty) {
      const float4x4& translate = GetTranslateMatrix();
      const float4x4& rotate = GetRotateMatrix();
      const float4x4& scale = GetScaleMatrix();
      m_model_matrix = translate * rotate * scale;
      ClearDirty(DIRTY_MASK_MODEL);
    }
    return m_model_matrix;
  }

  void SetModelChangedCallback(std::function<void(bool)> fn) {
    m_model_changed_callback = fn;
  }

private:
  void SetDirty(DirtyMask mask) const {
    if (mask & DIRTY_MASK_TRANSLATE) {
      m_translate_dirty = true;
    }

    if (mask & DIRTY_MASK_ROTATE) {
      m_rotate_dirty = true;
    }

    if (mask & DIRTY_MASK_SCALE) {
      m_scale_dirty = true;
    }

    m_model_dirty = m_translate_dirty | m_rotate_dirty | m_scale_dirty;
    if (m_model_dirty && m_model_changed_callback) {
      m_model_changed_callback(true);
    }
  }

  void ClearDirty(DirtyMask mask) const {
    if (mask & DIRTY_MASK_TRANSLATE) {
      m_translate_dirty = false;
    }

    if (mask & DIRTY_MASK_ROTATE) {
      m_rotate_dirty = false;
    }

    if (mask & DIRTY_MASK_SCALE) {
      m_scale_dirty = false;
    }

    if (mask & DIRTY_MASK_MODEL) {
      m_model_dirty = false;
    }
  }

private:
  enum RotationMode {
    ROTATION_MODE_EULER,      // Default
    ROTATION_MODE_AXIS_ANGLE,
  };

  RotationMode              m_rotation_mode     = ROTATION_MODE_EULER;
  RotationOrder             m_rotation_order    = ROTATION_ORDER_ZYX;
  float3                    m_translate         = float3(0, 0, 0);
  float3                    m_rotate_euler      = float3(0, 0, 0);
  float4                    m_rotate_axis_angle = float4(0, 0, 1, 0);
  float3                    m_scale             = float3(1, 1, 1);
  mutable bool              m_translate_dirty   = true;
  mutable bool              m_rotate_dirty      = true;
  mutable bool              m_scale_dirty       = true;
  mutable bool              m_model_dirty       = true;
  mutable float4x4          m_translate_matrix;
  mutable float4x4          m_rotate_matrix;
  mutable float4x4          m_scale_matrix;
  mutable float4x4          m_model_matrix;
  std::function<void(bool)> m_model_changed_callback;
*/
};

} // namespace tr

#endif // TR_TRANSFORM_H