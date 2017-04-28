

struct VSOutput {
	float4 Position : SV_POSITION;
  float3 Color : COLOR;
};

//
// glslang does not currently support DX style packing which
// allows for a float3 to be immediately after a float as long
// as they can fit into an aligned float4.
//
struct UniformsT {
  float  pad;
  float3 color;
};

ConstantBuffer<UniformsT> uniforms : register(b0);

VSOutput VSMain(float4 Position : POSITION)
{
	VSOutput result;
	result.Position = Position;
	result.Color = uniforms.color;
	return result;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
  return float4(input.Color, 1);
}