#include "VulkanDevice.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include "LeUtils.h"

#ifdef _DEBUG
	#define DEBUG_CHECK_VK(x) if (VK_SUCCESS != (x)) { std::cout << (#x) << std::endl; __debugbreak(); }
#else
	#define DEBUG_CHECK_VK(x) 
#endif

VulkanDevice::VulkanDevice(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	assert(physicalDevice);
	this->physicalDevice = physicalDevice;

	vkGetPhysicalDeviceProperties(physicalDevice, &properties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &features);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

	uint32_t queueFamilyCount;
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	assert(queueFamilyCount > 0);
	queueFamilyProperties.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

	uint32_t extCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, nullptr);
	std::vector<VkExtensionProperties> extensions(extCount);

	if (extCount > 0)
		if (vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extCount, &extensions.front()) == VK_SUCCESS)
			for (auto ext : extensions)
				supportedExtensions.push_back(ext.extensionName);

	for (uint32_t i = 0; i < queueFamilyCount; ++i)
	{
		if ((queueFamilyProperties[i].queueCount > 0) && (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT))
		{
			if (queueFamilyIndices.graphics == UINT32_MAX)
				queueFamilyIndices.graphics = i;

			VkBool32 canPresentSurface;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &canPresentSurface);

			if (canPresentSurface && queueFamilyIndices.present == UINT32_MAX)
				queueFamilyIndices.present = i;

			if (queueFamilyIndices.graphics != UINT32_MAX && queueFamilyIndices.present != UINT32_MAX)
				break;
		}
	}
}


VulkanDevice::~VulkanDevice()
{
	if (commandPool)
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);

	if (logicalDevice)
		vkDestroyDevice(logicalDevice, nullptr);
}

void VulkanDevice::CreateLogicalDevice(VkQueueFlags requestedQueueTypes)
{
	const float queue_priorities[] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = queueFamilyIndices.graphics;
	queueCreateInfo.queueCount = 1;
	queueCreateInfo.pQueuePriorities = queue_priorities;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	const char* device_extensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceInfo.enabledExtensionCount = 1;
	deviceInfo.ppEnabledExtensionNames = device_extensions;
	deviceInfo.pEnabledFeatures = &deviceFeatures;

	DEBUG_CHECK_VK(vkCreateDevice(physicalDevice, &deviceInfo, nullptr, &logicalDevice));

	volkLoadDevice(logicalDevice);
}

uint32_t VulkanDevice::GetMemoryType(uint32_t typeBits, VkMemoryPropertyFlags properties, VkBool32 *memTypeFound)
{
	for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				if (memTypeFound)
				{
					*memTypeFound = true;
				}
				return i;
			}
		}
		typeBits >>= 1;
	}
	if (memTypeFound)
	{
		*memTypeFound = false;
		return 0;
	}
	else
	{
		throw std::runtime_error("Could not find a matching memory type");
	}
}

uint32_t VulkanDevice::GetMemoryTypeIndex(uint32_t typeBits, VkFlags properties)
{
	for (uint32_t i = 0; i < 32; i++)
	{
		if ((typeBits & 1) == 1)
		{
			if ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
			{
				return i;
			}
		}
		typeBits >>= 1;
	}

	return 0;
}

void VulkanDevice::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, BufferHandle& buffer, void* data)
{
	buffer.SetDevice(logicalDevice);

	VkBufferCreateInfo bufferCreateInfo = LeUTILS::BufferCreateInfoUtils(usage, size);
	DEBUG_CHECK_VK(vkCreateBuffer(logicalDevice, &bufferCreateInfo, nullptr, &buffer.buffer));

	VkMemoryRequirements memReqs;
	VkMemoryAllocateInfo memAlloc = LeUTILS::MemoryAllocateInfoUtils();
	vkGetBufferMemoryRequirements(logicalDevice, buffer.buffer, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = GetMemoryType(memReqs.memoryTypeBits, properties);
	DEBUG_CHECK_VK(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &buffer.memory));

	buffer.alignment = memReqs.alignment;
	buffer.size = memAlloc.allocationSize;
	buffer.usageFlags = usage;
	buffer.memoryPropertyFlags = properties;

	if (data != nullptr)
	{
		DEBUG_CHECK_VK(buffer.MapMemory());
		memcpy(buffer.mapped, data, size);
		buffer.UnmapMemory();
	}

	buffer.SetupDescriptor();
	DEBUG_CHECK_VK(buffer.Bind());
}

void VulkanDevice::CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, BufferHandle& buffer, uint32_t arrayLayers, VkImageCreateFlags flags)
{
	buffer.SetDevice(logicalDevice);

	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.samples = numSamples;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = arrayLayers;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageInfo.flags = flags;

	DEBUG_CHECK_VK(vkCreateImage(logicalDevice, &imageInfo, nullptr, &buffer.image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(logicalDevice, buffer.image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, properties);

	DEBUG_CHECK_VK(vkAllocateMemory(logicalDevice, &allocInfo, nullptr, &buffer.memory));

	DEBUG_CHECK_VK(vkBindImageMemory(logicalDevice, buffer.image, buffer.memory, 0));
}

void VulkanDevice::CreateImageView(VkImage image, VkFormat format, VkImageView& imageView, uint32_t mipLevels, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	DEBUG_CHECK_VK(vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView));
}
