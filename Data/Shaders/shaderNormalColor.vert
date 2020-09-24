#version 450
#extension GL_ARB_separate_shader_objects : enable

struct LePointLight
{
  vec4 color;
  bool isVisible;
  vec4 position;
  float radius;
};

struct LeSpotLight
{
	vec4	color;
	bool	isVisible;
	vec4	position;
	float	radius;
	vec4	rotation;
	float	cutoff;
};

struct  LeDirectionalLight
{
	vec4 color;
	bool isVisible;
	vec4 rotation;	
};

layout(binding = 0) uniform UniformBufferObject 
{
    mat4 view;
    mat4 proj;
} ubo;

layout(binding = 1) uniform UniformNodeVertexBuffer 
{
    mat4 model;
} nvo;

layout(binding = 2) uniform LightUniformBufferObject 
{	
	LePointLight		pointLight[10];
	LeSpotLight			spotLight[10];
	LeDirectionalLight	directionLight[10];
} lbo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inColor;
layout(location = 3) in vec3 inNormal;
//layout(location = 2) in vec3 inCameraPosition;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec4 outPosition;
//layout(location = 2) out vec3 lightColor;
layout(location = 2) out vec3 outCameraPosition;
layout(location = 3) out vec2 fragTexCoord;
//layout(location = 3) out vec3 outNormal;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main()
{
//	outNormal = inNormal;
	outCameraPosition = vec3(2.0, 2.0, 2.0);
	outPosition = gl_Position;

	float lP = lbo.pointLight[0].position.x;
	   
	mat3 normalMatrix = transpose(inverse(mat3(nvo.model)));

	vec3 worldNormal = normalMatrix * inNormal;

    gl_Position = ubo.proj * ubo.view * nvo.model * vec4(inPosition, 1.0);

    fragColor = inColor;
    fragTexCoord = inTexCoord;
}