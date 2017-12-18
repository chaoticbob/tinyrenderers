// Tessellation based on https://github.com/spazzarama/Direct3D-Rendering-Cookbook

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

cbuffer Tessellation : register(b2)
{
  float tess_factor;
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
  float4 Position4 = float4(input.PositionOS, 1);
  VSOutput result;
  result.PositionWS  = mul(model_matrix, Position4).xyz;
  result.NormalWS    = mul(normal_matrix_world_space, input.NormalOS);
  return result;
}

// =============================================================================
// Hull Shader
// =============================================================================
struct HSInput {
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

struct HSOutput {
  float3 PositionWS : POSITION;
};

struct HSTrianglePatchConstant {
  float  EdgeTessFactor[3] : SV_TessFactor;
  float  InsideTessFactor  : SV_InsideTessFactor;
  float3 NormalWS[3]       : NORMAL;
};

HSTrianglePatchConstant HSPatchConstant(InputPatch<HSInput, 3> patch)
{
  float3 roundedEdgeTessFactor = tess_factor;
  float  roundedInsideTessFactor = 3;
  float  insideTessFactor = 1;
/*
  ProcessTriTessFactorsMax(tess_factor,
                           1.0,
                           roundedEdgeTessFactor,
                           roundedInsideTessFactor,
                           insideTessFactor);
*/

  HSTrianglePatchConstant result;

  // Edge and inside tessellation factors
  result.EdgeTessFactor[0] = roundedEdgeTessFactor.x;
  result.EdgeTessFactor[1] = roundedEdgeTessFactor.y;
  result.EdgeTessFactor[2] = roundedEdgeTessFactor.z;
  result.InsideTessFactor  = roundedInsideTessFactor;

  // Constant data
  result.NormalWS[0] = patch[0].NormalWS;
  result.NormalWS[1] = patch[1].NormalWS;
  result.NormalWS[2] = patch[2].NormalWS;

  return result;
}

[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("HSPatchConstant")]
HSOutput HSMain(
  InputPatch<HSInput, 3>  patch,
  uint                    id : SV_OutputControlPointID
)
{
  HSOutput output;
  output.PositionWS = patch[id].PositionWS;
  return output;
}

// =============================================================================
// Domain Shader
// =============================================================================
struct DSInput {
  float3 PositionWS : POSITION;
};

struct DSOutput
{
  float4 PositionCS : SV_Position;
  float3 PositionWS : POSITION;
  float3 NormalWS   : NORMAL;
};

float3 BarycentricInterpolate(float3 v0, float3 v1, float3 v2, float3 barycentric)
{
    return barycentric.z * v0 + barycentric.x * v1 + barycentric.y * v2;
}

float3 BarycentricInterpolate(float3 v[3], float3 barycentric)
{
    return BarycentricInterpolate(v[0], v[1], v[2], barycentric);
}

float3 ProjectOntoPlane(float3 planeNormal, float3 planePoint, float3 pointToProject)
{
    return pointToProject - dot(pointToProject - planePoint, planeNormal) * planeNormal;
}

[domain("tri")]
DSOutput DSMain(
  HSTrianglePatchConstant           constant_data,
  const OutputPatch<DSInput, 3>     patch,
  float3                            uvw : SV_DomainLocation
)
{
    float3 position = BarycentricInterpolate(patch[0].PositionWS,
                                             patch[1].PositionWS,
                                             patch[2].PositionWS,
                                             uvw);

    float3 normal = BarycentricInterpolate(constant_data.NormalWS, uvw);

    // Orthogonal projection in the tangent planes
    float3 pos_projected_U = ProjectOntoPlane(constant_data.NormalWS[0], patch[0].PositionWS, position);
    float3 pos_projected_V = ProjectOntoPlane(constant_data.NormalWS[1], patch[1].PositionWS, position);
    float3 pos_projected_W = ProjectOntoPlane(constant_data.NormalWS[2], patch[2].PositionWS, position);

    // Interpolate the projected points
    position = BarycentricInterpolate(pos_projected_U,
                                      pos_projected_V,
                                      pos_projected_W,
                                      uvw);

    DSOutput result;
    result.PositionCS = mul(view_projection_matrix, float4(position, 1));
    result.PositionWS = position;
    result.NormalWS   = normal;
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
  float3 V = view_direction;
  float3 R = reflect(L, N);

  float  A = 0.3;
  float  D = 0.5 * lambert(N, L);
  float  S = 2.0 * phong(N, L, V, R, 8.0);
  float3 C = color;
  float3 Co = C * (0.3 + D + S); // * (A + D + S);

  float4 oColor0 = float4(Co, 1);
  return oColor0;
}
