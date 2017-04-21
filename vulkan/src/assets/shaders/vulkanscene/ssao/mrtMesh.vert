#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inNormal;
layout (location = 4) in vec3 inTangent;

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec2 outUV;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outPos;
layout (location = 4) out vec3 outTangent;

// scene
layout (set = 0, binding = 0) uniform sceneBuffer 
{
	mat4 model;
	mat4 view;
	mat4 projection;
} scene;

// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	int boneIndex;
	vec3 padding;
	vec4 padding2[3];
	//mat4 boneIndex;
	mat4 g1;
	mat4 g2;
} instance;


void main() {
	
	gl_Position = scene.projection * scene.view * instance.model * inPos;
	
	outUV = inUV;
	outUV.t = 1.0 - outUV.t;// bc vulkan

	// Vertex position in world space
	outPos = vec3(instance.model * inPos);// world space position

	// Vertex position in view space
	//outPos = vec3(scene.view * instance.model * inPos);


	
	// Normal in world space
	// todo: add this matrix to the matrix nodes dynamic uniform buffer
	mat3 mNormal = transpose(inverse(mat3(instance.model)));
	//mat3 mNormal = transpose(inverse(mat3(instance.model)));
    outNormal = mNormal * normalize(inNormal);// world space normal
    //outNormal = inNormal;



	// Normal in view space
	mat3 normalMatrix = transpose(inverse(mat3(scene.view * instance.model)));
	//outNormal = normalMatrix * inNormal;

	outTangent = mNormal * inTangent;

	
	// Currently just vertex color
	outColor = inColor;
}