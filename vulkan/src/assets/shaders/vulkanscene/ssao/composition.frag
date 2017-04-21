#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (set = 3, binding = 1) uniform sampler2D samplerPosition;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;// this is a usampler(on ssao)
layout (set = 3, binding = 4) uniform sampler2D samplerSSAO;

//layout (set = 3, binding = 6) uniform sampler2DShadow samplerShadowMap;
layout (set = 3, binding = 6) uniform sampler2DArray samplerShadowMap;
//layout (set = 3, binding = 6) uniform sampler2D samplerShadowMap;

struct PointLight {
    vec4 position;
    vec4 color;

    float radius;
    float quadraticFalloff;
    float linearFalloff;
    float _pad;
};

// struct DirectionalLight {
//     vec4 position;// definitely remove
//     vec4 target;//remove?
//     vec4 color;
//     vec4 direction;
//     //float radius;
// };


struct SpotLight2 {
    vec4 position;
    vec4 target;
    vec4 color;
    mat4 viewMatrix;
};



#define NUM_POINT_LIGHTS 100
#define NUM_DIR_LIGHTS 1
#define NUM_SPOT_LIGHTS 1

#define SHADOW_FACTOR 0.25
#define AMBIENT_LIGHT 0.4
#define USE_PCF

const int SSAO_ENABLED = 1;
//const float AMBIENT_FACTOR = 0.1;

const int USE_SHADOWS = 1;



// todo: make this another set(1) rather than binding = 4
layout (set = 3, binding = 5) uniform UBO 
{
    PointLight pointlights[NUM_POINT_LIGHTS];
    SpotLight2 spotlights[3];
    //DirectionalLight directionalLights[NUM_DIR_LIGHTS];
    vec4 viewPos;
    mat4 model;// added
    mat4 view;// added
    //mat4 inverseViewProjection;// added 4/19/17
    mat4 projection;
} ubo;

layout (location = 0) in vec2 inUV;
layout (location = 0) out vec4 outFragcolor;





// reconstruct world position from depth buffer:
// this is slow, find a better solution
vec3 calculate_world_position(vec2 texture_coordinate, float depth_from_depth_buffer) {
    vec4 clip_space_position = vec4(texture_coordinate * 2.0 - vec2(1.0), 2.0 * depth_from_depth_buffer - 1.0, 1.0);

    //vec4 position = inverse_projection_matrix * clip_space_position; // Use this for view space
    //vec4 position = inverse_view_projection_matrix * clip_space_position; // Use this for world space
    // definitely don't do this:
    //vec4 position = ubo.inverseViewProjection * clip_space_position; // Use this for world space
    vec4 position = inverse(ubo.projection*ubo.view) * clip_space_position; // Use this for world space

    return(position.xyz / position.w);
}

// this is supposed to get the world position from the depth buffer
vec3 worldPosFromDepth(vec2 texCoord, float depth) {
    float z = depth * 2.0 - 1.0;

    vec4 clipSpacePosition = vec4(texCoord * 2.0 - 1.0, z, 1.0);
    //vec4 viewSpacePosition = projMatrixInv * clipSpacePosition;
    vec4 viewSpacePosition = inverse(ubo.projection) * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;

    //vec4 worldSpacePosition = viewMatrixInv * viewSpacePosition;
    vec4 worldSpacePosition = inverse(ubo.view) * viewSpacePosition;

    return worldSpacePosition.xyz;
}


// view space?
vec3 normal_from_depth(vec2 texCoord, float depth) {
  
  vec2 offset1 = vec2(0.0,0.001);
  vec2 offset2 = vec2(0.001,0.0);
  
  float depth1 = texture(samplerPosition, texCoord + offset1).a;
  float depth2 = texture(samplerPosition, texCoord + offset2).a;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}







float textureProj(vec4 P, float layer, vec2 offset) {
    float shadow = 1.0;
    vec4 shadowCoord = P / P.w;
    shadowCoord.st = shadowCoord.st * 0.5 + 0.5;
    
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)  {
        float dist = texture(samplerShadowMap, vec3(shadowCoord.st + offset, layer)).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z)  {
            shadow = SHADOW_FACTOR;
        }
    }
    return shadow;
}

float filterPCF(vec4 sc, float layer) {
    ivec2 texDim = textureSize(samplerShadowMap, 0).xy;
    float scale = 1.5;
    float dx = scale * 1.0 / float(texDim.x);
    float dy = scale * 1.0 / float(texDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;
    
    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            shadowFactor += textureProj(sc, layer, vec2(dx*x, dy*y));
            count++;
        }
    
    }
    return shadowFactor / count;
}

#define NEAR_PLANE 1.0
#define FAR_PLANE 512.0


float linearDepth(float depth) {
    float z = depth * 2.0f - 1.0f;
    return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

mat3 computeTBNMatrixFromDepth(in sampler2D depthTex, in vec2 uv) {
    // Compute the normal and TBN matrix
    //float ld = -getLinearDepth(depthTex, uv);
    float ld = -linearDepth(texture(depthTex, uv).a);
    vec3 a = vec3(uv, ld);
    vec3 x = vec3(uv.x + dFdx(uv.x), uv.y, ld + dFdx(ld));
    vec3 y = vec3(uv.x, uv.y + dFdy(uv.y), ld + dFdy(ld));
    //x = dFdx(x);
    //y = dFdy(y);`
    //x = normalize(x);
    //y = normalize(y);
    vec3 normal = normalize(cross(x - a, y - a));
    vec3 first_axis = cross(normal, vec3(1.0f, 0.0f, 0.0f));
    vec3 second_axis = cross(first_axis, normal);
    return mat3(normalize(first_axis), normalize(second_axis), normal);
}

vec3 normalFromDepth(in sampler2D depthTex, in vec2 uv) {

    //float ld = -linearDepth(texture(depthTex, uv).a);
    float ld = (texture(depthTex, uv).a);
    vec3 a = vec3(uv, ld);
    vec3 x = vec3(uv.x + dFdx(uv.x), uv.y, ld + dFdx(ld));
    vec3 y = vec3(uv.x, uv.y + dFdy(uv.y), ld + dFdy(ld));
    //x = dFdx(x);
    //y = dFdy(y);`
    //x = normalize(x);
    //y = normalize(y);
    vec3 normal = normalize(cross(x - a, y - a));
    return normal;
}




void main() {
    // Get G-Buffer values
    vec4 samplerPos = texture(samplerPosition, inUV).rgba;
    float depth = samplerPos.a;


    //vec3 fragPos = samplerPos.rgb;
    // find a better way:
    vec3 viewPos = vec3(ubo.view * vec4(samplerPos.rgb, 1.0));// calculate view space position
    vec3 worldPos = samplerPos.rgb;


    vec3 fragPos = viewPos;// view space position

    
    


    

    vec3 normal = texture(samplerNormal, inUV).rgb * 2.0 - 1.0;

    //vec3 worldNormal = normal_from_depth(inUV, depth);

    // unpack
    ivec2 texDim = textureSize(samplerAlbedo, 0);
    //uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);
    uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim ), 0);

    vec4 color;
    color.rg = unpackHalf2x16(albedo.r);
    color.ba = unpackHalf2x16(albedo.g);
    vec4 spec;
    spec.rg = unpackHalf2x16(albedo.b); 

    vec3 ambient = color.rgb * AMBIENT_LIGHT;

    vec3 fragcolor  = ambient;
    
    if (length(fragPos) == 0.0) {
        fragcolor = color.rgb;
    } else {


        // // screen space point lights:
        // for(int i = 0; i < NUM_POINT_LIGHTS; ++i) {
            // Light to fragment

            // vec3 lightPos = vec3(ubo.view * ubo.model * vec4(ubo.pointlights[i].position.xyz, 1.0));// view space light position
            // vec3 L = lightPos - viewPos;
            // float dist = length(L);
            // L = normalize(L);

            // // Viewer to fragment
            // // view space:
            // vec3 vPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));// view space position
            // vec3 V = vPos - viewPos;
            
            // //vec3 V = ubo.viewPos.xyz - worldPos;
            // V = normalize(V);

            // // Attenuation
            // float atten = ubo.pointlights[i].radius / (pow(dist, 2.0) + 1.0);
            // //float atten = 1.0 / (1.0 + ubo.pointlights[i].linearFalloff * dist + ubo.pointlights[i].quadraticFalloff * dist * dist);


            // // Diffuse part
            // vec3 N = normalize(normal);
            // float NdotL = max(0.0, dot(N, L));
            // vec3 diff = ubo.pointlights[i].color.rgb * color.rgb * NdotL * atten;

            // // Specular part
            // vec3 R = reflect(-L, N);
            // float NdotR = max(0.0, dot(R, V));
            // vec3 spec = ubo.pointlights[i].color.rgb * spec.r * pow(NdotR, 16.0) * (atten * 1.5);

            // fragcolor += diff + spec;
        // }


        // world space point lights:
        for(int i = 0; i < NUM_POINT_LIGHTS; ++i) {

            PointLight light = ubo.pointlights[i];

            vec3 lightPos = light.position.xyz;// world space light position
            vec3 lightVec = lightPos - worldPos;// world space light to fragment

            vec3 lightDir = normalize(lightVec);// direction
            float dist = length(lightVec);// distance from light to frag



            // Attenuation
            //float attenuation = ubo.pointlights[i].radius / (pow(dist, 2.0) + 1.0);
            //float attenuation = 1.0f / (light.radius + light.linearFalloff * dist + light.quadraticFalloff * (dist * dist));
            float attenuation = 1.0 / (light.radius + (light.linearFalloff * dist) + (light.quadraticFalloff * (dist * dist)));

            
            



            vec3 N = normalize(normal);// normalized normal

            // Diffuse part
            float NdotL = max(0.0, dot(N, lightDir));// NdotL
            vec3 diffuse = vec3(NdotL);
            //vec3 diffuse = NdotL * light.color.rgb * color.rgb * attenuation;





            // Specular part
            //float specularStrength = 0.5f;


            vec3 viewDir = normalize(viewPos - worldPos);
            //vec3 viewDir = normalize(ubo.viewPos.xyz - worldPos);
            vec3 reflectDir = reflect(-lightDir, N);// reflect

            //float NdotR = max(0.0, dot(R, V));
            //vec3 specular = light.color.rgb * spec.r * pow(NdotR, 16.0) * (attenuation * 1.5);

            float NdotR = /*pow(*/max(0.0, dot(viewDir, reflectDir))/*, 16.0)*/;// NdotR, pow?
            vec3 specular = vec3(spec.r * pow(NdotR, 16.0));

            fragcolor += (diffuse + specular) * color.rgb * light.color.rgb * attenuation;
        }








        // // spot lights world pos:
        // for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {
        //     // Vector to light
        //     vec3 L = ubo.spotlights[i].position.xyz - worldPos;
        //     // Distance from light to fragment position
        //     float dist = length(L);
        //     L = normalize(L);

        //     // Viewer to fragment
        //     vec3 V = ubo.viewPos.xyz - worldPos;
        //     //vec3 V = viewPos - fragPos;
        //     V = normalize(V);

        //     float lightCosInnerAngle = cos(radians(15.0));
        //     float lightCosOuterAngle = cos(radians(25.0));
        //     float lightRange = 100.0;

        //     // Direction vector from source to target
        //     vec3 dir = normalize(ubo.spotlights[i].position.xyz - ubo.spotlights[i].target.xyz);

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

        //     fragcolor += vec3((diff + spec) * spotEffect * heightAttenuation) * ubo.spotlights[i].color.rgb * color.rgb;
        // }

        // // spot lights world pos:
        // for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {
        //     // Vector to light

        //     // world space:
        //     vec3 lightPos = ubo.spotlights[i].position.xyz;// world space light position
        //     vec3 L = lightPos - worldPos;


        //     // view space:
        //     //vec3 lightPos = vec3(ubo.view * ubo.model * vec4(ubo.pointlights[i].position.xyz, 1.0));// view space light position
        //     //vec3 L = lightPos - viewPos;


        //     // Distance from light to fragment position
        //     float dist = length(L);
        //     L = normalize(L);

        //     // Viewer to fragment
        //     vec3 V = ubo.viewPos.xyz - worldPos;
        //     //vec3 V = viewPos - fragPos;
        //     V = normalize(V);

        //     // view space:
        //     //vec3 vPos = vec3(ubo.view * ubo.model * vec4(ubo.viewPos.xyz, 1.0));// view space position
        //     //vec3 V = vPos - viewPos;
        //     //V = normalize(V);

        //     float lightCosInnerAngle = cos(radians(15.0));
        //     float lightCosOuterAngle = cos(radians(25.0));
        //     float lightRange = 100.0;


        //     // Direction vector from source to target

        //     // world space:
        //     vec3 target = ubo.spotlights[i].target.xyz;// world space target
        //     vec3 dir = normalize(lightPos - target);

            
        //     //vec3 target = vec3(ubo.view * ubo.model * vec4(ubo.spotlights[i].target.xyz, 1.0));// view space target
        //     //vec3 dir = normalize(lightPos - target);

        //     //vec3 dir = normalize(ubo.spotlights[i].position.xyz - ubo.spotlights[i].target.xyz);

        //     // Dual cone spot light with smooth transition between inner and outer angle
        //     float cosDir = dot(L, dir);
        //     float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
        //     float heightAttenuation = smoothstep(lightRange, 0.0f, dist);

        //     // Diffuse lighting
        //     vec3 N = normalize(normal);
        //     float NdotL = max(0.0, dot(N, L));

        //     //vec3 diff = vec3(NdotL);
        //     vec3 diff = ubo.spotlights[i].color.rgb * color.rgb * NdotL * heightAttenuation;

        //     // Specular lighting
        //     vec3 R = reflect(-L, N);
        //     float NdotR = max(0.0, dot(R, V));
        //     //vec3 spec = vec3(pow(NdotR, 16.0) * color.a * 2.5);
        //     //vec3 spec = ubo.spotlights[i].color.rgb * spec.r * pow(NdotR, 16.0) /** (atten * 1.5)*/;
        //     vec3 spec = ubo.spotlights[i].color.rgb * spec.r * pow(NdotR, 16.0) * color.a;

        //     //fragcolor += vec3((diff + spec) * spotEffect * heightAttenuation) * ubo.spotlights[i].color.rgb * color.rgb;
        //     fragcolor += (diff + spec) * spotEffect;
        // }


        // world space spot lights:
        for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {

            SpotLight2 light = ubo.spotlights[i];

            vec3 N = normalize(normal);
            
            vec3 lightPos = light.position.xyz;// world space light position
            vec3 lightVec = (lightPos - worldPos);// world space light to fragment
            //vec3 lightVec = (worldPos - lightPos);// world space light to fragment// reversed??

            vec3 lightDir = normalize(lightVec);// direction
            float dist = length(lightVec);// distance from light to frag


            // Viewer to fragment
            vec3 V = ubo.viewPos.xyz - worldPos;
            //vec3 V = viewPos - fragPos;
            V = normalize(V);



            // Attenuation
            //float attenuation = 1.0f / (/*light.radius + (light.linearFalloff * dist) +*/ (light.quadraticFalloff * (dist * dist)));
            float attenuation = 1.0;


            // Diffuse
            float NdotL = max(0.0, dot(N, lightDir));// NdotL
            vec3 diffuse = vec3(NdotL) /** light.color.rgb * attenuation*/;
            

            // Specular
            vec3 viewDir = normalize(-viewPos - worldPos);
            vec3 reflectDir = reflect(-lightDir, N);// reflect

            //float NdotR = /*pow(*/max(0.0, dot(viewDir, reflectDir))/*, 16.0)*/;// NdotR, pow?
            float NdotR = max(0.0, dot(reflectDir, V));
            //vec3 specular = /*light.color.rgb */ spec.r * pow(NdotR, 16.0) /** attenuation*/;
            vec3 specular = vec3(pow(NdotR, 16.0) * albedo.a * 2.5);






            // todo: replace with light struct member variables
            float lightCosInnerAngle = cos(radians(40.0));
            float lightCosOuterAngle = cos(radians(45.0));
            //float lightCosInnerAngle = cos(5.0);
            //float lightCosOuterAngle = cos(15.0);
            float lightRange = 100.0;
            
            // Spotlight (soft edges)
            vec3 spotDir = normalize(vec3(light.position - light.target));

            float theta = dot(lightDir, normalize(-spotDir));
            float epsilon = (/*ubo.spotlights[i].cutOff*/12.5 - /*ubo.spotlights[i].outerCutOff*/0.9);
            //float epsilon = lightCosInnerAngle - lightCosOuterAngle;
            float intensity = clamp((theta - lightCosOuterAngle) / epsilon, 0.0, 1.0);
            //diffuse  *= intensity;
            //specular *= intensity;




            // Dual cone spot light with smooth transition between inner and outer angle
            float cosDir = dot(lightDir, spotDir);
            float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
            float heightAttenuation = smoothstep(lightRange, 0.0, dist);


            //diffuse  *= attenuation;
            //specular *= attenuation;



            fragcolor += vec3((diffuse + specular) * spotEffect * heightAttenuation) * light.color.rgb * color.rgb;
        }





        // Shadow calculations in a separate pass
        if (USE_SHADOWS > 0) {
            for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {

                //vec3 worldPos = calculate_world_position(inUV, depth);
                //vec3 worldPos = fragPos;
                //vec3 worldPos = samplerPos.rgb;
                //vec3 worldPos = worldPosFromDepth(inUV, depth);

                vec4 shadowClip = ubo.spotlights[i].viewMatrix * vec4(worldPos, 1.0);

                float shadowFactor;
                #ifdef USE_PCF
                    shadowFactor = filterPCF(shadowClip, i);
                #else
                    shadowFactor = textureProj(shadowClip, i, vec2(0.0));
                #endif

                fragcolor *= shadowFactor;
            }
        }







        if (SSAO_ENABLED == 1) {
            float ao = texture(samplerSSAO, inUV).r;
            fragcolor *= ao.rrr;
        }
    }
   
    outFragcolor = vec4(fragcolor, 1.0);

    //vec3 test = normalFromDepth(samplerPosition, inUV);
    //vec3 test = vec3(depth/512.0);
    //outFragcolor = vec4(test, 1.0);
}