
struct VSOutput {
  float4 Position      : SV_POSITION;
  float2 TexCoord      : TEXCOORD;
  float2 Unused        : PADDING; // Without this, Vulkan result will look wrong
  float  ColorScale[4] : COLOR_SCALE;
};

VSOutput VSMain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
  VSOutput result;
  result.Position = Position;
  result.TexCoord = TexCoord;
  result.ColorScale[0] = 1.0f;
  result.ColorScale[1] = 1.0f;
  result.ColorScale[2] = 1.5f;
  result.ColorScale[3] = 1.0f;
  return result;
}

Texture2D uTex0 : register(t0);
SamplerState uSampler0 : register(s1);

float4 PSMain(VSOutput input) : SV_TARGET
{
  float4 colorScale = float4(input.ColorScale[0], input.ColorScale[1], input.ColorScale[2], input.ColorScale[3]);
  return uTex0.Sample(uSampler0, input.TexCoord) * colorScale;
}
