#version 450
#extension GL_ARB_separate_shader_objects : enable
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec4 outPosition;
//layout(location = 2) in vec3 lightColor;
layout(location = 2) in vec3 outCameraPosition;
layout(location = 3) in vec2 fragTexCoord;
//layout(location = 3) in vec3 outNormal;

layout(location = 0) out vec4 outColor;

layout(binding = 2) uniform sampler2D texSampler;

vec3 lightDir = vec3(0.3, -1.0, 0.0);
vec3 cSpec = vec3(0.3, 0.3, 0.3);

void main() 
{
//	vec3 toto = vec3(0.0, 1.0, 0.5) / M_PI;
//	float s = (0.5 + 8.0) / (8.0 * M_PI);
//	
//	vec3 vectorV = outCameraPosition - outPosition.xyz;
//	vec3 vectorL = lightDir - outPosition.xyz;
//	vec3 vectorH = normalize(vec3(vectorV + vectorL));
//	float angleHandNormal = dot(vectorH, outNormal);
//	float cosH = cos(angleHandNormal);
//	
//	float angleBetweenNormalAndLightDir = dot(outNormal, lightDir);
//	float cosI = cos(angleBetweenNormalAndLightDir);
//	
//	vec3 E = fragColor;
//		
//   outColor = vec4((toto + s * cosH * cSpec) * E * cosI, 1.

	vec3 finalColor;

//	finalColor = fragColor;
	finalColor = vec3(1, 1, 1); //fragColor;

	ivec2 texSize = textureSize(texSampler, 0);
	
	if(texSize.x > 2)
	{
		finalColor *= texture(texSampler, fragTexCoord).rgb;
	}


    outColor = vec4(finalColor, 1.0);
//	outColor = vec4(fragColor, 1.0);

}