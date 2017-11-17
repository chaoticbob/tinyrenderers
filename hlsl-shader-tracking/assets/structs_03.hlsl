
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
  int           which;
};

Combined CreateCombined(Texture2D tex, SamplerState sam, int which)
{
  Combined s;
  s.tex   = tex;
  s.sam   = sam;
  s.which = which;
  return s;
}

float4 SampleCombined(Combined s, float2 uv)
{
  float4 value = s.tex.Sample(s.sam, uv);
  if (s.which == 1) {
    value.x += 0.5;
  }
  return value;
}

struct PSInput {
  float4 Position : SV_POSITION;
  float2 TexCoord : TEXCOORD;
};

float4 psmain(PSInput input) : SV_TARGET
{
  Combined s = CreateCombined(uTex0, uSampler0, 1);
  float4 output = SampleCombined(s, input.TexCoord);
  return output;
}