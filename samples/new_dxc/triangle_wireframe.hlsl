cbuffer UniformBlock0 : register(b0)
{
  float4x4 mvp;
};


// =============================================================================
// Vertex Shader
// =============================================================================
struct VSInput {
  float4 Position : POSITION;
  float2 TexCoord : TEXCOORD0;
};

struct VSOutput {
  float4 Position : SV_POSITION;
};

VSOutput VSMain(VSInput input)
{
  VSOutput result;
  result.Position = input.Position;
  return result;
}

// =============================================================================
// Geometry Shader
// =============================================================================
struct GSVertexInput {
  float4 Position : SV_POSITION;
};

struct GSVertexOutput {
  float4 Position : SV_POSITION;
  float3 Color    : COLOR;
};

static float3 k_colors[6] = {
  {1.0, 0.0, 0.0},
  {0.0, 1.0, 0.0},
  {0.0, 0.0, 1.0},
  {1.0, 1.0, 0.0},
  {1.0, 0.0, 1.0},
  {0.0, 1.0, 1.0},
};

[maxvertexcount(6)]
void GSMain(
  triangle GSVertexInput            input[3],
  uint                              primitive_id : SV_PRIMITIVEID,
  inout LineStream<GSVertexOutput>  output)
{
  float4 P[3];
  P[0] = mul(mvp, input[0].Position);
  P[1] = mul(mvp, input[1].Position);
  P[2] = mul(mvp, input[2].Position);

  GSVertexOutput vertex;
  vertex.Color = k_colors[primitive_id / 2];

  // Edge 0
  vertex.Position = P[0];
  output.Append(vertex);
  vertex.Position = P[1];
  output.Append(vertex);
  output.RestartStrip();

  // Edge 1
  vertex.Position = P[1];
  output.Append(vertex);
  vertex.Position = P[2];
  output.Append(vertex);
  output.RestartStrip();

  // Edge 2
  vertex.Position = P[2];
  output.Append(vertex);
  vertex.Position = P[0];
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
