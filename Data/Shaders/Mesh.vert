#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;	
	mat4 depthVP;
} ubo;

layout(binding = 1) uniform UniformNodeVertexBuffer 
{
    mat4 model;
} nvo;

layout(binding = 5) uniform Params
{
	int		brdf;
	float	gamma;
	bool	useShadow;
} params;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outFragTexCoord;
layout(location = 2) out vec3 outPos;
layout(location = 3) out vec3 outEyePos;
layout(location = 4) out vec4 outShadowCoord;

const mat4 biasMat = mat4( 
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 
);

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
	outPos = (ubo.view * nvo.model * vec4(inPosition, 1.0)).xyz;

	outFragTexCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
	
	mat3 normalMatrix = mat3(ubo.view) * mat3(transpose(inverse(nvo.model)));
	outNormal = normalMatrix * inNormal;

	outEyePos = vec3(0.0);

	gl_Position = ubo.proj * ubo.view * nvo.model * vec4(inPosition, 1.0);
	
	outShadowCoord = ( biasMat * ubo.depthVP * nvo.model ) * vec4(inPosition, 1.0);	
}