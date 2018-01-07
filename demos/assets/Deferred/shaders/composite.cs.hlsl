#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX     t0
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER       s1
#define DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX    u2

#define NUM_THREADS_X   8
#define NUM_THREADS_Y   8
#define NUM_THREADS_Z   1

Texture2D             InputTex  : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_INPUT_TEX);
SamplerState          Sampler   : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_SAMPLER);
RWTexture2D<float4>   OutputTex : register(DESCRIPTOR_BINDING_DEFERRED_COMPOSITE_OUTPUT_TEX);

float4 Sample(float2 coord, Texture2D tex, SamplerState sam)
{
  float4 value = tex.SampleLevel(sam, coord, 0);
  return value;
}

[numthreads(NUM_THREADS_X, NUM_THREADS_Y, NUM_THREADS_Z)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
  // Get output diemsions
  uint2 output_tex_size;
  OutputTex.GetDimensions(output_tex_size.x, output_tex_size.y);

  // Process if within bounds
  if ((tid.x < output_tex_size.x) && (tid.y < output_tex_size.y)) {
    float2 coord = (float2)tid.xy / (float2)output_tex_size;
    float4 value = Sample(coord, InputTex, Sampler);
    OutputTex[tid.xy] = value;
  }
}