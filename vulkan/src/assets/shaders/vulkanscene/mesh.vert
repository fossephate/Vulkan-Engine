#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;


#define MAX_BONES 64

struct matrixNode {
	mat4 model;
	//mat4 g1;
	//mat4 g2;
	//mat4 g3;
};

// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 view;
	mat4 projection;

	vec3 lightPos;
	vec3 cameraPos;
	// todo: definitely remove this
	mat4 bones[MAX_BONES];
} scene;

// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	//mat4 boneIndex;
	//vec4 boneIndex;
	//vec4 padding[3];
	mat4 g1;
	//mat4 garbage2;
	//mat4 garbage3;
} instance;

// layout (set = 1, binding = 0) uniform matrixBuffer
// {
// 	matrixNode instance;
// };

// bone data
layout (set = 4, binding = 0) uniform boneBuffer
{
	mat4 bones[/*MAX_BONES*64*/4096];
} boneData;



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
	mat4 MVP = scene.projection * scene.view * instance.model;

	gl_Position = MVP * vec4(inPos.xyz, 1.0);

	vec4 pos = modelView * vec4(inPos.xyz, 0.0);
	//outNormal = mat3(instance.model) * inNormal;// commented out for blinn-phong


	vec3 lPos = mat3(instance.model) * scene.lightPos.xyz;
	outLightVec = lPos - (instance.model * vec4(inPos.xyz, 0.0)).xyz;
	outViewVec = -(instance.model * vec4(inPos.xyz, 0.0)).xyz;

	// FragPos = vec3(model * vec4(position, 1.0f));

	// vec4 pos = modelView * inPos;
	// outViewVec = vec3(modelView * pos);


	// vec4 lightPos = vec4(scene.lightPos, 1.0) * modelView;

	// outLightVec = normalize(lightPos.xyz - outViewVec);



	// outNormal = inNormal;
	// outColor = inColor;
	// outUV = inUV;

	// mat4 modelView = scene.view * instance.model;

	// gl_Position = scene.projection * scene.view * instance.model * vec4(inPos.xyz, 1.0);
	
	// vec4 pos = modelView * inPos;
	// outNormal = mat3(instance.model) * inNormal;
	// vec3 lPos = mat3(instance.model) * scene.lightPos.xyz;

	// outLightVec = lPos - (instance.model * vec4(inPos.xyz, 0.0)).xyz;
	// outViewVec = -(instance.model * vec4(inPos.xyz, 0.0)).xyz;



}