
struct Data {
  uint  rgba;
};

ConsumeStructuredBuffer<Data>  BufferIn    : register(u0);
AppendStructuredBuffer<Data>   BufferOut   : register(u2);


struct DebugData {
  uint  control;
  uint4 data;
};

AppendStructuredBuffer<DebugData> BufferDebug : register(u4);

void printf(AppendStructuredBuffer<DebugData> buffer, uint value)
{
  DebugData data = (DebugData)0;
  data.control = 1;
  data.data = uint4(value, 0, 0, 0);
  buffer.Append(data);
}

void printf(AppendStructuredBuffer<DebugData> buffer, uint2 value)
{
  DebugData data = (DebugData)0;
  data.control = 2;
  data.data = uint4(value, 0, 0);
  buffer.Append(data);
}

void printf(AppendStructuredBuffer<DebugData> buffer, uint3 value)
{
  DebugData data = (DebugData)0;
  data.control = 3;
  data.data = uint4(value, 0);
  buffer.Append(data);
}

void printf(AppendStructuredBuffer<DebugData> buffer, uint4 value)
{
  DebugData data = (DebugData)0;
  data.control = 4;
  data.data = uint4(value);
  buffer.Append(data);
}

[numthreads(1, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID, uint gid : SV_GroupIndex)
{
  Data val = BufferIn.Consume();
  val.rgba = 0xFF000000 | 
             ((val.rgba & 0x000000FF) <<  8) | 
             ((val.rgba & 0x0000FF00) <<  8) | 
             ((val.rgba & 0x00FF0000) >> 16);
  val.rgba |= 0x7F;             
  BufferOut.Append(val);
    
  int index = tid.y * 1024 + tid.x;
  if (index & 1) {
    printf(BufferDebug, tid.xy);
  }
  else {
    printf(BufferDebug, tid);
  }
}