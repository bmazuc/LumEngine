#pragma once
#include <volk.h>
#include <array>

struct FrameBufferAttachment
{
	VkImage image;
	VkDeviceMemory memory;
	VkImageView view;
	VkFormat format;

	void Destroy(VkDevice device)
	{
		vkDestroyImage(device, image, nullptr);
		vkDestroyImageView(device, view, nullptr);
		vkFreeMemory(device, memory, nullptr);
	}
};

class FrameBuffer
{
public:
	int32_t width, height;
	VkFramebuffer frameBuffer;
	FrameBufferAttachment depth;
	VkRenderPass renderPass;
	
	void SetSize(int32_t w, int32_t h)
	{
		this->width = w;
		this->height = h;
	}
	
	void Destroy(VkDevice device)
	{
		depth.Destroy(device);
		vkDestroyFramebuffer(device, frameBuffer, nullptr);
		vkDestroyRenderPass(device, renderPass, nullptr);
	}
};

class OffscreenFrameBuffer : public FrameBuffer
{
public:
	std::array<FrameBufferAttachment, 3> attachments;
};