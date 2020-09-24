#version 450
#extension GL_ARB_separate_shader_objects : enable
#define M_PI 3.1415926535897932384626433832795

layout(location = 0) in vec3 inNormal;
layout(location = 1) in vec2 inFragTexCoord;
layout(location = 2) in vec3 inPos;
layout(location = 3) in vec3 inEyePos;
layout(location = 4) in vec4 inShadowCoord;
layout(location = 5) out vec4 test;

layout(location = 0) out vec4 outColor;

struct LeLight
{
	vec4	color;
	bool	isVisible;
	vec4	position;
	float	radius;
	float	intensity;
	vec4	rotation;
	float	outerAngle;
	float	innerAngle;
	bool	useSoftEdge;
};

layout(binding = 0) uniform UniformBufferObject 
{
    	mat4 view;
    	mat4 proj;	
	mat4 depthVP;
} ubo;

layout(binding = 2) uniform UniformMaterialBuffer
{	
	vec4	color;
	float	reflectance;
	float	perceptual_roughness;
	float	metallic;							// Slider but value other than 0.0 & 1.0 has no real meaning in physics
} material;

layout(binding = 3) uniform LightUniformBufferObject 
{	
	LeLight light[9];
} lbo;

layout(binding = 4) uniform Ambient
{
	vec4	skyColor;
	vec4	equatorColor;
	vec4	groundColor;
	vec4	ambientCube[6];
	float	ka;
	int		mode;
} ambient;

layout(binding = 5) uniform Params
{
	int		brdf;
	float	gamma;
	bool	useShadow;
} params;

layout(binding = 6) uniform sampler2D texSampler;
layout(binding = 7) uniform sampler2D normalMapSampler;
layout(binding = 8) uniform sampler2D specularMapSampler;
layout(binding = 9) uniform sampler2D metallicMapSampler;
layout(binding = 10) uniform sampler2D roughnessMapSampler;

layout(binding = 11) uniform sampler2D shadowMapSampler;

layout (binding = 12) uniform samplerCube samplerCubeMap;

// "Enums"
const int AmbientTrilight = 1;
const int AmbientCubeMap = 2;

const int GGX_Karis = 1;
const int Blinn_Phong = 2;

struct SurfaceOutput
{
	vec3	albedo;
	vec3	normal;
	vec3	specular;
	float	metallic;
	float	roughness;
};

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

//	-------------------------
//	|	Helper functions	|
//	-------------------------

float Saturate(float value)
{
	return clamp(value, 0.0, 1.0);
}

vec3 Saturate(vec3 value)
{
	return clamp(value, 0.0, 1.0);
}

vec3 RotateVectorByVector(vec3 vectorToRotate, vec3 rotation)
{
	float CP = cos(radians(rotation.y));
	float SP = sin(radians(rotation.y));
	float CY = cos(radians(rotation.z));
	float SY = sin(radians(rotation.z));
	float CR = cos(radians(rotation.x));
	float SR = sin(radians(rotation.x));

	mat4 rot = mat4(
		CP * CY,	SR * SP * CY - CR * SY, -(CR * SP * CY + SR * SY),	0.0,
		CP * SY,	SR * SP * SY + CR * CY, CY * SR - CR * SP * SY,		0.0,
		SP,			-SR * CP,				CR * CP,					0.0,
		0.0,		0.0,					0.0,						1.0
	);

	return (rot * vec4(vectorToRotate, 0.0)).xyz;
}

//	---------------------------------
//	|	Surface datas computation	|
//	---------------------------------

vec3 ComputeAlbedo()
{
	vec3 albedo = GammaToLinear(material.color.rgb);

	ivec2 texSize = textureSize(texSampler, 0);
	
	if(texSize.x > 2)
	{
		vec4 tex_color = texture(texSampler, inFragTexCoord);

		albedo *= pow(tex_color.rgb, vec3(params.gamma));
	}

	return albedo;
}

vec3 ComputeNormal()
{
	ivec2 texSize = textureSize(normalMapSampler, 0);
	
	if(texSize.x > 2)
	{
		vec3 q1 = dFdx(inPos);
		vec3 q2 = dFdy(inPos);
		vec2 st1 = dFdx(inFragTexCoord);
		vec2 st2 = dFdy(inFragTexCoord);

		vec3 N = normalize(inNormal);
		vec3 T = normalize(q1 * st2.t - q2 * st1.t);
		vec3 B = -normalize(cross(N, T));
		mat3 TBN = mat3(T, B, N);
		vec3 normal = normalize(2.0 * texture(normalMapSampler, inFragTexCoord).rgb - 1.0);
		return normalize(TBN * normal);
	}
	else
		return normalize(inNormal);
}

vec3 ComputeSpecular()
{
	ivec2 texSize = textureSize(specularMapSampler, 0);
	
	if(texSize.x > 2)
	{
		vec4 tex_color = texture(specularMapSampler, inFragTexCoord);
		return GammaToLinear(tex_color.rgb);
	}

	return vec3(1.0);
}

float ComputeMetallic()
{
	ivec2 texSize = textureSize(metallicMapSampler, 0);
	
	if(texSize.x > 2)
	{
		return texture(metallicMapSampler, inFragTexCoord).r;
	}

	return material.metallic;
}

float ComputeRoughness()
{
	float roughness = material.perceptual_roughness * material.perceptual_roughness;;
	ivec2 texSize = textureSize(roughnessMapSampler, 0);
	
	if(texSize.x > 2)
	{
		float r = texture(roughnessMapSampler, inFragTexCoord).r;
		r = r * r;
		roughness *= r;
	}

	return roughness;
}

float ComputeGlosiness(float roughness)
{
	roughness = max(roughness, 0.005);
	return ((2.0 / (roughness * roughness)) - 2.0);
}

//	-------------
//	|	BRDF	|
//	-------------

vec3 FresnelSchlickApproximation(vec3 f0, float theta)
{
	return f0 + (vec3(1.0) - f0) * pow(1 - theta, 5.0);
}

// roughness != perceptual_roughness <=> roughness is already squared
float NormalDistributionFunction_GGX(float roughness, float NdotH)
{
	float sqrNdotH = NdotH * NdotH;
	float sqrRoughnessMinusOne = (roughness * roughness) - 1;

	float d = sqrNdotH * sqrRoughnessMinusOne + 1;

	return (roughness * roughness) / (d * d);
}

// voir https://blog.selfshadow.com/publications/s2013-shading-course/karis/s2013_pbs_epic_notes_v2.pdf
float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotX;
    float denom = NdotX * (1.0 - k) + k;

    return nom / denom;
}

float SmithGGXKaris(float NdotL, float NdotV, float roughness)
{
	float ggxL = GeometrySchlickGGX(NdotL, roughness);
	float ggxV = GeometrySchlickGGX(NdotV, roughness);

	return ggxL * ggxV;
}

float SmithGGXCorrelated(float NdotL, float NdotV, float roughness)
{
	float r2 = roughness * roughness;
	//the " NdotL *" and " NdotV *" are explicitely inversed
	float ggxV = NdotL * sqrt((NdotV - NdotV * r2) * NdotV + r2);
	float ggxL = NdotV * sqrt((NdotL - NdotL * r2) * NdotL + r2);

	return 0.5f / (ggxV + ggxL);
}

vec3 ComputeDiffuseColor(vec3 baseColor, float metallic)
{
	return mix(baseColor, vec3(0.0), metallic);
}

vec3 ComputeSpecularColor(vec3 baseColor, float metallic)
{
	return mix(vec3(1.0), baseColor, metallic);
}

vec3 RemapReflectance(float reflectance, vec3 baseColor, float metallic)
{
	return mix(vec3(0.16 * reflectance * reflectance), baseColor, metallic);
}

vec3 ComputeCookTorranceSpecular(float roughness, vec3 f0, float NdotL, float NdotV, float VdotH, float NdotH)
{
	float D = NormalDistributionFunction_GGX(roughness, NdotH);
	vec3 F = FresnelSchlickApproximation(f0, VdotH);
	float G;
	if (params.brdf == GGX_Karis)
	{
		float G = SmithGGXKaris(NdotL, NdotV, roughness);
		return (D*F*G) / (4 * NdotL * NdotV);

	}
	else
	{
		float G = SmithGGXCorrelated(NdotL, NdotV, roughness);
		return (D*F*G);
	}
}

//	-------------
//	|	Shadow	|
//	-------------

#define shadowAmbient 0.1
float textureProj(vec4 shadowCoord, vec2 off)
{
	float shadow = 1.0;
	if ( shadowCoord.z > -1.0 && shadowCoord.z < 1.0 ) 
	{
		float dist = texture( shadowMapSampler, shadowCoord.st + off ).r;
		if ( shadowCoord.w > 0.0 && dist < shadowCoord.z ) 
		{
			shadow = shadowAmbient;
		}
	}
	return shadow;
}

float filterPCF(vec4 sc)
{
	ivec2 texDim = textureSize(shadowMapSampler, 0);
	float scale = 1.5;
	float dx = scale * 1.0 / float(texDim.x);
	float dy = scale * 1.0 / float(texDim.y);

	float shadowFactor = 0.0;
	int count = 0;
	int range = 1;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += textureProj(sc, vec2(dx*x, dy*y));
			count++;
		}
	
	}
	return shadowFactor / count;
}


//	-------------
//	|	Light	|
//	-------------

float SmoothDistanceAttenuation(float sqrDist, float invSqrRadius)
{
	float factor = sqrDist * invSqrRadius;
	float smoothFactor = Saturate(1.0 - factor * factor);
	return smoothFactor * smoothFactor;
}

float GetDistanceAttenuation(float dist, float invSqrRadius)
{
	float sqrDist = dist * dist;
	float attenuation = 1.0 / max(sqrDist, 0.01*0.01);
	attenuation *= SmoothDistanceAttenuation(sqrDist, invSqrRadius);

	return attenuation;
}

float GetAngleAttenuation(float theta, float outerAngle,float innerAngle)
{
	float cosOuter = cos(radians(outerAngle));
	float cosInner = cos(radians(innerAngle));

	float epsilon = max(cosInner - cosOuter, 0.001);
	return clamp((theta - cosOuter) / epsilon, 0.0, 1.0); // use mix() instead?
}

void ComputeLightType(LeLight light, inout vec3 fragToLight, inout float attenuation)
{
	float shadow = 1.0;

	if (params.useShadow)
	{
		shadow = filterPCF(inShadowCoord / inShadowCoord.w);
	}

	if (light.position.w > 0.0)	// Punctual light
	{
		if (light.outerAngle < 179.0) // Spot light
		{
			fragToLight = (ubo.view * light.position).xyz - inPos;

			// Invert cause direction is a direction pointing from the light source
			vec3 coneDirection = RotateVectorByVector(vec3(0.0, 0.0, 1.0), light.rotation.xyz);
			coneDirection = normalize((ubo.view * vec4(coneDirection, 0.0)).xyz);

			float theta = dot(normalize(fragToLight), coneDirection);

			if (light.useSoftEdge)
			{
				float dist = length(fragToLight);
				float invSqrRadius = 1 / (light.radius * light.radius); 
				attenuation = GetDistanceAttenuation(dist, invSqrRadius);

				attenuation *= GetAngleAttenuation(theta, light.outerAngle, light.innerAngle);
			}
			else
			{
				// use cos becausethe dot product returns a cosine value and not an angle so we can't directly compare an angle with a cosine value. 
				// To retrieve the angle we then have to calculate the inverse cosine of the dot product's result which is an expensive operation.
				if (theta > cos(radians(light.outerAngle)))
				{
					float dist = length(fragToLight);
					float invSqrRadius = 1 / (light.radius * light.radius); 
					attenuation = GetDistanceAttenuation(dist, invSqrRadius);
				}
				else
				{
					attenuation = 0.0;
				}
			}
		}
		else // Point light
		{
			fragToLight = (ubo.view * light.position).xyz - inPos;

			float dist = length(fragToLight);
			float invSqrRadius = 1 / (light.radius * light.radius); 
			attenuation = GetDistanceAttenuation(dist, invSqrRadius);
		}
	}
	else // Directional Light
	{
		// Invert cause direction is a direction pointing from the light source
		vec3 direction = RotateVectorByVector(vec3(0.0, 0.0, 1.0), light.rotation.xyz);
		fragToLight = (ubo.view * vec4(direction, 0.0)).xyz;

		attenuation = 1.0;
	}
	
	attenuation *= shadow;
	
}

vec3 ComputeLight(LeLight light, SurfaceOutput o, vec3 V)
{
	vec3 color = GammaToLinear(light.color.rgb);
	vec3 f0 = RemapReflectance(material.reflectance, color, o.metallic);
	vec3 fragToLight;
	float attenuation;

	ComputeLightType(light, fragToLight, attenuation);

	vec3 L = normalize(fragToLight);
	vec3 H = normalize(L + V);

	float NdotL = max(dot(o.normal,L), 0.001);
	float NdotH = max(dot(o.normal,H), 0.001);
	float VdotH = max(dot(V,H), 0.001);
	float NdotV = max(dot(o.normal,V), 0.001);

	vec3 kd = vec3(1.0) - FresnelSchlickApproximation(f0, NdotL);
	vec3 diffuse = kd * ComputeDiffuseColor(color, o.metallic) * o.albedo;
	vec3 specular = ComputeCookTorranceSpecular(o.roughness, f0, NdotL, NdotV, VdotH, NdotH) * o.specular;

	return (diffuse + specular) * NdotL * light.intensity * attenuation;
}

vec3 CalculateAmbientLight(SurfaceOutput o)
{
	vec3 ambientSky = GammaToLinear(ambient.skyColor.rgb);	
	vec3 ambientEquator = GammaToLinear(ambient.equatorColor.rgb);	
	vec3 ambientGround = GammaToLinear(ambient.groundColor.rgb);	

	if (ambient.mode == AmbientCubeMap)
	{
		vec3 sqrNormal = o.normal * o.normal;
		ivec3 isNegative = ivec3(o.normal.x < 0.0,
							o.normal.y < 0.0,
							o.normal.z < 0.0);

		return (sqrNormal.x * GammaToLinear(ambient.ambientCube[isNegative.x].rgb) 
		+ sqrNormal.y * GammaToLinear(ambient.ambientCube[isNegative.y + 2].rgb) 
		+ sqrNormal.z * GammaToLinear(ambient.ambientCube[isNegative.z + 4].rgb)) 
		* ambient.ka * o.albedo;
	}
	else if (ambient.mode == AmbientTrilight)
	{
		//Magic constants used to tweak ambient to approximate pixel shader spherical harmonics
		vec3 worldUp = vec3(0.0, 1.0, 0.0);
		float skyGroundDotMul = 2.5;
		float minEquatorMix = 0.5;
		float equatorColorBlur = 0.33;
     
		float upDot = dot(o.normal, worldUp);

		//Fade between a flat lerp from sky to ground and a 3 way lerp based on how bright the equator light is.
		//This simulates how directional lights get blurred using spherical harmonics
		//Work out color from ground and sky, ignoring equator
		float adjustedDot = upDot * skyGroundDotMul;
		vec3 skyGroundColor = mix(ambientGround, ambientSky, Saturate((adjustedDot + 1.0) * 0.5));

		//Work out equator lights brightness
		float equatorBright = Saturate(dot(ambientEquator, ambientEquator));

		//Blur equator color with sky and ground colors based on how bright it is.
		vec3 equatorBlurredColor = mix(ambientEquator, Saturate(ambientEquator + ambientGround + ambientSky), equatorBright * equatorColorBlur);

		//Work out 3 way lerp inc equator light
		float smoothDot = pow(abs(upDot), 1.0);
		vec3 equatorColor = mix(equatorBlurredColor, ambientGround, smoothDot) * step(upDot, 0) + mix(equatorBlurredColor, ambientSky, smoothDot) * step(0, upDot);
		  
		return mix(skyGroundColor, equatorColor, Saturate(equatorBright + minEquatorMix)) * ambient.ka * o.albedo;
	}
	else
	{
		//Flat ambient is just the sky color
		return ambientSky * ambient.ka * o.albedo;
	}
}

vec3 Reflect(vec3 normal, vec3 V)
{
	vec3 R = mat3(inverse(ubo.view)) * reflect(-V, normalize(normal));
	R.z *= -1;
    return texture(samplerCubeMap, R).rgb;
}



//	-------------
//	|	Main	|
//	-------------

void main() 
{
	SurfaceOutput o;
	o.albedo = ComputeAlbedo();
	o.normal = ComputeNormal();
	o.specular = ComputeSpecular();
	o.metallic = ComputeMetallic();
	o.roughness = ComputeRoughness();

	// Cause eye pos = vec3(0.0) in camera space
	vec3 V = normalize(-inPos);

	vec3 finalColor = CalculateAmbientLight(o);
	
	for	(int i = 0; i < 9; ++i)
	{
		LeLight light = lbo.light[i];
		if(light.isVisible)
			finalColor += ComputeLight(light, o, V);
	}

	if(o.metallic > 0.f)
	{
		finalColor += Reflect(o.normal, V) * o.metallic;
	}

	finalColor = LinearToGamma(finalColor);

	//if (params.useShadow)
	//{
	//	float shadow = filterPCF(inShadowCoord / inShadowCoord.w);//, vec2(0.0));
	//	finalColor *= shadow;
	//}
	
	outColor = vec4(finalColor, 1.0);
}