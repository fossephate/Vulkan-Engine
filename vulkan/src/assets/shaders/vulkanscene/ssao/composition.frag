#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerPosition;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;// this is a usampler(on ssao)
layout (set = 3, binding = 4) uniform sampler2D samplerSSAO;


struct PointLight {
    vec4 position;
    vec4 color;
    float radius;
    float quadraticFalloff;
    float linearFalloff;
    float _pad;
};

struct DirectionalLight {
    vec4 position;
    vec4 color;
    vec4 direction;
    vec4 target;
    //float radius;
};

struct SpotLight {
    vec4 position;
    vec4 color;
    float attenuation;
    float ambientCoefficient;
    float coneAngle;    // new
    vec4 coneDirection; // new
};



#define NUM_LIGHTS 100
#define NUM_DIR_LIGHTS 10

// todo: make this another set(1) rather than binding = 4
layout (set = 3, binding = 5) uniform UBO 
{
    PointLight lights[NUM_LIGHTS];
    DirectionalLight directionalLights[NUM_DIR_LIGHTS];
    vec4 viewPos;
    mat4 model;// added
    mat4 view;// added
} ubo;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;



/*layout (constant_id = 0) */const int SSAO_ENABLED = 1;
/*layout (constant_id = 1) */const float AMBIENT_FACTOR = 0.2;


void main() {
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

    vec3 ambient = color.rgb * AMBIENT_FACTOR; 
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
            //float atten = 1.0 / (1.0 + ubo.lights[i].linearFalloff * dist + ubo.lights[i].quadraticFalloff * dist * dist);


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





        // for(int i = 0; i < NUM_DIR_LIGHTS; ++i) {
        //     // Vector to light
        //     vec3 L = ubo.directionalLights[i].position.xyz - fragPos;
        //     // Distance from light to fragment position
        //     float dist = length(L);
        //     L = normalize(L);

        //     // Viewer to fragment
        //     vec3 viewPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));
        //     vec3 V = /*ubo.*/viewPos.xyz - fragPos;
        //     V = normalize(V);

        //     float lightCosInnerAngle = cos(radians(15.0));
        //     float lightCosOuterAngle = cos(radians(25.0));
        //     float lightRange = 100.0;

        //     // Direction vector from source to target
        //     vec3 dir = normalize(ubo.directionalLights[i].position.xyz - ubo.directionalLights[i].target.xyz);

        //     // Dual cone spot light with smooth transition between inner and outer angle
        //     float cosDir = dot(L, dir);
        //     float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
        //     float heightAttenuation = smoothstep(lightRange, 0.0f, dist);

        //     // Diffuse lighting
        //     vec3 N = normalize(normal);
        //     float NdotL = max(0.0, dot(N, L));
        //     vec3 diff = vec3(NdotL);

        //     // Specular lighting
        //     vec3 R = reflect(-L, N);
        //     float NdotR = max(0.0, dot(R, V));
        //     vec3 spec = vec3(pow(NdotR, 16.0) * albedo.a * 2.5);

        //     fragcolor += vec3((diff + spec) * spotEffect * heightAttenuation) * ubo.directionalLights[i].color.rgb * albedo.rgb;
        // }













        if (SSAO_ENABLED == 1) {
            float ao = texture(samplerSSAO, inUV).r;
            fragcolor *= ao.rrr;
        }
    }
   
    outFragcolor = vec4(fragcolor, 1.0);    
}