#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform sampler2D samplerPositionDepth;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform sampler2D ssaoNoise;

/*layout (constant_id = 0) */const int SSAO_KERNEL_SIZE = 32;// changed from 32
/*layout (constant_id = 1) */const float SSAO_RADIUS = 2.0;//2.0
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


// // reconstruct world position from depth buffer:
// // this is slow, find a better solution
// vec3 calculate_world_position(vec2 texture_coordinate, float depth_from_depth_buffer) {
//     vec4 clip_space_position = vec4(texture_coordinate * 2.0 - vec2(1.0), 2.0 * depth_from_depth_buffer - 1.0, 1.0);

//     //vec4 position = inverse_projection_matrix * clip_space_position; // Use this for view space
//     //vec4 position = inverse_view_projection_matrix * clip_space_position; // Use this for world space
//     // definitely don't do this:
//     //vec4 position = ubo.inverseViewProjection * clip_space_position; // Use this for world space
//     vec4 position = inverse(ubo.projection*ubo.view) * clip_space_position; // Use this for world space

//     return(position.xyz / position.w);
// }

// // this is supposed to get the world position from the depth buffer
// vec3 worldPosFromDepth(vec2 texCoord, float depth) {
//     float z = depth * 2.0 - 1.0;

//     vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
//     //vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;
//     vec4 viewSpacePosition = inverse(ubo.projection) * clipSpacePosition;

//     // Perspective division
//     viewSpacePosition /= viewSpacePosition.w;

//     //vec4 worldSpacePosition = viewMatrixInv * viewSpacePosition;
//     vec4 worldSpacePosition = inverse(ubo.view) * viewSpacePosition;

//     return worldSpacePosition.xyz;
// }







void main() {

	// Calculate out of the current fragment in screen space the view space position.
	//vec4 posView = getViewPos(inUV);

	// Get G-Buffer values

	vec3 samplerPos = texture(samplerPositionDepth, inUV).rgb;

	

	
	vec3 viewPos = vec3(ubo.view * vec4(samplerPos.rgb, 1.0));// calculate view space position
	//vec3 worldPos = samplerPos.rgb;

	vec3 fragPos = viewPos;// view space position
	//vec3 fragPos = samplerPos.rgb;



	vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);// view space normal

	//normal = vec3(normal.x, -normal.z, normal.y);
	//normal = vec3(0.0, 0.0, 0.0);

	float originalOriginalDepth = texture(samplerPositionDepth, inUV).a;
	float originalDepth = 1 - texture(samplerPositionDepth, inUV).a/512.0;


	// Get a random vector using a noise lookup
	ivec2 texDim = textureSize(samplerPositionDepth, 0);
	ivec2 noiseDim = textureSize(ssaoNoise, 0);


	const vec2 noiseScale = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y));

	vec3 randomVec = normalize(texture(ssaoNoise, inUV * noiseScale).xyz * 2.0 - 1.0);

	//randomVec = vec3(0.0, 0.0, 1.0);
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(normal, tangent);

	mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++) {

		// reorient sample vector in view space
		vec3 sampleDir = TBN * uboSSAOKernel.samples[i].xyz;
		//vec3 sampleDir = /*TBN **/ uboSSAOKernel.samples[i].xyz;
		//sampleDir = vec3(0.0, 0.0, 1.0);

		// calculate sample point.
		vec3 samplePos = fragPos + (sampleDir * SSAO_RADIUS);

		
		// project
		//vec4 samplePointNDC = ubo.projection * samplePos;// project on the near clipping plane
		vec4 samplePointNDC = ubo.projection * vec4(samplePos.xyz, 1.0);
		samplePointNDC /= samplePointNDC.w;// perform perspective divide

		// Create texture coordinate out of it.
		vec2 samplePointTexCoord = samplePointNDC.xy * 0.5 + vec2(0.5);

		//float sampleDepth = -texture(samplerPositionDepth, offset.xy).w;
		float sampleDepth = -texture(samplerPositionDepth, samplePointTexCoord.xy).a;

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
			float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
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

