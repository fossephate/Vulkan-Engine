#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


// material data
layout (set = 2, binding = 0) uniform materialBuffer
{
	vec4 ambient;
	vec4 diffuse;
	vec4 specular;
	float opacity;
} material;


// diffuse texture (from material)
layout (set = 3, binding = 0) uniform sampler2D samplerColorMap;


layout (location = 0) in vec3 inNormal;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inColor;
layout (location = 3) in vec3 inViewVec;
layout (location = 4) in vec3 inLightVec;

layout (location = 5) in vec3 inPos;
layout (location = 6) in vec3 inCamPos;
layout (location = 7) in vec3 inLightPos;



layout (location = 0) out vec4 outFragColor;



void main() 
{
    // vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);

    // vec3 N = normalize(inNormal);
    // vec3 L = normalize(inLightVec);
    // vec3 V = normalize(inViewVec);
    // vec3 R = reflect(-L, N);
    // vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);// * inColor;
    // vec3 specular = pow(max(dot(R, V), 0.0), 32.0) * vec3(0.5);
    // outFragColor = vec4(diffuse * color.rgb + specular, 1.0);




    vec3 color = texture(samplerColorMap, inUV).rgb * inColor;
    // Ambient
    //vec3 ambient = 0.08 * color;
    vec3 ambient = 0.18 * color;

    // Diffuse
    vec3 lightDir = normalize(inLightPos - inPos);
    vec3 normal = normalize(inNormal);
    float diff = max(dot(lightDir, normal), 0.0);
    vec3 diffuse = diff * color;
    

    //vec3 viewPos = vec3(scene.view[0].w, scene.view[1].w, scene.view[2].w);
    //vec3 viewPos = vec3(0.0, 1.0, 2.0);
    // Specular
    //vec3 viewDir = normalize(viewPos - inPos);
    vec3 viewDir = normalize(inCamPos - inPos);

    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = 0.0;

    vec3 halfwayDir = normalize(lightDir + viewDir);  
    spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);


    vec3 specular = vec3(0.3) * spec; // assuming bright white light color


    outFragColor = vec4(ambient + diffuse + specular, 1.0);






}