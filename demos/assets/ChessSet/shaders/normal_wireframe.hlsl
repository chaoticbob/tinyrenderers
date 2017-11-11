cbuffer UniformBlock0 : register(b0)
{
  float4x4 mv_matrix;
  float4x4 proj_matrix;
  float3x3 normal_matrix;
};


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
  float3 Normal      : NORMAL;
};

VSOutput VSMain(VSInput input)
{
  VSOutput result;
  result.SV_Position = float4(input.Position, 1.0);
  result.Normal      = normalize(input.Normal);
  return result;
}

// =============================================================================
// Geometry Shader
// =============================================================================
struct GSVertexInput {
  float4 Position : SV_POSITION;
  float3 Normal   : NORMAL;
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

  float3 P0 = mul(mv_matrix, input[0].Position).xyz;
  float3 P1 = mul(mv_matrix, input[1].Position).xyz;
  float3 P2 = mul(mv_matrix, input[2].Position).xyz;
  float3 N0 = mul(normal_matrix, input[0].Normal);
  float3 N1 = mul(normal_matrix, input[1].Normal);
  float3 N2 = mul(normal_matrix, input[2].Normal);

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
  Pa = mul(proj_matrix, Pa);
  Pb = mul(proj_matrix, Pb);

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
  float4 Q0 = mul(proj_matrix, float4(P0, 1.0));
  float4 Q1 = mul(proj_matrix, float4(P1, 1.0));
  float4 Q2 = mul(proj_matrix, float4(P2, 1.0));

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
  float4 Position : SV_POSITION;
  float3 Color    : COLOR;
};

float4 PSMain(PSInput input) : SV_TARGET
{
  return float4(input.Color, 1);
}
