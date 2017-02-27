#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// scene is not visible from fragment shader
// layout (set = 0, binding = 0) uniform sceneBuffer
// {
// 	mat4 model;
// 	mat4 view;
// 	mat4 projection;
// 	mat4 normal;
// 	vec3 lightPos;
// 	vec3 cameraPos;
// } scene;







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



// float specpart(vec3 L, vec3 N, vec3 H)
// {
// 	if (dot(N, L) > 0.0) {
// 		return pow(clamp(dot(H, N), 0.0, 1.0), 64.0);
// 	}
// 	return 0.0;
// }










void main() 
{

	//vec3 eyePos = vec3(scene.view[0].w, scene.view[1].w, scene.view[2].w);



	// vec3 Eye = normalize(-inEyePos);


	// vec3 Reflected = normalize(reflect(-inLightVec, inNormal)); 

	// vec3 halfVec = normalize(inLightVec + inEyePos);
	// float diff = clamp(dot(inLightVec, inNormal), 0.0, 1.0);
	// float spec = specpart(inLightVec, inNormal, halfVec);
	// float intensity = 0.1 + diff + spec;
 
	// vec4 IAmbient = vec4(0.2, 0.2, 0.2, 1.0);

	// vec4 IDiffuse = vec4(0.5, 0.5, 0.5, 0.5) * max(dot(inNormal, inLightVec), 0.0);
	
	// float shininess = 0.75;
	// vec4 ISpecular = vec4(0.5, 0.5, 0.5, 1.0) * pow(max(dot(Reflected, Eye), 0.0), 2.0) * shininess; 

	// outFragColor = vec4((IAmbient + IDiffuse) * vec4(inColor, 1.0) + ISpecular);
 
	// // Some manual saturation
	// if (intensity > 0.95)
	// {
	// 	outFragColor *= 2.25;
	// }
	// if (intensity < 0.15)
	// {
	// 	outFragColor = vec4(0.1);
	// }
















    // Ambient
    //vec3 ambient = light.ambient * material.ambient;
    // vec3 ambient = vec3(0.1,0.1,0.1) * material.ambient;
  	
    // // Diffuse 
    // vec3 norm = normalize(Normal);
    // //vec3 lightDir = normalize(light.position - FragPos);
    // vec3 lightDir = normalize(scene.lightPos - FragPos);// fix

    // float diff = max(dot(norm, lightDir), 0.0);
    // vec3 diffuse = light.diffuse * (diff * material.diffuse);
    
    // // Specular
    // vec3 viewDir = normalize(viewPos - FragPos);
    // vec3 reflectDir = reflect(-lightDir, norm);  
    // float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    // vec3 specular = light.specular * (spec * material.specular);  
        
    // vec3 result = ambient + diffuse + specular;
    // color = vec4(result, 1.0f);


	// vec4 color = vec4(inColor, 1.0);
	// vec3 N = normalize(inNormal);
	// vec3 L = normalize(inLightVec);
	// vec3 V = normalize(inViewVec);
	// vec3 R = reflect(-L, N);
	// vec3 diffuse = max(dot(N, L), 0.0) * vec3(25.0, 25.0, 25.0);
	// vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * material.specular.rgb;
	// outFragColor = vec4((vec3(90.0, 10.0, 25.0) + diffuse) * color.rgb + specular, 1.0-1.0);



	//vec4 color = texture(samplerColorMap, inUV) * vec4(inColor, 1.0);
	//vec4 color = vec4(inColor, 1.0);





	// vec4 color = texture(samplerColorMap, inUV);
	// vec3 N = normalize(inNormal);
	// vec3 L = normalize(inLightVec);
	// vec3 V = normalize(inViewVec);
	// vec3 R = reflect(-L, N);

	// vec3 ambient = vec3(0.5, 0.5, 0.5);
	
	// vec3 diffuse = max(dot(N, L), 0.0) * material.diffuse.rgb;

	// vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * material.specular.rgb;
	// float opacity = 1.0-material.opacity;

	// outFragColor = vec4((ambient + diffuse) * color.rgb + specular, opacity);

	//vec3 lPos = vec3(4.0, 1.0, 0.0);

	


    vec3 color = texture(samplerColorMap, inUV).rgb * inColor;
    // Ambient
    //vec3 ambient = 0.08 * color;
    vec3 ambient = 0.38 * color;

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