
struct Data {
  uint  rgba;
};

// 
// Vulkan version requires 'buffer' for source
// buffer - so just match it.
// 
ConsumeStructuredBuffer<Data>  BufferIn : register(u0);
AppendStructuredBuffer<Data>  BufferOut : register(u1);

[numthreads(16, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  Data val = BufferIn.Consume();
  val.rgba = 0xFF000000 | 
             ((val.rgba & 0x000000FF) <<  8) | 
             ((val.rgba & 0x0000FF00) <<  8) | 
             ((val.rgba & 0x00FF0000) >> 16);
  BufferOut.Append(val);
}