
struct BufferElement {
  uint color;
};

StructuredBuffer<BufferElement>   Buffer0 : register(t0);
RWStructuredBuffer<BufferElement> BufferOut : register(u1);

[numthreads(1, 1, 1)]
void CSMain(uint3 tid : SV_DispatchThreadID)
{
  uint val = Buffer0[tid.x].color;
  BufferOut[tid.x].color = (val & 0xFF000000) |
                           ((val & 0x000000FF) <<  8) |
                           ((val & 0x0000FF00) <<  8) |
                           ((val & 0x00FF0000) >> 16);
}