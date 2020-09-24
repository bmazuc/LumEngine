#pragma once

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

struct LeLight
{
	alignas(16) glm::vec4	color;
	alignas(4)	bool		isVisible;
	alignas(16) glm::vec4	position = glm::vec4(0.f, 0.f, 0.f, 1.f);
	alignas(4)	float		radius;
	alignas(4)	float		intensity = 1.f;
	alignas(16) glm::vec4	rotation;
	alignas(4)	float		outerAngle = 180.f;
	alignas(4)	float		innerAngle;
	alignas(4)	bool		useSoftEdge;
};

enum LightType
{
	Point,
	Spot,
	Directional
};

enum AmbientMode
{
	Flat = 0,
	Trilight,
	CubeMap
	//Skybox
};

struct AmbientUniformBufferObject
{
	/*alignas(16)*/ glm::vec4 skyColor;
	/*alignas(16)*/ glm::vec4 equatorColor;
	/*alignas(16)*/ glm::vec4 groundColor;
	/*alignas(128)*/ glm::vec4 ambientCube[6];
	/*alignas(4)*/ float ka;
	/*alignas(4)*/ int mode;
};

struct LightUniformBufferObject
{
	LeLight	light[9];
};

struct LightPropertyObject
{
    int lightType;
    LeLight* lightData;

	/* /!\ Could change outer / inner angle value */
    void UpdateDataType()
    {
        switch (lightType)
        {
            case Point:
            {
                lightData->position.w = 1.0f;
                lightData->outerAngle = 180.f;
            }
            break;
            case Spot:
            {
                lightData->position.w = 1.0f;
				// OuterAngle is clamp between 1.f and 179.f but we clamp it at 60 here to see more clearly the light type change
				lightData->outerAngle = glm::clamp(lightData->outerAngle, 1.f, 60.f);
                lightData->innerAngle = glm::max(lightData->outerAngle - 10.f, 1.f);
            }
            break;
            case Directional:
            {
                lightData->position.w = 0.0f;
            }
            break;
            default: ;
        }
    }

    void ChangeDataType(LightType newType)
    {
        lightType = newType;
        UpdateDataType();
    }
};

enum BRDF
{
	GGX = 0,
	GGX_KARIS
};

struct LightParamsUniformBufferObject
{
	alignas(4) int		brdf;
	alignas(4) float	gamma;
	alignas(4) bool		useShadow;
};