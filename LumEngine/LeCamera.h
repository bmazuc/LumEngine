#pragma once
#include "SceneNode.h"
#include <GLFW/glfw3.h>
#include "InputManager.h"
#include <iostream>

class LeCamera
{
public:
	
	glm::vec3 cameraPos = glm::vec3(0.f, 8.0f, 14.0f);
	glm::vec3 front;
	float speed = 0.2f;

	float yaw = -90.0f;
	float pitch = -32.0f;

	float mouseSpeed = 0.005f;

	bool  firstMouse = true;
	float lastX;
	float lastY;

	enum CameraType
	{
		LOOK_AT,
		VALUE,
		MOUSE
	} type;

	LeCamera()
	{
		type = LOOK_AT;
	}

	~LeCamera() = default;

	void SetInitialWindowSize(float wWidth, float wHeight)
	{
		lastX = wWidth / 2;
		lastY = wHeight / 2;
	}

	void ChangeType(CameraType newType)
	{
		type = newType;
	}

	glm::mat4 ReturnViewMatrix()
	{
		switch (type)
		{
		case LOOK_AT:
			{			
				return glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			}
		case VALUE:
			{			
				ComputeFrontValue();

				return glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			}
		case MOUSE:
			{
				ComputeMouseRotation();
				ComputeCameraKeyInput();
				ComputeFrontValue();
				
				return glm::lookAt(cameraPos, cameraPos + front, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			}
		default:
			{			
				return glm::mat4();
			}
		}
	}

	void ComputeFrontValue()
	{
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		front = glm::normalize(front);
	}

	void ComputeCameraKeyInput()
	{
		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_W))
			cameraPos += front * speed;

		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_S))
			cameraPos -= front * speed;
			   
		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_Q))
			cameraPos.y += 0.5f * speed;

		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_E))
			cameraPos.y -= 0.5f * speed;
				
		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_D))
			cameraPos += glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)) * speed;
			   
		if (InputManager::instance->GetKeyInputDown(GLFW_KEY_A))
			cameraPos -= glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)) * speed;

	}


	void ComputeMouseRotation()
	{
		float xpos, ypos;
		InputManager::instance->GetMousePosition(xpos, ypos);

		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;
		lastX = xpos;
		lastY = ypos;

		float sensitivity = 0.05f;
		xoffset *= sensitivity;
		yoffset *= sensitivity;

		yaw += xoffset;
		pitch += yoffset;

		if (pitch > 89.0f)
			pitch = 89.0f;
		if (pitch < -89.0f)
			pitch = -89.0f;
	}

	void EnterMouseMode(GLFWwindow*	window)
	{
		firstMouse = true;
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	void ExitCameraMode(GLFWwindow*	window)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}
};

