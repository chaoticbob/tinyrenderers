
#define SHADER_PARAMS_MAX_POINT_LIGHTS  16

struct ViewTransformData {
  // View
  float3    EyePosition; 
  float4x4  ViewMatrix;
  float4x4  ProjectionMatrix;
  float4x4  ViewProjectionMatrix;
  // Transform
  float4x4  ModelMatrix;
  float4x4  ModelViewMatrix;
  float4x4  ModelViewProjectionMatrix;
  float3x3  NormalMatrixWS;
  float3x3  NormalMatrixVS;
  float3    DebugColor;
};

struct SimpleMaterialData {
  float3  Color;            // { 0.1f, 0.15f, 0.3f };
  float   Power;            // { 2.0f };
  float   Roughness;        // { 0.0f };
  float3  SpecularColor;    // { 1.0f, 1.0f, 1.0f };  
  float   kA;               // { 0.2f };
  float   kD;               // { 0.7f };
  float   kS;               // { 4.0f };
};

struct AmbientLightData {
  float3  Color;
  float   Intensity;
};

struct PointLightData {
  float3  Position;
  float3  Color;
  float   Intensity;
  float   FallOff;
};

struct TessData {
  float   InsideTessFactor;
  float   OutsideTessFactor;
};

struct SimpleShaderData {
  ViewTransformData   ViewTransform;
  SimpleMaterialData  Material;
  AmbientLightData    AmbientLight;
  PointLightData      PointLights[SHADER_PARAMS_MAX_POINT_LIGHTS];
  TessData            Tess;
};

ConstantBuffer<SimpleShaderData> ShaderParams : register(b0);

// =============================================================================
// Support Functions Forward Declarations
// =============================================================================
float Lambert(float3 N, float3 L);
float Phong(float3 N, float3 L, float3 E, float3 R, float power);
float BlinnPhong(float3 N, float3 L, float3 E, float power);
float OrenNayar(float3 N, float3 L, float3 E, float roughness);
float Minnaert(float3 N, float3 L, float3 E, float power);
float Schlick(float3 N, float3 L, float3 E, float3 R, float power);
float CookTorrance(float3 N, float3 L, float3 E, float3 R, float power, float roughness);

// =============================================================================
// Vertex Shader
// =============================================================================
struct VSInput {
  float3 PositionOS : POSITION;
  float3 NormalOS   : NORMAL;
  float2 TexCoord   : TEXCOORD0;
};

struct VSOutput {
  float4 SV_Position : SV_Position;
  float3 PositionWS  : POSITION;
  float3 NormalWS    : NORMAL;
};

VSOutput vsmain(VSInput input)
{
  float4 Position4 = float4(input.PositionOS, 1);

  VSOutput result;
  result.SV_Position = mul(ShaderParams.ViewTransform.ModelViewProjectionMatrix, Position4);
  result.PositionWS  = mul(ShaderParams.ViewTransform.ModelMatrix, Position4).xyz;
  result.NormalWS    = normalize(mul(ShaderParams.ViewTransform.NormalMatrixWS, input.NormalOS));
  return result;
}

// =============================================================================
// Pixel Shader Input
// =============================================================================
struct PSInput {
  float4 SV_Position : SV_Position;
  float3 PositionWS  : POSITION;
  float3 NormalWS    : NORMAL;
};


// =============================================================================
// Pixel Shader: psmain_lambert
// =============================================================================
float4 psmain_lambert(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = 0;
  const float3 color = ShaderParams.Material.Color;
  const float  power = 0;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = 0;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float3 R = reflect(L, N);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Lambert(N, L) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = 0;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_phong
// =============================================================================
float4 psmain_phong(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  power = ShaderParams.Material.Power;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float3 R = reflect(-L, N);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Lambert(N, L) * intensity;
    specular += Phong(N, L, E, R, power) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = kS * specular;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_blinn_phong
// =============================================================================
float4 psmain_blinn_phong(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  power = ShaderParams.Material.Power;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Lambert(N, L) * intensity;
    specular += BlinnPhong(N, L, E, power) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = kS * specular;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_oren_nayar
// =============================================================================
float4 psmain_oren_nayar(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  roughness = ShaderParams.Material.Roughness;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += OrenNayar(N, L, E, roughness) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = 0;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_minnaert
// =============================================================================
float4 psmain_minnaert(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  power = ShaderParams.Material.Power;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Minnaert(N, L, E, power) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = 0;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_cook_torrance
// =============================================================================
float4 psmain_schlick(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  power = ShaderParams.Material.Power;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float3 R = reflect(-L, N);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Lambert(N, L) * intensity;
    specular += Schlick(N, L, E, R, power) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = kS * specular;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}

// =============================================================================
// Pixel Shader: psmain_cook_torrance
// =============================================================================
float4 psmain_cook_torrance(PSInput input) : SV_TARGET
{
  const float3 N = normalize(input.NormalWS);
  const float3 E = normalize(ShaderParams.ViewTransform.EyePosition - input.PositionWS);  
  const float3 color = ShaderParams.Material.Color;
  const float  power = ShaderParams.Material.Power;
  const float  roughness = ShaderParams.Material.Roughness;
  const float  kA = ShaderParams.Material.kA;
  const float  kD = ShaderParams.Material.kD;
  const float  kS = ShaderParams.Material.kS;

  float ambient  = ShaderParams.AmbientLight.Intensity;
  float diffuse  = 0;
  float specular = 0;
  for (uint i = 0; i < SHADER_PARAMS_MAX_POINT_LIGHTS; ++i) {
    float3 L = normalize(ShaderParams.PointLights[i].Position - input.PositionWS);
    float3 R = reflect(-L, N);
    float intensity = ShaderParams.PointLights[i].Intensity;
    diffuse += Lambert(N, L) * intensity;
    specular += CookTorrance(N, L, E, R, power, roughness) * intensity;
  }

  float  A  = kA * ambient;
  float  D  = kD * diffuse;
  float  S  = kS * specular;
  float3 Ci = color;
  float3 Co = (A + D + S) * Ci;

  return float4(Co, 1);
}


// =============================================================================
// Support Functions
// =============================================================================

/*! @fn Lambert 

*/
float Lambert(float3 N, float3 L)
{
  float LdotN   = dot(L, N);
  float result  = max(0.0, LdotN);
  return result;  
}

/*! @fn Phong 

*/
float Phong(float3 N, float3 L, float3 E, float3 R, float power)
{
  float RdotE   = dot(R, E);
  float result  = pow(max(0.0, RdotE), power);
  return result;
}

/*! @fn BlinnPhong 

*/
float BlinnPhong(float3 N, float3 L, float3 E, float power)
{
  float3 H          = normalize(L + E);
  float NdotH       = saturate(dot(N, H));
  float specular    = pow(NdotH, power);
  float result      = specular;
  return result;
}

/*! @fn OrenNayar 

*/
float OrenNayar(float3 N, float3 L, float3 E, float roughness)
{
  float  LdotN        = dot(L, N);
  float  EdotN        = dot(E, N);
  float  angleLdotN   = acos(LdotN);
  float  angleEdotV   = acos(EdotN);
  float3 s            = normalize(L - N * LdotN);
  float3 t            = normalize(E - N * EdotN);
  float  gamma        = max(0.0, dot(s, t));
  float  alpha        = max(angleLdotN, angleEdotV);
  float  beta         = min(angleLdotN, angleEdotV);
  float  sigma2       = roughness * roughness;
  float  A            = 1.0 - (0.5 * sigma2) / (sigma2 + 0.33);
  float  B            = (0.45 * sigma2) / (sigma2 + 0.09);
  float  result       = max(0.0, LdotN) * (A + B * gamma *sin(alpha) * tan(beta));
  return result;
}

/*! @fn Minnaert 

*/
float Minnaert(float3 N, float3 L, float3 E, float power)
{
  float  LdotN        = dot(L, N);
  float  EdotN        = dot(E, N);
  float  Irradiance   = max(0.0, LdotN);
  float  result       = pow(max(0.0, EdotN * LdotN), power) * Irradiance;
  return result;
}

float Fresnel(float f0, float u)
{
  // Schlick
  float result = f0 + (1.0 - f0) * pow(1.0 - u, 5.0);
  return result;
}

float Fresnel2(float n2, float u)
{
  // Classic Schlick
  n2            = 1.0 - n2;
  float r1      = 1.0 - n2;
  float r2      = 1.0 + n2;
  float f0      = r1 * r1 / r2 / r2;
  float result  = (f0 + (1.0 - f0) * pow(1.0 - u, 5.0));
  return result;
}

/*! @fn Schlick

*/
float Schlick(float3 N, float3 L, float3 V, float3 R, float power)
{
  float NdotV   = dot(N, V);
  float result  = Fresnel2(power, NdotV);
  return result;
}

float F_Fresnel(float f0, float u)
{
  float result = Fresnel2(f0, u);
  return result; 
}

float D_Beckmann(float m, float t)
{
  float M = m * m;
  float T = t * t;
  float result = exp((T - 1.0) / (M * T)) / (M * T * T);
  return result;
}

float G_Default(float HdotN, float HdotV, float LdotN, float NdotV)
{
  float G1  = 2.0 * HdotN * NdotV / HdotV;
  float G2  = 2.0 * HdotN * LdotN / HdotV;
  float result = min(1.0, min(G1, G2));
  return result;
}

/*! @fn CookTorrance

*/
float CookTorrance(float3 N, float3 L, float3 V, float3 R, float power, float roughness)
{
  float3 H      = normalize(L + V);  
  float NdotV   = dot(N, V);
  float HdotN   = dot(H, N);
  float LdotN   = dot(L, N);
  float HdotV   = dot(H, V);
  float F       = F_Fresnel(power, NdotV);
  float D       = D_Beckmann(roughness, HdotN);
  float G       = G_Default(HdotN, HdotV, LdotN, NdotV);
  // result
  float result  = F * D * G / (LdotN * NdotV);
  return result;
}