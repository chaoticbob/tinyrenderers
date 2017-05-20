
struct Data {
  uint  rgba;
};

/*

If compiling with glslang, se --auto-map-bindings. This will
create the proper bindings for the counter buffer. The bindings
should look like this:
    BufferIn data   - binding = 0
    BufferIn count  - binding = 1
    BufferOut data  - binding = 2
    BufferOut count - binding = 4

Resulting SPIR-V should look something like this:
    ...
    MemberDecorate 16(Data) 0 Offset 0
    Decorate 17 ArrayStride 4
    MemberDecorate 18(BufferIn) 0 Offset 0
    Decorate 18(BufferIn) BufferBlock
    Decorate 20(BufferIn) DescriptorSet 0
  * Decorate 20(BufferIn) Binding 0
    MemberDecorate 23(BufferIn@count) 0 Offset 0
    Decorate 23(BufferIn@count) BufferBlock
    Decorate 25(BufferIn@count) DescriptorSet 0
  * Decorate 25(BufferIn@count) Binding 1
    Decorate 61(BufferOut) DescriptorSet 0
  * Decorate 61(BufferOut) Binding 2
    Decorate 62(BufferOut@count) DescriptorSet 0
  * Decorate 62(BufferOut@count) Binding 3
    ...

*/
ConsumeStructuredBuffer<Data>  BufferIn : register(u0);
AppendStructuredBuffer<Data>  BufferOut : register(u2);

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  Data val = BufferIn.Consume();
  val.rgba = 0xFF000000 | 
             ((val.rgba & 0x000000FF) <<  8) | 
             ((val.rgba & 0x0000FF00) <<  8) | 
             ((val.rgba & 0x00FF0000) >> 16);
  val.rgba |= 0x7F;             
  BufferOut.Append(val);
}