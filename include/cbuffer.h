#ifndef __cplusplus
  #error "C++ is required"
#endif

#ifndef CBUFFER_H
#define CBUFFER_H

#include <cstring>
#include "camera.h"
#include "transform.h"

namespace tr {

// =================================================================================================
// HLSL INT
// =================================================================================================

/*! @struct hlsl_int

*/
template <size_t PaddedSize> struct hlsl_int {
  union {
    int value;
    int padded[PaddedSize] = {};
  };

  hlsl_int() {}

  hlsl_int(int x) : value(x) {}

  ~hlsl_int() {}

  operator int() const {
    return value;
  }

  hlsl_int& operator=(const int& rhs) {
    this->value = rhs;
    return*this;
  }

  int* value_ptr() {
    return &value;
  }

  const int* value_ptr() const {
    return &value;
  }
};

/*! @struct hlsl_int2

*/
template <size_t PaddedSize> struct hlsl_int2 {
  union {
    int2  value;
    int   padded[PaddedSize] = {};
  };

  hlsl_int2() {}

  hlsl_int2(int x, int y) : value(x, y) {}

  hlsl_int2(const int2& obj) : value(obj) {}

  ~hlsl_int2() {}

  operator int2() const {
    return value;
  }

  hlsl_int2& operator=(const int2& rhs) {
    this->value = rhs;
    return*this;
  }
};

/*! @struct hlsl_int3

*/
template <size_t PaddedSize> struct hlsl_int3 {
  union {
    int3  value;
    int   padded[PaddedSize] = {};
  };

  hlsl_int3() {}

  hlsl_int3(int x, int y, int z) : value(x, y, z) {}

  hlsl_int3(const int3& obj) : value(obj) {}

  ~hlsl_int3() {}

  operator int3() const {
    return value;
  }

  hlsl_int3& operator=(const int3& rhs) {
    this->value = rhs;
    return*this;
  }
};

/*! @struct hlsl_int4

*/
template <size_t PaddedSize> struct hlsl_int4 {
  union {
    int4    value = {};
    float   padded[PaddedSize];
  };

  hlsl_int4() {}

  hlsl_int4(int x, int y, int z, int w) : value(x, y, z, w) {}

  hlsl_int4(const int4& obj) : value(obj) {}

  ~hlsl_int4() {}

  operator int4() const {
    return value;
  }

  hlsl_int4& operator=(const int4& rhs) {
    this->value = rhs;
    return*this;
  }
};

// =================================================================================================
// HLSL UINT
// =================================================================================================

/*! @struct hlsl_uint

*/
template <size_t PaddedSize> struct hlsl_uint {
  union {
    uint value;
    uint padded[PaddedSize] = {};
  };

  hlsl_uint() {}

  hlsl_uint(uint x) : value(x) {}

  hlsl_uint(const uint& obj) : value(obj) {}

  ~hlsl_uint() {}

  operator uint() const {
    return value;
  }

  uint& operator=(const uint& rhs) {
    this->value = rhs;
    return*this;
  }

  uint* value_ptr() {
    return &value;
  }

  const int32_t* value_ptr() const {
    return &value;
  }
};

/*! @struct hlsl_uint2

*/
template <size_t PaddedSize> struct hlsl_uint2 {
  union {
    uint2  value;
    uint   padded[PaddedSize] = {};
  };

  hlsl_uint2() {}

  hlsl_uint2(uint x, uint y) : value(x, y) {}

  hlsl_uint2(const uint2& obj) : value(obj) {}

  ~hlsl_uint2() {}

  operator uint2() const {
    return value;
  }

  hlsl_uint2& operator=(const uint2& rhs) {
    this->value = rhs;
    return*this;
  }
};

/*! @struct hlsl_uint3

*/
template <size_t PaddedSize> struct hlsl_uint3 {
  union {
    uint3  value;
    uint   padded[PaddedSize] = {};
  };

  hlsl_uint3() {}

  hlsl_uint3(uint x, uint y, uint z) : value(x, y, z) {}

  hlsl_uint3(const uint3& obj) : value(obj) {}

  ~hlsl_uint3() {}

  operator uint3() const {
    return value;
  }

  hlsl_uint3& operator=(const uint3& rhs) {
    this->value = rhs;
    return*this;
  }
};

/*! @struct hlsl_uint4

*/
template <size_t PaddedSize> struct hlsl_uint4 {
  union {
    uint4   value = {};
    float   padded[PaddedSize];
  };

  hlsl_uint4() {}

  hlsl_uint4(uint x, uint y, uint z, uint w) : value(x, y, z, w) {}

  hlsl_uint4(const uint4& obj) : value(obj) {}

  ~hlsl_uint4() {}

  operator uint4() const {
    return value;
  }

  hlsl_uint4& operator=(const uint4& rhs) {
    this->value = rhs;
    return*this;
  }
};

// =================================================================================================
// HLSL FLOAT
// =================================================================================================

/*! @struct hlsl_float

*/
template <size_t PaddedSize> struct hlsl_float {
  union {
    float value;
    float padded[PaddedSize] = {};
  };

  hlsl_float() {}

  hlsl_float(float x) : value(x) {}

  ~hlsl_float() {}

  operator float() const {
    return value;
  }

  hlsl_float& operator=(const float& rhs) {
    this->value = rhs;
    return*this;
  }

  float* value_ptr() {
    return &value;
  }

  const float* value_ptr() const {
    return &value;
  }
};

/*! @struct hlsl_float2

*/
template <size_t PaddedSize> struct hlsl_float2 {
  union {
    float2  value;
    float   padded[PaddedSize] = {};
  };

  hlsl_float2() {}

  hlsl_float2(float x, float y) : value(x, y) {}

  hlsl_float2(const float2& obj) : value(obj) {}

  ~hlsl_float2() {}

  operator float2() const {
    return value;
  }

  hlsl_float2& operator=(const float2& rhs) {
    this->value = rhs;
    return*this;
  }
};

/*! @struct hlsl_float3

*/
template <size_t PaddedSize> struct hlsl_float3 {
  union {
    float3  value;
    float   padded[PaddedSize] = {};
  };

  hlsl_float3() {}

  hlsl_float3(float x, float y, float z) : value(x, y, z) {}

  hlsl_float3(const float3& obj) : value(obj) {}

  ~hlsl_float3() {}

  operator float3() const {
    return value;
  }

  hlsl_float3& operator=(const float3& rhs) {
    this->value = rhs;
    return*this;
  }

  float3* value_ptr() {
    return &value;
  }

  const float3* value_ptr() const {
    return &value;
  }
};

/*! @struct hlsl_float4

*/
template <size_t PaddedSize> struct hlsl_float4 {
  union {
    float4  value = {};
    float   padded[PaddedSize];
  };

  hlsl_float4() {}

  hlsl_float4(float x, float y, float z, float w) : value(x, y, z, w) {}

  hlsl_float4(const float4& obj) : value(obj) {}

  ~hlsl_float4() {}

  operator float4() const {
    return value;
  }

  hlsl_float4& operator=(const float4& rhs) {
    this->value = rhs;
    return*this;
  }
};

// =================================================================================================
// HLSL FLOAT2x2
// =================================================================================================

/*! @struct float2x2

*/
struct hlsl_float2x2 {
  union {
    hlsl_float2<4>  value[2];
    float           padded[8] = {};
  };

  hlsl_float2x2() {};

  hlsl_float2x2(const float2x2& mat) { 
    value[0] = mat[0];
    value[1] = mat[1];
  }

  ~hlsl_float2x2() {};

  operator float2x2() const {
    float2x2 m(value[0].value,
               value[1].value);
    return m;
  }

  hlsl_float2x2& operator=(const float2x2& rhs) {
    value[0] = rhs[0];
    value[1] = rhs[1];
    return *this;
  }

  float2x2 as_float2x2() const {
    float2x2 m = *this;
    return m;
  }
};

// =================================================================================================
// HLSL FLOAT3x3
// =================================================================================================

/*! @struct float3x3

*/
struct hlsl_float3x3 {
  union {
    hlsl_float3<4>  value[3];
    float           padded[12] = {};
  };

  hlsl_float3x3() {};

  hlsl_float3x3(const float3x3& mat) { 
    value[0] = mat[0];
    value[1] = mat[1];
    value[2] = mat[2];
  }

  ~hlsl_float3x3() {};

  operator float3x3() const {
    float3x3 m(value[0].value,
               value[1].value,
               value[2].value);
    return m;
  }

  hlsl_float3x3& operator=(const float3x3& rhs) {
    value[0] = rhs[0];
    value[1] = rhs[1];
    value[2] = rhs[2];
    return *this;
  }

  float3x3 as_float3x3() const {
    float3x3 m = *this;
    return m;
  }
};

// =================================================================================================
// HLSL FLOAT4x4
// =================================================================================================

/*! @struct hlsl_float4x4

*/
struct hlsl_float4x4 {
  union {
    hlsl_float4<4>  value[4];
    float           padded[16] = {};
  };

  hlsl_float4x4() {
    value[0] = hlsl_float4<4>(float4(1, 0, 0, 0));
    value[1] = hlsl_float4<4>(float4(0, 1, 0, 0));
    value[2] = hlsl_float4<4>(float4(0, 0, 1, 0));
    value[3] = hlsl_float4<4>(float4(0, 0, 0, 1));
  };

  hlsl_float4x4(const float4x4& mat) { 
    value[0] = mat[0];
    value[1] = mat[1];
    value[2] = mat[2];
    value[3] = mat[3];
  }

  ~hlsl_float4x4() {};

  operator float4x4() const {
    float4x4 m(value[0].value,
               value[1].value,
               value[2].value,
               value[3].value);
    return m;
  }

  hlsl_float4x4& operator=(const float4x4& rhs) {
    value[0] = rhs[0];
    value[1] = rhs[1];
    value[2] = rhs[2];
    value[3] = rhs[3];
    return *this;
  }

  hlsl_float4x4 operator*(const hlsl_float4x4& rhs) const {
    const hlsl_float4x4& lhs = *this;
    float4x4 a = float4x4(lhs.value[0].value, 
                          lhs.value[1].value, 
                          lhs.value[2].value, 
                          lhs.value[3].value);
    float4x4 b = float4x4(rhs.value[0].value, 
                          rhs.value[1].value, 
                          rhs.value[2].value,
                          rhs.value[3].value);
    float4x4 c = a * b;
    return c;
  }

  float4x4 as_float4x4() const {
    float4x4 m = *this;
    return m;
  }
};

// =================================================================================================
// ConstantBuffer
// =================================================================================================

/*! @class ConstantBuffer

*/
template <typename ConstantDataT>
class ConstantBuffer {
public:
  ConstantBuffer(void* p_target = nullptr) {
    m_data = &m_storage;

    if (p_target == nullptr) {
      p_target = &m_storage;
    }
    SetTarget(p_target);

    m_data_size = sizeof(*m_data);

    // Debug    
    m_debug_float_count = (m_data_size / 4) 
                        + ((m_data_size % 4) != 0 ? 1 :0);

    m_debug_float4x4_count = (m_data_size / sizeof(float4x4))
                           + ((m_data_size % sizeof(float4x4)) != 0 ? 1 : 0);
  }

  ~ConstantBuffer() {}

  void SetTarget(void* p_target, bool copy_current = true) {
    assert(p_target != nullptr);

    if (copy_current) {
      std::memcpy(p_target, m_data, m_data_size);
    }

    m_data = reinterpret_cast<ConstantDataT*>(p_target);
    // Debug   
    m_debug_floats = reinterpret_cast<float*>(m_data);
    m_debug_float4x4s = reinterpret_cast<float4x4*>(m_data);
  }

  uint32_t GetDataSize() const {
    return static_cast<uint32_t>(m_data_size);
  }

  ConstantDataT& GetData() {
    return *m_data;
  }

  const ConstantDataT& GetData() const {
    return *m_data;    
  }

  const ConstantDataT* GetDataPtr() const {
    return m_data;
  }

  void Write(void* p_dst_buffer) {
    if (p_dst_buffer == m_data) {
      return;
    }

    const void* p_src_buffer = GetDataPtr();
    size_t size = GetDataSize();
    // Empty struct
    if (size == 1) {
      return;
    }
    assert((size % 4) == 0);
    std::memcpy(p_dst_buffer, p_src_buffer, size);
  }

protected:
  ConstantDataT   m_storage = {};
  size_t          m_data_size = 0;
  ConstantDataT*  m_data = nullptr;
  // Debug
  size_t          m_debug_float_count = 0;
  float*          m_debug_floats = nullptr;
  size_t          m_debug_float4x4_count = 0;
  float4x4*       m_debug_float4x4s = nullptr;
};

// =================================================================================================
// Null
// =================================================================================================

/*! @struct NullData

*/
struct NullData {};

/*! @class NullParams

*/
class NullParams : public ConstantBuffer<NullData> {
public:
  NullParams() {}
  ~NullParams() {}
};

// =================================================================================================
// View
// =================================================================================================

/*! @struct ViewData

*/
struct ViewData {
  hlsl_float3<4>  EyePosition;
  hlsl_float4x4   ViewMatrix;
  hlsl_float4x4   ProjectionMatrix;
  hlsl_float4x4   ViewProjectionMatrix;
};

/*! @struct ViewParams

*/
class ViewParams : public ConstantBuffer<ViewData> {
public:
  ViewParams() {}
  ViewParams(const Camera& camera) { SetView(camera); }
  ~ViewParams() {}

  void SetView(const Camera& camera) {
    ViewData& data = GetData();

    data.EyePosition = camera.GetEyePosition();

    data.ViewMatrix = camera.GetViewMatrix();

    data.ProjectionMatrix = camera.GetProjectionMatrix();

    data.ViewProjectionMatrix = data.ProjectionMatrix 
                              * data.ViewMatrix;
  }
};

// =================================================================================================
// Transform
// =================================================================================================

/*! @struct TransformData

*/
struct TransformData {
  hlsl_float4x4   ModelMatrix;
  hlsl_float4x4   ViewMatrix;
  hlsl_float4x4   ProjectionMatrix;
  hlsl_float4x4   ModelViewMatrix;
  hlsl_float4x4   ModelViewProjectionMatrix;
  hlsl_float3x3   NormalMatrixWS;
  hlsl_float3x3   NormalMatrixVS;
  hlsl_float3<4>  DebugColor;
};

/*! @struct TransformParams

*/
class TransformParams : public ConstantBuffer<TransformData> {
public:
  TransformParams() {}
  TransformParams(const Transform& transform) { SetModelTransform(transform); }
  ~TransformParams() {}

  void SetModelTransform(const Transform& transform) {
    TransformData& data = GetData();

    // Model, view, and projection matrices
    const float4x4& model_matrix = transform.GetModelMatrix();
    const float4x4 view_matrix = data.ViewMatrix.as_float4x4();
    const float4x4 projection_matrix = data.ProjectionMatrix.as_float4x4();
    const float4x4 model_view_matrix = view_matrix * model_matrix;
    const float4x4 mvp_matrix = projection_matrix * view_matrix * model_matrix;

    // Set model matrix
    data.ModelMatrix = model_matrix;

    // Set model view matrix
    data.ModelViewMatrix = model_view_matrix;

    // Set MVP
    data.ModelViewProjectionMatrix = mvp_matrix;
    
    // Calculate world space normal matrix
    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(model_matrix)));
    data.NormalMatrixWS = normal_matrix;
    
    // Calculate view space normal matrix
    normal_matrix = float3x3(glm::transpose(glm::inverse(model_view_matrix)));
    data.NormalMatrixVS = normal_matrix;
  }

  void SetViewTransform(const Camera& camera) {
    TransformData& data = GetData();

    // Model, view, and projection matrices
    const float4x4 model_matrix = data.ModelMatrix.as_float4x4();
    const float4x4& view_matrix = camera.GetViewMatrix();
    const float4x4& projection_matrix = camera.GetProjectionMatrix();
    const float4x4 model_view_matrix = view_matrix * model_matrix;
    const float4x4 mvp_matrix = projection_matrix * view_matrix * model_matrix;

    // Set view matrix
    data.ViewMatrix = view_matrix;

    // Set projection matrix
    data.ProjectionMatrix = mvp_matrix;

    // Set model view matrix
    data.ModelViewMatrix = model_view_matrix;

    // Set MVP
    data.ModelViewProjectionMatrix = mvp_matrix;

    // Calculate view space normal matrix
    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(model_view_matrix)));
    data.NormalMatrixVS = normal_matrix;
  }

  void SetDebugColor(const float3& color) {
    TransformData& data = GetData();
    data.DebugColor = color;
  }

  void SetDebugColor(float r, float g, float b) {
    TransformData& data = GetData();
    data.DebugColor = float3(r, g, b);
  }
};

// =================================================================================================
// ViewTransform
// =================================================================================================

/*! @struct ViewTransformData

*/
struct ViewTransformData {
  // View
  hlsl_float3<4>  EyePosition;
  hlsl_float4x4   ViewMatrix;
  hlsl_float4x4   ProjectionMatrix;
  hlsl_float4x4   ViewProjectionMatrix;
  // Transform
  hlsl_float4x4   ModelMatrix;
  hlsl_float4x4   ModelViewMatrix;
  hlsl_float4x4   ModelViewProjectionMatrix;
  hlsl_float3x3   NormalMatrixWS;
  hlsl_float3x3   NormalMatrixVS;
  hlsl_float3<4>  DebugColor;
};

/*! @class ViewTransformBuffer

*/
class ViewTransformBuffer : public ConstantBuffer<ViewTransformData> {
public:
  ViewTransformBuffer() {}
  ~ViewTransformBuffer() {}
 
  void SetView(const Camera& camera) {
    ViewTransformData& data = GetData();

    data.EyePosition = camera.GetEyePosition();

    data.ViewMatrix = camera.GetViewMatrix();

    data.ProjectionMatrix = camera.GetProjectionMatrix();

    data.ModelViewMatrix = data.ViewMatrix.as_float4x4()
                         * data.ModelMatrix.as_float4x4();

    data.ViewProjectionMatrix = data.ProjectionMatrix.as_float4x4()
                              * data.ViewMatrix.as_float4x4();

    data.ModelViewProjectionMatrix = data.ProjectionMatrix.as_float4x4()
                                   * data.ViewMatrix.as_float4x4()
                                   * data.ModelMatrix.as_float4x4();

    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(data.ModelViewMatrix.as_float4x4())));
    data.NormalMatrixVS = normal_matrix;
  }

  void SetTransform(const Transform& transform) {
    ViewTransformData& data = GetData();

    data.ModelMatrix = transform.GetModelMatrix();

    data.ModelViewMatrix = data.ViewMatrix 
                         * data.ModelMatrix;

    data.ModelViewProjectionMatrix = data.ProjectionMatrix
                                   * data.ViewMatrix
                                   * data.ModelMatrix;

    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(data.ModelMatrix.as_float4x4())));
    data.NormalMatrixWS = normal_matrix;

    normal_matrix = float3x3(glm::transpose(glm::inverse(data.ModelViewMatrix.as_float4x4())));
    data.NormalMatrixVS = normal_matrix;
  }

  void SetDebugColor(const float3& color) {
    ViewTransformData& data = GetData();
    data.DebugColor = color;
  }

  void SetDebugColor(float r, float g, float b) {
    ViewTransformData& data = GetData();
    data.DebugColor = float3(r, g, b);
  }
};

// =================================================================================================
// BRDFMaterialData
// =================================================================================================

/*! @struct BRDFMaterialData

*/
struct BRDFMaterialData {
  hlsl_float3<3>  BaseColor       = { 0.82f, 0.67f, 0.16f }; // offset = 0
  hlsl_float<1>   Metallic        = { 0.0f };
  hlsl_float<1>   Subsurface      = { 0.0f };             // offset = 4
  hlsl_float<1>   Specular        = { 0.5f };
  hlsl_float<1>   Roughness       = { 0.5f };
  hlsl_float<1>   SpecularTint    = { 0.0f };
  hlsl_float<1>   Anisotropic     = { 0.0f };             // offset = 8
  hlsl_float<1>   Sheen           = { 0.0f };
  hlsl_float<1>   SheenTint       = { 0.5f };
  hlsl_float<1>   ClearCoat       = { 0.0f };
  hlsl_float<1>   ClearCoatGloss  = { 1.0f };             // offset = 12
  hlsl_float<1>   kA              = { 0.0f };
  hlsl_float<1>   kD              = { 0.0f };
  hlsl_float<1>   kS              = { 0.0f };
};

/*! @class BRDFMaterialParams

*/
class BRDFMaterialParams : public ConstantBuffer<BRDFMaterialData> {
public:
  BRDFMaterialParams() {}
  ~BRDFMaterialParams() {}
};

// =================================================================================================
// Light Data
// =================================================================================================

struct AmbientLightData {
  hlsl_float3<3>  Color;
  hlsl_float<1>   Intensity;
};

struct PointLightData {
  hlsl_float3<4>  Position;             // Offset = 0
  hlsl_float3<3>  Color;                // Offset = 4
  hlsl_float<1>   Intensity = { 0.0f }; // Offset = 7
  hlsl_float<4>   FallOff;              // Offset = 8
};

struct SpotLightData {
  hlsl_float3<4>  Position;
  hlsl_float3<4>  Color;
  hlsl_float<1>   Intensity;
  hlsl_float<3>   FallOff;
  hlsl_float3<3>  Direction;
  hlsl_float<1>   ConeAngle;
};

struct DirectionalLightData {
  hlsl_float3<4>  Direction;
  hlsl_float3<3>  Color;
  hlsl_float<1>   Intensity;
};

// =================================================================================================
// Lighting Data
// =================================================================================================

/*! @struct LightingData

*/
template <size_t MAX_POINT_LIGHTS, size_t MAX_SPOT_LIGHTS, size_t MAX_DIRECTIONAL_LIGHTS>
struct LightingData {
  hlsl_float3<4>        EyePosition;
  AmbientLightData      AmbientLight;
  PointLightData        PointLights[MAX_POINT_LIGHTS];
  SpotLightData         SpotLights[MAX_SPOT_LIGHTS];
  DirectionalLightData  DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
};

/*! @class LightingParams

*/
template <size_t MAX_POINT_LIGHTS, size_t MAX_SPOT_LIGHTS, size_t MAX_DIRECTIONAL_LIGHTS> 
class LightingParams : public ConstantBuffer<LightingData<MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS, MAX_DIRECTIONAL_LIGHTS>> {
public:
  using DataT = LightingData<MAX_POINT_LIGHTS, MAX_SPOT_LIGHTS, MAX_DIRECTIONAL_LIGHTS>;

  LightingParams() {
    assert(MAX_POINT_LIGHTS       > 0);
    assert(MAX_SPOT_LIGHTS        > 0);
    assert(MAX_DIRECTIONAL_LIGHTS > 0);
  }

  virtual ~LightingParams() {}

  void ApplyView(const Camera& camera) {
    DataT& data = GetData();
    data.EyePosition = camera.GetEyePosition();
  }
};

// =================================================================================================
// TessData
// =================================================================================================

/*! @struct TessData

*/
struct TessData {
  hlsl_float<1> InsideTessFactor;
  hlsl_float<3> OutsideTessFactor;
};

/*! @class TessParams

*/
class TessParams : public ConstantBuffer<TessData> {
public:
  TessParams() {}
  ~TessParams() {}
};

// =================================================================================================
// SimpleMaterialData
// =================================================================================================

enum SimpleMaterialProfile {
  SIMPLE_MATERIAL_PROFILE_LAMBERT = 0,
  SIMPLE_MATERIAL_PROFILE_PHONG,
  SIMPLE_MATERIAL_PROFILE_BLINN_PHONG,
  SIMPLE_MATERIAL_PROFILE_MINNAERT,
  SIMPLE_MATERIAL_PROFILE_OREN_NAYAR,
  SIMPLE_MATERIAL_PROFILE_SCHLICK,  
  SIMPLE_MATERIAL_PROFILE_COOK_TORRANCE,
};

/*! @struct SimpleMaterialData

*/
struct SimpleMaterialData {
  hlsl_float3<3>  Color           = { 0.4f, 0.5f, 0.9f };   // offset = 0
  hlsl_float<1>   Power           = { 0.0f };
  hlsl_float<1>   Roughness       = { 1.0f };               // offset = 4
  hlsl_float3<3>  SpecularColor   = { 1.0f, 1.0f, 1.0f };
  hlsl_float<1>   kA              = { 0.2f };               // offset = 8
  hlsl_float<1>   kD              = { 0.7f };
  hlsl_float<2>   kS              = { 2.0f };
};

/*! @class SimpleMaterialParams

*/
class SimpleMaterialParams : public ConstantBuffer<SimpleMaterialData> {
public:
  SimpleMaterialParams() {}
  ~SimpleMaterialParams() {}

  void SetColor(const float3& Color) {
    SimpleMaterialData& data = GetData();
    data.Color = Color;
  }

  void SetRoughness(float roughness) {
    SimpleMaterialData& data = GetData();
    data.Roughness = roughness;
  }

  void SetPower(float power) {
    SimpleMaterialData& data = GetData();
    data.Power = power;
  }

  void SetSpecularColor(const float3& specular_color) {
    SimpleMaterialData& data = GetData();
    data.SpecularColor = specular_color;
  }

  void SetKA(float kA) {
    SimpleMaterialData& data = GetData();
    data.kA = kA;
  }

  void SetKD(float kD) {
    SimpleMaterialData& data = GetData();
    data.kD = kD;
  }

  void SetKS(float kS) {
    SimpleMaterialData& data = GetData();
    data.kS = kS;
  }

  void SetProfile(SimpleMaterialProfile profile) {
    SimpleMaterialData& data = GetData();
    SetProfile(profile, &data);
  }

  static void SetProfile(SimpleMaterialProfile profile, SimpleMaterialData* p_data) {
    switch (profile) {
      default:
      case SIMPLE_MATERIAL_PROFILE_LAMBERT       : SimpleMaterialParams::SetProfileLambert(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_PHONG         : SimpleMaterialParams::SetProfilePhong(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_BLINN_PHONG   : SimpleMaterialParams::SetProfileBlinnPhong(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_MINNAERT      : SimpleMaterialParams::SetProfileMinnaert(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_OREN_NAYAR    : SimpleMaterialParams::SetProfileOrenNayar(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_SCHLICK       : SimpleMaterialParams::SetProfileSchlick(p_data); break;
      case SIMPLE_MATERIAL_PROFILE_COOK_TORRANCE : SimpleMaterialParams::SetProfileCookTorrance(p_data); break;
    }
  }

  static void SetProfileLambert(SimpleMaterialData* p_data) {
    // Nothing =)
  }

  static void SetProfilePhong(SimpleMaterialData* p_data) {
    p_data->Power = 20.0f;
    p_data->kS = 3.0f;
  }

  static void SetProfileBlinnPhong(SimpleMaterialData* p_data) {
    p_data->Power = 20.0f;
    p_data->kS = 3.0f;
  }  

  static void SetProfileOrenNayar(SimpleMaterialData* p_data) {
    p_data->Roughness = 0.3f;
  }
  static void SetProfileMinnaert(SimpleMaterialData* p_data) {
    p_data->Power = 0.8f;
  }

  static void SetProfileSchlick(SimpleMaterialData* p_data) {
    p_data->Power = 0.2f;
    p_data->kS = 0.8f;
  }

  static void SetProfileCookTorrance(SimpleMaterialData* p_data) {
    p_data->Power = 0.6f;
    p_data->Roughness = 0.1f;
    p_data->kS = 0.9f;
  }
};

// =================================================================================================
// SimpleShaderData
// =================================================================================================

enum {
  SHADER_PARAMS_MAX_POINT_LIGHTS = 16
};

/*! @struct SimpleShaderData

*/
struct SimpleShaderData {
  ViewTransformData   ViewTransform;
  SimpleMaterialData  Material;
  AmbientLightData    AmbientLight;
  PointLightData      PointLights[SHADER_PARAMS_MAX_POINT_LIGHTS];
  TessData            Tess;
};

/*! @class SimpleShaderParams

*/
class SimpleShaderParams : public ConstantBuffer<SimpleShaderData> {
public:
  SimpleShaderParams() {}
  virtual ~SimpleShaderParams() {}

  void SetView(const Camera& camera) {
    SimpleShaderData& data = GetData();

    data.ViewTransform.EyePosition = camera.GetEyePosition();

    data.ViewTransform.ViewMatrix = camera.GetViewMatrix();

    data.ViewTransform.ProjectionMatrix = camera.GetProjectionMatrix();

    data.ViewTransform.ModelViewMatrix = data.ViewTransform.ViewMatrix.as_float4x4()
                                       * data.ViewTransform.ModelMatrix.as_float4x4();

    data.ViewTransform.ViewProjectionMatrix = data.ViewTransform.ProjectionMatrix.as_float4x4()
                                            * data.ViewTransform.ViewMatrix.as_float4x4();

    data.ViewTransform.ModelViewProjectionMatrix = data.ViewTransform.ProjectionMatrix.as_float4x4()
                                                 * data.ViewTransform.ViewMatrix.as_float4x4()
                                                 * data.ViewTransform.ModelMatrix.as_float4x4();

    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(data.ViewTransform.ModelViewMatrix.as_float4x4())));
    data.ViewTransform.NormalMatrixVS = normal_matrix;
  }

  void SetTransform(const Transform& transform) {
    SimpleShaderData& data = GetData();

    data.ViewTransform.ModelMatrix = transform.GetModelMatrix();

    data.ViewTransform.ModelViewMatrix = data.ViewTransform.ViewMatrix 
                                       * data.ViewTransform.ModelMatrix;

    data.ViewTransform.ModelViewProjectionMatrix = data.ViewTransform.ProjectionMatrix
                                                 * data.ViewTransform.ViewMatrix
                                                 * data.ViewTransform.ModelMatrix;

    float3x3 normal_matrix = float3x3(glm::transpose(glm::inverse(data.ViewTransform.ModelMatrix.as_float4x4())));
    data.ViewTransform.NormalMatrixWS = normal_matrix;

    normal_matrix = float3x3(glm::transpose(glm::inverse(data.ViewTransform.ModelViewMatrix.as_float4x4())));
    data.ViewTransform.NormalMatrixVS = normal_matrix;
  }

  void SetDebugColor(const float3& color) {
    SimpleShaderData& data = GetData();
    data.ViewTransform.DebugColor = color;
  }

  void SetDebugColor(float r, float g, float b) {
    SimpleShaderData& data = GetData();
    data.ViewTransform.DebugColor = float3(r, g, b);
  }
};

} // namespace tr

#endif // CBUFFER_H