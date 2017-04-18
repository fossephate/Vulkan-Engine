#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable



#define LIGHT_COUNT 3

layout (triangles, invocations = LIGHT_COUNT) in;
layout (triangle_strip, max_vertices = 3) out;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 mvp[LIGHT_COUNT];
	vec4 pos;
} ubo;

layout (location = 0) in int inInstanceIndex[];

//layout (location = 1) in mat4 inTestMVP[];
//layout (location = 20) in vec4 inPos[];


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


	const mat4 testMVP = mat4( 
		0.0, 0.708880484, 0.535902381, 0.535064995,
		-0.839099526, 0.0, 0.0, 0.0,
		0.0, 0.448972762, -0.846133053, -0.844810963,
		-6.12674285e-07, -0.414078623, 25.8597565, 25.9193497 );




	vec4 instancedPos = vec4(0.0);
	//vec4 instancedPos = ubo.pos;

	for (int i = 0; i < gl_in.length(); i++) {
		gl_Layer = gl_InvocationID;
		vec4 tmpPos = gl_in[i].gl_Position;
		//vec4 tmpPos = ubo.mvp[gl_InvocationID] * gl_in[i].gl_Position;
		//gl_Position = ubo.mvp[gl_InvocationID] * tmpPos;
		gl_Position = tmpPos;
		//gl_Position = testMVP * tmpPos;
		EmitVertex();
	}
	EndPrimitive();

}
