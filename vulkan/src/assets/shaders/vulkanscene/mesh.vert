#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec4 inPos;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec2 inUV;
layout (location = 3) in vec3 inColor;

// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 model;// not used
	mat4 view;
	mat4 projection;
	mat4 normal;
	vec3 lightpos;
} scene;

// matrix data
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
} matrices;



layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outNormal;
layout (location = 2) out vec3 outColor;
layout (location = 3) out vec3 outViewVec;
layout (location = 4) out vec3 outLightVec;

void main() 
{
	outUV = inUV;
	outNormal = normalize(mat3(scene.normal) * inNormal);
	outColor = inColor;
	
	mat4 modelView = scene.view * matrices.model;
	mat4 MVP = scene.projection * scene.view * matrices.model;

	gl_Position = MVP * vec4(inPos.xyz, 1.0);

	vec4 pos = modelView * inPos;
	outViewVec = vec3(modelView * pos);


	vec4 lightPos = vec4(scene.lightpos, 1.0) * modelView;

	outLightVec = normalize(lightPos.xyz - outViewVec);



	// outNormal = inNormal;
	// outColor = inColor;
	// outUV = inUV;

	// mat4 modelView = scene.view * matrices.model;

	// gl_Position = scene.projection * scene.view * matrices.model * vec4(inPos.xyz, 1.0);
	
	// vec4 pos = modelView * inPos;
	// outNormal = mat3(matrices.model) * inNormal;
	// vec3 lPos = mat3(matrices.model) * scene.lightpos.xyz;

	// outLightVec = lPos - (matrices.model * vec4(inPos.xyz, 0.0)).xyz;
	// outViewVec = -(matrices.model * vec4(inPos.xyz, 0.0)).xyz;



}