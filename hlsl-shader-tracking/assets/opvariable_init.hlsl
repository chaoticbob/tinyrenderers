
float4 vsmain(float4 PositionOS : Position) : SV_Position
{
  return PositionOS;
}

float4 psmain(float4 PositionCS : SV_Position) : SV_Target
{
  static int r = 16;
  static int g = 32;
  static int b = 64;
  return float4(float(r + 16)/64.0, // 0.5
                float(g - 32)/32.0, // 0.0
                float(b - 32)/32.0, // 1.0
                1);
}
