#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#define LIGHT_COUNT 3

layout (location = 0) in vec4 inPos;

layout (location = 0) out int outInstanceIndex;

// layout (location = 1) out float test;

//layout (location = 1) out mat4 outTestMVP[3];

//layout (location = 20) out vec4 outPos;




// layout (set = 0, binding = 0) uniform UBO 
// {
// 	mat4 mvp[LIGHT_COUNT];
// 	vec4 pos;
// } ubo;

out gl_PerVertex
{
	vec4 gl_Position;
};


void main() {

	outInstanceIndex = gl_InstanceIndex;

	// outTestMVP[0] = ubo.mvp[0];
	// outTestMVP[1] = ubo.mvp[1];
	// outTestMVP[2] = ubo.mvp[2];

	//test = ubo.instancePos[0].x;

	//outPos = ubo.instancePos[0];

	//gl_Position = depthMVP * inPos;
	gl_Position = inPos;
}
