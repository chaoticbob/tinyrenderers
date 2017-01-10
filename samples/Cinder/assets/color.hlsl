

struct VSOutput {
	float4 Position : SV_POSITION;
	float3 Color    : COLOR;
};

VSOutput VSMain(float4 Position : POSITION, float3 Color : COLOR)
{
	VSOutput result;
	result.Position = Position;
	result.Color = Color;
	return result;
}

Texture2D uTex0 : register(t1);
SamplerState uSampler0 : register(s2);

float4 PSMain(VSOutput input) : SV_TARGET
{
	return float4(input.Color, 1);
}