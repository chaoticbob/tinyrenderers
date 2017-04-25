
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (std430, set=0, binding=0) buffer Buffer0T {
  uint color[];
} Buffer0;

layout (std430, set=0, binding=1) buffer BufferOutT{
  uint color[];
} BufferOut;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{
  uint idx = gl_GlobalInvocationID.x;
  uint val = Buffer0.color[idx];
  BufferOut.color[idx] = (val & 0xFF000000) |
                         ((val & 0x000000FF) <<  8) |
                         ((val & 0x0000FF00) <<  8) |
                         ((val & 0x00FF0000) >> 16);
}