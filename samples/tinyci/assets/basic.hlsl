
struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput VSMain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
	VSOutput result;
	result.Position = Position;
	result.TexCoord = TexCoord;
	return result;
}

Texture2D uTex0 : register(t0);
SamplerState uSampler0 : register(s1);

float4 PSMain(VSOutput input) : SV_TARGET
{
	//return float4(input.TexCoord, 0, 1);
	return uTex0.Sample(uSampler0, input.TexCoord);
}