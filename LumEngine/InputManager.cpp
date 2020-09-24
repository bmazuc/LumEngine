#include "InputManager.h"

InputManager* InputManager::instance;

InputManager::InputManager(GLFWwindow* window, std::vector<int> watchedKeys)
{
	this->window = window;

	if (watchedKeys.size() < 1)
		return;

	for (size_t i = 0; i < watchedKeys.size(); i++)
		activeKeys[watchedKeys[i]] = false;

	glfwSetKeyCallback(window, KeyCallback);
	glfwSetMouseButtonCallback(window, MouseCallback);
	glfwSetCursorPosCallback(window, MouseCallback);
}


InputManager::~InputManager()
{

}

bool InputManager::GetKeyInputDown(int keyID)
{
	bool isDown = false;
	std::map<int, bool>::iterator it = activeKeys.find(keyID);
	if (it != activeKeys.end())
		isDown = activeKeys[keyID];

	return isDown;
}

void InputManager::GetMousePosition(float& xpos, float &ypos)
{
	xpos = xPos;
	ypos = yPos;
}

void InputManager::SetKeyInputDown(int keyID, bool isDown)
{
	std::map<int, bool>::iterator it = activeKeys.find(keyID);
	if (it != activeKeys.end())
		activeKeys[keyID] = isDown;
}

void InputManager::MouseCallback(GLFWwindow* window, int button, int action, int mods)
{
	instance->SetKeyInputDown(button, action != GLFW_RELEASE);
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
		io.KeysDown[key] = true;
	if (action == GLFW_RELEASE)
		io.KeysDown[key] = false;

	instance->SetKeyInputDown(key, action != GLFW_RELEASE);
}

void InputManager::MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
	instance->xPos = xpos;
	instance->yPos = ypos;
}
