#include "LeSwapChain.h"
#include <iostream>
#include <fstream>

#if defined(_WIN32)
	#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__APPLE__)
#elif defined(_GNUC_)
#endif

#include <GLFW/glfw3native.h>

#ifdef _DEBUG
	#define DEBUG_CHECK_VK(x) if (VK_SUCCESS != (x)) { std::cout << (#x) << std::endl; __debugbreak(); }
#else
	#define DEBUG_CHECK_VK(x) 
#endif

LeSwapChain::LeSwapChain()
{
}


LeSwapChain::~LeSwapChain()
{
}

void LeSwapChain::CleanUp(VkDevice device)
{
	for (SwapChainBuffer swapChainbuffer : buffers)
	{
		//vkDestroyImage(device, swapChainbuffer.image, nullptr);
		//swapChainbuffer.image = VK_NULL_HANDLE;
		vkDestroyImageView(device, swapChainbuffer.view, nullptr);
		swapChainbuffer.view = VK_NULL_HANDLE;
	}

	vkDestroySwapchainKHR(device, swapChain, nullptr);
	vkDestroySurfaceKHR(instance, surface, nullptr);

	surface = VK_NULL_HANDLE;
	swapChain = VK_NULL_HANDLE;

}

void LeSwapChain::Connect(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device)
{
	if (instance != VK_NULL_HANDLE)
		this->instance = instance;

	if (physicalDevice != VK_NULL_HANDLE)
		this->physicalDevice = physicalDevice;

	if (device != VK_NULL_HANDLE)
		this->device = device;
}

void LeSwapChain::InitializeSurface(GLFWwindow* window)
{
#if defined(USE_GLFW_SURFACE)
	if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
	{
		throw std::runtime_error("échec de la création de la swap chain!");
	}
#else
#if defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR surfaceInfo = {};
	surfaceInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceInfo.hinstance = GetModuleHandle(NULL);
	surfaceInfo.hwnd = glfwGetWin32Window(window);

	DEBUG_CHECK_VK(vkCreateWin32SurfaceKHR(instance, &surfaceInfo, nullptr, &surface));
#endif
#endif
}

void LeSwapChain::InitializeColor()
{
	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, NULL);

	std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
	vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, surfaceFormats.data());

	if ((formatCount == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED))
		colorFormat = VK_FORMAT_B8G8R8A8_UNORM;
	else
		colorFormat = surfaceFormats[0].format;

	colorSpace = surfaceFormats[0].colorSpace;
}

void LeSwapChain::Create(uint32_t* width, uint32_t* height)
{
	VkResult err;
	VkSwapchainKHR oldSwapchain = swapChain;

	uint32_t presentModeCount;
	VkSurfaceFormatKHR surfaceFormats[1];

	VkSurfaceCapabilitiesKHR surfCaps;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfCaps);

	swapchainExtent = surfCaps.currentExtent;

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, NULL);

	std::vector<VkPresentModeKHR> presentModes(presentModeCount);

	vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data());

	VkExtent2D swapchainExtent = {};

	if (surfCaps.currentExtent.width == (uint32_t)-1)
	{
		swapchainExtent.width = *width;
		swapchainExtent.height = *height;
	}
	else
	{
		swapchainExtent = surfCaps.currentExtent;
		*width = surfCaps.currentExtent.width;
		*height = surfCaps.currentExtent.height;
	}

	VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
	
	uint32_t desiredNumberOfSwapchainImages = surfCaps.minImageCount + 1;
	if ((surfCaps.maxImageCount > 0) && (desiredNumberOfSwapchainImages > surfCaps.maxImageCount))
	{
		desiredNumberOfSwapchainImages = surfCaps.maxImageCount;
	}

	VkSurfaceTransformFlagsKHR preTransform;
	if (surfCaps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}
	else
	{
		preTransform = surfCaps.currentTransform;
	}

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.pNext = NULL;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = desiredNumberOfSwapchainImages;
	swapchainCreateInfo.imageFormat = colorFormat;
	swapchainCreateInfo.imageColorSpace = colorSpace;
	swapchainCreateInfo.imageExtent = { swapchainExtent.width, swapchainExtent.height };
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchainCreateInfo.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchainCreateInfo.queueFamilyIndexCount = 0;
	swapchainCreateInfo.pQueueFamilyIndices = NULL;
	swapchainCreateInfo.presentMode = swapchainPresentMode;
	swapchainCreateInfo.oldSwapchain = oldSwapchain;
	swapchainCreateInfo.clipped = VK_TRUE;
	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	DEBUG_CHECK_VK(vkCreateSwapchainKHR(device, &swapchainCreateInfo, nullptr, &swapChain));

	if (oldSwapchain != VK_NULL_HANDLE)
	{
		for (uint32_t i = 0; i < imageCount; i++)
		{
			vkDestroyImageView(device, buffers[i].view, nullptr);
		}
		vkDestroySwapchainKHR(device, oldSwapchain, nullptr);
	}

	DEBUG_CHECK_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr));

	images.resize(imageCount);
	DEBUG_CHECK_VK(vkGetSwapchainImagesKHR(device, swapChain, &imageCount, images.data()));

	buffers.resize(imageCount);
	for (uint32_t i = 0; i < imageCount; i++)
	{
		VkImageViewCreateInfo colorAttachmentView = {};
		colorAttachmentView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		colorAttachmentView.pNext = NULL;
		colorAttachmentView.format = colorFormat;
		colorAttachmentView.components = 
		{
			VK_COMPONENT_SWIZZLE_R,
			VK_COMPONENT_SWIZZLE_G,
			VK_COMPONENT_SWIZZLE_B,
			VK_COMPONENT_SWIZZLE_A
		};
		colorAttachmentView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		colorAttachmentView.subresourceRange.baseMipLevel = 0;
		colorAttachmentView.subresourceRange.levelCount = 1;
		colorAttachmentView.subresourceRange.baseArrayLayer = 0;
		colorAttachmentView.subresourceRange.layerCount = 1;
		colorAttachmentView.viewType = VK_IMAGE_VIEW_TYPE_2D;
		colorAttachmentView.flags = 0;

		buffers[i].image = images[i];

		colorAttachmentView.image = buffers[i].image;

		DEBUG_CHECK_VK(vkCreateImageView(device, &colorAttachmentView, nullptr, &buffers[i].view));
	}
}

VkSwapchainKHR LeSwapChain::GetSwapChain()
{
	return swapChain;
}
