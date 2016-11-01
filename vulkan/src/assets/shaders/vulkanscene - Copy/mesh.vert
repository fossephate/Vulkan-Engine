#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

layout (binding = 0) uniform UBO 
{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 normal;
	vec3 lightpos;
} ubo;

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outEyePos;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outUV = inUV;
	outNormal = normalize(mat3(ubo.normal) * inNormal);
	outColor = inColor;

	mat4 modelView = ubo.view * ubo.model;
	vec4 pos = modelView * inPos;
	
	//gl_Position = ubo.projection * pos;
	gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPos.xyz, 1.0);

	outEyePos = vec3(modelView * pos);
	
	//vec4 lightPos = vec4(ubo.lightpos, 1.0) * modelView;
	vec4 lightPos = vec4(ubo.lightpos, 1.0) * ubo.view;

	outLightVec = normalize(lightPos.xyz - outEyePos);
}