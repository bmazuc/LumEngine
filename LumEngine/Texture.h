#pragma once

#include "BufferHandle.h"
#include <string>
#include <glm/vec2.hpp>

class Texture
{
public:

	Texture();
	Texture(std::string filename);
	~Texture();

	uint32_t mipLevels = 1;
	BufferHandle buffer;
	VkImageView textureImageView	= VK_NULL_HANDLE;
	VkSampler	textureSampler		= VK_NULL_HANDLE;

	void* GetData();

	bool LoadFile(std::string filename, bool supportMipMap = false);
	int GetMemorySize();

	glm::ivec2 GetDimensions();
	void Clear();
	bool isLoad() { return data != nullptr; }
private:
	int			width = 0;
	int			height = 0;
	uint8_t*	data = nullptr;


	void CreateEmptyTex();
};

