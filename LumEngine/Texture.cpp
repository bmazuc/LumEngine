#include "Texture.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <algorithm>

Texture::Texture()
{
	CreateEmptyTex();
}

Texture::Texture(std::string filename)
{
	LoadFile(filename);
}

Texture::~Texture()
{
}

void Texture::Clear()
{
	if (data)
	{
		delete[] data;
		data = nullptr;
	}

	buffer.Clear();
}

void Texture::CreateEmptyTex()
{
	Clear();

	width = 1;
	height = 1;
	data = new uint8_t[4];
	for (int i = 0; i < 4; ++i)
		data[i] = 0;
}

void* Texture::GetData()
{
	return data;
}

bool Texture::LoadFile(std::string filename, bool supportMipMap)
{
	int texChannels;

	stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		throw std::runtime_error("échec du chargement d'une image!");
		return false;
	}

	if (supportMipMap)
		mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;

	VkDeviceSize imageSize = GetMemorySize();

	data = new uint8_t[imageSize];
	memcpy(data, pixels, imageSize);

	stbi_image_free(pixels);

	return true;
}

int Texture::GetMemorySize()
{
	return width * height * 4;
}

glm::ivec2 Texture::GetDimensions()
{
	return glm::ivec2(width, height);
}