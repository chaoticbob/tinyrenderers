
// 
// Vulkan version requires 'buffer' for source
// buffer - so just match it.
// 
RWByteAddressBuffer BufferIn : register(u0);
RWByteAddressBuffer BufferOut : register(u1);

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  int addr = 4 * tid.x;
  uint val = BufferIn.Load(addr);
  BufferOut.InterlockedAdd(addr, val);

  /*
  uint val = BufferIn.Load(addr);
  val = 0xFF000000 |
        ((val & 0x000000FF) <<  8) |
        ((val & 0x0000FF00) <<  8) |
        ((val & 0x00FF0000) >> 16);
  BufferOut.Store(addr, val);
  */
}