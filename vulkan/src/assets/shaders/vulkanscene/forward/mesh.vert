#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;
//layout (location = 5) in vec4 inBoneWeights;
//layout (location = 6) in ivec4 inBoneIDs;

#define MAX_BONES 64

// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 view;
	mat4 projection;

	vec3 lightPos;
	vec3 cameraPos;
} scene;

// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	mat4 boneIndex;
	mat4 g1;
	mat4 g2;
} instance;



layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

layout (location = 5) out vec3 outPos;
layout (location = 6) out vec3 outCamPos;
layout (location = 7) out vec3 outLightPos;




void main() 
{
	outUV = inUV;
	//outNormal = normalize(mat3(instance.normal) * inNormal);
	outNormal = inNormal;// uncommented for blinn-phong
	outColor = inColor;

	outPos = inPos.xyz;
	outCamPos = scene.cameraPos;
	outLightPos = scene.lightPos;
	
	mat4 modelView = scene.view * instance.model;
	//mat4 MVP = scene.projection * scene.view * instance.model;

	gl_Position = scene.projection * scene.view * instance.model * inPos;

	vec4 pos = modelView * vec4(inPos.xyz, 0.0);


	vec3 lPos = mat3(instance.model) * scene.lightPos.xyz;
	outLightVec = lPos - (instance.model * vec4(inPos.xyz, 0.0)).xyz;
	outViewVec = -(instance.model * vec4(inPos.xyz, 0.0)).xyz;

}