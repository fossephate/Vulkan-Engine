#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerPosition;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform sampler2D samplerAlbedo;


struct Light {
    vec4 position;
    vec4 color;
    float radius;
    float quadraticFalloff;
    float linearFalloff;
    float _pad;
};

#define NUM_LIGHTS 100

// todo: make this another set(1) rather than binding = 4
layout (set = 3, binding = 4) uniform UBO 
{
    Light lights[NUM_LIGHTS];
    vec4 viewPos;
    mat4 model;// added
    mat4 view;// added
} ubo;

layout (location = 0) in vec2 inUV;

layout (location = 0) out vec4 outFragcolor;






void main() 
{
    // Get G-Buffer values
    vec3 fragPos = texture(samplerPosition, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb;
    vec4 albedo = texture(samplerAlbedo, inUV);
    
	#define lightCount 100
	#define ambient 0.05
	#define specularStrength 0.15
	
	// Ambient part
    vec3 fragcolor  = albedo.rgb * ambient;
	
    vec3 viewVec = normalize(ubo.viewPos.xyz - fragPos);
	
    for(int i = 0; i < lightCount; ++i)
    {
        // Distance from light to fragment position
        float dist = length(ubo.lights[i].position.xyz - fragPos);
		
        if(dist < ubo.lights[i].radius)
        {
			// Get vector from current light source to fragment position
            vec3 lightVec = normalize(ubo.lights[i].position.xyz - fragPos);
            // Diffuse part
            vec3 diffuse = max(dot(normal, lightVec), 0.0) * albedo.rgb * ubo.lights[i].color.rgb;
            // Specular part (specular texture part stored in albedo alpha channel)
            vec3 halfVec = normalize(lightVec + viewVec);  
            vec3 specular = ubo.lights[i].color.rgb * pow(max(dot(normal, halfVec), 0.0), 16.0) * albedo.a * specularStrength;
            // Attenuation with linearFalloff and quadraticFalloff falloff
            float attenuation = 1.0 / (1.0 + ubo.lights[i].linearFalloff * dist + ubo.lights[i].quadraticFalloff * dist * dist);
            fragcolor += (diffuse + specular) * attenuation;
        }
		
    }    	
  
  float alpha = 1.0;

  if(fragcolor == vec3(0, 0, 0)) {
    alpha = 0.0;
  }

  outFragcolor = vec4(fragcolor, alpha);

}