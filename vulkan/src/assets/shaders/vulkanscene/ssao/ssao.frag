#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 0, binding = 0) uniform sampler2D samplerPositionDepth;
layout (set = 0, binding = 1) uniform sampler2D samplerNormal;
layout (set = 0, binding = 2) uniform sampler2D ssaoNoise;

/*layout (constant_id = 0) */const int SSAO_KERNEL_SIZE = 32;// changed from 32
/*layout (constant_id = 1) */const float SSAO_RADIUS = 2.0;
/*layout (constant_id = 2) */const float SSAO_POWER = 1.5;


layout (set = 0, binding = 3) uniform UBOSSAOKernel
{
	vec4 samples[SSAO_KERNEL_SIZE];
} uboSSAOKernel;

layout (set = 0, binding = 4) uniform UBO 
{
	mat4 projection;
	//mat4 g1;
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out float outFragColor;


float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}


void main() {






	//vec4 samples2[SSAO_KERNEL_SIZE];







	// Get G-Buffer values
	vec3 fragPos = texture(samplerPositionDepth, inUV).rgb;
	vec3 normal = normalize(texture(samplerNormal, inUV).rgb * 2.0 - 1.0);



	// Get a random vector using a noise lookup
	ivec2 texDim = textureSize(samplerPositionDepth, 0);
	ivec2 noiseDim = textureSize(ssaoNoise, 0);

	//const vec2 noiseUV = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y)) * inUV;
	//vec3 randomVec = texture(ssaoNoise, noiseUV).xyz * 2.0 - 1.0;


	const vec2 noiseScale = vec2(float(texDim.x)/float(noiseDim.x), float(texDim.y)/(noiseDim.y));
	//const vec2 noiseScale = vec2(1280.0/4.0, 720.0/4.0);
	vec3 randomVec = texture(ssaoNoise, inUV * noiseScale).xyz * 2.0 - 1.0;

	float a = sin(rand(vec2(inUV.s, inUV.t)));
	float b = sin(rand(vec2(inUV.s+1, inUV.t+99)));
	float c = sin(rand(vec2(inUV.s+25, inUV.t+12))) * 0.5 + 0.5;

	randomVec = normalize(vec3(a,b,c));



	//randomVec = vec3(0.0, 0.0, 0.0);
	
	// Create TBN matrix
	vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
	vec3 bitangent = cross(tangent, normal);
	//vec3 bitangent = cross(normal, tangent);
	mat3 TBN = mat3(tangent, bitangent, normal);

	// Calculate occlusion value
	float occlusion = 0.0f;
	for(int i = 0; i < SSAO_KERNEL_SIZE; i++) {

		vec3 samplePos = TBN * uboSSAOKernel.samples[i].xyz; 
		samplePos = fragPos + samplePos * SSAO_RADIUS;
		//samplePos = samplePos * SSAO_RADIUS + fragPos;

		
		// project
		vec4 offset = vec4(samplePos, 1.0f);
		offset = ubo.projection * offset;// project on the near clipping plane
		// offset.xyz /= offset.w; 
		// offset.xyz = offset.xyz * 0.5f + 0.5f;
		offset.xy /= offset.w;// perform perspective divide
		//offset.xy = offset.xy * 0.5f + 0.5f;
		offset.xy = offset.xy * 0.5 + vec2(0.5); // transform to (0,1) range

		
		float sampleDepth = -texture(samplerPositionDepth, offset.xy).w;
		//float sampleDepth = -texture(samplerPositionDepth, offset.xy).r;
		//float sampleDepth = texture(samplerPositionDepth, offset.xy).b;

		// range check & accumulate:
		float rangeCheck = abs(fragPos.z - sampleDepth) < SSAO_RADIUS ? 1.0 : 0.0;
		occlusion += (sampleDepth <= samplePos.z ? 1.0 : 0.0) * rangeCheck;



// #define RANGE_CHECK 0
// #ifdef RANGE_CHECK
// 		// Range check
// 		float rangeCheck = smoothstep(0.0f, 1.0f, SSAO_RADIUS / abs(fragPos.z - sampleDepth));
// 		occlusion += (sampleDepth >= samplePos.z ? 1.0f : 0.0f) * rangeCheck;           
// #else
// 		occlusion += (sampleDepth >= samplePos.z ? 1.0f : 0.0f); 
// #endif
	}
	occlusion = 1.0 - (occlusion / float(SSAO_KERNEL_SIZE));
	occlusion = pow(occlusion, SSAO_POWER);

	outFragColor = occlusion;

	//outFragColor = randomVec.x;

	//outFragColor = normal.x;

	//outFragColor = fragPos.x;

	//outFragColor = uboSSAOKernel.samples[3].x;

	//vec3 t = 

	//outFragColor = rand(vec2(inUV));

	

	//outFragColor = 0.2;
	//outFragColor = texture(samplerPositionDepth, inUV).a;
}

