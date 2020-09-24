#define VULKAN_ENABLE_VALIDATION
#include "VulkanDriver.h"
#include "MeshLoader.h"

int main() 
{
	VulkanDriver vkDriver = VulkanDriver(1820, 980);
	Scene* scene = vkDriver.CreateEmptyInitialScene();

	scene->AddSkybox("", MeshLoader::LoadDefaultCube());
	scene->AddShadowDebugQuad(MeshLoader::LoadDefaultQuad());

	Mesh* ironManMesh = MeshLoader::LoadMesh("../Data/Models/Sphere.FBX");

	float xOffset = 0.f;
	for (int x = 0; x < 7; ++x)
	{
		float yOffset = 0.f;
		for (int y = 0; y < 7; ++y)
		{
			Mesh* sphere = MeshLoader::LoadMesh("../Data/Models/Sphere.FBX");
			sphere->GetMaterial()->params.color = glm::vec4(1.f, 0.f, 0.f, 1.f);
			sphere->GetMaterial()->params.roughness = x / 6.f;
			sphere->GetMaterial()->params.metallic = y / 6.f;
			scene->AddMeshNode(sphere, glm::vec3(-425.f + xOffset, -650.f + yOffset, 0.f), glm::vec3(0.02f, 0.02f, 0.02f));
			yOffset += 130.f;
		}
		xOffset += 130.f;
	}

	Mesh* ironRusty = MeshLoader::LoadMesh("../Data/Models/Sphere.FBX");
	ironRusty->GetMaterial()->texture->LoadFile("../Data/Models/RustyIron/albedo.png");
	ironRusty->GetMaterial()->normalMap->LoadFile("../Data/Models/RustyIron/normal.png");
	ironRusty->GetMaterial()->metallicMap->LoadFile("../Data/Models/RustyIron/metallic.png");
	ironRusty->GetMaterial()->roughnessMap->LoadFile("../Data/Models/RustyIron/roughness.png");
	scene->AddMeshNode(ironRusty, glm::vec3(0.f, 600.f, 0.f), glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.f, 0.f, 0.f));

	// Shadow display
	Mesh* shadowGroundMesh = MeshLoader::LoadMesh("../Data/Models/cube.obj");
	scene->AddMeshNode(shadowGroundMesh, glm::vec3(0.f, 200.f, 0.f), glm::vec3(18.060f, 0.040f, 16.640f), glm::vec3(0.f, 0.f, 0.f));
	Mesh* shadowWallMesh = MeshLoader::LoadMesh("../Data/Models/cube.obj");
	scene->AddMeshNode(shadowWallMesh, glm::vec3(0.0f, 1.27f, -208.5f), glm::vec3(18.060f, 10.4f, 0.04f), glm::vec3(0.f, 0.f, 0.f));
	Mesh* ironManMesh4 = MeshLoader::LoadMesh("../Data/Models/ironman/ironman.fbx");
	scene->AddMeshNode(ironManMesh4, glm::vec3(-4.7f, 8.25f, 0.f), glm::vec3(1.0f, 1.0f, 1.0f), glm::vec3(0.f, 0.f, 0.f));
	Mesh* shadowSphere = MeshLoader::LoadMesh("../Data/Models/Sphere.FBX");
	scene->AddMeshNode(shadowSphere, glm::vec3(-80.f, 450.f, 0.f), glm::vec3(0.02f, 0.02f, 0.02f), glm::vec3(0.f, 0.f, 0.f));

	Mesh* brickCube = MeshLoader::LoadMesh("../Data/Models/cube.obj");
	brickCube->GetMaterial()->texture->LoadFile("../Data/Models/brickwall.jpg");
	brickCube->GetMaterial()->normalMap->LoadFile("../Data/Models/normal_mapping_normal_map.png");
	scene->AddMeshNode(brickCube, glm::vec3(5.0f, 8.85f, 0.f));
	Mesh* texCube = MeshLoader::LoadMesh("../Data/Models/cube.obj");
	texCube->GetMaterial()->texture->LoadFile("../Data/Models/brickwall.jpg");
	scene->AddMeshNode(texCube, glm::vec3(1.8f, 8.850f, 0.f));

	Mesh* window = MeshLoader::LoadMesh("../Data/Models/plane_z.obj");
	window->GetMaterial()->texture->LoadFile("../Data/Models/blending_transparent_window.png");
	scene->AddMeshNode(window, glm::vec3(0.f, 26.f, 18.25f), glm::vec3(0.5f, 0.5f, 0.5f), glm::vec3(0.f, 90.f, 0.f))->isTransparent = true;

	while (!glfwWindowShouldClose(vkDriver.window))
	{
		glfwPollEvents();

		vkDriver.PrepareSceneDrawing();
		vkDriver.PrepareDrawing();
		vkDriver.PrepareMeshDrawing();
		vkDriver.SubmitDrawing();
	}

	return 0;
}