#version 450

layout (binding = 1) uniform samplerCube samplerCubeMap;

layout (location = 0) in vec3 inUVW;

layout (location = 0) out vec4 outFragColor;

void main() 
{
//	outFragColor = vec4(1.0, 0.0, 0.0, 1.0);
	outFragColor = texture(samplerCubeMap, inUVW);
}