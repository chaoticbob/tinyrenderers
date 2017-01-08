
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

float4 PSMain(VSOutput input) : SV_TARGET
{
	return float4(input.TexCoord, 0, 1);
}