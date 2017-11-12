cbuffer UniformBlock0 : register(b0)
{
  float4x4  model_view_matrix;
  float4x4  proj_matrix;
  float3    color;
};

// =============================================================================
// Vertex Shader
// =============================================================================
float3 VSMain(float4 Position : Position) : Position
{  
  return mul(model_view_matrix, Position).xyz;
}

// =============================================================================
// Hull Shader
// =============================================================================
struct HSPatchConstantOutput {
  float edges[2] : SV_TessFactor;
};

HSPatchConstantOutput HSPatchConstant()
{
  HSPatchConstantOutput output;
  output.edges[0] = 1.0f;
  output.edges[1] = 8.0f;
  return output;
}

struct HSInput {
  float3 CPoint : Position;
};

struct HSOutput {
  float3 CPoint : ControlPoint;
};

[domain("isoline")]
[partitioning("integer")]
[outputtopology("line")]
[outputcontrolpoints(4)]
[patchconstantfunc("HSPatchConstant")]
HSOutput HSMain(InputPatch<HSInput, 4> patch, uint id : SV_OutputControlPointID)
{
  HSOutput output;
  output.CPoint = patch[id].CPoint.xyz;
  return output;
}

// =============================================================================
// Domain Shader
// =============================================================================
struct DSOutput {
  float4 SV_Position : SV_Position;
};

[domain("isoline")]
DSOutput DSMain(HSPatchConstantOutput input, float2 uv : SV_DomainLocation, const OutputPatch<HSOutput, 4> patch)
{
  DSOutput output;

  float3 P0 = patch[0].CPoint;
  float3 P1 = patch[1].CPoint;
  float3 P2 = patch[2].CPoint;
  float3 P3 = patch[3].CPoint;

  float3 a0 = -0.5*P0 + 1.5*P1 - 1.5*P2 + 0.5*P3;
  float3 a1 = P0 - 2.5*P1 + 2*P2 - 0.5*P3;
  float3 a2 = -0.5*P0 + 0.5*P2;
  float3 a3 = P1;

  float t = uv.x;
  float3 pos = a0*t*t*t
             + a1*t*t
             + a2*t
             + a3;

  output.SV_Position = mul(proj_matrix, float4(pos, 1.0));
  return output;
}

// =============================================================================
// Pixel Shader
// =============================================================================
float4 PSMain(float4 SV_Position : SV_Position) : SV_Target
{
  return float4(color, 1);
}