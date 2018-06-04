
Texture2D<float4>  BufferIn : register(t0);
RWTexture2D<float4>  BufferOut : register(u1);

float ColorAdjust(float f0, float f1, float f2, float f3, float x) 
{
  float float_index = lerp(-3.1, 3.1, x);
  float index[4] = { f0, f1, f2, f3 };

  // Seems to cause a hang in DX and 
  // a crash in Vulkan.
  float v = index[min(int(float_index), 3)];
  
  // WORKING:
  //float v = index[max(0, min(int(float_index), 3))];

  return v;
}

[numthreads(16, 16, 1)]
void main(uint3 tid : SV_DispatchThreadID)
{
  float2 tc = (float2)tid.xy / float2(512, 512);
  float r = ColorAdjust(-1, 0, -2, -3, tc.x);
  float4 adjust = float4(r, 0, 0, 0);   
  float4 val = BufferIn[tid.xy];
  BufferOut[tid.xy] = val.brga + adjust;
}
