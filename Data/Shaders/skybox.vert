#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 view;
	mat4 proj;
} ubo;

layout (location = 0) out vec3 outUVW;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	outUVW = inPos;
	outUVW.z *= -1.0;
	
	mat4 fView = mat4(mat3(ubo.view));

	vec4 pos = ubo.proj * fView * vec4(inPos.xyz, 1.0);
	gl_Position = pos.xyww;
	   

	//gl_Position = ubo.proj * fView * vec4(inPos, 1.0);

}
