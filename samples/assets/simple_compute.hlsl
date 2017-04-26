
// 
// Vulkan version requires imageLoad for source
// image - so just match it in D3D12.
//

#if defined(D3D12)
RWTexture2D<float4>  BufferIn : register(u0);
RWTexture2D<float4>  BufferOut : register(u1);

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  float4 val = BufferIn[tid.xy];
  BufferOut[tid.xy] = val.brga;
}

#else // glslang/Vulkan
//
// glslang seems to want a type that matches the
// incoming image format, using uint for rgba8.
//
RWTexture2D<uint>    BufferIn : register(u0);
RWTexture2D<float4>  BufferOut : register(u1);

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  uint val = BufferIn[tid.xy];
  BufferOut[tid.xy] = float4(((val >> 16) & 0xFF) / 255.0,
                             ((val >>  0) & 0xFF) / 255.0,
                             ((val >>  8) & 0xFF) / 255.0,
                             1);
}
#endif