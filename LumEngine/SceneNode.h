#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_XYZW_ONLY
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>

class SceneNode
{
public:
	SceneNode();
	~SceneNode();

	void		SetPosition(glm::vec3 newPosition);
	glm::vec3	GetPosition();
	glm::vec3	GetRotation();
	glm::vec3	GetScale();
	void		SetRotation(glm::vec3 newEulerAngles);
	void		SetScale(glm::vec3 newScale);
	
	void		SetInitialValue(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, bool visible);

	glm::mat4x4 GetTransformation();

	void Reset();
	void ResetPosition();
	void ResetRotation();
	void ResetScale();

	bool isVisible;
	bool isTransparent;
	bool isScaleHomothety;

private:
	glm::vec3 position;
	glm::vec3 rotation;
	glm::vec3 scale;

	glm::vec3	initialPosition;
	glm::vec3	initialRotation;
	glm::vec3	initialScale;
	bool		initialIsVisible;

};

