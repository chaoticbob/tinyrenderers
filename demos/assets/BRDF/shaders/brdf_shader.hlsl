

struct ViewData {
  float3    EyePosition;
  float4x4  ViewMatrix;
  float4x4  ProjectionMatrix;
  float4x4  ViewProjectionMatrix;
};

struct TransformData {
  float4x4  ModelMatrix;
  float4x4  ModelViewMatrix;
  float4x4  ModelViewProjectionMatrix;
  float3x3  NormalMatrixWS;
  float3x3  NormalMatrixVS;
  float3    DebugColor;
};

struct BRDFMaterialData {
  float3    BaseColor;
  float     Metallic;
  float     Subsurface;
  float     Specular;
  float     Roughness;
  float     SpecularTint;
  float     Anisotropic;
  float     Sheen;
  float     SheenTint;
  float     ClearCoat;
  float     ClearCoatGloss;
  float     kA;
  float     kD;
  float     kS;
};

// =================================================================================================
// Lights
// =================================================================================================
#define MAX_POINT_LIGHTS        16
#define MAX_SPOT_LIGHTS         1
#define MAX_DIRECTIONAL_LIGHTS  1

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
  float3    Position;
  float3    Color;
  float     Intensity;
  float3    Direction;
};

struct LightingData {
  AmbientLightData  AmbientLight;
  PointLightData    PointLights[MAX_POINT_LIGHTS];
  uint              PointLightCount;
  SpotLightData     SpotLights[MAX_SPOT_LIGHTS];
  uint              SpotLightCount;
  PointLightData    DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
  uint              DirectionalLightCount;
};

// =================================================================================================
// Resources
// =================================================================================================
ConstantBuffer<ViewData>          ViewParams      : register(b0);
ConstantBuffer<TransformData>     TransformParams : register(b1);
ConstantBuffer<BRDFMaterialData>  MaterialParams  : register(b2);
ConstantBuffer<LightingData>      LightingParams  : register(b3);

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
  float3  TangentWS   : TANGENT;
  float3  BitangentWS : BITANGENT;
};

typedef VSOutput PSInput;

// =================================================================================================
// vsmain
// =================================================================================================
void ComputeTangentVectors(float3 N, out float3 U, out float3 V)
{
    U = (abs(N.x) < 0.999) ? float3(1,0, 0) : float3(0, 1,0);
    U = normalize(cross(N, U));
    V = normalize(cross(N, U));
}

VSOutput vsmain(VSInput input)
{
  float4 Position4 = float4(input.PositionOS, 1);

  VSOutput output = (VSOutput)0;
  output.PositionCS = mul(TransformParams.ModelViewProjectionMatrix, Position4);
  output.PositionWS = mul(TransformParams.ModelMatrix, Position4);
  output.NormalWS = mul(TransformParams.NormalMatrixWS, input.NormalOS);
  output.TexCoord = input.TexCoord;
  ComputeTangentVectors(output.NormalWS, output.TangentWS, output.BitangentWS);
  return output;
}

// =================================================================================================
// psmain
// =================================================================================================
static const float PI = 3.14159265358979323846;

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


float3 BRDF(float3 L, float3 V, float3 N, float3 X, float3 Y)
{
  float3 baseColor      = MaterialParams.BaseColor;
  float  metallic       = MaterialParams.Metallic;
  float  subsurface     = MaterialParams.Subsurface;
  float  specular       = MaterialParams.Specular;
  float  roughness      = MaterialParams.Roughness;
  float  specularTint   = MaterialParams.SpecularTint;
  float  anisotropic    = MaterialParams.Anisotropic;
  float  sheen          = MaterialParams.Sheen;
  float  sheenTint      = MaterialParams.SheenTint;
  float  clearcoat      = MaterialParams.ClearCoat;
  float  clearcoatGloss = MaterialParams.ClearCoatGloss;

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

  return ((1/PI) * lerp(Fd, ss, subsurface) * Cdlin + Fsheen)
                 * (1 - metallic)
                 + Gs * Fs * Ds + 0.25 * clearcoat * Gr * Fr * Dr;
}

float4 psmain(PSInput input) : SV_Target
{
  float3 LP = float3(0, 10, 10);
  float3 L  = normalize(LP - input.PositionWS);
  float3 V  = ViewParams.EyePosition - input.PositionWS;
  float3 N  = input.NormalWS;
  float3 X  = input.TangentWS;
  float3 Y  = input.BitangentWS;

  float3 brdf   = max(BRDF(L, V, N, X, Y), float3(0, 0, 0));
  float  LdotN  = dot(L, N);

  float3 result = LdotN * brdf;

  return float4(result, 1);
}
