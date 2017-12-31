
#include "common.hlsl"
#include "brdf.hlsl"

#if defined(WRITE_DEBUG)
// =================================================================================================
// Debug
// =================================================================================================
PSDeferredOutput psmain() 
{
  PSDeferredOutput output = 0;
  output.oColor0 = float4(1, 0, 0, 1);
  output.oColor1 = float4(0, 1, 0, 1);
  output.oColor2 = float4(0, 0, 1, 1);
  output.oColor3 = float4(1, 1, 0, 1);
  return output;
}
#else
// =================================================================================================
// Implementation
// =================================================================================================
PSDeferredOutput psmain(PSInput input) 
{
  PSDeferredOutput output = (PSDeferredOutput)0;
  return output;
}
#endif