#pragma once
#include "BufferHandle.h"

class UniformBufferHandle
{
public:
	UniformBufferHandle()
	{
	}

	~UniformBufferHandle()
	{
	}
	
	BufferHandle	stagingBuffer;
	BufferHandle	buffers;
	void*			data;
	VkCommandBuffer commandBuffer;
};

