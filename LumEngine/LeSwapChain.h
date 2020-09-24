#pragma once
#define VK_NO_PROTOTYPES
#include "volk.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>

struct SwapChainBuffer 
{
	VkImage image;
	VkImageView view;
};

class LeSwapChain
{
public:
	LeSwapChain();
	~LeSwapChain();

	void CleanUp(VkDevice device);

	VkFormat colorFormat;
	VkColorSpaceKHR colorSpace;
	VkSurfaceKHR surface;
	uint32_t imageCount;
	std::vector<SwapChainBuffer> buffers;
	VkExtent2D swapchainExtent;

	void Connect(VkInstance instance = VK_NULL_HANDLE, VkPhysicalDevice physicalDevice = VK_NULL_HANDLE, VkDevice device = VK_NULL_HANDLE);
	void InitializeSurface(GLFWwindow* window);
	void InitializeColor();
	void Create(uint32_t *width, uint32_t *height);
	VkSwapchainKHR GetSwapChain();
private:
	VkInstance instance;
	VkDevice device;
	VkPhysicalDevice physicalDevice;
	
	VkSwapchainKHR swapChain = VK_NULL_HANDLE;
	
	std::vector<VkImage> images;

	struct
	{
		uint32_t graphics;
		uint32_t present;
	} queueFamilyIndices;
};

