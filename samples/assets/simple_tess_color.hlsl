cbuffer UniformBlock0 : register(b0)
{
  float4x4  model_view_matrix;
  float4x4  proj_matrix;
  float3    color;
};

// =============================================================================
// Vertex Shader
// =============================================================================
float4 VSMain(float4 Position : Position) : SV_Position
{
  Position = mul(model_view_matrix, Position);
  return Position;
}

// =============================================================================
// Pixel Shader
// =============================================================================
float4 PSMain(float4 SV_Position : SV_Position) : SV_Target
{
  return float4(color, 1);
}