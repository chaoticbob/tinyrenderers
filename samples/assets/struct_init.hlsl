
Texture2D MyTexture : register(t0);
SamplerState MySampler : register(s0);

struct ColorScale {
  float4 scale;
};

struct Combined {
  Texture2D     tex;
  SamplerState  sam;
  ColorScale    scale;
};

float4 GetSample(Combined combined, float2 texCoord)
{
  return combined.tex.Sample(combined.sam, texCoord);
}

static Combined combined = { MyTexture, MySampler, {float4(1, 1, 1, 1)} };

float4 main(float2 texCoord : TEXCOORD) : SV_TARGET0
{
  //Combined combined = { MyTexture, MySampler };
  return GetSample(combined, texCoord);
}