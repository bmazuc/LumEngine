#include "SceneNode.h"

SceneNode::SceneNode()
{
	position = { 0.f, 0.f, 0.f };
	rotation = { 0.f, 0.f, 0.f };
	scale = { 1.0f, 1.0f, 1.0f };
	isVisible = true;
	isTransparent = false;
}

SceneNode::~SceneNode()
{

}

void SceneNode::SetPosition(glm::vec3 newPosition)
{
	position = newPosition;
}

glm::vec3 SceneNode::GetPosition()
{
	return position;
}

glm::vec3 SceneNode::GetRotation()
{
	return rotation;
}

void SceneNode::SetRotation(glm::vec3 newEulerAngles)
{
	rotation = newEulerAngles;
}

glm::vec3 SceneNode::GetScale()
{
	return scale;
}

void SceneNode::SetScale(glm::vec3 newScale)
{
	scale = newScale;
}

void SceneNode::SetInitialValue(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale, bool visible)
{
	initialPosition = position;
	initialRotation = rotation;
	initialScale = scale;
	initialIsVisible = visible;

	if (scale.x - scale.y < 0.005f 
		&& scale.y - scale.z < 0.005f
		&& scale.x - scale.z < 0.005f)
	{		
		isScaleHomothety = true;
	}
	else
		isScaleHomothety = false;
}
#include <iostream>
glm::mat4x4 SceneNode::GetTransformation()
{
	glm::mat4x4 transformation = glm::mat4(1.0);

    transformation = glm::scale(transformation, scale);
    
	const glm::vec3 xAxis(1.0, 0.0, 0.0), yAxis(0.0, 1.0, 0.0), zAxis(0.0, 0.0, 1.0);

	transformation = glm::rotate(transformation, glm::radians(rotation.y), yAxis);
	transformation = glm::rotate(transformation, glm::radians(rotation.x), xAxis);
	transformation = glm::rotate(transformation, glm::radians(rotation.z), zAxis);
    
    transformation = glm::translate(transformation, position);

	return transformation;
}

void SceneNode::ResetPosition()
{
	position = initialPosition;
}

void SceneNode::ResetRotation()
{
	rotation = initialRotation;
}

void SceneNode::ResetScale()
{
	scale = initialScale;
}

void SceneNode::Reset()
{
	ResetPosition();
	ResetRotation(); 
	ResetScale();
	isVisible = initialIsVisible;
}
