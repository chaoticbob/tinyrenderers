
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct Input {
  uint  color;
  float r;
  float g;
  float b;
};

layout (std430, set=0, binding=0) buffer BufferInT {
  Input elem[];
} BufferIn;

layout (std430, set=0, binding=1) buffer BufferOutT{
  uint color[];
} BufferOut;

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

void main()
{
  uint idx = gl_GlobalInvocationID.x;
  uint val = BufferIn.elem[idx].color;
  float r = float((val >>  0) & 0xFF) * BufferIn.elem[idx].r;
  float g = float((val >>  8) & 0xFF) * BufferIn.elem[idx].g;
  float b = float((val >> 16) & 0xFF) * BufferIn.elem[idx].b;
  BufferOut.color[idx] = 0xFF000000 |
                         ((uint(floor(r)) & 0xFF) <<  0) |
                         ((uint(floor(g)) & 0xFF) <<  8) |
                         ((uint(floor(b)) & 0xFF) << 16); 
}