#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef CBUFFER_H
#define CBUFFER_H

#include <cstring>
#include "camera.h"
#include "transform.h"

namespace tr {

/*! @class ConstantBuffer

*/
template <typename ConstantDataT>
class ConstantBuffer {
public:
  ConstantDataT data = {};

  ConstantBuffer() {}
  ~ConstantBuffer() {}

  uint32_t GetDataSize() const {
    uint32_t size = sizeof(data);
    return size;
  }

  const ConstantDataT* GetData() const {
    const ConstantDataT* p_data = &data;
    return p_data;
  }

  void Write(void* p_dst_buffer) {
    const void* p_src_buffer = GetData();
    size_t size = GetDataSize();
    // Empty struct
    if (size == 1) {
      return;
    }
    assert((size % 4) == 0);
    std::memcpy(p_dst_buffer, p_src_buffer, size);
  }
};

/*! @struct NullData

*/
struct NullData {};

/*! @class NullBuffer

*/
class NullBuffer : public ConstantBuffer<NullData> {
public:
  NullBuffer() {}
  ~NullBuffer() {}
};

/*! @struct ViewData

*/
struct ViewTransformData {
  float4x4  model_matrix;                   // float4x4 in HLSL and C++
  float4x4  view_matrix;                    // float4x4 in HLSL and C++
  float4x4  projection_matrix;              // float4x4 in HLSL and C++
  float4x4  model_view_matrix;              // float4x4 in HLSL and C++
  float4x4  view_projection_matrix;         // float4x4 in HLSL and C++
  float4x4  model_view_projection_matrix;   // float4x4 in HLSL and C++
  float3x4  normal_matrix_world_space;      // float3x3 in HLSL, float3x4 in C++ to pad for alignment
  float3x4  normal_matrix_view_space;       // float3x3 in HLSL, float3x4 in C++ to pad for alignment
  float4    view_direction;                 // float3 in HLSL, float4 in C++ to pad for alignment
  float4    color;                          // float3 in HLSL, float4 in C++ to pad for alignment
};

/*! @class ViewTransformBuffer

*/
class ViewTransformBuffer : public ConstantBuffer<ViewTransformData> {
public:
  ViewTransformBuffer() {}
  ~ViewTransformBuffer() {}
 
  void SetView(const Camera& camera) {
    data.view_matrix = camera.GetViewMatrix();

    data.projection_matrix = camera.GetProjectionMatrix();

    data.model_view_matrix = data.view_matrix 
                           * data.model_matrix;

    data.view_projection_matrix = data.projection_matrix 
                                * data.view_matrix;

    data.model_view_projection_matrix = data.projection_matrix
                                      * data.view_matrix
                                      * data.model_matrix;

    data.view_direction = float4(camera.GetViewDirection(), 0);

    float3x3 normal = float3x3(glm::transpose(glm::inverse(data.model_view_matrix)));
    data.normal_matrix_view_space[0] = float4(normal[0], 0.0f);
    data.normal_matrix_view_space[1] = float4(normal[1], 0.0f);
    data.normal_matrix_view_space[2] = float4(normal[2], 0.0f);
  }

  void SetTransform(const Transform& transform) {
    data.model_matrix = transform.GetModelMatrix();

    data.model_view_matrix = data.view_matrix 
                           * data.model_matrix;

    data.model_view_projection_matrix = data.projection_matrix
                                      * data.view_matrix
                                      * data.model_matrix;

    float3x3 normal = float3x3(glm::transpose(glm::inverse(data.model_matrix)));
    data.normal_matrix_world_space[0] = float4(normal[0], 0.0f);
    data.normal_matrix_world_space[1] = float4(normal[1], 0.0f);
    data.normal_matrix_world_space[2] = float4(normal[2], 0.0f);

    normal = float3x3(glm::transpose(glm::inverse(data.model_view_matrix)));
    data.normal_matrix_view_space[0] = float4(normal[0], 0.0f);
    data.normal_matrix_view_space[1] = float4(normal[1], 0.0f);
    data.normal_matrix_view_space[2] = float4(normal[2], 0.0f);
  }

  void SetColor(const float3& color) {
    data.color = float4(color, 0);
  }

  void SetColor(float r, float g, float b) {
    data.color = float4(r, g, b, 0);
  }
};

/*! @struct BlinnPhongData

*/
struct BlinnPhongData {
  // float3 in HLSL, float4 in C++ to pad for alignment
  float4  base_color;       // 0.82, 0.67, 0.16
  float4  specular_color;   // 1.00, 1.00, 1.00
  float4  specular_power;   // 12.0, 12.0, 12.0
  float4  kA;               // 0.30, 0.30, 0.30
  float4  kD;               // 0.50, 0.50, 0.50
  float4  kS;               // 1.00, 1.00, 1.00
};

/*! @class BlingPhongBuffer

*/
class BlinnPhongBuffer : public ConstantBuffer<BlinnPhongData> {
public:
  BlinnPhongBuffer() {}
  ~BlinnPhongBuffer() {}

  void SetBaseColor(const float3& base_color) {
    data.base_color = float4(base_color, 0);
  }

  void SetSpecularColor(const float3& specular_color) {
    data.specular_color = float4(specular_color, 0);
  }

  void SetSpecularPower(const float3& specular_power) {
    data.specular_power = float4(specular_power, 0);
  }

  void SetKA(const float3& kA) {
    data.kA = float4(kA, 0);
  }

  void SetKD(const float3& kD) {
    data.kD = float4(kD, 0);
  }

  void SetKS(const float3& kS) {
    data.kS = float4(kS, 0);
  }
};

/*! @struct GGXData

*/
struct GGXData {
  // float3 in HLSL, float4 in C++ to pad for alignment
  float4  base_color;       // 0.82, 0.67, 0.16
  float4  metallic;         // 0.00, 1.00, 0.00 
  float4  subsurface;       // 0.00, 1.00, 0.00 
  float4  specular;         // 0.00, 1.00, 0.50
  float4  roughness;        // 0.00, 1.00, 0.50
  float4  specular_tint;    // 0.00, 1.00, 0.00 
  float4  anisotropic;      // 0.00, 1.00, 0.00 
  float4  sheen;            // 0.00, 1.00, 0.00 
  float4  sheen_tint;       // 0.00, 1.00, 0.50
  float4  clearcoat;        // 0.00, 1.00, 0.00 
  float4  clearcoat_gloss;  // 0.00, 1.00, 1.00
  float4  kA;               // 0.30, 0.30, 0.30
  float4  kD;               // 0.50, 0.50, 0.50
  float4  kS;               // 1.00, 1.00, 1.00
};

/*! @class GGXBuffer

*/
class GGXBuffer : public ConstantBuffer<GGXData> {
public:
  GGXBuffer();
  ~GGXBuffer();

  void SetKA(const float3& kA) {
    data.kA = float4(kA, 0);
  }

  void SetKD(const float3& kD) {
    data.kD = float4(kD, 0);
  }

  void SetKS(const float3& kS) {
    data.kS = float4(kS, 0);
  }
};

} // namespace tr

#endif // CBUFFER_H