#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable



#define NUM_SPOT_LIGHTS 3
#define NUM_DIR_LIGHTS 1

layout (triangles, invocations = NUM_SPOT_LIGHTS) in;
layout (triangle_strip, max_vertices = 3) out;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 mvp[NUM_SPOT_LIGHTS];
	
} ubo;

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

layout (location = 0) in int inInstanceIndex[];


out gl_PerVertex
{
	vec4 gl_Position;
};

void main() {

	// vec4 instancedPos = ubo.instancePos[inInstanceIndex[0]];

	// for (int i = 0; i < gl_in.length(); i++) {

	// 	gl_Layer = gl_InvocationID;
	// 	vec4 tmpPos = gl_in[i].gl_Position + instancedPos;
	// 	gl_Position = ubo.mvp[gl_InvocationID] * tmpPos;
	// 	EmitVertex();

	// }
	// EndPrimitive();

	//mat4 placeholder = mat4(1.0);

	//vec4 instancedPos = ubo.instancePos[inInstanceIndex[0]];
	//vec4 instancedPos = inPos[0];

	// for (int i = 0; i < gl_in.length(); i++) {

	// 	gl_Layer = gl_InvocationID;
	// 	vec4 tmpPos = gl_in[i].gl_Position;
	// 	//gl_Position = inTestMVP[gl_InvocationID] * tmpPos;
	// 	gl_Position = placeholder * tmpPos;
	// 	EmitVertex();

	// }
	// EndPrimitive();


	// const mat4 testMVP = mat4( 
	// 	0.5, 0.0, 0.0, 0.0,
	// 	0.0, 0.5, 0.0, 0.0,
	// 	0.0, 0.0, 1.0, 0.0,
	// 	0.5, 0.5, 0.0, 1.0 );



	//vec4 instancedPos = vec4(0.0);
	//vec4 instancedPos = ubo.pos;
	//vec4 instancedPos = ubo.pos[inInstanceIndex[0]];
	//mat4 test1 = mat4(1.0);
	//mat4 test2 = ubo.mvp[0];


	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;
		//vec4 tmpPos = /*ubo.mvp[gl_InvocationID] **/ (gl_in[i].gl_Position + instancedPos);
		//vec4 tmpPos = ubo.mvp[gl_InvocationID] * gl_in[i].gl_Position * instance.model;
		vec4 tmpPos = ubo.mvp[gl_InvocationID] * instance.model * gl_in[i].gl_Position;
		gl_Position = tmpPos;
		EmitVertex();
	}
	EndPrimitive();

}
