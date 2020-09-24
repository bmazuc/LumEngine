#pragma once

#include <glm/glm.hpp>
#include "MeshBuffer.h"
#include "LeMaterial.h"

struct UniformNodeVertexBuffer
{
	glm::mat4 model;
};

class Mesh
{
public:
	Mesh() = default;
	~Mesh() = default;

	std::string name = "";

	MeshBuffer* AddMeshBuffer()
	{
		buffers.push_back(MeshBuffer());
		return &buffers[buffers.size() - 1];
	}
	
	size_t GetMeshBufferCount()
	{
		return buffers.size();
	}

	MeshBuffer* GetMeshBuffer(unsigned i)
	{
		if (i >= buffers.size())
			return nullptr;

		return &buffers[i];
	}

	LeMaterial* GetMaterial()
	{
		return material;
	}

	void CreateMaterial()
	{
		material = new LeMaterial();
	}

private:
	std::vector<MeshBuffer> buffers;
	LeMaterial* material;
};

