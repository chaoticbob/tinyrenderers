
float4 vsmain(float4 PositionOS : Position) : SV_Position
{
  return PositionOS;
}

float4 psmain(float4 PositionCS : SV_Position) : SV_Target
{
  float values[2];
  values[0] = 0.0;
  values[1] = 1.0;

  float input[2]; 
  
  // Works - final output is red
  //input = values;
  
  // Works - final output is red
  //input[0] = values[0];
  //input[1] = values[1];
  
  // DXC/SPIRV: Only works with -O0, otherwise final output is yellow
  // 
  // UPDATE: This issue was addressed in SPIRV-Tools around this commit:
  //         059fe0822a8075511aee8622f9daeb9a9abedb1b 
  //
  //
  //for (int i = 0; i < 2; ++i) {
  //  input[i] = values[i];
  //}
  //
  //values[0] = input[1];
  //values[1] = input[0];

  // This variation also works with the updates to SPIRV-Tools
  input = values;
  for (int i = 0; i < 1; ++i) {
    values[i] = input[i + 1];
    values[i + 1] = input[i];
  }
  
  return float4(values[0], values[1], 0, 1);
}
