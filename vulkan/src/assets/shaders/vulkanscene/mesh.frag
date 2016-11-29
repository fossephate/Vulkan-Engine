#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// scene
layout (set = 0, binding = 0) uniform sceneBuffer
{
	mat4 model;
	mat4 view;
	mat4 projection;
	mat4 normal;
	vec3 lightpos;
} scene;

layout (set = 0, binding = 1) uniform sampler2D samplerColorMap;

layout (location = 0) in vec2 inUV;
layout (location = 1) in vec3 inNormal;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inEyePos;
layout (location = 4) in vec3 inLightVec;




// material data
layout (set = 2, binding = 0) uniform materialBuffer
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	vec4 emissive;
	float opacity;
} material;


layout (location = 0) out vec4 outFragColor;



float specpart(vec3 L, vec3 N, vec3 H)
{
	if (dot(N, L) > 0.0) {
		return pow(clamp(dot(H, N), 0.0, 1.0), 64.0);
	}
	return 0.0;
}

void main() 
{

	// vec3 eyePos = vec3(scene.view[0].w, scene.view[1].w, scene.view[2].w);



	// vec3 Eye = normalize(-inEyePos);


	// vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	// vec3 halfVec = normalize(inLightVec + inEyePos);
	// float diff = clamp(dot(inLightVec, inNormal), 0.0, 1.0);
	// float spec = specpart(inLightVec, inNormal, halfVec);
	// float intensity = 0.1 + diff + spec;
 
	// vec4 IAmbient = vec4(0.2, 0.2, 0.2, 1.0);
	// vec4 IDiffuse = vec4(0.5, 0.5, 0.5, 0.5) * max(dot(inNormal, inLightVec), 0.0);
	// float shininess = 0.75;
	// vec4 ISpecular = vec4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.0) * shininess; 

	// outFragColor = vec4((IAmbient + IDiffuse) * vec4(inColor, 1.0) + ISpecular);
 
	// // Some manual saturation
	// if (intensity > 0.95)
	// {
	// 	outFragColor *= 2.25;
	// }
	// if (intensity < 0.15)
	// {
	// 	outFragColor = vec4(0.1);
	// }




	vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);
	vec3 N = normalize(inNormal);
	vec3 L = normalize(inLightVec);
	vec3 V = normalize(inViewVec);
	vec3 R = reflect(-L, N);
	vec3 diffuse = max(dot(N, L), 0.0) * material.diffuse.rgb;
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * material.specular.rgb;
	outFragColor = vec4((material.ambient.rgb + diffuse) * color.rgb + specular, 1.0-material.opacity);



}