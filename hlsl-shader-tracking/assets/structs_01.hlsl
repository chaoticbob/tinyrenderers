
// =============================================================================
// Vertex Shader
// =============================================================================

struct VSInput {
  float4 Position : POSITION;
  float2 TexCoord : TEXCOORD0;
};

struct VSOutput {
  float4 Position : SV_POSITION;
  float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(VSInput input)
{
  VSOutput output;
  output.Position = input.Position;
  output.TexCoord = input.TexCoord;
  return output;
}

// =============================================================================
// Pixel Shader
// =============================================================================

Texture2D     uTex0 : register(t0);
SamplerState  uSampler0 : register(s1);

struct Combined {
  Texture2D     tex;
  SamplerState  sam;
};

float4 SampleCombined(Combined s, float2 uv)
{
  float4 value = s.tex.Sample(s.sam, uv);
  return value;
}

struct PSInput {
  float4 Position : SV_POSITION;
  float2 TexCoord : TEXCOORD;
};

float4 psmain(PSInput input) : SV_TARGET
{
  Combined s;
  s.tex = uTex0;
  s.sam = uSampler0;
  float4 output = SampleCombined(s, input.TexCoord);
  return output;
}