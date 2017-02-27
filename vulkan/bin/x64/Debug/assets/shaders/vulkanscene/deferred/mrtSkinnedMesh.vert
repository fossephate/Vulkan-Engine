#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec4 inBoneWeights;
layout (location = 5) in ivec4 inBoneIDs;


//layout (location = 4) in vec3 inTangent;

#define MAX_BONES 64

#define MAX_SKINNED_MESHES 10


// scene
layout (set = 0, binding = 0) uniform sceneBuffer 
{
	mat4 model;
	mat4 view;
	mat4 projection;


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
	mat4 g1;
	mat4 g2;
} instance;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outWorldPos;
layout (location = 4) out vec3 outTangent;

void main() 
{

	int index = int(instance.boneIndex[0][0]);

	int offset = index*MAX_BONES;

	mat4 boneTransform = boneData.bones[inBoneIDs[0]+offset] * inBoneWeights[0];
	boneTransform     += boneData.bones[inBoneIDs[1]+offset] * inBoneWeights[1];
	boneTransform     += boneData.bones[inBoneIDs[2]+offset] * inBoneWeights[2];
	boneTransform     += boneData.bones[inBoneIDs[3]+offset] * inBoneWeights[3];

	gl_Position = scene.projection * scene.view * instance.model * boneTransform * inPos;
	
	outUV = inUV;
	outUV.t = 1.0 - outUV.t;

	// Vertex position in world space
	vec4 tmpPos = inPos;
	outWorldPos = vec3(instance.model * tmpPos);
	
	// GL to Vulkan coord space
	//outWorldPos.y = -outWorldPos.y;
	
	// Normal in world space
	// todo: do the inverse transpose on cpu
	mat3 mNormal = transpose(inverse(mat3(instance.model)));
    outNormal = mNormal * normalize(inNormal);
    //outTangent = mNormal * normalize(inTangent);
	
	// Currently just vertex color
	outColor = inColor;
}