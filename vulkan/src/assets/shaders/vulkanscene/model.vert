#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;
layout (location = 4) in vec4 inBoneWeights;
layout (location = 5) in ivec4 inBoneIDs;


// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 view;
	mat4 projection;
	mat4 normal;

	vec3 lightPos;
	vec3 cameraPos;
} scene;


#define MAX_BONES 64

// layout (binding = 0) uniform UBO 
// {
// 	mat4 projection;
// 	mat4 view;
// 	mat4 model;
// 	mat4 bones[MAX_BONES];	
// 	vec4 lightPos;
// 	vec4 viewPos;
// } ubo;

// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	mat4 g1;
	mat4 bones[MAX_BONES];
	mat4 g2;
} matrices;



layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

layout (location = 5) out vec3 outPos;
layout (location = 6) out vec3 outCamPos;
layout (location = 7) out vec3 outLightPos;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{

	mat4 boneTransform = matrices.bones[inBoneIDs[0]] * inBoneWeights[0];
	boneTransform     += matrices.bones[inBoneIDs[1]] * inBoneWeights[1];
	boneTransform     += matrices.bones[inBoneIDs[2]] * inBoneWeights[2];
	boneTransform     += matrices.bones[inBoneIDs[3]] * inBoneWeights[3];

	//mat4 boneTransform = matrices.bones[inBoneIDs[0]] * inBoneWeights[0];
	// boneTransform     += matrices.bones[inBoneIDs[1]] * 0.00001;
	// boneTransform     += matrices.bones[inBoneIDs[2]] * 0.00001;
	// boneTransform     += matrices.bones[inBoneIDs[3]] * 0.00001;

	//mat4 boneTransform = mat4(1.0);

	outUV = inUV;
	outNormal = inNormal;
	outColor = inColor;

	outPos = inPos.xyz;
	outCamPos = scene.cameraPos;
	outLightPos = scene.lightPos;

	mat4 modelView = scene.view * matrices.model;
	mat4 MVP = scene.projection * scene.view * matrices.model;

	gl_Position = scene.projection * scene.view * matrices.model * boneTransform * vec4(inPos.xyz, 1.0);

	vec4 pos = matrices.model * vec4(inPos, 1.0);

	// outNormal = mat3(inverse(transpose(ubo.model * boneTransform))) * inNormal;
	// outLightVec = ubo.lightPos.xyz - pos.xyz;
	// outViewVec = ubo.viewPos.xyz - pos.xyz;		
	
	vec3 lPos = mat3(matrices.model) * scene.lightPos.xyz;
	outLightVec = lPos - (matrices.model * vec4(inPos.xyz, 0.0)).xyz;
	outViewVec = -(matrices.model * vec4(inPos.xyz, 0.0)).xyz;
}