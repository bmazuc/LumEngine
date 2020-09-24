#pragma once
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <map>
#include <vector>
#include <imgui.h>


class InputManager
{
public:
	static InputManager* instance;

	GLFWwindow* window;

	InputManager(GLFWwindow* window, std::vector<int> watchedKeys);
	~InputManager();

	bool GetKeyInputDown(int keyID);
	void GetMousePosition(float& xpos, float &ypos);

private:
	std::map<int, bool> activeKeys;
	float xPos;
	float yPos;
	void SetKeyInputDown(int keyID, bool isDown);

	static void MouseCallback(GLFWwindow* window, int button, int action, int mods);
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void	MouseCallback(GLFWwindow* window, double xpos, double ypos);
};

