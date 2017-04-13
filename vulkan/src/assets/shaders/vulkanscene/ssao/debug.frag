#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerPosition;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;
layout (set = 3, binding = 4) uniform sampler2D samplerSSAO;

//layout (set = 3, binding = 6) uniform sampler2D samplerShadowMap;
layout (set = 3, binding = 6) uniform sampler2DArray samplerShadowMap;

layout (location = 0) in vec3 inUV;// vec3

layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec3 components[3];
	components[0] = texture(samplerPosition, inUV.st).rgb;
	//components[0] = vec3(texture(samplerPosition, inUV.st).a);

	components[1] = texture(samplerNormal, inUV.st).rgb;  
	ivec2 texDim = textureSize(samplerAlbedo, 0);
	uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim ), 0);
//	uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);

	vec4 color;
	color.rg = unpackHalf2x16(albedo.r);
	color.ba = unpackHalf2x16(albedo.g);
	vec4 spec;
	spec.rg = unpackHalf2x16(albedo.b);
	//components[2] = vec3(spec.r);
	//components[0] = vec3(spec.r);

	//vec4 ssao = texture(samplerSSAO, inUV.st);
	//components[2] = vec3(ssao.r);

	//vec4 shadow = texture(samplerShadowMap, inUV.st);
	vec4 shadow = texture(samplerShadowMap, vec3(inUV));
	components[2] = vec3(shadow.r);

	//components[2] = vec3(ssao);

	//components[2] = texture(samplerAlbedo, inUV.st).rgb;

	//components[0] = color.rgb;



	// Select component depending on z coordinate of quad
	highp int index = int(inUV.z);
	outFragColor.rgb = components[index];
}