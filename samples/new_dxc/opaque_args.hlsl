
struct VSOutput {
  float4 Position      : SV_POSITION;
  float2 TexCoord      : TEXCOORD;
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

struct CombinedImageSampler {
  Texture2D     tex;
  SamplerState  sam;
};

float4 GetSample(CombinedImageSampler combined, float2 texCoord)
{
  float4 res = combined.tex.Sample(combined.sam, texCoord);
  return res;
}

float4 PSMain(VSOutput input) : SV_TARGET
{
  CombinedImageSampler combined;
  combined.tex = uTex0;
  combined.sam = uSampler0;
  return GetSample(combined, input.TexCoord) * float4(1.25f, 1.0f, 1.0f, 1.0f);
}