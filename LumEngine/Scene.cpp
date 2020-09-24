#include "Scene.h"

Scene::Scene()
{
}


Scene::~Scene()
{

}

MeshSceneNode* Scene::AddMeshNode(Mesh* meshNode, glm::vec3 position, glm::vec3 scale, glm::vec3 rotation)
{
	MeshSceneNode* newMeshSceneNode = new MeshSceneNode(meshNode);
	newMeshSceneNode->SetPosition(position);
	newMeshSceneNode->SetScale(scale);
	newMeshSceneNode->SetRotation(rotation);
	newMeshSceneNode->SetInitialValue(position, rotation, scale, true);
	//vk->prepareMeshSceneNode(newMeshSceneNode);
	nodes.push_back(newMeshSceneNode);
	return newMeshSceneNode;

}

MeshSceneNode* Scene::AddSkybox(std::string texturePath, Mesh* skyboxMesh)
{
	MeshSceneNode* sBMesh = new MeshSceneNode(skyboxMesh);
	skyboxNode = sBMesh;
	return sBMesh;
}

MeshSceneNode* Scene::AddShadowDebugQuad(Mesh* quadMesh)
{
	MeshSceneNode* qdMesh = new MeshSceneNode(quadMesh);
	shadowDebugNode = qdMesh;
	return qdMesh;
}

void Scene::UpdateLightsCubesTransform()
{
	int lightIndex = 0;
	for (auto node : lightsCubesNodes)
	{
		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);

		LeLight* lightData = lightProperty[lightIndex].lightData;

		node->SetPosition(lightData->position);
		meshNode->GetMesh()->GetMaterial()->params.color = lightData->color;

		++lightIndex;
	}
}
