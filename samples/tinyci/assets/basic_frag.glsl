#version 150
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform texture2D uTex0;
layout (set = 0, binding = 1) uniform sampler   uSampler0;

layout (location = 0) in vec2 TexCoord0;

layout (location = 0) out vec4 oColor;

void main( void )
{
	vec4 samp = texture(sampler2D(uTex0, uSampler0), TexCoord0);
	oColor = vec4(samp);
}
