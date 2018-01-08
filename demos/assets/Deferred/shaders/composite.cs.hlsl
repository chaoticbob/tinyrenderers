#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX     t0
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER       s1
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX    u2

#define NUM_THREADS_X   8
#define NUM_THREADS_Y   8
#define NUM_THREADS_Z   1

Texture2D             InputTex  : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX);
SamplerState          Sampler   : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER);
RWTexture2D<float4>   OutputTex : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX);

float4 LoadValue(uint2 coord, Texture2D tex)
{
  float4 value = tex.Load(int3(coord, 0)); //tex.SampleLevel(sam, coord, 0);
  return value;
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
  //// Get output diemsions
  //uint2 output_tex_size;
  //OutputTex.GetDimensions(output_tex_size.x, output_tex_size.y);

  // No need to bounds check since group count and thread count
  // should cleanly multiply out to match texture width and height.
  uint2 coord = tid.xy;
  float4 value = LoadValue(coord, InputTex);
  OutputTex[tid.xy] = value;
}
