#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//layout (set = 0, binding = 0) uniform sampler2D samplerPositionDepth;
layout (set = 0, binding = 0) uniform sampler2D samplerDepth;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform sampler2D ssaoNoise;

/*layout (constant_id = 0) */const int SSAO_KERNEL_SIZE = 64;// changed from 32
/*layout (constant_id = 1) */const float SSAO_RADIUS = 1.2;//2.0
/*layout (constant_id = 2) */const float SSAO_POWER = 2.5;//1.5;

// This constant removes artifacts caused by neighbour fragments with minimal depth difference.
#define CAP_MIN_DISTANCE 0.0001
// This constant avoids the influence of fragments, which are too far away.
#define CAP_MAX_DISTANCE 0.005


layout (set = 0, binding = 3) uniform UBOSSAOKernel
{
	vec4 samples[SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout (set = 0, binding = 4) uniform UBO 
{
	mat4 projection;
	mat4 view;// added 4/20/17
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outFragColor;


float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}



// vec4 getViewPos(vec2 texCoord) {
// 	// Calculate out of the fragment in screen space the view space position.
// 	float x = texCoord.s * 2.0 - 1.0;
// 	float y = texCoord.t * 2.0 - 1.0;
// 	// (don't) assume we have a normal depth range between 0.0 and 1.0
// 	//float z = (texture(samplerPositionDepth, inUV).a/512.0) * 2.0 - 1.0;
// 	float z = (texture(samplerPositionDepth, inUV).a);
// 	vec4 posProj = vec4(x, y, z, 1.0);
// 	vec4 posView = inverse(ubo.projection) * posProj;
// 	posView /= posView.w;
// 	return posView;
// }





// mat3 computeTBNMatrixFromDepth(in sampler2D depthTex, in vec2 uv) {
//     // Compute the normal and TBN matrix
//     float ld = -getLinearDepth(depthTex, uv);
//     vec3 x = vec3(uv.x, 0., ld);
//     vec3 y = vec3(0., uv.y, ld);
//     x = dFdx(x);
//     y = dFdy(y);
//     x = normalize(x);
//     y = normalize(y);
//     vec3 normal = normalize(cross(x, y));
//     return mat3(x, y, normal);
// }



#define NEAR_PLANE 1.0
#define FAR_PLANE 512.0


float linearDepth(float depth) {
	float z = depth * 2.0f - 1.0f;
	return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

mat3 computeTBNMatrixFromDepth(in sampler2D depthTex, in vec2 uv) {
    // Compute the normal and TBN matrix
    //float ld = -getLinearDepth(depthTex, uv);
    //float ld = -linearDepth(texture(depthTex, uv).a);
    float ld = (texture(depthTex, uv).a);
    vec3 a = vec3(uv, ld);
    vec3 x = vec3(uv.x + dFdx(uv.x), uv.y, ld + dFdx(ld));
    vec3 y = vec3(uv.x, uv.y + dFdy(uv.y), ld + dFdy(ld));
    //x = dFdx(x);
    //y = dFdy(y);`
    //x = normalize(x);
    //y = normalize(y);
    vec3 normal = normalize(cross(x - a, y - a));
    vec3 first_axis = cross(normal, vec3(1.0f, 0.0f, 0.0f));
    vec3 second_axis = cross(first_axis, normal);
    return mat3(normalize(first_axis), normalize(second_axis), normal);
}

vec3 normalFromDepth(in sampler2D depthTex, in vec2 uv) {

    //float ld = -linearDepth(texture(depthTex, uv).a);
    float ld = (texture(depthTex, uv).a);
    vec3 a = vec3(uv, ld);
    vec3 x = vec3(uv.x + dFdx(uv.x), uv.y, ld + dFdx(ld));
    vec3 y = vec3(uv.x, uv.y + dFdy(uv.y), ld + dFdy(ld));
    //x = dFdx(x);
    //y = dFdy(y);`
    //x = normalize(x);
    //y = normalize(y);
    vec3 normal = normalize(cross(x - a, y - a));
    return normal;
}



vec3 normalFromDepth2(in sampler2D depthTex, in vec2 texCoords) {
  
	const float off = 0.0001;// 0.001

	vec2 offset1 = vec2(0.0,off);
	vec2 offset2 = vec2(off,0.0);

	float depth = texture(depthTex, texCoords).a;
	float depth1 = texture(depthTex, texCoords + offset1).a;
	float depth2 = texture(depthTex, texCoords + offset2).a;

	vec3 p1 = vec3(offset1, depth1 - depth);
	vec3 p2 = vec3(offset2, depth2 - depth);

	vec3 normal = cross(p1, p2);

	float diff = dot(p1, p2);

	//normal.z = -normal.z;

	vec3 temp = normal;
	normal.x = temp.x;
	normal.y = -temp.y;
	normal.z = -temp.z;

	return normalize(normal);
}


vec3 normalFromDepth3(vec3 viewPos) {
    vec3 normal = normalize(cross(dFdx(viewPos), dFdy(viewPos)));
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
	return normalize(normal);
}



// get world position from depth buffer
vec3 worldPosFromDepth(vec2 texCoord, float depth) {

    vec4 clipSpacePosition;
    clipSpacePosition.xy = texCoord * 2.0 - 1.0;
	clipSpacePosition.z = depth;
	clipSpacePosition.w = 1.0;

	mat4 invViewProj = inverse(ubo.projection * ubo.view);


    vec4 worldSpacePosition = invViewProj * clipSpacePosition;

    // Perspective division
    worldSpacePosition /= worldSpacePosition.w;

    return worldSpacePosition.xyz;
}

vec3 viewPosFromDepth(vec2 texCoord, float depth) {

    vec4 clipSpacePosition;
    clipSpacePosition.xy = texCoord * 2.0 - 1.0;
    clipSpacePosition.z = depth;
    clipSpacePosition.w = 1.0;

    mat4 invProj = inverse(ubo.projection);
    
    vec4 viewSpacePosition = invProj * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}




void main() {

	// Calculate out of the current fragment in screen space the view space position.
	//vec4 posView = getViewPos(inUV);

	// Get G-Buffer values

	//vec4 samplerPosDepth = texture(samplerPositionDepth, inUV).rgba;

	float depth = texture(samplerDepth, inUV.st).r;

	vec3 worldPos = worldPosFromDepth(inUV.st, depth);
	vec3 viewPos = vec3(ubo.view * vec4(worldPos, 1.0));// calculate view space position

	//vec3 worldPos = samplerPos.rgb;




	//normal = vec3(normal.x, -normal.z, normal.y);
	//normal = vec3(0.0, 0.0, 0.0);

	//float originalOriginalDepth = texture(samplerPositionDepth, inUV).a;
	//float originalDepth = 1 - texture(samplerPositionDepth, inUV).a/512.0;

	//vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);// view space normal
	//vec3 normal = normalFromDepth2(samplerPositionDepth, inUV);// construct from depth
	//vec3 normal = normalFromDepth(samplerPositionDepth, inUV);// construct from depth

	vec3 normal = normalFromDepth3(viewPos);// construct from depth?

	// testing normal reconstruction:
	// vec2 depth_size = vec2(1280, 720);

	// vec4 pos = vec4((inUV.st - depth_size.xy * 0.5) / (depth_size* 0.5), w, 1.0) * w * inverse(ubo.projection);
	// vec3 n = normalize(cross(dFdx(pos.xyz), dFdy(pos.xyz))) * 0.5 + 0.5;
	// normal = n;



	// Get a random vector using a noise lookup
	//ivec2 texDim = textureSize(samplerPositionDepth, 0);
	ivec2 texDim = textureSize(samplerDepth, 0);
	ivec2 noiseDim = textureSize(ssaoNoise, 0);


	const vec2 noiseScale = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y));

	vec3 randomVec = normalize(texture(ssaoNoise, inUV * noiseScale).xyz * 2.0 - 1.0);

	//randomVec = vec3(0.0, 0.0, 1.0);
	
	// Create TBN matrix


	
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	//mat3 TBN = computeTBNMatrixFromDepth(samplerPositionDepth, inUV);


	// Calculate occlusion value
	float occlusion = 0.0f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++) {

		// reorient sample vector in view space
		vec3 sampleDir = TBN * uboSSAOKernel.samples[i].xyz;
		//vec3 sampleDir = /*TBN **/ uboSSAOKernel.samples[i].xyz;
		//sampleDir = vec3(0.0, 0.0, 1.0);

		// calculate sample point.
		vec3 samplePos = viewPos + (sampleDir * SSAO_RADIUS);

		
		// project

		vec4 samplePointNDC = ubo.projection * vec4(samplePos.xyz, 1.0);// project on the near clipping plane
		samplePointNDC /= samplePointNDC.w;// perform perspective divide

		// Create texture coordinate out of it.
		vec2 samplePointTexCoord = samplePointNDC.xy * 0.5 + vec2(0.5);

		//float sampleDepth = -texture(samplerPositionDepth, offset.xy).w;
		//float sampleDepth = -texture(samplerPositionDepth, samplePointTexCoord.xy).a;
		float sampleDepth = -texture(samplerDepth, samplePointTexCoord.xy).r;

		// float delta = samplePointNDC.z - zSceneNDC;
		// // If scene fragment is before (smaller in z) sample point, increase occlusion.
		// if (delta > CAP_MIN_DISTANCE && delta < CAP_MAX_DISTANCE) {
		// 	occlusion += 1.0;
		// }


		// // range check & accumulate:
		// float rangeCheck = abs(fragPos.z - sampleDepth) < SSAO_RADIUS ? 1.0 : 0.0;
		// //float rangeCheck = abs(posView.z - sampleDepth) < SSAO_RADIUS ? 1.0 : 0.0;
		// rangeCheck = 1.0;
		// occlusion += (sampleDepth >= samplePos.z ? 1.0 : 0.0) * rangeCheck;

		#define RANGE_CHECK 1
		#ifdef RANGE_CHECK
			// Range check
			float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(viewPos.z - sampleDepth));
			occlusion += (sampleDepth >= samplePos.z ? 1.0f : 0.0f) * rangeCheck;
		#else
			occlusion += (sampleDepth >= samplePos.z ? 1.0f : 0.0f);
		#endif
	}
	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
	occlusion = pow(occlusion, SSAO_POWER);

	//occlusion = 1 - occlusion;
	outFragColor = occlusion;
	//outFragColor = originalDepth;
	//outFragColor = gl_FragCoord.x/1280;
	//outFragColor = randomVec.x;
	//outFragColor = normal.x;
	//outFragColor = fragPos.x;

	//outFragColor = worldPosFromDepth(inUV, originalOriginalDepth).b;

	//outFragColor = calculate_world_position(inUV, originalOriginalDepth).b;

}

