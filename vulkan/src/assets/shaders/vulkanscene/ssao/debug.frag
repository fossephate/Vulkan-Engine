#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerDepth;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;
layout (set = 3, binding = 4) uniform sampler2D samplerSSAO;

//layout (set = 3, binding = 6) uniform sampler2D samplerShadowMap;
//layout (set = 3, binding = 6) uniform sampler2DShadow samplerShadowMap;
layout (set = 3, binding = 5) uniform sampler2DArray samplerShadowMap;
//layout (set = 3, binding = 6) uniform sampler2DArrayShadow samplerShadowMap;
//layout (set = 3, binding = 7) uniform sampler2D samplerDepth;

layout (location = 0) in vec3 inUV;// vec3

layout (location = 0) out vec4 outFragColor;

#define NEAR_PLANE 0.1
#define FAR_PLANE 64.0
// #define NEAR_PLANE 1.0
// #define FAR_PLANE 512.0


float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f;
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}


void main() {
	vec3 components[3];

	//components[2] = texture(samplerPosDepth, inUV.st).rgb;
	//components[0] = vec3(texture(samplerPosition, inUV.st).r);
	//components[0] = vec3(texture(samplerPosition, inUV.st).a);

	//components[1] = texture(samplerNormal, inUV.st).rgb;
	components[1] = normalize(texture(samplerNormal, inUV.st).rgb * 2.0 - 1.0);

	ivec2 texDim = textureSize(samplerAlbedo, 0);
	uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim), 0);
//	uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);

	vec4 color;
	color.rg = unpackHalf2x16(albedo.r);
	color.ba = unpackHalf2x16(albedo.g);
	vec4 spec;
	spec.rg = unpackHalf2x16(albedo.b);
	//components[2] = vec3(spec.r);
	//components[0] = vec3(spec.r);

	vec4 ssao = texture(samplerSSAO, inUV.st);
	components[0] = vec3(ssao.r);

	// float depth = texture(samplerPosition, inUV.st).a;
	// components[0] = vec3(linearDepth(depth));

	// vec4 shadow1 = texture(samplerShadowMap, vec3(inUV.st, 1));
	// components[0] = vec3(shadow1.r);

	// float shadow1 = texture(samplerShadowMap, vec3(inUV.st, 1));
	// components[0] = vec3(shadow1);

	// vec4 shadow2 = texture(samplerShadowMap, vec3(inUV.st, 2));
	// components[0] = vec3(shadow2.r);

	//components[2] = color.rgb;

	// float depth = texture(samplerDepth, inUV.st).r;
	// components[2] = vec3(depth);

	//vec4 shadow = texture(samplerShadowMap, inUV.st);
	// inUV.w = number of light source
	//vec4 shadow = texture(samplerShadowMap, inUV.st);
	//vec4 shadow = texture(samplerShadowMap, vec3(inUV));

	vec4 shadow = texture(samplerShadowMap, vec3(inUV.st, 0));
	components[2] = vec3(shadow.r);

	// vec4 shadow2 = texture(samplerShadowMap, vec3(inUV.st, 3));
	// components[1] = vec3(shadow2.r);

	// vec4 shadow3 = texture(samplerShadowMap, vec3(inUV.st, 4));
	// components[2] = vec3(shadow3.r);

	//components[2] = vec3(linearDepth(shadow.r));

	// float shadow = texture(samplerShadowMap, vec3(inUV.st, 0));
	// components[2] = vec3(shadow);



	//shadow = texture(samplerShadowMap, vec3(inUV.st, 1));
	//components[1] = vec3(shadow.r);
	//components[2] = vec3(linearDepth(shadow.r));


	// components[2] = vec3(ssao);
	// components[2] = texture(samplerAlbedo, inUV.st).rgb;

	//components[0] = color.rgb;



	// Select component depending on z coordinate of quad
	highp int index = int(inUV.z);
	outFragColor.rgb = components[index];
}