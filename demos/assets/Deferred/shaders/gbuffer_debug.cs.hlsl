#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_PARAMS                b0
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER0_TEX          t1
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER1_TEX          t2
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER2_TEX          t3
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER3_TEX          t4
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER_DEPTH_TEX     t5
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_SAMPLER               s6
#define DESCRIPTOR_BINDING_DEFERRED_DEBUG_OUTPUT_TEX            u7

#define DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION                 1
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_NORMAL                   2
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_ALBEDO                   3
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_ROUGHNESS                4
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_METALLIC                 5
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_SPECULAR                 6
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL                  7
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL_POWER            8
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_DEPTH                    9
#define DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION_FROM_DEPTH      10

#define NUM_THREADS_X   8
#define NUM_THREADS_Y   8
#define NUM_THREADS_Z   1

struct DebugData {
  int       GBufferElement;
  float4x4  InverseViewMatrix;
  float4x4  InverseProjectionMatrix;
};

ConstantBuffer<DebugData> DebugParams     : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_PARAMS);

Texture2D                 GBuffer0Tex     : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER0_TEX);
Texture2D                 GBuffer1Tex     : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER1_TEX);
Texture2D                 GBuffer2Tex     : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER2_TEX);
Texture2D                 GBuffer3Tex     : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER3_TEX);
Texture2D                 GBufferDepthTex : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_GBUFFER_DEPTH_TEX);

SamplerState              Sampler         : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_SAMPLER);
RWTexture2D<float4>       OutputTex       : register(DESCRIPTOR_BINDING_DEFERRED_DEBUG_OUTPUT_TEX);


// =================================================================================================
// Support Functions
// =================================================================================================
float3 VSPositionFromDepth(float2 coord, Texture2D tex, SamplerState sam)
{
  // Get the depth value for this pixel
  float z = tex.SampleLevel(sam, coord, 0).x;
  // Get x/w and y/w from the viewport position
  float x = coord.x * 2 - 1;
  float y = (1 - coord.y) * 2 - 1;
  float4 positionProjected = float4(x, y, z, 1);
  // Transform by the inverse projection matrix
  float4 positionVS = mul(DebugParams.InverseProjectionMatrix, positionProjected);
  // Divide by w to get the view-space position
  positionVS /= positionVS.w;
  positionVS = mul(DebugParams.InverseViewMatrix, positionVS);
  return positionVS.xyz * (1 - z);
}

// =================================================================================================
// Unpack
// =================================================================================================
float4 Sample(float2 coord, Texture2D tex, SamplerState sam)
{
  float4 value = tex.SampleLevel(sam, coord, 0);
  return value;
}

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

// NOTE: This version of UnpackGBuffer uses SampleLevel and not Load.
GBufferData UnpackGBufferSample(float2 coord, Texture2D tex0, Texture2D tex1, Texture2D tex2, Texture2D tex3, SamplerState sam)
{
  float4 gbuffer0 = Sample(coord, tex0, sam);
  float4 gbuffer1 = Sample(coord, tex1, sam);
  float4 gbuffer2 = Sample(coord, tex2, sam);
  float4 gbuffer3 = Sample(coord, tex3, sam);

  GBufferData data = (GBufferData)0;
  // Gbuffer0
  data.Position     = gbuffer0.xyz;
  // Gbuffer1
  data.Normal       = gbuffer1.xyz;
  // GBuffer2
  data.Albedo       = gbuffer2.xyz;
  data.Roughness    = gbuffer2.w;
  // GBuffer3
  data.Metallic     = gbuffer3.x;
  data.Specular     = gbuffer3.y;
  data.Fresnel      = gbuffer3.z;
  data.FresnelPower = gbuffer3.w;
  return data;
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
  // Get output diemsions
  uint2 output_tex_size;
  OutputTex.GetDimensions(output_tex_size.x, output_tex_size.y);

  // Process if within bounds
  if ((tid.x < output_tex_size.x) && (tid.y < output_tex_size.y)) {
    float2 coord = (float2)tid.xy / (float2)output_tex_size;
    float3 value = (float3)0;

    GBufferData gbuffer_data = UnpackGBufferSample(coord, GBuffer0Tex, GBuffer1Tex, GBuffer2Tex, GBuffer3Tex, Sampler);

    switch (DebugParams.GBufferElement) {
      case DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION: {
        value = gbuffer_data.Position;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_NORMAL: {
        value = gbuffer_data.Normal;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_ALBEDO: {
        value = gbuffer_data.Albedo;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_ROUGHNESS: {
        value = (float3)gbuffer_data.Roughness;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_METALLIC: {
        value = (float3)gbuffer_data.Metallic;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_SPECULAR: {
        value = (float3)gbuffer_data.Specular;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL: {
        value = (float3)gbuffer_data.Fresnel;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_FRESNEL_POWER: {
        value = (float3)gbuffer_data.FresnelPower;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_DEPTH: {
        float4 depth_value = Sample(coord, GBufferDepthTex, Sampler);
        value = (float3)depth_value.x;
      }
      break;

      case DEFERRED_DEBUG_GBUFFER_ELEMENT_POSITION_FROM_DEPTH: {
        //float4 depth_value = Sample(coord, GBufferDepthTex, Sampler);
        value = VSPositionFromDepth(coord, GBufferDepthTex, Sampler);
      }
      break;
    }

    OutputTex[tid.xy] = float4(value, 1);
  }
}
