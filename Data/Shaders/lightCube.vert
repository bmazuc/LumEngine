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

layout(binding = 2) uniform Params
{
	int		brdf;
	float	gamma;
	bool	useShadow;
	bool	useCameraSpace;
} params;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inTangent;

layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outFragTexCoord;
layout(location = 2) out vec3 outPos;
layout(location = 3) out vec3 outEyePos;
layout(location = 4) out vec4 outShadowCoord;
layout(location = 5) out mat4 outCameraSpace;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
	outCameraSpace = ubo.view;

	if (params.useCameraSpace)
		outPos = (ubo.view * nvo.model * vec4(inPosition, 1.0)).xyz;
	else
		outPos = (nvo.model * vec4(inPosition, 1.0)).xyz;

	outFragTexCoord = vec2(inTexCoord.x, 1.0 - inTexCoord.y);
	
	if (params.useCameraSpace)
	{
		mat3 normalMatrix = mat3(transpose(inverse(ubo.view * nvo.model)));
		outNormal = normalMatrix * inNormal;
	}
	else
	{
		mat3 normalMatrix = mat3(transpose(inverse(nvo.model)));
		outNormal = normalMatrix * inNormal;
	}

	if (params.useCameraSpace)
		outEyePos = (ubo.view * vec4(-vec3(ubo.view[3]), 1.0)).xyz;
	else
		outEyePos = -vec3(ubo.view[3]);

	gl_Position = ubo.proj * ubo.view * nvo.model * vec4(inPosition, 1.0);
	
	outShadowCoord = ( ubo.depthVP * nvo.model ) * vec4(inPosition, 1.0);	
}