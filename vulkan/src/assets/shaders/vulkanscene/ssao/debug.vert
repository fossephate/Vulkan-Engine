#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;// vec2
layout (location = 3) in vec3 inNormal;

layout (set = 3, binding = 0) uniform UBO 
{
	mat4 model;
	mat4 projection;
} ubo;

layout (location = 0) out vec3 outUV;// vec3

void main() {
	outUV = vec3(inUV.st, inNormal.z);
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}