#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//layout (set = 3, binding = 1) uniform sampler2D samplerPositionDepth;
layout (set = 3, binding = 1) uniform sampler2D samplerDepth;
layout (set = 3, binding = 2) uniform sampler2D samplerNormal;
layout (set = 3, binding = 3) uniform usampler2D samplerAlbedo;// this is a usampler(on ssao)
layout (set = 3, binding = 4) uniform sampler2D samplerSSAO;

//layout (set = 3, binding = 6) uniform sampler2DShadow samplerShadowMap;
//layout (set = 3, binding = 6) uniform sampler2DArrayShadow samplerShadowMap;
layout (set = 3, binding = 5) uniform sampler2DArray samplerShadowMap;
//layout (set = 3, binding = 6) uniform sampler2D samplerShadowMap;
//layout (set = 3, binding = 7) uniform sampler2D samplerDepth;


struct PointLight {
    vec4 position;
    vec4 color;

    float radius;
    float quadraticFalloff;
    float linearFalloff;
    float _pad;
};


struct SpotLight {
    vec4 position;
    vec4 target;
    vec4 color;
    mat4 viewMatrix;

    float innerAngle;
    float outerAngle;
    float zNear;
    float zFar;

    float range;
    float pad1;
    float pad2;
    float pad3;
};

struct DirectionalLight {
    vec4 direction;
    vec4 color;
    mat4 viewMatrix;

    float zNear;
    float zFar;
    float size;

    float pad1;

    // todo: better solution may be possible:
    float cascadeNear;
    float cascadeFar;

    float pad2;
	float pad3;


	// float pad4;

	// float pad5;
	// float pad6;
	// float pad7;
	// float pad8;
};



#define NUM_POINT_LIGHTS 70
#define NUM_SPOT_LIGHTS 2
#define NUM_DIR_LIGHTS 3
#define NUM_LIGHTS_TOTAL 5

#define SHADOW_FACTOR 0.4//0.25//0.7
#define AMBIENT_LIGHT 0.2
#define SPOT_LIGHT_FOV_OFFSET 15

#define NEAR_PLANE 0.1
#define FAR_PLANE 256.0

// #define SSAO_ENABLED 1;
// #define USE_SHADOWS 1;
// #define USE_PCF 1;

const int SSAO_ENABLED = 1;
const int USE_SHADOWS = 1;
const int USE_PCF = 1;
const float PI = 3.14159265359;



// todo: make this another set(1) rather than binding = 4
layout (set = 3, binding = 6) uniform UBO 
{
    vec4 viewPos;
    mat4 model;
    mat4 view;
    mat4 projection;
    mat4 invViewProj;
    
    PointLight pointlights[NUM_POINT_LIGHTS];
    SpotLight spotlights[NUM_SPOT_LIGHTS];
    DirectionalLight directionalLights[NUM_DIR_LIGHTS];
    

} ubo;

layout (location = 0) in vec2 inUV;
//layout (location = 1) in vec3 inCamPos;// added

layout (location = 0) out vec4 outFragcolor;








// view space?
vec3 normal_from_depth(vec2 texCoord, float depth) {
  
  vec2 offset1 = vec2(0.0,0.001);
  vec2 offset2 = vec2(0.001,0.0);
  
  float depth1 = texture(samplerDepth, texCoord + offset1).r;
  float depth2 = texture(samplerDepth, texCoord + offset2).r;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}







// float textureProj(vec4 P, float layer, vec2 offset) {
// 	float shadow = 1.0;
// 	vec4 shadowCoord = P / P.w;
// 	shadowCoord.st = shadowCoord.st * 0.5 + 0.5;

// 	if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0)  {
// 		float dist = texture(samplerShadowMap, vec3(shadowCoord.st + offset, layer)).r;
// 		if (shadowCoord.w > 0.0 && dist < shadowCoord.z)  {
// 			shadow = SHADOW_FACTOR;
// 		}
// 	}

// 	//if(shadowCoord.z > 1.0) {// added
// 		//shadow = 0.0;
// 	//}

// 	return shadow;
// }

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

// check whether a point lies in a view frustrum or not:
bool in_frustum(mat4 M, vec3 p) {
    vec4 Pclip = M * vec4(p, 1.);
    return abs(Pclip.x) < Pclip.w && 
           abs(Pclip.y) < Pclip.w && 
           0 < Pclip.z && 
           Pclip.z < Pclip.w;
}



bool in_spotlight(SpotLight light, vec3 worldPos) {

	vec3 lightVec = (light.position.xyz - worldPos);// world space light to fragment

	vec3 lightDir = normalize(lightVec);// direction

	vec3 spotDir = normalize(vec3(light.position.xyz - light.target.xyz));

	float cosDir = dot(lightDir, spotDir);// theta

	if(cosDir > cos(radians(light.innerAngle-SPOT_LIGHT_FOV_OFFSET))) {
		return true;
	}
	return false;
}


float filterPCF(vec4 sc, float layer) {
    ivec2 texDim = textureSize(samplerShadowMap, 0).xy;
    float scale = 1.5;//1.5
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




float linearizeDepth(float depth) {
    float z = depth * 2.0f - 1.0f;
    return (2.0f * NEAR_PLANE * FAR_PLANE) / (FAR_PLANE + NEAR_PLANE - z * (FAR_PLANE - NEAR_PLANE));
}

mat3 computeTBNMatrixFromDepth(in sampler2D depthTex, in vec2 uv) {
    // Compute the normal and TBN matrix
    //float ld = -getlinearizeDepth(depthTex, uv);
    float ld = -linearizeDepth(texture(depthTex, uv).a);
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

    //float ld = -linearizeDepth(texture(depthTex, uv).a);
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

vec3 normalFromDepth2(in sampler2D depthTex, in vec2 texCoords) {
  
    const float off = 0.01;// 0.001

    vec2 offset1 = vec2(0.0,off);
    vec2 offset2 = vec2(off,0.0);

    float depth = texture(depthTex, texCoords).a;
    float depth1 = texture(depthTex, texCoords + offset1).a;
    float depth2 = texture(depthTex, texCoords + offset2).a;

    vec3 p1 = vec3(offset1, depth1 - depth);
    vec3 p2 = vec3(offset2, depth2 - depth);

    vec3 normal = cross(p1, p2);
    //normal.z = -normal.z;

    float diff = abs(depth2 - depth1);

    // if(diff > 0.005) {
    //     normal.xyz = vec3(1.0, 1.0, 1.0);
    // }

    if(diff == 0) {
        normal.xyz = vec3(0.0, 0.0, 0.0);
    }

    vec3 temp = normal;
    normal.x = temp.x;
    normal.y = -temp.y;
    normal.z = -temp.z;

    return normalize(normal);
}

vec3 normalFromDepth3(vec3 viewPos) {
    vec3 normal = normalize(cross(dFdx(viewPos), dFdy(viewPos)));
    normal.x = -normal.x;
    normal.y = -normal.y;
    normal.z = -normal.z;
    return normalize(normal);
}


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

// get world position from depth buffer
vec3 worldPosFromDepth(vec2 texCoord, float depth) {

    vec4 clipSpacePosition;
    clipSpacePosition.xy = texCoord * 2.0 - 1.0;
	clipSpacePosition.z = depth;
	clipSpacePosition.w = 1.0;

    vec4 worldSpacePosition = ubo.invViewProj * clipSpacePosition;

    // Perspective division
    worldSpacePosition /= worldSpacePosition.w;

    return worldSpacePosition.xyz;
}

vec3 viewPosFromDepth(vec2 texCoord, float depth) {

    vec4 clipSpacePosition;
    clipSpacePosition.xy = texCoord * 2.0 - 1.0;
    clipSpacePosition.z = depth;
    clipSpacePosition.w = 1.0;

    mat4 invProj = inverse(ubo.projection);

    vec4 viewSpacePosition = invProj * clipSpacePosition;

    // Perspective division
    viewSpacePosition /= viewSpacePosition.w;
    return viewSpacePosition.xyz;
}
















// vec3 materialcolor()
// {
// 	//return vec3(material.r, material.g, material.b);
// 	return vec3(0.0, 1.0, 0.0);
// }

// // Normal Distribution function --------------------------------------
// float D_GGX(float dotNH, float roughness)
// {
// 	float alpha = roughness * roughness;
// 	float alpha2 = alpha * alpha;
// 	float denom = dotNH * dotNH * (alpha2 - 1.0) + 1.0;
// 	return (alpha2)/(PI * denom*denom); 
// }

// // Geometric Shadowing function --------------------------------------
// float G_SchlicksmithGGX(float dotNL, float dotNV, float roughness)
// {
// 	float r = (roughness + 1.0);
// 	float k = (r*r) / 8.0;
// 	float GL = dotNL / (dotNL * (1.0 - k) + k);
// 	float GV = dotNV / (dotNV * (1.0 - k) + k);
// 	return GL * GV;
// }

// // Fresnel function ----------------------------------------------------
// vec3 F_Schlick(float cosTheta, float metallic, vec3 materialColor)
// {
// 	vec3 F0 = mix(vec3(0.04), /*materialcolor()*/materialColor, metallic); // * material.specular
// 	vec3 F = F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0); 
// 	return F;    
// }

// // Specular BRDF composition --------------------------------------------

// vec3 BRDF(vec3 L, vec3 V, vec3 N, float metallic, float roughness, vec3 materialColor)
// {
// 	// Precalculate vectors and dot products	
// 	vec3 H = normalize (V + L);
// 	float dotNV = clamp(dot(N, V), 0.0, 1.0);
// 	float dotNL = clamp(dot(N, L), 0.0, 1.0);
// 	float dotLH = clamp(dot(L, H), 0.0, 1.0);
// 	float dotNH = clamp(dot(N, H), 0.0, 1.0);

// 	// Light color fixed
// 	vec3 lightColor = vec3(1.0);

// 	vec3 color = vec3(0.0);

// 	if (dotNL > 0.0)
// 	{
// 		float rroughness = max(0.05, roughness);
// 		// D = Normal distribution (Distribution of the microfacets)
// 		float D = D_GGX(dotNH, roughness); 
// 		// G = Geometric shadowing term (Microfacets shadowing)
// 		float G = G_SchlicksmithGGX(dotNL, dotNV, roughness);
// 		// F = Fresnel factor (Reflectance depending on angle of incidence)
// 		//vec3 F = F_Schlick(dotNV, metallic);
// 		vec3 F = F_Schlick(dotNV, metallic, materialColor);

// 		vec3 spec = D * F * G / (4.0 * dotNL * dotNV);

// 		color += spec * dotNL * lightColor;
// 	}

// 	return color;
// }

























void main() {

    // Get G-Buffer values

    //vec4 samplerPosDepth = texture(samplerPosition, inUV).rgba;
    //float depth = samplerPosDepth.a;

    float depth = texture(samplerDepth, inUV.st).r;
    float linearDepth = linearizeDepth(depth);

    vec3 worldPos = worldPosFromDepth(inUV.st, depth);
    vec3 viewPos = vec3(ubo.view * vec4(worldPos, 1.0));// calculate view space position
    
    

    vec3 normal = texture(samplerNormal, inUV).rgb * 2.0 - 1.0;// world space normal
    //vec3 normal = texture(samplerDepth, inUV).gba * 2.0 - 1.0;// world space normal

    //vec3 worldNormal = normal_from_depth(inUV, depth);

    // unpack
    ivec2 texDim = textureSize(samplerAlbedo, 0);
    //uvec4 albedo = texture(samplerAlbedo, inUV.st, 0);
    uvec4 albedo = texelFetch(samplerAlbedo, ivec2(inUV.st * texDim), 0);

    vec4 color;
    color.rg = unpackHalf2x16(albedo.r);
    color.ba = unpackHalf2x16(albedo.g);
    vec4 spec;
    spec.rg = unpackHalf2x16(albedo.b); 

    vec3 ambient = color.rgb * AMBIENT_LIGHT;

    vec3 fragcolor = ambient;
    
    //if (length(fragPos) == 0.0) {
    if (length(viewPos) == 0.0) {
        fragcolor = color.rgb;
    } else {


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

            float NdotR = pow(max(0.0, dot(viewDir, reflectDir)), 16.0);// NdotR, pow?
            vec3 specular = vec3(spec.r * NdotR);

            fragcolor += (diffuse + specular) * color.rgb * light.color.rgb * attenuation;
        }

  //       float time = (ubo.directionalLights[0].pad2+ubo.directionalLights[0].pad1);
		// vec3 N = normalize(normal);
		// vec3 V = normalize(-ubo.viewPos.xyz - worldPos.xyz);

		// //float roughness = cos(time*5)*5;
		// float roughness = 0.5;
		// //float metalness = sin(time*1)*5;
		// float metalness = 0.1;

		// //roughness = max(roughness, step(fract(worldPos.y * 2.02), 0.5));
		// vec3 Lo = vec3(0.0);

  //       for(int i = 0; i < NUM_POINT_LIGHTS; ++i) {
  //       	PointLight light = ubo.pointlights[i];


		// 	vec3 L = normalize(light.position.xyz - worldPos.xyz);
		// 	Lo += BRDF(L, V, N, metalness, roughness, color.rgb);
  //       }
  //       vec3 color = Lo;
  //       color = pow(color, vec3(0.4545));
  //       fragcolor += color;







        // world space spot lights:
        for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {

            SpotLight light = ubo.spotlights[i];

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
            vec3 specular = vec3(pow(NdotR, 16.0) * /*albedo.a **/ 2.5);






            
            float lightCosInnerAngle = cos(radians(light.innerAngle-SPOT_LIGHT_FOV_OFFSET));
            //float lightCosOuterAngle = cos(radians(light.outerAngle));
            float lightCosOuterAngle = cos(radians(light.innerAngle-SPOT_LIGHT_FOV_OFFSET));
            float lightRange = light.range;

            // float lightCosInnerAngle = cos(radians(45));
            // float lightCosOuterAngle = cos(radians(46));
            // float lightRange = 100.0;
            
            // Spotlight (soft edges)
            vec3 spotDir = normalize(vec3(light.position - light.target));

            // float theta = dot(lightDir, normalize(-spotDir));
            // //float epsilon = (/*ubo.spotlights[i].cutOff*/12.5 - /*ubo.spotlights[i].outerCutOff*/0.9);
            // float epsilon = lightCosInnerAngle - lightCosOuterAngle;
            // float intensity = clamp((theta - lightCosOuterAngle) / epsilon, 0.0, 1.0);


            // Dual cone spot light with smooth transition between inner and outer angle
            float cosDir = dot(lightDir, spotDir);// theta
            float spotEffect = smoothstep(lightCosOuterAngle, lightCosInnerAngle, cosDir);
            float heightAttenuation = smoothstep(lightRange, 0.0, dist);

            fragcolor += vec3((diffuse + specular) * spotEffect * heightAttenuation) * light.color.rgb * color.rgb;
        	
        }




        // directional lights:
        for(int i = 0; i < NUM_DIR_LIGHTS; ++i) {

        	DirectionalLight light = ubo.directionalLights[i];

        	vec3 N = normalize(normal);

        	vec3 viewDir = normalize(-viewPos - worldPos);// changed to -viewPos

		    vec3 lightDir = normalize(-vec3(light.direction));
		    // Diffuse shading
		    float diffuse = max(dot(normal, lightDir), 0.0);// angle between normal and light direction
		    

            // Specular shading
            vec3 reflectDir = reflect(-lightDir, N);// reflect


            vec3 V = normalize(ubo.viewPos.xyz - worldPos);
            float NdotR = max(0.0, dot(reflectDir, V));
            vec3 specular = vec3(pow(NdotR, 16.0) * /*albedo.a */ 2.5);

		    // Specular shading
		    //vec3 reflectDir = reflect(-lightDir, normal);
		    //float specular = pow(max(dot(viewDir, reflectDir), 0.0), /*material.shininess*/5.0);

		    fragcolor += vec3((diffuse + specular)) * light.color.rgb * color.rgb;



        }


        // Shadow calculations in a separate pass
        if (USE_SHADOWS > 0) {


            for(int i = 0; i < NUM_SPOT_LIGHTS; ++i) {
                vec4 shadowClip = ubo.spotlights[i].viewMatrix * vec4(worldPos, 1.0);

                float shadowFactor;
                
                // if the fragment isn't in the shadow map's view frustrum, it can't be in shadow
                // if the fragment isn't in the spotlight's cone, it can't be in shadow
                // seems to fix lots of problems
         		if(!in_frustum(ubo.spotlights[i].viewMatrix, worldPos) || !in_spotlight(ubo.spotlights[i], worldPos)) {       	
                	shadowFactor = 1.0;
                } else {
	                if(USE_PCF > 0) {
	                    shadowFactor = filterPCF(shadowClip, i);
	                } else {
	                    shadowFactor = textureProj(shadowClip, i, vec2(0.0));
	                }

                }

                fragcolor *= shadowFactor;
            }

            // directional lights:
            // for(int i = 0; i < NUM_DIR_LIGHTS; ++i) {
            //     vec4 shadowClip = ubo.directionalLights[i].viewMatrix * vec4(worldPos, 1.0);

            //     float shadowFactor;
                
            //     // if the fragment isn't in the light's view frustrum, it can't be in shadow, either
            //     // seems to fix lots of problems
            //     if(!in_frustum(ubo.directionalLights[i].viewMatrix, worldPos)) {
            //     	shadowFactor = 1.0;
            //     } else {
	           //      if(USE_PCF > 0) {
	           //          shadowFactor = filterPCF(shadowClip, i+NUM_SPOT_LIGHTS);
	           //      } else {
	           //          shadowFactor = textureProj(shadowClip, i+NUM_SPOT_LIGHTS, vec2(0.0));
	           //      }

            //     }

            //     fragcolor *= shadowFactor;
            // }

            // float endClipSpaces[4];
            // endClipSpaces[0] = 0.1;
            // endClipSpaces[1] = 4.0;
            // endClipSpaces[2] = 20.0;
            // endClipSpaces[3] = 256.0;

            for(int i = 0; i < NUM_DIR_LIGHTS; ++i) {


                vec4 shadowClip = ubo.directionalLights[i].viewMatrix * vec4(worldPos, 1.0);

                float shadowFactor = 1.0;

                int offset = NUM_SPOT_LIGHTS;

                if((linearDepth < ubo.directionalLights[i].cascadeNear) || (linearDepth > ubo.directionalLights[i].cascadeFar)) {
                	continue;
                }

                // if(linearDepth < ubo.directionalLights[i].cascadeNear) {
                // 	continue;
                // }

                // if(linearDepth > 0.1 && linearDepth < 4.0) {
                // 	fragcolor += vec3(0.2, 0.0, 0.0);
                // } else if (linearDepth > 4.0 && linearDepth < 20.0) {
                // 	fragcolor += vec3(0.0, 0.2, 0.0);
                // } else if (linearDepth > 4.0 && linearDepth < 20.0) {

                // }

                if(i == 0) {
                	fragcolor += vec3(0.2, 0.0, 0.0);
                } else if(i == 1) {
                	fragcolor += vec3(0.0, 0.2, 0.0);
                } else if(i == 2) {
                	fragcolor += vec3(0.0, 0.0, 0.5);
                }
                
                // if the fragment isn't in the light's view frustrum, it can't be in shadow, either
                // seems to fix lots of problems
                //if(!in_frustum(ubo.directionalLights[i].viewMatrix, worldPos)) {
                	//shadowFactor = 1.0;
                //} else {
	                if(USE_PCF > 0) {
	                    shadowFactor = filterPCF(shadowClip, i+offset);
	                } else {
	                    shadowFactor = textureProj(shadowClip, i+offset, vec2(0.0));
	                }

                //}

                fragcolor *= shadowFactor;
            }



            // csm lights:
            //for(int i = 0; i < NUM_CSM_LIGHTS; ++i) {
            // for(int i = 0; i < 1; ++i) {
            //     //vec4 shadowClip = ubo.csmlights[i].viewMatrix * vec4(worldPos, 1.0);
            //     vec4 shadowClip = ubo.directionalLights[i].viewMatrix * vec4(worldPos, 1.0);

            //     float shadowFactor = 1.0;
            //     int offset = NUM_SPOT_LIGHTS+NUM_DIR_LIGHTS;

            //     if(linearizeDepth(depth) > 4.0) {
            //     	//fragcolor += vec3(0.4, 0.0, 0.0);
            //     	offset += 1;
            //     }
            //     if(linearizeDepth(depth) > 20.0) {
            //     	//fragcolor += vec3(0.4, 0.0, 0.0);
            //     	//offset += 1;
            //     	continue;
            //     }
            //     //offset += 1;
                
            //     // if the fragment isn't in the light's view frustrum, it can't be in shadow, either
            //     // seems to fix lots of problems
            //     //if(!in_frustum(ubo.csmlights[i].viewMatrix, worldPos)) {
            //     	//shadowFactor = 1.0;
            //     //} else {
	           //      if(USE_PCF > 0) {
	           //          shadowFactor = filterPCF(shadowClip, i+offset);
	           //      } else {
	           //          shadowFactor = textureProj(shadowClip, i+offset, vec2(0.0));
	           //      }

            //     //}

            //     fragcolor *= shadowFactor;
            // }



        }

        if (SSAO_ENABLED > 0) {
            float ao = texture(samplerSSAO, inUV).r;
            fragcolor *= ao.rrr;
        }
    }
   
    outFragcolor = vec4(fragcolor, 1.0);
}





















// // // Generally all the code comes from https://github.com/achlubek/venginenative/ which you can star if you like it!
 
// #define time iGlobalTime
// //#define mouse (iMouse.xy / iResolution.xy)
// #define resolution iResolution.xy
// //afl_ext 2017

// // lower this if your GPU cries for mercy (set to 0 to remove clouds!)
// #define CLOUDS_STEPS 0
// #define ENABLE_SSS 1

// float iGlobalTime = ubo.directionalLights[0].pad1;
// const vec2 iResolution = vec2(1280, 720);
// vec2 mouse = vec2(0.0, 0.0);


// // its from here https://github.com/achlubek/venginenative/blob/master/shaders/include/WaterHeight.glsl 
// float wave(vec2 uv, vec2 emitter, float speed, float phase){
//     float dst = distance(uv, emitter);
//     return pow((0.5 + 0.5 * sin(dst * phase - time * speed)), 5.0);
// }

// #define GOLDEN_ANGLE_RADIAN 2.39996
// float getwaves(vec2 position){
//     position *= 0.1;
//     float w = wave(position, vec2(70.3, 60.4), 1.0, 4.5) * 0.5;
//     w += wave(position, vec2(60.3, -55.4), 1.0, 5.0) * 0.5;

//     w += wave(position, vec2(-74.3, 50.4), 1.0, 4.0) * 0.5;
//     w += wave(position, vec2(-60.3, -70.4), 1.0, 4.7) * 0.5;


//     w += wave(position, vec2(-700.3, 500.4), 2.1, 8.0) * 0.1;
//     w += wave(position, vec2(700.3, -500.4), 2.4, 8.8) * 0.1;
    
//     w += wave(position, vec2(-700.3, -500.4), 2.6, 9.0) * 0.1;
//     w += wave(position, vec2(-700.3, -500.4), 2.7, 9.6) * 0.1;

//     w += wave(position, vec2(300.3, -760.4), 2.0, 12.0) * 0.08;
//     w += wave(position, vec2(-300.3, -400.4), 2.230, 13.0) * 0.08;
    
//     w += wave(position, vec2(-100.3, -760.4), 2.0, 14.0) * 0.08;
//     w += wave(position, vec2(-100.3, 400.4), 2.230, 15.0) * 0.08;

//     w += wave(position, vec2(300.3, -760.4), 2.0, 22.0) * 0.08;
//     w += wave(position, vec2(-300.3, -400.4), 2.230, 23.0) * 0.08;
    
//     w += wave(position, vec2(-100.3, -760.4), 2.0, 24.0) * 0.08;
//     w += wave(position, vec2(-100.3, 400.4), 2.230, 22.0) * 0.08;


//     return w * 0.44;
// }



// float hashx( float n ){
//     return fract(sin(n)*758.5453);
// }

// float noise2X( vec2 x2 , float a, float b, float off){
//     vec3 x = vec3(x2.x, x2.y,time * 0.1 + off);
//     vec3 p = floor(x);
//     vec3 f = fract(x);
//     f       = f*f*(3.0-2.0*f);

//     float h2 = a;
//      float h1 = b;
//     #define h3 (h2 + h1)

//      float n = p.x + p.y*h1+ h2*p.z;
//     return mix(mix( mix( hashx(n+0.0), hashx(n+1.0),f.x),
//             mix( hashx(n+h1), hashx(n+h1+1.0),f.x),f.y),
//            mix( mix( hashx(n+h2), hashx(n+h2+1.0),f.x),
//             mix( hashx(n+h3), hashx(n+h3+1.0),f.x),f.y),f.z);
// }

// float supernoise(vec2 x){
//     float a = noise2X(x, 55.0, 92.0, 0.0);
//     float b = noise2X(x + 0.5, 133.0, 326.0, 0.5);
//     return (a * b);
// }
// float supernoise3d(vec3 x);

// #define cosinelinear(a) ((cos(a * 3.1415) * -sin(a * 3.1415) * 0.5 + 0.5))
// #define snoisesinpow(a,b) pow(1.0 - abs(supernoise3d(vec3(a, Time)) - 0.5) * 2.0, b)
// #define XX(a,b) pow(1.0 - abs((a) - 0.5) * 2.0, b)
// #define snoisesinpow2(a,b) pow(cosinelinear(supernoise(a)), b)
// #define snoisesinpow3(a,b) pow(1.0 - abs(supernoise(a ) - 0.5) * 2.0, b)
// #define snoisesinpow4X(a,b) pow(1.0 - 2.0 * abs(supernoise(a) - 0.5), b)
// #define snoisesinpow4(a,b) pow(cosinelinear(1.0 - 2.0 * abs(supernoise(a) - 0.5)), b)
// #define snoisesinpow5(a,b) pow(1.0 - abs(0.5 - supernoise3d(vec3(a, Time * 0.3 * WaterSpeed))) * 2.0, b)
// #define snoisesinpow6(a,b) pow(1.0 - abs(0.5 - supernoise3d(vec3(a, Time * 0.3 * WaterSpeed))) * 2.0, b)

// float heightwaterHI2(vec2 pos){
//     float res = 0.0;
//     pos *= 6.0;
//     float w = 0.0;
//     float wz = 1.0;
//     float chop = 6.0;
//     float tmod = 5210.1;

//     for(int i=0;i<6;i++){
//         vec2 t = vec2(tmod * time*0.00018);
//         float x1 = snoisesinpow4X(pos + t, chop);
//         float x2 = snoisesinpow4(pos + t, chop);
//         res += wz * mix(x1 * x2, x2, supernoise(pos + t) * 0.5 + 0.5) * 2.5;
//         w += wz * 1.0;
//         x1 = snoisesinpow4X(pos - t, chop);
//         x2 = snoisesinpow4(pos - t, chop);
//         res += wz * mix(x1 * x2, x2, supernoise(pos - t) * 0.5 + 0.5) * 2.5;
//         w += wz * 1.0;
//         chop = mix(chop, 5.0, 0.3);
//         wz *= 0.4;
//         pos *= vec2(2.1, 1.9);
//         tmod *= 0.8;
//     }
//     w *= 0.55;
//     return (pow(res / w * 2.0, 1.0));
// }
// float getwavesHI(vec2 uv, float details){
//     return (getwaves(uv)) + details * 0.09 * heightwaterHI2(uv * 0.1  );
// }

// float H = 0.0;
// vec3 normal(vec2 pos, float e, float depth){
//     vec2 ex = vec2(e, 0);
//     float inf = 1.0 ;
//     H = getwavesHI(pos.xy, inf) * depth;
//     vec3 a = vec3(pos.x, H, pos.y);
//     return normalize(cross(normalize(a-vec3(pos.x - e, getwavesHI(pos.xy - ex.xy, inf) * depth, pos.y)), 
//                            normalize(a-vec3(pos.x, getwavesHI(pos.xy + ex.yx, inf) * depth, pos.y + e))));
// }
// mat3 rotmat(vec3 axis, float angle)
// {
//     axis = normalize(axis);
//     float s = sin(angle);
//     float c = cos(angle);
//     float oc = 1.0 - c;
    
//     return mat3(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s, 
//     oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s, 
//     oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c);
// }

// vec3 getRay(vec2 uv){
//    uv = (uv * 2.0 - 1.0)* vec2(resolution.x / resolution.y, 1.0);
//     vec3 proj = normalize(vec3(uv.x, uv.y, 1.5));   
//     if(resolution.y < 400.0) return proj;
//     vec3 ray = rotmat(vec3(0.0, -1.0, 0.0), 3.0 * (mouse.x * 2.0 - 1.0)) * rotmat(vec3(1.0, 0.0, 0.0), 1.5 * (mouse.y * 2.0 - 1.0)) * proj;
//     return ray;
// }

// float rand2sTime(vec2 co){
//     co *= time;
//     return fract(sin(dot(co.xy,vec2(12.9898,78.233))) * 43758.5453);
// }
// float raymarchwater2(vec3 camera, vec3 start, vec3 end, float depth){
//     float stepsize = 1.0 / 6.0;
//     float iter = 0.0;
//     vec3 pos = start;
//     float h = 0.0;
//     for(int i=0;i<7;i++){
//         pos = mix(start, end, iter);
//         h = getwaves(pos.xz) * depth - depth;
//         if(h > pos.y) {
//             return distance(pos, camera);
//         }
//         iter += stepsize;
//     }
//     return -1.0;
// }

// float raymarchwater(vec3 camera, vec3 start, vec3 end, float depth){
//     float stepsize = 1.0 / 66.0;
//     float iter = 0.0;
//     vec3 pos = start;
//     float h = 0.0;
//     for(int i=0;i<67;i++){
//         pos = mix(start, end, iter);
//         h = getwaves(pos.xz) * depth - depth;
//         if(h > pos.y) {
//             return raymarchwater2(camera, mix(start, end, iter - stepsize), mix(start, end, iter), depth);
//         }
//         iter += stepsize;
//     }
//     return -1.0;
// }

// float intersectPlane(vec3 origin, vec3 direction, vec3 point, vec3 normal)
// { 
//     return clamp(dot(point - origin, normal) / dot(direction, normal), -1.0, 9991999.0); 
// }
// #define PI 3.141592
// #define iSteps 16
// #define jSteps 8

// vec2 rsi(vec3 r0, vec3 rd, float sr) {
//     // ray-sphere intersection that assumes
//     // the sphere is centered at the origin.
//     // No intersection when result.x > result.y
//     float a = dot(rd, rd);
//     float b = 2.0 * dot(rd, r0);
//     float c = dot(r0, r0) - (sr * sr);
//     float d = (b*b) - 4.0*a*c;
//     if (d < 0.0) return vec2(1e5,-1e5);
//     return vec2(
//         (-b - sqrt(d))/(2.0*a),
//         (-b + sqrt(d))/(2.0*a)
//     );
// }

// float hash( float n ){
//     return fract(sin(n)*758.5453);
//     //return fract(mod(n * 2310.7566730, 21.120312534));
// }

// float noise3d( in vec3 x ){
//     vec3 p = floor(x);
//     vec3 f = fract(x);
//     f       = f*f*(3.0-2.0*f);
//     float n = p.x + p.y*157.0 + 113.0*p.z;

//     return mix(mix( mix( hash(n+0.0), hash(n+1.0),f.x),
//             mix( hash(n+157.0), hash(n+158.0),f.x),f.y),
//            mix( mix( hash(n+113.0), hash(n+114.0),f.x),
//             mix( hash(n+270.0), hash(n+271.0),f.x),f.y),f.z);
// }

// float noise2d( in vec2 x ){
//     vec2 p = floor(x);
//     vec2 f = smoothstep(0.0, 1.0, fract(x));
//     float n = p.x + p.y*57.0;
//     return mix(mix(hash(n+0.0),hash(n+1.0),f.x),mix(hash(n+57.0),hash(n+58.0),f.x),f.y);
// }
//  float configurablenoise(vec3 x, float c1, float c2) {
//     vec3 p = floor(x);
//     vec3 f = fract(x);
//     f       = f*f*(3.0-2.0*f);

//     float h2 = c1;
//      float h1 = c2;
//     #define h3 (h2 + h1)

//      float n = p.x + p.y*h1+ h2*p.z;
//     return mix(mix( mix( hash(n+0.0), hash(n+1.0),f.x),
//             mix( hash(n+h1), hash(n+h1+1.0),f.x),f.y),
//            mix( mix( hash(n+h2), hash(n+h2+1.0),f.x),
//             mix( hash(n+h3), hash(n+h3+1.0),f.x),f.y),f.z);

// }

// float supernoise3d(vec3 p){

//     float a =  configurablenoise(p, 883.0, 971.0);
//     float b =  configurablenoise(p + 0.5, 113.0, 157.0);
//     return (a + b) * 0.5;
// }
// float supernoise3dX(vec3 p){

//     float a =  configurablenoise(p, 883.0, 971.0);
//     float b =  configurablenoise(p + 0.5, 113.0, 157.0);
//     return (a * b);
// }
// float fbmHI(vec3 p){
//    // p *= 0.1;
//     p *= 0.0000169;
//     p.x *= 0.489;
//     p += time * 0.02;
//     //p += getWind(p * 0.2) * 6.0;
//     float a = 0.0;
//     float w = 1.0;
//     float wc = 0.0;
//     for(int i=0;i<3;i++){
//         //p += noise(vec3(a));
//         a += clamp(2.0 * abs(0.5 - (supernoise3dX(p))) * w, 0.0, 1.0);
//         wc += w;
//         w *= 0.5;
//         p = p * 3.0;
//     }
//     return a / wc;// + noise(p * 100.0) * 11;
// }
// #define MieScattCoeff 2.0
// vec3 wind(vec3 p){
//     return vec3(
//         supernoise3d(p),
//         supernoise3d(p.yzx),
//         supernoise3d(-p.xzy)
//         ) * 2.0 - 1.0;
// }
// struct Ray { vec3 o; vec3 d; };
// struct Sphere { vec3 pos; float rad; };

// float planetradius = 6378000.1;
// float minhit = 0.0;
// float maxhit = 0.0;
// float rsi2(in Ray ray, in Sphere sphere)
// {
//     vec3 oc = ray.o - sphere.pos;
//     float b = 2.0 * dot(ray.d, oc);
//     float c = dot(oc, oc) - sphere.rad*sphere.rad;
//     float disc = b * b - 4.0 * c;
//     vec2 ex = vec2(-b - sqrt(disc), -b + sqrt(disc))/2.0;
//     minhit = min(ex.x, ex.y);
//     maxhit = max(ex.x, ex.y);
//     return mix(ex.y, ex.x, step(0.0, ex.x));
// }
// vec3 atmosphere(vec3 r, vec3 r0, vec3 pSun, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g) {
//     // Normalize the sun and view directions.
//     pSun = normalize(pSun);
//     r = normalize(r);

//     // Calculate the step size of the primary ray.
//     vec2 p = rsi(r0, r, rAtmos);
//     if (p.x > p.y) return vec3(0,0,0);
//     p.y = min(p.y, rsi(r0, r, rPlanet).x);
//     float iStepSize = (p.y - p.x) / float(iSteps);
//     float rs = rsi2(Ray(r0, r), Sphere(vec3(0), rAtmos));
//     vec3 px = r0 + r * rs;
// shMie *= ( (pow(fbmHI(px  ) * (supernoise3dX(px* 0.00000669 + time * 0.001)*0.5 + 0.5) * 1.3, 3.0) * 0.8 + 0.5));
    
//     // Initialize the primary ray time.
//     float iTime = 0.0;

//     // Initialize accumulators for Rayleigh and Mie scattering.
//     vec3 totalRlh = vec3(0,0,0);
//     vec3 totalMie = vec3(0,0,0);

//     // Initialize optical depth accumulators for the primary ray.
//     float iOdRlh = 0.0;
//     float iOdMie = 0.0;

//     // Calculate the Rayleigh and Mie phases.
//     float mu = dot(r, pSun);
//     float mumu = mu * mu;
//     float gg = g * g;
//     float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
//     float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

//     // Sample the primary ray.
//     for (int i = 0; i < iSteps; i++) {

//         // Calculate the primary ray sample position.
//         vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);

//         // Calculate the height of the sample.
//         float iHeight = length(iPos) - rPlanet;

//         // Calculate the optical depth of the Rayleigh and Mie scattering for this step.
//         float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
//         float odStepMie = exp(-iHeight / shMie) * iStepSize;

//         // Accumulate optical depth.
//         iOdRlh += odStepRlh;
//         iOdMie += odStepMie;

//         // Calculate the step size of the secondary ray.
//         float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);

//         // Initialize the secondary ray time.
//         float jTime = 0.0;

//         // Initialize optical depth accumulators for the secondary ray.
//         float jOdRlh = 0.0;
//         float jOdMie = 0.0;

//         // Sample the secondary ray.
//         for (int j = 0; j < jSteps; j++) {

//             // Calculate the secondary ray sample position.
//             vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);

//             // Calculate the height of the sample.
//             float jHeight = length(jPos) - rPlanet;

//             // Accumulate the optical depth.
//             jOdRlh += exp(-jHeight / shRlh) * jStepSize;
//             jOdMie += exp(-jHeight / shMie) * jStepSize;

//             // Increment the secondary ray time.
//             jTime += jStepSize;
//         }

//         // Calculate attenuation.
//         vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

//         // Accumulate scattering.
//         totalRlh += odStepRlh * attn;
//         totalMie += odStepMie * attn;

//         // Increment the primary ray time.
//         iTime += iStepSize;

//     }

//     // Calculate and return the final color.
//     return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
// }
// vec3 getatm(vec3 ray){
//     vec3 sd = rotmat(vec3(1.0, 1.0, 0.0), time * 0.25) * normalize(vec3(0.0, 1.0, 0.0)); 
//     vec3 color = atmosphere(
//         ray,           // normalized ray direction
//         vec3(0,6372e3,0),               // ray origin
//         sd,                        // position of the sun
//         22.0,                           // intensity of the sun
//         6371e3,                         // radius of the planet in meters
//         6471e3,                         // radius of the atmosphere in meters
//         vec3(5.5e-6, 13.0e-6, 22.4e-6), // Rayleigh scattering coefficient
//         21e-6,                          // Mie scattering coefficient
//         8e3,                            // Rayleigh scale height
//         1.2e3 * MieScattCoeff,                          // Mie scale height
//         0.758                           // Mie preferred scattering direction
//     );      
//     return color;
    
// }

// float sun(vec3 ray){
//     vec3 sd = rotmat(vec3(1.0, 1.0, 0.0), time * 0.25) * normalize(vec3(0.0, 1.0, 0.0)); 

//     return pow(max(0.0, dot(ray, sd)), 228.0) * 110.0;
// }
// float smart_inverse_dot(float dt, float coeff){
//     return 1.0 - (1.0 / (1.0 + dt * coeff));
// }
// #define VECTOR_UP vec3(0.0,1.0,0.0)
// vec3 getSunColorDirectly(float roughness){
//     vec3 sunBase = vec3(15.0);
//     vec3 sd = rotmat(vec3(1.0, 1.0, 0.0), time * 0.25) * normalize(vec3(0.0, 1.0, 0.0)); 

//     float dt = max(0.0, (dot(sd, VECTOR_UP)));
//     float dtx = smoothstep(-0.0, 0.1, dt);
//     float dt2 = 0.9 + 0.1 * (1.0 - dt);
//     float st = max(0.0, 1.0 - smart_inverse_dot(dt, 11.0));
//     vec3 supersundir = max(vec3(0.0),   vec3(1.0) - st * 4.0 * pow(vec3(50.0/255.0, 111.0/255.0, 153.0/255.0), vec3(2.4)));
// //    supersundir /= length(supersundir) * 1.0 + 1.0;
//     return supersundir * 4.0 ;
//     //return mix(supersundir * 1.0, sunBase, st);
//     //return  max(vec3(0.3, 0.3, 0.0), (  sunBase - vec3(5.5, 18.0, 20.4) *  pow(1.0 - dt, 8.0)));
// }
// #define xsupernoise3d(a) abs(0.5 - noise3d(a))*2.0
// #define xsupernoise3dx(a) abs(0.5 - supernoise3d(a))*2.0
// float fbmHxI(vec3 p){
//    // p *= 0.1;
//     p *= 0.021;
//     float a = 0.0;
//     float w = 0.5;
//     for(int i=0;i<7;i++){
//     float x = xsupernoise3dx(p);
//         a += x * w;
//         p = p * (2.9 + x * w * 0.006 );
//         w *= 0.60;
//     }
//     return a;
// }

// #define CloudsFloor 700.0
// #define CloudsCeil 4000.0

// float getHeightOverSea(vec3 p){
//     vec3 atmpos = vec3(0.0, planetradius, 0.0) + p;
//     return length(atmpos) - planetradius;
// }
// vec2 cloudsDensity3D(vec3 pos){
//     vec3 ps = pos;

//     vec3 p = ps * 0.009;
//     vec3 timev = vec3(time*0.4, time * 0.2, 0.0);
//     vec3 windpos = p * 0.01 + timev * 0.01;
//     float density = fbmHxI( p + timev * 8.0 ) * supernoise3d(p*0.008+ timev * 0.05);
    
//     float measurement = (CloudsCeil - CloudsFloor) * 0.5;
//     float mediana = (CloudsCeil + CloudsFloor) * 0.5;
//     float h = getHeightOverSea(pos);
//     float mlt = (( 1.0 - (abs( h - mediana ) / measurement )));
//     float init = smoothstep(0.12, 0.19, density * mlt);
//     return  vec2(init, (h - CloudsFloor) / (CloudsCeil - CloudsFloor)) ;
// }

// vec2 UV = vec2(0.0);
// vec4 internalmarchconservative(vec3 atm, vec3 p1, vec3 p2, float noisestrength){
//     int stepsmult = int(abs(CloudsFloor - CloudsCeil) * 0.001);
//     int stepcount = CLOUDS_STEPS;
//     float stepsize = 1.0 / float(stepcount);
//     float rd = fract(rand2sTime(UV)) * stepsize * noisestrength;
//     float c = 0.0;
//     float w = 0.0;
//     float coverageinv = 1.0;
//     vec3 pos = vec3(0);
//     float clouds = 0.0;
//     vec3 color = vec3(0.0);
//     float colorw = 1.01;                      
//     float godr = 0.0;
//     float godw = 0.0;
//     float depr = 0.0;
//     float depw = 0.0;
//     float iter = 0.0;
//     vec3 lastpos = p1;
//     //depr += distance(CAMERA, lastpos);
//     depw += 1.0;
//     float linear = distance(p1, mix(p1, p2, stepsize));
//     for(int i=0;i<CLOUDS_STEPS;i++){
//         pos = mix(p1, p2, iter + rd);
//         vec2 as = cloudsDensity3D(pos);
//         clouds = as.x;
//         float W = clouds * max(0.0, coverageinv);
//         color += W * vec3(as.y * as.y);
//         colorw += W;

//         coverageinv -= clouds;
//         depr += step(0.99, coverageinv) * distance(lastpos, pos);
//         if(coverageinv <= 0.0) break;
//         lastpos = pos;
//         iter += stepsize;
//         //rd = fract(rd + iter * 124.345345);
//     }
//     if(coverageinv > 0.99) depr = 0.0;
//     float cv = 1.0 - clamp(coverageinv, 0.0, 1.0);
//     color *= getSunColorDirectly(0.0) * 3.0;
//     vec3 sd = rotmat(vec3(1.0, 1.0, 0.0), time * 0.25) * normalize(vec3(0.0, 1.0, 0.0)); 
//     return vec4(sqrt(max(0.0, (dot(sd, VECTOR_UP)))) * mix((color / colorw) + atm * min(0.6, 0.01 * depr), atm * 0.41, min(0.99, 0.00001 * depr)), cv);
// }

// void main()
// {
//     iGlobalTime = ubo.directionalLights[0].pad1 + ubo.directionalLights[0].pad2;

//     mouse.x = iGlobalTime/4.0;
//     mouse.y = 0.5;

//     vec2 fragCoord = inUV.st;
//     fragCoord.t = 1.0 - fragCoord.t;

//     //vec2 uv = fragCoord.xy / resolution.xy;
//     vec2 uv = fragCoord.xy;
//     UV = uv;
//     vec3 sd = rotmat(vec3(1.0, 1.0, 0.0), time * 0.25) * normalize(vec3(0.0, 1.0, 0.0)); 
//     float waterdepth = 2.1;
//     vec3 wfloor = vec3(0.0, -waterdepth, 0.0);
//     vec3 wceil = vec3(0.0, 0.0, 0.0);
//     vec3 orig = vec3(0.0, 2.0, 0.0);
//     vec3 ray = getRay(uv);
    
//     float spherehit = rsi2(Ray(orig, ray), Sphere(vec3(-2.0, 3.0, 0.0), 1.0));
//     float fff = 1.0;
    
    
//     float hihit = intersectPlane(orig, ray, wceil, vec3(0.0, 1.0, 0.0));
    
//     Sphere sphere1 = Sphere(vec3(0), planetradius + CloudsFloor);
//     Sphere sphere2 = Sphere(vec3(0), planetradius + CloudsCeil);
    
//     if(ray.y >= -0.01){
//         vec3 atm = getatm(ray);
//         vec3 C = atm * 2.0 + sun(ray);
        
//         vec3 atmorg = vec3(0,planetradius,0);
//         Ray r = Ray(atmorg, ray);
//         float hitfloor = rsi2(r, sphere1);
//         float hitceil = rsi2(r, sphere2);
//         vec4 clouds = internalmarchconservative(atm, ray * hitfloor, ray * hitceil, 1.0);
//         C = mix(C, clouds.xyz, clouds.a);
//         C *= fff;
//         //tonemapping
//         C = normalize(C) * sqrt(length(C));
//         outFragcolor = vec4( C,1.0);   
//         return;
//     }
//     float lohit = intersectPlane(orig, ray, wfloor, vec3(0.0, 1.0, 0.0));
//     vec3 hipos = orig + ray * hihit;
//     vec3 lopos = orig + ray * lohit;
//     float dist = raymarchwater(orig, hipos, lopos, waterdepth);
//     vec3 pos = orig + ray * dist;

//     vec3 N = normal(pos.xz, 0.01, waterdepth);
//     N = mix(N, VECTOR_UP, 0.8 * min(1.0, sqrt(dist*0.01) * 1.1));
//     vec2 velocity = N.xz * (1.0 - N.y);
//     vec3 R = normalize(reflect(ray, N));
//     vec3 RF = normalize(refract(ray, N, 0.66)); 
//     float fresnel = (0.04 + (1.0-0.04)*(pow(1.0 - max(0.0, dot(-N, ray)), 5.0)));
            
//     R.y = abs(R.y);
//     vec3 reflection = getatm(R);
//     vec3 atmorg = vec3(0,planetradius,0) + pos;
//     Ray r = Ray(atmorg, R);
//     float hitfloor = rsi2(r, sphere1);
//     float hitceil = rsi2(r, sphere2);
//    // vec4 clouds = internalmarchconservative(reflection, R * hitfloor, R * hitceil, 0.0);
//     //vec3 C = fresnel * mix(reflection + sun(R), clouds.xyz, clouds.a * 0.3) * 2.0;
    
//     vec3 C = fresnel * (reflection + sun(R)) * 2.0;
    
//     float superscat = pow(max(0.0, dot(RF, sd)), 16.0) ;
// #if ENABLE_SSS
//     C += vec3(0.5,0.9,0.8) * superscat * getSunColorDirectly(0.0) * 81.0;
//     vec3 waterSSScolor =  vec3(0.01, 0.33, 0.55)*  0.171 ;
//     C += waterSSScolor * getSunColorDirectly(0.0) * (0.3 + getwaves(pos.xz)) * waterdepth * 0.3 * max(0.0, dot(sd, VECTOR_UP));
//     //tonemapping
//     #endif
//         C *= fff;
//     C = normalize(C) * sqrt(length(C));
    
//     outFragcolor = vec4(C,1.0);
// }