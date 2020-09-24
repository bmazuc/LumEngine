#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include <vector>
#include "BufferHandle.h"

class VulkanDevice
{
public:
	VulkanDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	~VulkanDevice();

	VkPhysicalDevice physicalDevice;

	VkDevice logicalDevice;

	VkPhysicalDeviceProperties			 properties;
	VkPhysicalDeviceFeatures			 features;
	VkPhysicalDeviceMemoryProperties	 memoryProperties;
	std::vector<VkQueueFamilyProperties> queueFamilyProperties;
	std::vector<std::string>			 supportedExtensions;
	VkSampleCountFlagBits				 msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	
	VkCommandPool commandPool = VK_NULL_HANDLE;

	struct
	{
		uint32_t graphics = UINT32_MAX;
		uint32_t present = UINT32_MAX;
		//uint32_t compute;
		//uint32_t transfer;
	} queueFamilyIndices;

	void CreateLogicalDevice(VkQueueFlags requestedQueueTypes = VK_QUEUE_GRAPHICS_BIT /*| VK_QUEUE_COMPUTE_BIT*/);
	uint32_t GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32* memTypeFound = nullptr);
	uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkFlags properties);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, BufferHandle& buffer, void *data = nullptr);
	void CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, BufferHandle& buffer, uint32_t arrayLayers, VkImageCreateFlags flags);
	void CreateImageView(VkImage image, VkFormat format, VkImageView& imageView, uint32_t mipLevels, VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT);
};

