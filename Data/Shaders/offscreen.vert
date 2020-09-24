#version 450

layout(location = 0) in vec3 inPosition;

layout (set = 0, binding = 0) uniform UBO 
{
	mat4 depthVP;
	int lightType;
} ubo;

layout(set = 1, binding = 1) uniform UniformNodeVertexBuffer 
{
    mat4 model;
} nvo;

out gl_PerVertex 
{
    vec4 gl_Position;
};

 
void main()
{
	if(ubo.lightType == 1) // SPOT LIGHT
	{
		gl_Position = ubo.depthVP * nvo.model * vec4(inPosition, 1.f);
	}
	else if	(ubo.lightType == 2) // DIRECTIONAL LIGHT
	{
		gl_Position = ubo.depthVP * nvo.model * vec4(inPosition, 1.0);
	}
	else
	{
		gl_Position = vec4(0.f, 0.f, 0.f, 0.f);
	}
}