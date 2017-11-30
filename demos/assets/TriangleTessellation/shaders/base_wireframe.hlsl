cbuffer UniformBlock0 : register(b0)
{
  float4x4 model_view_matrix;
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
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

VSOutput VSMain(VSInput input)
{
  VSOutput result;
  result.PositionWS = mul(model_view_matrix, float4(input.PositionOS, 1.0)).xyz;
  result.NormalWS   = normalize(mul(normal_matrix, input.NormalOS));
  return result;
}

// =============================================================================
// Geometry Shader
// =============================================================================
struct GSVertexInput {
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

struct GSVertexOutput {
  float4 PositionCS : SV_POSITION;
  float3 Color      : COLOR;
};

[maxvertexcount(6)]
void GSMain(
  triangle GSVertexInput            input[3],
  inout LineStream<GSVertexOutput>  output
)
{

  float3 P0 = input[0].PositionWS.xyz;
  float3 P1 = input[1].PositionWS.xyz;
  float3 P2 = input[2].PositionWS.xyz;
  float3 N0 = input[0].NormalWS;
  float3 N1 = input[1].NormalWS;
  float3 N2 = input[2].NormalWS;

  float4 Pa;
  float4 Pb;
  float4 N;
  float  s;

  s  = 0.05;
  N  = float4((N0 + N1 + N2) / 3.0, 0.0);
  Pa = float4((P0 + P1 + P2) / 3.0, 1.0);
  Pb = Pa + s * N;

  GSVertexOutput vertex;

  // Edges
  vertex.Color = float3(0.2, 0.8, 0.2);
  // Totally hacky...
  P0.z += 0.001;
  P1.z += 0.001;
  P2.z += 0.001;
  float4 Q0 = mul(proj_matrix, float4(P0, 1.0));
  float4 Q1 = mul(proj_matrix, float4(P1, 1.0));
  float4 Q2 = mul(proj_matrix, float4(P2, 1.0));

  // Edge 0
  vertex.PositionCS = Q0;
  output.Append(vertex);
  vertex.PositionCS = Q1;
  output.Append(vertex);
  output.RestartStrip();

  // Edge 1
  vertex.PositionCS = Q1;
  output.Append(vertex);
  vertex.PositionCS = Q2;
  output.Append(vertex);
  output.RestartStrip();

  // Edge 2
  vertex.PositionCS = Q2;
  output.Append(vertex);
  vertex.PositionCS = Q0;
  output.Append(vertex);
  output.RestartStrip();
}

// =============================================================================
// Pixel Shader
// =============================================================================
struct PSInput {
  float4 PositionRS : SV_POSITION;
  float3 Color      : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET
{
  return float4(input.Color, 1);
}
