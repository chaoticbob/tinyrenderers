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
  float3 NormalOS    : NORMAL;
};

VSOutput VSMain(VSInput input)
{
  VSOutput result;
  result.SV_Position = float4(input.PositionOS, 1.0);
  result.NormalOS    = normalize(input.NormalOS);
  return result;
}

// =============================================================================
// Geometry Shader
// =============================================================================
struct GSVertexInput {
  float4 PositionOS : SV_POSITION;
  float3 NormalOS   : NORMAL;
};

struct GSVertexOutput {
  float4 Position : SV_POSITION;
  float3 Color    : COLOR;
};

[maxvertexcount(6)]
void GSMain(
  triangle GSVertexInput            input[3],
  inout LineStream<GSVertexOutput>  output)
{

  float3 P0 = mul(model_view_matrix, input[0].PositionOS).xyz;
  float3 P1 = mul(model_view_matrix, input[1].PositionOS).xyz;
  float3 P2 = mul(model_view_matrix, input[2].PositionOS).xyz;
  float3 N0 = mul(normal_matrix_world_space, input[0].NormalOS);
  float3 N1 = mul(normal_matrix_world_space, input[1].NormalOS);
  float3 N2 = mul(normal_matrix_world_space, input[2].NormalOS);

  float4 Pa;
  float4 Pb;
  float4 N;
  float  s;

  s  = 0.05;
  N  = float4((N0 + N1 + N2) / 3.0, 0.0);
  Pa = float4((P0 + P1 + P2) / 3.0, 1.0);
  Pb = Pa + s * N;

  GSVertexOutput vertex;

  // Face normal
  vertex.Color = float3(0.9, 0.9, 0.0);
  Pa = mul(projection_matrix, Pa);
  Pb = mul(projection_matrix, Pb);

  vertex.Position = Pa;
  output.Append(vertex);
  vertex.Position = Pb;
  output.Append(vertex);
  output.RestartStrip();


  // Edges
  vertex.Color = float3(0.65, 0, 0.65);
  // Totally hacky...
  P0.z += 0.001;
  P1.z += 0.001;
  P2.z += 0.001;
  float4 Q0 = mul(projection_matrix, float4(P0, 1.0));
  float4 Q1 = mul(projection_matrix, float4(P1, 1.0));
  float4 Q2 = mul(projection_matrix, float4(P2, 1.0));

  // Edge 0
  vertex.Position = Q0;
  output.Append(vertex);
  vertex.Position = Q1;
  output.Append(vertex);
  output.RestartStrip();

  // Edge 1
  vertex.Position = Q1;
  output.Append(vertex);
  vertex.Position = Q2;
  output.Append(vertex);
  output.RestartStrip();

  // Edge 2
  vertex.Position = Q2;
  output.Append(vertex);
  vertex.Position = Q0;
  output.Append(vertex);
  output.RestartStrip();
}

// =============================================================================
// Pixel Shader
// =============================================================================
struct PSInput {
  float4 PositionCS : SV_POSITION;
  float3 Color      : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET
{
  return float4(input.Color, 1);
}
