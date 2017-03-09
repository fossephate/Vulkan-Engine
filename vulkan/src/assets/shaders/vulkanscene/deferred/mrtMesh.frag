#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inWorldPos;

layout (location = 0) out vec4 outPosition;
layout (location = 1) out vec4 outNormal;
layout (location = 2) out vec4 outAlbedo;



//layout (set = 0, binding = 1) uniform sampler2D samplerColorMap;
//layout (set = 0, binding = 2) uniform sampler2D samplerNormalMap;


// diffuse texture (from material)
layout (set = 2, binding = 0) uniform sampler2D samplerColor;
layout (set = 2, binding = 1) uniform sampler2D samplerSpecular;
layout (set = 2, binding = 2) uniform sampler2D samplerNormal;


void main() 
{
	outPosition = vec4(inWorldPos, 1.0);
	outNormal = vec4(inNormal, 1.0);
	outAlbedo = texture(samplerColor, inUV);
}