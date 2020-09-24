#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>

class BufferHandle
{
public:

	VkDeviceMemory memory = VK_NULL_HANDLE;

	VkBuffer buffer = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	void* mapped = nullptr;

	VkDeviceSize alignment = 0;
	VkDescriptorBufferInfo descriptor;

	VkBufferUsageFlags usageFlags;
	VkMemoryPropertyFlags memoryPropertyFlags;

	size_t size = 0;

	void SetDevice(VkDevice vkDevice)
	{
		if (device == VK_NULL_HANDLE)
			device = vkDevice;
	}

	VkResult MapMemory(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		return vkMapMemory(device, memory, offset, size, 0, &mapped);
	}

	void UnmapMemory()
	{
		if (mapped)
		{
			vkUnmapMemory(device, memory);
			mapped = nullptr;
		}
	}

	VkResult Bind(VkDeviceSize offset = 0)
	{
		return vkBindBufferMemory(device, buffer, memory, offset);
	}

	void SetupDescriptor(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0)
	{
		descriptor.offset = offset;
		descriptor.buffer = buffer;
		descriptor.range = size;
	}

	void Clear()
	{
		if (buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, buffer, nullptr);
			buffer = VK_NULL_HANDLE;
		}
		if (image != VK_NULL_HANDLE)
		{
			vkDestroyImage(device, image, nullptr);
			image = VK_NULL_HANDLE;
		}
		if (memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, memory, nullptr);
			memory = VK_NULL_HANDLE;
		}
	}

private:
	VkDevice device;

};