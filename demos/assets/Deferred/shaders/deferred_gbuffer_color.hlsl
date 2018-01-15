
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_VIEW_PARAMS         b0
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TRANSFORM_PARAMS    b1
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_MATERIAL_PARAMS     b2
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TESS_PARAMS         b3
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_ALBEDO_TEX          t4
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_SPECULAR_TEX        t5
#define DESCRIPTOR_BINDING_DEFERRED_GBUFFER_NORMAL_TEX          t6

struct ViewData {
  float3    EyePosition;
  float4x4  ViewMatrix;
  float4x4  ProjectionMatrix;
  float4x4  ViewProjectionMatrix;
};

struct TransformData {
  float4x4  ModelMatrix;
  float4x4  ViewMatrix;
  float4x4  ProjectionMatrix;
  float4x4  ModelViewMatrix;
  float4x4  ModelViewProjectionMatrix;
  float3x3  NormalMatrixWS;
  float3x3  NormalMatrixVS;
  float3    DebugColor;
};

struct DeferredMaterialData {
  float3  Color;
  float   Roughness;
  float   Metallic;
  float   Specular;
  float   Fresnel;
  float   FresnelPower;
};

// =================================================================================================
// Resources
// =================================================================================================
ConstantBuffer<ViewData>              ViewParams      : register(DESCRIPTOR_BINDING_DEFERRED_GBUFFER_VIEW_PARAMS);
ConstantBuffer<TransformData>         TransformParams : register(DESCRIPTOR_BINDING_DEFERRED_GBUFFER_TRANSFORM_PARAMS);
ConstantBuffer<DeferredMaterialData>  MaterialParams  : register(DESCRIPTOR_BINDING_DEFERRED_GBUFFER_MATERIAL_PARAMS);

// =================================================================================================
// Signatures
// =================================================================================================
struct VSInput {
  float3  PositionOS  : POSITION;
  float3  NormalOS    : NORMAL;
  float2  TexCoord    : TEXCOORD;
};

struct VSOutput {
  float4  PositionCS  : SV_Position;
  float3  PositionWS  : POSITION;
  float3  NormalWS    : NORMAL;
  float2  TexCoord    : TEXCOORD;
  float3  PositionVS  : POSITION_VS;
};

typedef VSOutput PSInput;

struct PSOutput {
  // XYZ  = Position
  // A    = ?
  float4  GBuffer0 : SV_Target0;
  // XYZ  = Normal
  // A    = ?
  float4  GBuffer1 : SV_Target1;
  // XYZ  = Albedo (aka Diffuse)
  // A    = Roughness
  float4  GBuffer2 : SV_Target2;
  // X    = Metallic
  // Y    = Specular
  // Z    = Fresnel
  // A    = FresnelPower
  float4  GBuffer3 : SV_Target3;
};

// =================================================================================================
// vsmain
// =================================================================================================
VSOutput vsmain(VSInput input)
{
  float4 Position4 = float4(input.PositionOS, 1);

  VSOutput output = (VSOutput)0;
  output.PositionCS = mul(TransformParams.ModelViewProjectionMatrix, Position4);
  output.PositionWS = mul(TransformParams.ModelMatrix, Position4);
  output.NormalWS = normalize(mul(TransformParams.NormalMatrixWS, input.NormalOS));
  output.TexCoord = input.TexCoord;

  float4 PositionVS4 = mul(TransformParams.ModelViewMatrix, Position4);
  output.PositionVS = PositionVS4.xyz / Position4.w;
  return output;
}

// =================================================================================================
// psmain
// =================================================================================================
struct GBufferData {
  float3  Position;
  float3  Normal;
  float3  Albedo;
  float   Roughness;
  float   Metallic;
  float   Specular;
  float   Fresnel;
  float   FresnelPower;
};

PSOutput PackGBuffer(GBufferData data)
{
  PSOutput output = (PSOutput)0;
  // Gbuffer0
  output.GBuffer0.xyz = data.Position;
  // Gbuffer1
  output.GBuffer1.xyz = data.Normal;
  // GBuffer2
  output.GBuffer2.xyz = data.Albedo;
  output.GBuffer2.w   = data.Roughness;
  // GBuffer3
  output.GBuffer3.x   = data.Metallic;
  output.GBuffer3.y   = data.Specular;
  output.GBuffer3.z   = data.Fresnel;
  output.GBuffer3.w   = data.FresnelPower;
  return output;
}

PSOutput psmain(PSInput input) : SV_Target
{
  // Fill out gbuffer values
  GBufferData data  = (GBufferData)0;
  data.Position     = input.PositionWS;
  data.Normal       = input.NormalWS;
  data.Albedo       = MaterialParams.Color;
  data.Roughness    = MaterialParams.Roughness;
  data.Metallic     = MaterialParams.Metallic;
  data.Specular     = MaterialParams.Specular;
  data.Fresnel      = MaterialParams.Fresnel;
  data.FresnelPower = MaterialParams.FresnelPower;
  // Pack it!
  PSOutput output = PackGBuffer(data);
  return output;
}
