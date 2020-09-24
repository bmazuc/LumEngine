#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;

layout (location = 0) out vec2 outUV;

out gl_PerVertex 
{
	vec4 gl_Position;   
};

void main() 
{
	//outUV = vec2((gl_VertexIndex << 1) & 2, gl_VertexIndex & 2);
	outUV = inTexCoord;
	gl_Position = vec4(inPosition, 1.0f);
}
