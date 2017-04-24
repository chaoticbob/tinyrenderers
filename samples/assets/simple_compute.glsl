
#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Read only image requires exact format
layout (set=0, binding=0, rgba8) uniform readonly image2D Buffer0;
// Read/write image format is flexible
layout (set=0, binding=1, rgba32f) uniform image2D BufferOut;

layout (local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

void main()
{  
  vec4 src = imageLoad(Buffer0, ivec2(gl_GlobalInvocationID.xy));
  imageStore(BufferOut, ivec2(gl_GlobalInvocationID.xy), src.brga);
}