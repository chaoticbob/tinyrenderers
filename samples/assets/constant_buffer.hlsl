

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
  //float  pad;
  float3 color;
};

// Currently, glslang produces SPIR-V  from this that  some
// cards do not like, so use the old form below.
//ConstantBuffer<UniformsT> uniforms : register(b0);

cbuffer uniforms : register(b0)
{
	//float  pad;
	float3 color;
};

VSOutput VSMain(float4 Position : POSITION)
{
	VSOutput result;
	result.Position = Position;
	result.Color = color;
	return result;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
  return float4(input.Color, 1);
}
