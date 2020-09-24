#pragma once
//#include "VulkanDriver.h"
#include "MeshSceneNode.h"
#include <list>
#include "LeLight.h"

class Scene
{
public:
	Scene();
	~Scene();

	MeshSceneNode* AddMeshNode(Mesh* meshNode, glm::vec3 position = { 0.f, 0.f, 0.f }, glm::vec3 scale = { 1.f, 1.f, 1.f }, glm::vec3 rotation = { 0.f, 0.f, 0.f });
	MeshSceneNode* AddSkybox(std::string texturePath, Mesh* skyboxMesh);
	MeshSceneNode* AddShadowDebugQuad(Mesh * quadMesh);

	void UpdateLightsCubesTransform();

	//VulkanDriver* vkDriver;
	MeshSceneNode* skyboxNode;
	MeshSceneNode* shadowDebugNode;
	std::list<SceneNode*> lightsCubesNodes;
    LightPropertyObject lightProperty[9];
	std::list<SceneNode*> nodes;
};

