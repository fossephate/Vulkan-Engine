#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
//layout (location = 4) in vec3 inTangent;
layout (location = 5) in vec4 inBoneWeights;
layout (location = 6) in ivec4 inBoneIDs;



#define MAX_BONES 64

#define MAX_SKINNED_MESHES 10


// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 view;
	mat4 projection;

	vec4 lightPos;
	vec4 cameraPos;

} scene;


// bone data
layout (set = 0, binding = 1) uniform boneBuffer
{
	mat4 bones[MAX_BONES*MAX_SKINNED_MESHES];
} boneData;



// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	mat4 boneIndex;
} instance;


// // bone data
// layout (set = 4, binding = 0) uniform boneBuffer
// {
// 	mat4 bones[MAX_BONES*MAX_SKINNED_MESHES];
// } boneData;



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

void main() {


	int index = int(instance.boneIndex[0][0]);

	int offset = index*MAX_BONES;

	mat4 boneTransform = boneData.bones[inBoneIDs[0]+offset] * inBoneWeights[0];
	boneTransform     += boneData.bones[inBoneIDs[1]+offset] * inBoneWeights[1];
	boneTransform     += boneData.bones[inBoneIDs[2]+offset] * inBoneWeights[2];
	boneTransform     += boneData.bones[inBoneIDs[3]+offset] * inBoneWeights[3];

	//mat4 boneTransform = mat4(1.0);

	outUV = inUV;
	outNormal = inNormal;
	outColor = inColor;

	outPos = inPos.xyz;
	outCamPos = scene.cameraPos.xyz;
	outLightPos = scene.lightPos.xyz;

	mat4 modelView = scene.view * instance.model;
	mat4 MVP = scene.projection * scene.view * instance.model;

	gl_Position = scene.projection * scene.view * instance.model * boneTransform * inPos;


	vec4 pos = instance.model * vec4(inPos.xyz, 1.0);

	// outNormal = mat3(inverse(transpose(ubo.model * boneTransform))) * inNormal;
	// outLightVec = ubo.lightPos.xyz - pos.xyz;
	// outViewVec = ubo.viewPos.xyz - pos.xyz;		
	
	vec3 lPos = mat3(instance.model) * scene.lightPos.xyz;
	outLightVec = lPos - (instance.model * vec4(inPos.xyz, 0.0)).xyz;
	outViewVec = -(instance.model * vec4(inPos.xyz, 0.0)).xyz;


}