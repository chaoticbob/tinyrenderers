
ByteAddressBuffer BufferIn : register(t0);
RWByteAddressBuffer BufferOut : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  int addr = 4 * tid.x;
  uint val = BufferIn.Load(addr);
  BufferOut.InterlockedAdd(addr, val);
}