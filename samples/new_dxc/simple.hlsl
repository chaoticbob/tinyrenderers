
float4 VSMain(float4 Position : POSITION) : SV_POSITION
{
	return Position;
}

float4 PSMain(float4 Position : SV_POSITION) : SV_TARGET
{
	return float4(1, 0, 0, 1);
}