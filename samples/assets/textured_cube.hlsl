
cbuffer UniformBlock0 : register(b0)
{
	float4x4 mvp;
};

struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput VSMain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
	VSOutput result;
	result.Position = mul(mvp, Position);
	result.TexCoord = TexCoord;
	return result;
}

Texture2D uTex0 : register(t1);
SamplerState uSampler0 : register(s2);

float ColorAdjust(float f0, float f1, float f2, float f3, float x) 
{
  float float_index = lerp(-3.1, 3.1, x);
  float index[4] = { f0, f1, f2, f3 };

  // WILL CRASH DX + VK: 
  //float v = index[min(int(float_index), 3)];
  
  // WORKING:
  float v = index[max(0, min(int(float_index), 3))];

  return v;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
  float r = ColorAdjust(-1, 0, -2, -3, input.TexCoord.x);
  float4 adjust = float4(r, 0, 0, 0); 
	return uTex0.Sample(uSampler0, input.TexCoord) + adjust;
}