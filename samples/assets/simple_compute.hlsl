
Texture2D<float4>   Buffer0 : register(t0);
RWTexture2D<float4> BufferOut : register(u1);

[numthreads(16, 16, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
  float4 src = Buffer0[tid.xy];
  BufferOut[tid.xy] = src.brga;
}