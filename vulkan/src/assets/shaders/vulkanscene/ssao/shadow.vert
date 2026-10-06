#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable
// Lets the vertex shader write gl_Layer (VK_EXT_shader_viewport_index_layer).
// This replaces the old geometry-shader layer replication, which cannot run on
// MoltenVK/Metal (no geometry shaders). The scene is drawn with
// instanceCount = NUM_LIGHTS_TOTAL and each instance targets one shadow layer.
#extension GL_ARB_shader_viewport_layer_array : require

#define NUM_SPOT_LIGHTS 2
#define NUM_DIR_LIGHTS 3

layout (location = 0) in vec4 inPos;

layout (set = 0, binding = 0) uniform UBO
{
	mat4 spotlightMVP[NUM_SPOT_LIGHTS];
	mat4 dirlightMVP[NUM_DIR_LIGHTS];
} ubo;

// matrix data (same dynamic uniform block the geometry shader used)
layout (set = 1, binding = 0) uniform matrixBuffer
{
	mat4 model;
	int boneIndex;
	vec3 padding;
	vec4 padding2[3];
	mat4 g1;
	mat4 g2;
} instance;

out gl_PerVertex
{
	vec4 gl_Position;
};

void main() {
	int layer = gl_InstanceIndex;
	gl_Layer = layer;

	vec4 pos = vec4(inPos.xyz, 1.0);

	if (layer < NUM_SPOT_LIGHTS) {
		gl_Position = ubo.spotlightMVP[layer] * instance.model * pos;
	} else {
		gl_Position = ubo.dirlightMVP[layer - NUM_SPOT_LIGHTS] * instance.model * pos;
	}
}
