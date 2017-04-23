

//struct BufferElement {
//  uint  color;
//};
//StructuredBuffer<BufferElement>   Buffer0 : register(t0);
//RWStructuredBuffer<BufferElement> BufferOut : register(u1);

Texture2D<float4>   Buffer0 : register(t0);
RWTexture2D<float4> BufferOut : register(u1);

[numthreads(16, 16, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
  //uint index = threadId.y * 1024 + threadId.x;
  //BufferOut[index].color = 0xFF0000FF;

  float4 src = Buffer0[tid.xy];
  BufferOut[tid.xy] = src.bgra;
}