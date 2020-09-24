#pragma once
#include "SceneNode.h"
#include "Mesh.h"

class MeshSceneNode : public SceneNode
{
public:

	MeshSceneNode(Mesh* mesh = nullptr);
	~MeshSceneNode();

	Mesh* GetMesh();
	BufferHandle uniformNodeMeshStagingBuffer;
	BufferHandle uniformNodeMeshBuffer;

	BufferHandle uniformNodeMaterialStagingBuffer;
	BufferHandle uniformNodeMaterialBuffer;
	

	VkCommandBuffer _copyCommandBuffer = VK_NULL_HANDLE;
	Mesh* mesh;

private:


};

