#version 450
#extension GL_ARB_separate_shader_objects : enable
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inFragTexCoord;
layout(location = 2) in vec3 inPos;
layout(location = 3) in vec3 inEyePos;
layout(location = 4) in vec4 inShadowCoord;
layout(location = 5) in mat4 inCameraSpace;

layout(location = 0) out vec4 outColor;

//struct LeLight
//{
//	vec4	color;
//	bool	isVisible;
//	vec4	position;
//	float	radius;
//	vec4	rotation;
//	float	outerAngle;
//	float	innerAngle;
//	bool	useSoftEdge;
//};

//layout(push_constant) uniform PushConsts {
//	int lightIndex;
//} pushConsts;
//
//layout(binding = 3) uniform LightCubeUniformBufferObject 
//{	
//	LeLight light[9];
//} lbo;

layout(binding = 2) uniform Params
{
	int		brdf;
	float	gamma;
	bool	useShadow;
	bool	useCameraSpace;
} params;

layout(binding = 3) uniform UniformMaterialBuffer
{	
	vec4	color;
	float	reflectance;
	float	perceptual_roughness;
	float	metallic;							// Slider but value other than 0.0 & 1.0 has no real meaning in physics
} material;

//	-------------------------
//	|	Gamma Correction	|
//	-------------------------

vec3 GammaToLinear(vec3 value)
{
	return pow(value, vec3(params.gamma));
}

vec3 LinearToGamma(vec3 value)
{
	return pow(value, vec3(1/params.gamma));
}

//	-------------
//	|	Main	|
//	-------------

void main() 
{		

	vec3 finalColor = vec3(0.f, 0.f, 0.f);

	finalColor = material.color.xyz;
	
	finalColor = LinearToGamma(finalColor);
		
	outColor = vec4(finalColor, 1.0);
}