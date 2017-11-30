cbuffer UniformBlock0 : register(b0)
{
  float4x4 model_matrix;
  float4x4 proj_matrix;
  float4x4 model_view_proj_matrix;
  float3x3 normal_matrix;
  float3   color;
  float3   view_dir;
};


// =============================================================================
// Vertex Shader
// =============================================================================
struct VSInput {
  float3 PositionOS : POSITION;
  float3 NormalOS   : NORMAL;
  float2 TexCoord   : TEXCOORD0;
};

struct VSOutput {
  float4 PositionCS : SV_Position;
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

VSOutput VSMain(VSInput input)
{
  float4 Position4 = float4(input.PositionOS, 1);
  VSOutput result;
  result.PositionCS  = mul(model_view_proj_matrix, Position4);
  result.PositionWS  = mul(model_matrix, Position4).xyz;
  result.NormalWS    = normalize(mul(normal_matrix, input.NormalOS));
  return result;
}

// =============================================================================
// Pixel Shader
// =============================================================================
struct PSInput {
  float4 PositionRS : SV_Position;
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

float lambert(float3 N, float3 L)
{
  float result = max(0.0, dot(N, L));
  return result;
}

float phong(float3 N, float3 L, float3 V, float3 R, float specularExponent)
{
  float angle    = max(0.0, dot(N, L));
  float base     = max(dot(R, V), 0.0);
  float specular = pow(base, specularExponent);
  float result   = angle * specular;
  return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
  float3 LP = float3(15, 3, 40);
  float3 P = input.PositionWS;
  float3 N = input.NormalWS;
  float3 L = normalize(LP - P);
  float3 V = view_dir;
  float3 R = reflect(L, N);

  float  A = 0.3;
  float  D = 0.5 * lambert(N, L);
  float  S = 2.0 * phong(N, L, V, R, 8.0);
  float3 C = color;
  float3 Co = C * (0.3 + D + S); // * (A + D + S);

  float4 oColor0 = float4(Co, 1);
  return oColor0;
}
