#pragma once

#define VK_NO_PROTOTYPES
#include <volk.h>

#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "BufferHandle.h"
#include "Texture.h"

struct Vertex
{
	glm::vec3 pos;
	glm::vec2 uv;
	glm::vec3 color;
	glm::vec3 normal;
};

class MeshBuffer
{
public:
	MeshBuffer() = default;
	~MeshBuffer() = default;

	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	BufferHandle vertexBuffer;
	BufferHandle indexBuffer;

	VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

private:


};
