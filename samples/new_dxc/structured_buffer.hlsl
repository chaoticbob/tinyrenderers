
struct Input {
  uint  color;
  float r;
  float g;
  float b;
};

struct Output {
  uint color;
};

StructuredBuffer<Input>  BufferIn : register(t0);
RWStructuredBuffer<Output> BufferOut : register(u1);

[numthreads(16, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  uint val = BufferIn[tid.x].color;
  float r = (float)((val >>  0) & 0xFF) * BufferIn[tid.x].r;
  float g = (float)((val >>  8) & 0xFF) * BufferIn[tid.x].g;
  float b = (float)((val >> 16) & 0xFF) * BufferIn[tid.x].b;
  BufferOut[tid.x].color = 0xFF000000 |
                           ((((uint)floor(r)) & 0xFF) <<  0) |
                           ((((uint)floor(g)) & 0xFF) <<  8) |
                           ((((uint)floor(b)) & 0xFF) << 16);
}