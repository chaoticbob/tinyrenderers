#version 150
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 vPosition;
layout (location = 1) in vec2 vTexCoord0;

layout (std140, set=0, binding=0) uniform UniformBlock0T {
	uniform mat4 mvp;
} UniformBlock0;

layout (location = 0) out vec2 TexCoord0;

void main( void )
{
	mat4 mvp = UniformBlock0.mvp;
	gl_Position = mvp * vPosition;
	TexCoord0 = vTexCoord0;
}
