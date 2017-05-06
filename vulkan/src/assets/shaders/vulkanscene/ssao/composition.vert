#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;



layout (set = 3, binding = 0) uniform UBO 
{
	mat4 model;// this is used in debug.vert
	mat4 projection;// consider hardcoding in? probably not worth it
} ubo;

layout (location = 0) out vec2 outUV;

void main() {
	outUV = inUV;
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}