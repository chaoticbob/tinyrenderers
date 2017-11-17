
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

struct Primitive {
  float4        colors[4];
  int           index;
};

struct Opaque {
  Texture2D     tex;
  SamplerState  sam;
};

struct Combined {
  Primitive     primitives;
  Opaque        opaques;
  int           which;
};

Combined CreateCombined(Texture2D tex, SamplerState sam, int which)
{
  Combined s;
  if (which >= 4) {
    s.opaques.tex = tex;
    s.opaques.sam = sam;
  }
  else {
    s.primitives.colors[0] = float4(1, 0, 0, 0);
    s.primitives.colors[1] = float4(0, 1, 0, 0);
    s.primitives.colors[2] = float4(0, 0, 1, 0);
    s.primitives.colors[3] = float4(1, 1, 0, 0);
    s.primitives.index = which;
  }
  s.which = which;
  return s;
}

float4 SampleCombined(Combined s, float2 uv)
{
  float4 value = float4(0, 0, 0, 0);
  if (s.which > 4) {
    value = s.opaques.tex.Sample(s.opaques.sam, uv);
    value.z += 0.5;
  }
  else {
    value = s.primitives.colors[s.primitives.index];
  }
  return value;
}

struct PSInput {
  float4 Position : SV_POSITION;
  float2 TexCoord : TEXCOORD;
};

float4 psmain(PSInput input) : SV_TARGET
{
  Combined s = CreateCombined(uTex0, uSampler0, 5);
  float4 output = SampleCombined(s, input.TexCoord);
  return output;
}