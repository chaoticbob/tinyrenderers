#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_LIGHTING_PARAMS   b0
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER0_TEX      t1
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER1_TEX      t2
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER2_TEX      t3
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER3_TEX      t4
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_SAMPLER           s5
#define DESCRIPTOR_BINDING_DEFERRED_LIGHTING_OUTPUT_TEX        u6

#define NUM_THREADS_X   8
#define NUM_THREADS_Y   8
#define NUM_THREADS_Z   1

// =================================================================================================
// Lights
// =================================================================================================
#define DEFERRED_MAX_POINT_LIGHTS        16
#define DEFERRED_MAX_SPOT_LIGHTS         1
#define DEFERRED_MAX_DIRECTIONAL_LIGHTS  2

struct AmbientLightData {
  float3    Color;
  float     Intensity;
};

struct PointLightData {
  float3    Position;
  float3    Color;
  float     Intensity;
  float     FallOff;
};

struct SpotLightData {
  float3    Position;
  float3    Color;
  float     Intensity;
  float     FallOff;
  float3    Direction;
  float     ConeAngle;
};

struct DirectionalLightData {
  float3    Direction;
  float3    Color;
  float     Intensity;
};

struct LightingData {
  float3                EyePosition;
  AmbientLightData      AmbientLight;
  PointLightData        PointLights[DEFERRED_MAX_POINT_LIGHTS];
  SpotLightData         SpotLights[DEFERRED_MAX_SPOT_LIGHTS];
  DirectionalLightData  DirectionalLights[DEFERRED_MAX_DIRECTIONAL_LIGHTS];
};

// =================================================================================================
// Resources
// =================================================================================================
ConstantBuffer<LightingData>  LightingParams  : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_LIGHTING_PARAMS);

Texture2D                     InputTex0       : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER0_TEX);
Texture2D                     InputTex1       : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER1_TEX);
Texture2D                     InputTex2       : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER2_TEX);
Texture2D                     InputTex3       : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_GBUFFER3_TEX);

SamplerState                  Sampler         : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_SAMPLER);

RWTexture2D<float4>           OutputTex       : register(DESCRIPTOR_BINDING_DEFERRED_LIGHTING_OUTPUT_TEX);


// =================================================================================================
// Support Functions
// =================================================================================================
float4 Sample(float2 coord, Texture2D tex, SamplerState sam)
{
  float4 value = tex.SampleLevel(sam, coord, 0);
  return value;
}

// =================================================================================================
// Unpack
// =================================================================================================
struct GBufferData {
  float3  Position;
  float3  Normal;
  float3  Albedo;
  float   Roughness;
  float   Metallic;
  float   Specular;
  float   Subsurface;
  float   ClearCoat;
};

GBufferData UnpackGBuffer(float2 coord, Texture2D tex0, Texture2D tex1, Texture2D tex2, Texture2D tex3, SamplerState sam)
{
  float4 gbuffer0 = Sample(coord, tex0, sam);
  float4 gbuffer1 = Sample(coord, tex1, sam);
  float4 gbuffer2 = Sample(coord, tex2, sam);
  float4 gbuffer3 = Sample(coord, tex3, sam);

  GBufferData data = (GBufferData)0;
  // Gbuffer0
  data.Position   = gbuffer0.xyz;
  // Gbuffer1
  data.Normal     = gbuffer1.xyz;
  // GBuffer2
  data.Albedo     = gbuffer2.xyz;
  data.Roughness  = gbuffer2.w;
  // GBuffer3
  data.Metallic   = gbuffer3.x;
  data.Specular   = gbuffer3.y;
  data.Subsurface = gbuffer3.z;
  data.ClearCoat  = gbuffer3.w;
  return data; 
}

// =================================================================================================
// Lighting Function Forward Declares
// =================================================================================================
float   Diffuse(float3 N, float3 L);
void    ComputeTangentVectors(float3 N, out float3 U, out float3 V);
float3  BRDF(float3 L, float3 V, float3 N, float3 X, float3 Y, GBufferData materiaData);

// =================================================================================================
// csmain
// =================================================================================================
[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
  // Get output diemsions
  uint2 output_tex_size;
  OutputTex.GetDimensions(output_tex_size.x, output_tex_size.y);

  // Process if within bounds
  if ((tid.x < output_tex_size.x) && (tid.y < output_tex_size.y)) {
    float2 coord = (float2)tid.xy / (float2)output_tex_size;
    GBufferData data = UnpackGBuffer(coord, InputTex0, InputTex1, InputTex2, InputTex3, Sampler);

    float3 P = data.Position;
    float3 N = data.Normal;
    float3 V = normalize(LightingParams.EyePosition - data.Position);

    float3 X;
    float3 Y;
    ComputeTangentVectors(N, X, Y);

    float  ambient  = LightingParams.AmbientLight.Intensity;
    float  diffuse  = 0;
    float3 specular = (float3)0;

    // Point lights
    int light_index;
    for (light_index = 0; light_index < DEFERRED_MAX_POINT_LIGHTS; ++light_index) {
      float3 LP = LightingParams.PointLights[light_index].Position;
      float  intensity = LightingParams.PointLights[light_index].Intensity;
      float3 L = normalize(LP - P);
      float  LdotN = max(0.0, dot(L, N));
      diffuse += LdotN * intensity;
      specular += LdotN * BRDF(L, V, N, X, Y, data) * intensity;
    }

    // Directional lights
    for (light_index = 0; light_index < DEFERRED_MAX_DIRECTIONAL_LIGHTS; ++light_index) {
      float3 L = -normalize(LightingParams.DirectionalLights[light_index].Direction);
      float  intensity = LightingParams.DirectionalLights[light_index].Intensity;
      float  LdotN = max(0.0, dot(L, N));
      diffuse += LdotN * intensity;
      specular += LdotN * BRDF(L, V, N, X, Y, data) * intensity;
    }    

    float3 Co = ((diffuse + ambient) * data.Albedo) + specular;
    OutputTex[tid.xy] = float4(Co, 1);
  }
}

// =================================================================================================
// BRDF Functions
// =================================================================================================
static const float PI = 3.14159265358979323846;

void ComputeTangentVectors(float3 N, out float3 U, out float3 V)
{
    U = (abs(N.x) < 0.999) ? float3(1,0, 0) : float3(0, 1,0);
    U = normalize(cross(N, U));
    V = normalize(cross(N, U));
}

float sqr(float x)
{
  return x * x;
}

float SchlickFresnel(float u)
{
  float m = clamp(1-u, 0, 1);
  float m2 = m*m;
  return m2*m2*m; // pow(m,5)
}

float GTR1(float NdotH, float a)
{
  if (a >= 1) {
    return 1/PI;
  }

  float a2 = a*a;
  float t = 1 + (a2-1)*NdotH*NdotH;
  return (a2-1) / (PI*log(a2)*t);
}

float GTR2(float NdotH, float a)
{
  float a2 = a*a;
  float t = 1 + (a2-1)*NdotH*NdotH;
  return a2 / (PI * t*t);
}

float GTR2_aniso(float NdotH, float HdotX, float HdotY, float ax, float ay)
{
  return 1 / (PI * ax*ay * sqr( sqr(HdotX/ax) + sqr(HdotY/ay) + NdotH*NdotH ));
}

float smithG_GGX(float NdotV, float alphaG)
{
  float a = alphaG*alphaG;
  float b = NdotV*NdotV;
  return 1 / (NdotV + sqrt(a + b - a*b));
}

float smithG_GGX_aniso(float NdotV, float VdotX, float VdotY, float ax, float ay)
{
  return 1 / (NdotV + sqrt( sqr(VdotX*ax) + sqr(VdotY*ay) + sqr(NdotV) ));
}

float3 mon2lin(float3 x)
{
  return float3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}


float3 BRDF(float3 L, float3 V, float3 N, float3 X, float3 Y, GBufferData materiaData)
{
  float3 baseColor      = materiaData.Albedo;
  float  metallic       = materiaData.Metallic;
  float  subsurface     = materiaData.Subsurface;
  float  specular       = materiaData.Specular;
  float  roughness      = materiaData.Roughness;
  float  specularTint   = 0;
  float  anisotropic    = 0;
  float  sheen          = 0;
  float  sheenTint      = 0;
  float  clearcoat      = materiaData.ClearCoat;
  float  clearcoatGloss = 0;

  float NdotL = dot(N,L);
  float NdotV = dot(N,V);
  if ((NdotL < 0) || (NdotV < 0)) {
    return float3(0, 0, 0);
  }

  float3 H = normalize(L+V);
  float NdotH = dot(N,H);
  float LdotH = dot(L,H);

  float3 Cdlin = mon2lin(baseColor);
  float Cdlum = 0.3 * Cdlin[0] + 0.6 * Cdlin[1] + 0.1 * Cdlin[2]; // luminance approx.

  float3 Ctint = Cdlum > 0 ? Cdlin / Cdlum : float3(1, 1, 1); // normalize lum. to isolate hue+sat
  float3 Cspec0 = lerp(specular * 0.08 * lerp(float3(1, 1, 1), Ctint, specularTint), Cdlin, metallic);
  float3 Csheen = lerp(float3(1, 1, 1), Ctint, sheenTint);

  // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
  // and lerp in diffuse retro-reflection based on roughness
  float FL = SchlickFresnel(NdotL), FV = SchlickFresnel(NdotV);
  float Fd90 = 0.5 + 2 * LdotH*LdotH * roughness;
  float Fd = lerp(1.0, Fd90, FL) * lerp(1.0, Fd90, FV);

  // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
  // 1.25 scale is used to (roughly) preserve albedo
  // Fss90 used to "flatten" retroreflection based on roughness
  float Fss90 = LdotH * LdotH * roughness;
  float Fss = lerp(1.0, Fss90, FL) * lerp(1.0, Fss90, FV);
  float ss = 1.25 * (Fss * (1 / (NdotL + NdotV) - 0.5) + 0.5);

  // specular
  float aspect = sqrt(1 - (anisotropic * 0.9));
  float ax = max(0.001, sqr(roughness) / aspect);
  float ay = max(0.001, sqr(roughness) * aspect);
  float Ds = GTR2_aniso(NdotH, dot(H, X), dot(H, Y), ax, ay);
  float FH = SchlickFresnel(LdotH);
  float3 Fs = lerp(Cspec0, float3(1, 1, 1), FH);
  float Gs = smithG_GGX_aniso(NdotL, dot(L, X), dot(L, Y), ax, ay)
           * smithG_GGX_aniso(NdotV, dot(V, X), dot(V, Y), ax, ay);

  // sheen
  float3 Fsheen = FH * sheen * Csheen;

  // clearcoat (ior = 1.5 -> F0 = 0.04)
  float Dr = GTR1(NdotH, lerp(0.1, 0.001, clearcoatGloss));
  float Fr = lerp(0.04, 1.0, FH);
  float Gr = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);

  return ((1 / PI) * lerp(Fd, ss, subsurface) * Cdlin + Fsheen)
                   * (1 - metallic)
                   + Gs * Fs * Ds + 0.25 * clearcoat * Gr * Fr * Dr;
}