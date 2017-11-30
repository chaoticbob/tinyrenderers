cbuffer ViewTransform : register(b0)
{
  float4x4  model_matrix;
  float4x4  view_matrix;
  float4x4  projection_matrix;
  float4x4  model_view_matrix;
  float4x4  view_projection_matrix;
  float4x4  model_view_projection_matrix;
  float3x3  normal_matrix_world_space;
  float3x3  normal_matrix_view_space;
  float3    view_direction;
  float3    color;
};

cbuffer BlinnPhong : register(b1) {
  float4  base_color;
  float4  specular_color;
  float4  specular_power;
  float4  kA;
  float4  kD;
  float4  kS;
}


// =============================================================================
// Vertex Shader
// =============================================================================
struct VSInput {
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
  float2 TexCoord : TEXCOORD0;
};

struct VSOutput {
  float4 SV_Position : SV_Position;
  float3 PositionWS  : POSITION;
  float3 Normal      : NORMAL;
};

VSOutput VSMain(VSInput input)
{
  float4 Position4 = float4(input.Position, 1);

  VSOutput result;
  result.SV_Position = mul(model_view_projection_matrix, Position4);
  result.PositionWS  = mul(model_matrix, input.Position);
  result.Normal      = normalize(mul(normal_matrix_world_space, input.Normal));
  return result;
}

// =============================================================================
// Pixel Shader
// =============================================================================
struct PSInput {
  float4 SV_Position : SV_Position;
  float3 PositionWS  : POSITION;
  float3 NormalWS    : NORMAL;
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
  float3 LP = float3(5, 8, 10);
  float3 N  = input.NormalWS;
  float3 L  = normalize(LP - input.PositionWS);
  float3 V  = view_direction;
  float3 R  = reflect(L, N);

  float  A = 0.3;
  float  D = 0.5 * lambert(N, L);
  float  S = 4.0 * phong(N, L, V, R, 6.0);
  float3 C = color;
  float3 Co = C * (0.3 + D + S); // * (A + D + S);

  float4 oColor0 = float4(Co, 1);
  return oColor0;
}
