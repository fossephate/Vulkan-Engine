#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerPosition;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;// this is a usampler(on ssao)
layout (set = 3, binding = 5) uniform sampler2D samplerSSAO;


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



/*layout (constant_id = 0) */const int SSAO_ENABLED = 1;
/*layout (constant_id = 1) */const float AMBIENT_FACTOR = 0.005;



// void main() 
// {
//     // Get G-Buffer values
//     vec3 fragPos = texture(samplerPosition, inUV).rgb;
//     vec3 normal = texture(samplerNormal, inUV).rgb;
//     vec4 albedo = texture(samplerAlbedo, inUV);
    
//     #define lightCount 100
//     #define ambient 0.05
//     #define specularStrength 0.15
    
//     // Ambient part
//     vec3 fragcolor  = albedo.rgb * ambient;
    
//     vec3 viewVec = normalize(ubo.viewPos.xyz - fragPos);
    
//     for(int i = 0; i < lightCount; ++i)
//     {
//         // Distance from light to fragment position
//         float dist = length(ubo.lights[i].position.xyz - fragPos);
        
//         if(dist < ubo.lights[i].radius)
//         {
//             // Get vector from current light source to fragment position
//             vec3 lightVec = normalize(ubo.lights[i].position.xyz - fragPos);
//             // Diffuse part
//             vec3 diffuse = max(dot(normal, lightVec), 0.0) * albedo.rgb * ubo.lights[i].color.rgb;
//             // Specular part (specular texture part stored in albedo alpha channel)
//             vec3 halfVec = normalize(lightVec + viewVec);  
//             vec3 specular = ubo.lights[i].color.rgb * pow(max(dot(normal, halfVec), 0.0), 16.0) * albedo.a * specularStrength;
//             // Attenuation with linearFalloff and quadraticFalloff falloff
//             float attenuation = 1.0 / (1.0 + ubo.lights[i].linearFalloff * dist + ubo.lights[i].quadraticFalloff * dist * dist);
//             fragcolor += (diffuse + specular) * attenuation;
//         }
        
//     }       
  
//   float alpha = 1.0;

//   if(fragcolor == vec3(0, 0, 0)) {
//     alpha = 0.0;
//   }

//   outFragcolor = vec4(fragcolor, alpha);
//   //outFragcolor = vec4(1.0, 0.0, 0.0, 1.0);

// }


void main() 
{
    // Get G-Buffer values
    vec3 fragPos = texture(samplerPosition, inUV).rgb;
    vec3 normal = texture(samplerNormal, inUV).rgb * 2.0 - 1.0;

    // unpack
    ivec2 texDim = textureSize(samplerAlbedo, 0);
    //uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);
    uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim ), 0);

    vec4 color;
    color.rg = unpackHalf2x16(albedo.r);
    color.ba = unpackHalf2x16(albedo.g);
    vec4 spec;
    spec.rg = unpackHalf2x16(albedo.b); 

    //color = albedo;

    vec3 ambient = color.rgb * AMBIENT_FACTOR;
    //ambient.r = 1.0;

    vec3 fragcolor  = ambient;


    
    if (length(fragPos) == 0.0) {
        fragcolor = color.rgb;
    } else {   
        for(int i = 0; i < NUM_LIGHTS; ++i) {
            // Light to fragment
            vec3 lightPos = vec3(ubo.view * ubo.model * vec4(ubo.lights[i].position.xyz, 1.0));
            vec3 L = lightPos - fragPos;
            float dist = length(L);
            L = normalize(L);

            // Viewer to fragment
            vec3 viewPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));
            vec3 V = viewPos - fragPos;
            V = normalize(V);

            // Attenuation
            float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

            // Diffuse part
            vec3 N = normalize(normal);
            float NdotL = max(0.0, dot(N, L));
            vec3 diff = ubo.lights[i].color.rgb * color.rgb * NdotL * atten;

            // Specular part
            vec3 R = reflect(-L, N);
            float NdotR = max(0.0, dot(R, V));
            vec3 spec = ubo.lights[i].color.rgb * spec.r * pow(NdotR, 16.0) * (atten * 1.5);

            fragcolor += diff + spec;               
        }       

        if (SSAO_ENABLED == 1) {
            float ao = texture(samplerSSAO, inUV).r;
            fragcolor *= ao.rrr;
        }
    }
   
    outFragcolor = vec4(fragcolor, 1.0);    
}

















// void main() 
// {
//     // Get G-Buffer values
//     vec3 fragPos = texture(samplerPosition, inUV).rgb;
//     vec3 normal = texture(samplerNormal, inUV).rgb * 2.0 - 1.0;

//     // unpack
//     //ivec2 texDim = textureSize(samplerAlbedo, 0);
//     //uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);
//     //uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim ), 0);

//     //vec4 color;
//     //color.rg = unpackHalf2x16(albedo.r);
//     //color.ba = unpackHalf2x16(albedo.g);

//     vec4 color = texture(samplerAlbedo, inUV.st);


//     // vec4 spec;
//     // spec.rg = unpackHalf2x16(albedo.b); 

//     float specular = color.a;

//     vec4 spec;
//     //spec.rg = vec2(specular, 0.0);
//     spec = vec4(specular);

//     //color = albedo;

//     vec3 ambient = color.rgb * AMBIENT_FACTOR;
//     //ambient.r = 1.0;

//     vec3 fragcolor  = ambient;


    
//     if (length(fragPos) == 0.0) {
//         fragcolor = color.rgb;
//     } else {   
//         for(int i = 0; i < NUM_LIGHTS; ++i) {
//             // Light to fragment
//             vec3 lightPos = vec3(ubo.view * ubo.model * vec4(ubo.lights[i].position.xyz, 1.0));
//             vec3 L = lightPos - fragPos;
//             float dist = length(L);
//             L = normalize(L);

//             // Viewer to fragment
//             vec3 viewPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));
//             vec3 V = viewPos - fragPos;
//             V = normalize(V);

//             // Attenuation
//             float atten = ubo.lights[i].radius / (pow(dist, 2.0) + 1.0);

//             // Diffuse part
//             vec3 N = normalize(normal);
//             float NdotL = max(0.0, dot(N, L));
//             vec3 diff = ubo.lights[i].color.rgb * color.rgb * NdotL * atten;

//             // Specular part
//             vec3 R = reflect(-L, N);
//             float NdotR = max(0.0, dot(R, V));
//             vec3 spec = ubo.lights[i].color.rgb * spec.r * pow(NdotR, 16.0) * (atten * 1.5);

//             fragcolor += diff + spec;               
//         }       

//         if (SSAO_ENABLED == 1) {
//             float ao = texture(samplerSSAO, inUV).r;
//             fragcolor *= ao.rrr;
//         }
//     }
   
//     outFragcolor = vec4(fragcolor, 1.0);    
// }