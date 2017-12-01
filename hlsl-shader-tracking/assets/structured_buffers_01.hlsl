// =================================================================================================
// Vertex
// =================================================================================================
struct VSOutput {
	float4 Position : SV_POSITION;
	float2 TexCoord : TEXCOORD;
};

VSOutput vsmain(float4 Position : POSITION, float2 TexCoord : TEXCOORD0)
{
	VSOutput result;
	result.Position = Position;
	result.TexCoord = TexCoord;
	return result;
}

// =================================================================================================
// Pixel
// =================================================================================================
Texture2D uTex0 : register(t0);
SamplerState uSampler0 : register(s1);

float4 psmain(VSOutput input) : SV_TARGET
{
	return uTex0.Sample(uSampler0, input.TexCoord);
}

// =================================================================================================
// Compute
// =================================================================================================
#define R_SHIFT  0
#define G_SHIFT  8
#define B_SHIFT 16
#define A_SHIFT 24

struct CSInput {
  uint  color;
  float r;
  float g;
  float b;
};

struct CSOutput {
  uint color;
};

StructuredBuffer<CSInput>     BufferIn  : register(t0);
RWStructuredBuffer<CSOutput>  BufferOut : register(u1);

struct BufferPair {
  StructuredBuffer<CSInput>     ro;
  RWStructuredBuffer<CSOutput>  rw;
};

float ReadRed(uint x, uint y, BufferPair pair)
{
  uint index = y * 1024 + x;
  uint color = pair.ro[index].color;
  // Read R channel
  float value = (float)((color >> R_SHIFT) & 0xFF);
  // Scale by 'r' component
  value *= pair.ro[index].r;
  return value;
}

float ReadGreen(uint x, uint y, BufferPair pair)
{
  uint index = y * 1024 + x;
  uint color = pair.ro[index].color;
  // Read G channel
  float value = (float)((color >> G_SHIFT) & 0xFF);
  // Scale by 'g' component
  value *= pair.ro[index].g;
  return value;
}

void WriteRedGreen(uint x, uint y, float r, float g, BufferPair pair)
{
  uint index = y * 1024 + x;
  uint A = 0xFF000000;
  uint R = (((uint)floor(r)) & 0xFF) << 0;
  uint G = (((uint)floor(g)) & 0xFF) << 8;
  uint val = A | R | G;
  pair.rw[index].color = val;
}

[numthreads(16, 16, 1)]
void csmain(uint3 tid : SV_DispatchThreadID)
{
  BufferPair pair;
  pair.ro = BufferIn;
  pair.rw = BufferOut;

  float r = ReadRed(tid.x, tid.y, pair);
  float g = 255.0 - ReadGreen(tid.x, tid.y, pair);
  WriteRedGreen(tid.x, tid.y, r, g, pair);

  // Write constant value or B channel
  uint index = tid.y * 1024 + tid.x;
  uint B = 0x7F << B_SHIFT;
  BufferOut[index].color |= B;
}