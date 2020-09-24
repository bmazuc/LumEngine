#include "VulkanDriver.h"
#include "LeUtils.h"
#include "MeshBuffer.h"

#include <iostream>
#include <fstream>
#include <cassert>
#include <array>
#include "LeMaterial.h"
#include <glm/common.hpp>
#include <glm/common.hpp>
#include "MeshLoader.h"

#define VULKAN_ENABLE_VALIDATION

#ifdef _DEBUG
	#define DEBUG_CHECK_VK(x) if (VK_SUCCESS != (x)) { std::cout << "Fatal : VkResult is \"" << LeUTILS::GetErrorString(x) << "\" in " << __FILE__ << " at line " << __LINE__ << std::endl; assert((x) == VK_SUCCESS); }
#else
	#define DEBUG_CHECK_VK(x) 
#endif

static void check_vk_result(VkResult err)
{
	if (err == 0) return;
	printf("VkResult %d\n", err);
	if (err < 0)
		abort();
}

VulkanDriver::VulkanDriver(unsigned int width, unsigned int height)
{
	DEBUG_CHECK_VK(volkInitialize());

	windowWidth = width;
	windowHeight = height;

	// Initialize the engine initial objects
	drvCreateWindow();
	
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsClassic();
	ImGui_ImplGlfw_InitForVulkan(window, true);

	InputManager::instance = new InputManager(window, 
		{ GLFW_KEY_ESCAPE, GLFW_MOUSE_BUTTON_RIGHT, GLFW_KEY_Q, GLFW_KEY_W, GLFW_KEY_E, GLFW_KEY_A, GLFW_KEY_S, GLFW_KEY_D });

	CreateInstance();
	swapChain.Connect(instance, VK_NULL_HANDLE, VK_NULL_HANDLE);
	swapChain.InitializeSurface(window);
	CreateDebugCallback();
	CreatePhysicalDevice();
	GetPhysicalDeviceLimitations();
	CreateQueues();
	swapChain.Connect(VK_NULL_HANDLE, physicalDevice, logicalDevice);
	swapChain.InitializeColor();

	camera.SetInitialWindowSize(windowWidth, windowHeight);
    	
	// Prepare frame render objects and logic
	CreateCommandPool();
	CreateCommandBuffer(setupCommandBuffer, true);
	CreateTextureSampler();
	swapChain.Create(&windowWidth, &windowHeight);
	CreateDescriptorPool();
	InitilizeRessourcesManager();
	drawCommandBuffer.resize(swapChain.imageCount);
	CreateCommandBuffer(drawCommandBuffer.data(), drawCommandBuffer.size());
	CreateSyncObjects();
	SetupDepthStencilFormat();
	CreatePipelineCache();
	FlushCommanderBuffer(setupCommandBuffer, graphicQueue, true, true);
	CreateCommandBuffer(setupCommandBuffer, true);
	SetupVertexDescriptions();
	PrepareSceneUniformBuffer();

	// Prepare Scene render objects
	CreateMsaaRessources();
	CreateRenderPass();
	CreateSceneDescriptorSetLayout();
	CreateGraphicPipeline();

	// Prepare skybox render objects
	CreateSkyboxDescriptorSetLayout();
	CreateSkyboxPipeline();
	CreateSkyboxSampler();
	LoadCubeMap();

	// Prepare Light Cube render objects
	CreateLightCubeDescriptorSetLayout();
	CreateLightCubePipeline();

	// Create Screne rendering frame buffer destination
	CreateFrameBuffer();
	
	// Create quad for light shadow map debug
	CreateQuadDebugShadowDescriptorSetLayout();
	CreateQuadDebugShadowPipeline();

	// Prepare Shadow render objects
	PrepareOffscreenRendering();
	CreateShadowDescriptorSetLayout();
	CreateShadowPipeline();
	   
	InitializeImGui();
}

VulkanDriver::~VulkanDriver()
{
	CleanUp();
}

void VulkanDriver::drvCreateWindow()
{
	glfwInit();
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	window = glfwCreateWindow(windowWidth, windowHeight, "LumEngine", NULL, NULL);
}

Scene* VulkanDriver::CreateEmptyInitialScene()
{
	Scene* newScene = new Scene();
    currentScene = newScene;

    for (size_t i = 0; i < 9; i++)
    {
        newScene->lightProperty[i].lightData = &lightUniformBufferObject.light[i];
		
		MeshSceneNode* newMeshSceneNode = new MeshSceneNode(MeshLoader::LoadDefaultCube());
		newMeshSceneNode->SetPosition(glm::vec3(0.f, 0.f, 0.f));
		newMeshSceneNode->SetScale(glm::vec3(0.8f, 0.8f, 0.8f));
		newMeshSceneNode->SetRotation(glm::vec3(0.f, 0.f, 0.f));
		newMeshSceneNode->SetInitialValue(glm::vec3(0.f, 0.f, 0.f), glm::vec3(0.f, 0.f, 0.f), glm::vec3(1.f, 1.f, 1.f), true);

		currentScene->lightsCubesNodes.push_back(newMeshSceneNode);
    }
	
    SetupSampleValues();
	return newScene;
}

void VulkanDriver::CreateImGuiInterface()
{
	if (hideGUI)
		return;

	ImGui::Begin("Cam - Lights parameters", nullptr, ImVec2(475.f, windowHeight), 0.55f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoTitleBar);
	ImGui::SetWindowPos(ImVec2(0.f, 0.f), true);
	ImGui::Text("Camera parameter");
	ImGui::SameLine();
	
	ImGui::RadioButton("Look At", &cameraButtonValue, 0); ImGui::SameLine();
	ImGui::RadioButton("Value", &cameraButtonValue, 1); ImGui::SameLine();
	ImGui::RadioButton("Mouse", &cameraButtonValue, 2);
	ImGui::SameLine();
	ImGui::InputFloat("Speed##cameraSpeed", &camera.speed);

	if (static_cast<int>(camera.type) != cameraButtonValue)
	{
		if (cameraButtonValue == camera.MOUSE)
		{
			hideGUI = true;
			camera.EnterMouseMode(window);
		}

		camera.type = static_cast<LeCamera::CameraType>(cameraButtonValue);
	}

	ImGui::DragFloat3("Camera position", glm::value_ptr(camera.cameraPos));
		
	//ImGui::DragFloat("Camera Yaw", &camera.yaw);
	//ImGui::SameLine();
	//ImGui::DragFloat("Camera Pitch", &camera.pitch);

	ImGui::Spacing();

	ImGui::Separator();

	// MeshNode parameters
	int meshCount = currentScene->nodes.size();	   
	ImGui::BeginChild("Child2", ImVec2(0.f, 20.f + meshCount * 20.f), true);
	ImGui::Text("MeshNode list");
	ImGui::Spacing();
	int nodeIndex = 0;

	static int nodeSelected = -1;

	int nodeNameIndex = 0;
	std::vector<std::string> meshNames = std::vector<std::string>(meshCount);

	for (SceneNode* node : currentScene->nodes)
	{
		MeshSceneNode* meshSceneNode = static_cast<MeshSceneNode*>(node);
		meshNames[nodeNameIndex] = meshSceneNode->mesh->name;
		++nodeNameIndex;
	}

	for (int n = 0; n < meshCount; n++)
	{
		std::string selName = "Mesh : " + meshNames[n] + "##" + std::to_string(n);
		if (ImGui::Selectable(selName.c_str(), nodeSelected == n))
			nodeSelected = n;
	}
	
	ImGui::EndChild();

	int totalLightsCount = sizeof(lightUniformBufferObject.light) / sizeof(LeLight);
	std::vector<std::string> lightsNames = std::vector<std::string>();

	for (size_t i = 0; i < totalLightsCount; i++)
		lightsNames.push_back("Light " + std::to_string(i));
	
	static int lightSelected = -1;

	// Point Light parameters
	ImGui::BeginChild("Child3", ImVec2(0.f, 190.f), true);
	ImGui::Text("Point Light parameters");
	ImGui::Spacing();

	for (int n = 0; n < totalLightsCount; n++)
	{
		std::string ligName = lightsNames[n] + "##" + std::to_string(n);
		if (ImGui::Selectable(ligName.c_str(), lightSelected == n))
			lightSelected = n;
	}

	ImGui::EndChild();

	// Ambient Light parameters
	float aLightHeightSize;
	if (ambientUniformBufferObject.mode == 0)
		aLightHeightSize = 105.f;
	else if (ambientUniformBufferObject.mode == 1)
		aLightHeightSize = 150.f;
	else if (ambientUniformBufferObject.mode == 2)
		aLightHeightSize = 210.f;

	ImGui::BeginChild("Child6", ImVec2(0.f, aLightHeightSize), true);
	ImGui::Text("Ambient Light parameters");
	ImGui::Spacing();

	if (ambientUniformBufferObject.mode == 0 || ambientUniformBufferObject.mode == 1)
	{
		std::string skyString = "Sky Color";
		ImGui::ColorEdit3(skyString.c_str(), glm::value_ptr(ambientUniformBufferObject.skyColor));
	}

	if (ambientUniformBufferObject.mode == 1)
	{
		std::string equatorString = "Equator Color";
		ImGui::ColorEdit3(equatorString.c_str(), glm::value_ptr(ambientUniformBufferObject.equatorColor));

		std::string groundString = "Ground Color";
		ImGui::ColorEdit3(groundString.c_str(), glm::value_ptr(ambientUniformBufferObject.groundColor));
	}

	std::string collapName = "CubeMap";
	if (ambientUniformBufferObject.mode == 2)
	{
		std::string colorString0 = "Color (+X)";
		ImGui::ColorEdit3(colorString0.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[0]));

		std::string colorString1 = "Color (-X)";
		ImGui::ColorEdit3(colorString1.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[1]));

		std::string colorString2 = "Color (+Y)";
		ImGui::ColorEdit3(colorString2.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[2]));

		std::string colorString3 = "Color (-Y)";
		ImGui::ColorEdit3(colorString3.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[3]));

		std::string colorString4 = "Color (+Z)";
		ImGui::ColorEdit3(colorString4.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[4]));

		std::string colorString5 = "Color (-Z)";
		ImGui::ColorEdit3(colorString5.c_str(), glm::value_ptr(ambientUniformBufferObject.ambientCube[5]));
	}

	std::string kaString = "Ka";
	ImGui::SliderFloat(kaString.c_str(), &ambientUniformBufferObject.ka, 0.f, 1.f);

	std::string modeString = "Mode";
	ImGui::Combo(modeString.c_str(), &ambientUniformBufferObject.mode, "Flat\0Trilight\0CubeMap\0");

	ImGui::EndChild();

	ImGui::BeginChild("Child7", ImVec2(0, 130), true/*, ImGuiWindowFlags_MenuBar*/);
	ImGui::Text("Global lights parameters");
	ImGui::Spacing();

	std::string brdfString = "BRDF";
	ImGui::Combo(brdfString.c_str(), &lightParamsUniformBufferObject.brdf, "GGX\0GGX_Karis\0");

	std::string gammaString = "Gamma";
	ImGui::DragFloat(gammaString.c_str(), &lightParamsUniformBufferObject.gamma);

	std::string useShadowString = "Use shadow ? ";
	ImGui::Checkbox(useShadowString.c_str(), &lightParamsUniformBufferObject.useShadow);

	std::string showShadowMap = "Show debug shadow map ? ";
	ImGui::Checkbox(showShadowMap.c_str(), &showShadowMapDebug);

	ImGui::EndChild();

	ImGui::End();
	
	if (lightSelected > -1)
	{
		openLightSetting = true;
		
		float lgParamWindowHeight = 250.f;

		int previousType = currentScene->lightProperty[lightSelected].lightType;

		if (previousType == LightType::Point)
			lgParamWindowHeight = 195.f;
		else if (previousType == LightType::Spot)
			lgParamWindowHeight = 287.f;
		else if (previousType == LightType::Directional)
			lgParamWindowHeight = 170.f;

		std::string windowName = "Light parameters##" + std::to_string(lightSelected);
		ImGui::Begin(windowName.c_str(), &openLightSetting, ImVec2(475.f, lgParamWindowHeight), 0.55f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
		ImGui::SetWindowPos(ImVec2(475.f, windowHeight - lgParamWindowHeight), true);
		ImGui::SetWindowSize(ImVec2(475.f, lgParamWindowHeight));

		std::string lgName = "Light " + std::to_string(lightSelected) + " parameters";
		ImGui::Text(lgName.c_str());
		ImGui::Spacing();

        ImGui::Combo("Light type", &currentScene->lightProperty[lightSelected].lightType, "Point\0Spot\0Directional\0");
        ImGui::Spacing();

		int currentType = currentScene->lightProperty[lightSelected].lightType;

		std::string isVisibleString = "Is Visible##" + std::to_string(lightSelected);
		ImGui::Checkbox(isVisibleString.c_str(), &lightUniformBufferObject.light[lightSelected].isVisible);

		std::string colorString = "Color##" + std::to_string(lightSelected);
		ImGui::ColorEdit4(colorString.c_str(), glm::value_ptr(lightUniformBufferObject.light[lightSelected].color));

	    if (currentType != LightType::Directional)
	    {
            std::string positionString = "Position##" + std::to_string(lightSelected);
            ImGui::DragFloat3(positionString.c_str(), glm::value_ptr(lightUniformBufferObject.light[lightSelected].position), 0.05f);
	    }

        if (currentType != LightType::Point)
        {
            std::string rotationString = "Rotation##" + std::to_string(lightSelected);
            ImGui::DragFloat3(rotationString.c_str(), glm::value_ptr(lightUniformBufferObject.light[lightSelected].rotation));
        }

        if (currentType != LightType::Directional)
        {
            std::string radiusString = "Radius##" + std::to_string(lightSelected);
            ImGui::SliderFloat(radiusString.c_str(), &lightUniformBufferObject.light[lightSelected].radius, 0.f, 100.f);
        }

		std::string intensityString = "Intensity##" + std::to_string(lightSelected);
		ImGui::DragFloat(intensityString.c_str(), &lightUniformBufferObject.light[lightSelected].intensity);

		lightUniformBufferObject.light[lightSelected].intensity = std::max(lightUniformBufferObject.light[lightSelected].intensity, 0.0f);

        if (currentType == LightType::Spot)
        {
            std::string outerString = "OuterAngle##" + std::to_string(lightSelected);
            ImGui::SliderFloat(outerString.c_str(), &lightUniformBufferObject.light[lightSelected].outerAngle, 1.f, 179.f);

	        if (lightUniformBufferObject.light[lightSelected].outerAngle < lightUniformBufferObject.light[lightSelected].innerAngle)
				lightUniformBufferObject.light[lightSelected].innerAngle = lightUniformBufferObject.light[lightSelected].outerAngle;

            std::string innerString = "InnerAngle##" + std::to_string(lightSelected);
            ImGui::SliderFloat(innerString.c_str(), &lightUniformBufferObject.light[lightSelected].innerAngle, 1.f, 179.f);

	        if (lightUniformBufferObject.light[lightSelected].innerAngle > lightUniformBufferObject.light[lightSelected].outerAngle)
				lightUniformBufferObject.light[lightSelected].outerAngle = lightUniformBufferObject.light[lightSelected].innerAngle;

            std::string softEdgeString = "Use Soft Edge ?##" + std::to_string(lightSelected);
            ImGui::Checkbox(softEdgeString.c_str(), &lightUniformBufferObject.light[lightSelected].useSoftEdge);
        }
        
	    if (previousType != currentType)
            currentScene->lightProperty[lightSelected].UpdateDataType();

		if (openLightSetting == false)
			lightSelected = -1;

		ImGui::End();
	}

	if (nodeSelected > -1)
	{
		openMeshSetting = true;

		std::list<SceneNode*>::iterator iterator = currentScene->nodes.begin();
		std::advance(iterator, nodeSelected);
		SceneNode* node = *iterator;

		int meshNodeWindowsSizeX = 475.f;
		std::string windowName = "Nodes parameters##" + std::to_string(nodeSelected);
		ImGui::Begin(windowName.c_str(), &openMeshSetting, ImVec2(meshNodeWindowsSizeX, 265.f), 0.55f, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings /*| ImGuiWindowFlags_NoTitleBar*/);
		ImGui::SetWindowPos(ImVec2(windowWidth - meshNodeWindowsSizeX, 15.f), true);
		MeshSceneNode* meshSceneNode = static_cast<MeshSceneNode*>(node);
		std::string meshname = meshSceneNode->mesh->name;

		ImGui::Spacing();
		std::string idxString = "Node " + std::to_string(nodeIndex);
		std::string meshName = idxString + " | Model : " + meshname;
		ImGui::Text(meshName.c_str());
		ImGui::SameLine();
		std::string visibleString = "Is Visible##" + std::to_string(nodeIndex);
		ImGui::Checkbox(visibleString.c_str(), &node->isVisible);
		ImGui::SameLine();
		std::string transparentString = "Is Transparent##" + std::to_string(nodeIndex);
		ImGui::Checkbox(transparentString.c_str(), &node->isTransparent);
		ImGui::SameLine();
		std::string resAllString = "Reset##" + std::to_string(nodeIndex);
		if (ImGui::Button(resAllString.c_str()))
			node->Reset();

		glm::vec3 pos = node->GetPosition();
		std::string meshPosString = "Position##" + idxString;
		ImGui::DragFloat3(meshPosString.c_str(), glm::value_ptr(pos), 0.05f);
		node->SetPosition(pos);
		ImGui::SameLine();
		std::string resPosString = "RP##" + std::to_string(nodeIndex);
		if (ImGui::Button(resPosString.c_str()))
			node->ResetPosition();


		glm::vec3 rot = node->GetRotation();
		std::string meshRotString = "Rotation##" + idxString;
		ImGui::DragFloat3(meshRotString.c_str(), glm::value_ptr(rot));
		node->SetRotation(rot);
		ImGui::SameLine();
		std::string resRotString = "RR##" + std::to_string(nodeIndex);
		if (ImGui::Button(resRotString.c_str()))
			node->ResetRotation();

		std::string meshSclString = "Scale##" + idxString;
		if (node->isScaleHomothety)
		{
			float scl = node->GetScale().x;
			ImGui::DragFloat(meshSclString.c_str(), (float*)&scl);
			node->SetScale(glm::vec3(scl, scl, scl));
		}
		else
		{
			glm::vec3 scl = node->GetScale();
			ImGui::DragFloat3(meshSclString.c_str(), glm::value_ptr(scl));
			node->SetScale(scl);
		}
		ImGui::SameLine();
		std::string resSclString = "RS##" + std::to_string(nodeIndex);
		if (ImGui::Button(resSclString.c_str()))
			node->ResetScale();

		if (meshSceneNode)
		{
			ImGui::Spacing();
			ImGui::Text("Material");

			std::string colorString = "Color##" + std::to_string(nodeIndex);
			ImGui::ColorEdit4(colorString.c_str(), glm::value_ptr(meshSceneNode->GetMesh()->GetMaterial()->params.color));

			std::string reflectanceString = "Reflectance##" + std::to_string(nodeIndex);
			ImGui::SliderFloat(reflectanceString.c_str(), &meshSceneNode->GetMesh()->GetMaterial()->params.reflectance, 0.0f, 1.0f);

			std::string roughnessString = "Roughness##" + std::to_string(nodeIndex);
			ImGui::SliderFloat(roughnessString.c_str(), &meshSceneNode->GetMesh()->GetMaterial()->params.roughness, 0.0f, 1.0f);

			std::string metallicString = "Metallic##" + std::to_string(nodeIndex);
			ImGui::SliderFloat(metallicString.c_str(), &meshSceneNode->GetMesh()->GetMaterial()->params.metallic, 0.0f, 1.0f);

			std::string templateMaterialString = "Template##" + std::to_string(nodeIndex);
			ImGui::Combo(templateMaterialString.c_str(), &meshSceneNode->GetMesh()->GetMaterial()->selectedMaterialTemplate, "Default\0Silver\0Aluminium\0Platinum\0Iron\0Titanium\0Copper\0Gold\0Brass\0Coal\0Rubber\0Mud\0Wood\0Vegetation\0Brick\0Sand\0Concrete\0");

			ImGui::SameLine();
			std::string templateMaterialButtonString = "Apply##" + std::to_string(nodeIndex);
			if (ImGui::Button(templateMaterialButtonString.c_str()))
			{
				meshSceneNode->GetMesh()->GetMaterial()->CopyMaterial(LeMaterial::GetMaterialTemplate((LeMaterialTemplate)meshSceneNode->GetMesh()->GetMaterial()->selectedMaterialTemplate));
			}
		}

		if (openMeshSetting == false)
			nodeSelected = -1;

		ImGui::End();
	}

}

void VulkanDriver::SetupSampleValues()
{
	// Shadow Light
    currentScene->lightProperty[0].ChangeDataType(LightType::Spot);
	lightUniformBufferObject.light[0].isVisible = true;
	lightUniformBufferObject.light[0].position = glm::vec4(0.f, 14.5f, 8.85f, 1.f);
	lightUniformBufferObject.light[0].color = glm::vec4(0.604f, 0.921f, 0.941, 1.f);
	lightUniformBufferObject.light[0].intensity = 20.f;
	lightUniformBufferObject.light[0].radius = 44.5f;
	lightUniformBufferObject.light[0].rotation.x = -47.f;
	lightUniformBufferObject.light[0].outerAngle = 62.7f;
	lightUniformBufferObject.light[0].innerAngle = 45.2f;
	lightUniformBufferObject.light[0].useSoftEdge = true;
	
    currentScene->lightProperty[1].ChangeDataType(LightType::Point);
	lightUniformBufferObject.light[1].isVisible = true;
	lightUniformBufferObject.light[1].color = glm::vec4(112.f, 70.f, 159.f, 255.f) / 255.f;
	lightUniformBufferObject.light[1].position = glm::vec4(4.1f, 12.9f, -3.7f, 1.f);
	lightUniformBufferObject.light[1].radius = 19.04f;
	lightUniformBufferObject.light[1].intensity = 30.f;
	
	currentScene->lightProperty[2].ChangeDataType(LightType::Spot);
	lightUniformBufferObject.light[2].isVisible = true;
	lightUniformBufferObject.light[2].position = glm::vec4(-4.15f, 17.1f, 5.22f, 1.f);
	lightUniformBufferObject.light[2].color = glm::vec4(0.456f, 0.966f, 0.052f, 1.f);
	lightUniformBufferObject.light[2].intensity = 24.f;
	lightUniformBufferObject.light[2].radius = 22.1f;
	lightUniformBufferObject.light[2].rotation.x = -63.f;
	lightUniformBufferObject.light[2].rotation.y = -3.f;
	lightUniformBufferObject.light[2].outerAngle = 25.21f;
	lightUniformBufferObject.light[2].innerAngle = 13.1f;
	lightUniformBufferObject.light[2].useSoftEdge = true;
	// !! Shadow Light



    currentScene->lightProperty[8].ChangeDataType(LightType::Directional);
	lightUniformBufferObject.light[8].isVisible = false;
	lightUniformBufferObject.light[8].color = glm::vec4(255.f, 244.f, 214.f, 255.f) / 255.f;
	lightUniformBufferObject.light[8].rotation.x = 50;
	lightUniformBufferObject.light[8].rotation.y = -30;

 //   currentScene->lightProperty[2].ChangeDataType(LightType::Spot);
	//lightUniformBufferObject.light[2].isVisible = true;
	//lightUniformBufferObject.light[2].position = glm::vec4(0.f, 7.000f, -4.080, 1.f);
	//lightUniformBufferObject.light[2].color = glm::vec4(0.430f, 0.784f, 0.150, 1.f);
	//lightUniformBufferObject.light[2].intensity = 16.f;
	//lightUniformBufferObject.light[2].radius = 30.f;
	//lightUniformBufferObject.light[2].rotation.x = -90.f;
	//lightUniformBufferObject.light[2].outerAngle = 57.455f;
	//lightUniformBufferObject.light[2].innerAngle = 47.455f;
	//lightUniformBufferObject.light[2].useSoftEdge = true;

	ambientUniformBufferObject.skyColor = glm::vec4(0.f, 0.f, 1.f, 1.f);
	ambientUniformBufferObject.equatorColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
	ambientUniformBufferObject.groundColor = glm::vec4(0.f, 1.f, 0.f, 1.f);

	ambientUniformBufferObject.ambientCube[0] = glm::vec4(1.f, 0.f, 0.f, 1.f); //+x
	ambientUniformBufferObject.ambientCube[1] = glm::vec4(1.f, 1.f, 0.f, 1.f); //-x
	ambientUniformBufferObject.ambientCube[2] = glm::vec4(0.f, 1.f, 0.f, 1.f); //+y
	ambientUniformBufferObject.ambientCube[3] = glm::vec4(0.f, 1.f, 1.f, 1.f); //-y
	ambientUniformBufferObject.ambientCube[4] = glm::vec4(1.f, 0.f, 1.f, 1.f); //+z
	ambientUniformBufferObject.ambientCube[5] = glm::vec4(0.f, 0.f, 1.f, 1.f); //+z

	ambientUniformBufferObject.ka = 0.02f;
	ambientUniformBufferObject.mode = AmbientMode::Flat;

	lightParamsUniformBufferObject.brdf = BRDF::GGX_KARIS;
	lightParamsUniformBufferObject.gamma = 2.2f;
	lightParamsUniformBufferObject.useShadow = false;
}

void VulkanDriver::CleanUp()
{
	delete ressourcesList.pipelineLayouts;
	delete ressourcesList.pipelines;
	delete ressourcesList.descriptorSetLayouts;
	delete ressourcesList.descriptorSets;	

	for (size_t i = 0; i < swapChain.imageCount; i++)
		vkDestroyFramebuffer(logicalDevice, frameBuffers[i], nullptr);

	vkDestroyRenderPass(logicalDevice, mainRenderPass, nullptr);
	vkDestroyPipelineCache(logicalDevice, pipelineCache, nullptr);

	vkFreeCommandBuffers(logicalDevice, commandPool, 1, &setupCommandBuffer);

	for (size_t i = 0; i < swapChain.imageCount; i++)
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &drawCommandBuffer[i]);

	vkDestroySampler(logicalDevice, depthSampler, nullptr);
	vkDestroySampler(logicalDevice, normalMapSampler, nullptr);
	vkDestroySampler(logicalDevice, specularMapSampler, nullptr);
	vkDestroySampler(logicalDevice, metallicMapSampler, nullptr);
	vkDestroySampler(logicalDevice, roughnessMapSampler, nullptr);

	msaaRenderTarget.buffer.Clear();
	vkDestroyImageView(logicalDevice, msaaRenderTarget.view, nullptr);

	msaaDepthStencil.buffer.Clear();
	vkDestroyImageView(logicalDevice, msaaDepthStencil.view, nullptr);

	offscreenFramebuffer.Destroy(logicalDevice);

	sceneUniformBuffer->stagingBuffer.Clear();
	sceneUniformBuffer->buffers.Clear();
	delete sceneUniformBuffer;

	lightUniformBuffer->stagingBuffer.Clear();
	lightUniformBuffer->buffers.Clear();
	delete lightUniformBuffer;

	ambientUniformBuffer->stagingBuffer.Clear();
	ambientUniformBuffer->buffers.Clear();
	delete ambientUniformBuffer;
	
	lightParametersUniformBuffer->stagingBuffer.Clear();
	lightParametersUniformBuffer->buffers.Clear();
	delete lightParametersUniformBuffer;

	shadowMatrixUniformBuffer->stagingBuffer.Clear();
	shadowMatrixUniformBuffer->buffers.Clear();
	delete shadowMatrixUniformBuffer;

	skyboxUniformData->stagingBuffer.Clear();
	skyboxUniformData->buffers.Clear();
	delete skyboxUniformData;

	vkDestroySampler(logicalDevice, skyboxMapSampler, nullptr);
	skyboxCubeMap->buffer.Clear();
	skyboxCubeMap->Clear();
	vkDestroyImageView(logicalDevice, skyboxCubeMap->textureImageView, nullptr);
	
	currentScene->skyboxNode->uniformNodeMeshStagingBuffer.Clear();
	currentScene->skyboxNode->uniformNodeMeshBuffer.Clear();
	currentScene->skyboxNode->uniformNodeMaterialStagingBuffer.Clear();
	currentScene->skyboxNode->uniformNodeMaterialBuffer.Clear();
	
	MeshBuffer* skyboxNodeMeshBuffer = currentScene->skyboxNode->GetMesh()->GetMeshBuffer(0);
	skyboxNodeMeshBuffer->vertexBuffer.Clear();
	skyboxNodeMeshBuffer->indexBuffer.Clear();

	currentScene->shadowDebugNode->uniformNodeMeshStagingBuffer.Clear();
	currentScene->shadowDebugNode->uniformNodeMeshBuffer.Clear();
	currentScene->shadowDebugNode->uniformNodeMaterialStagingBuffer.Clear();
	currentScene->shadowDebugNode->uniformNodeMaterialBuffer.Clear();

	MeshBuffer* shadowDebugNodeMeshBuffer = currentScene->shadowDebugNode->GetMesh()->GetMeshBuffer(0);		
	shadowDebugNodeMeshBuffer->vertexBuffer.Clear();
	shadowDebugNodeMeshBuffer->indexBuffer.Clear();

	for (SceneNode* node : currentScene->nodes)
	{
		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
				
		if (meshNode)
		{
			meshNode->uniformNodeMeshStagingBuffer.Clear();
			meshNode->uniformNodeMeshBuffer.Clear();
			meshNode->uniformNodeMaterialStagingBuffer.Clear();
			meshNode->uniformNodeMaterialBuffer.Clear();

			int meshCount = meshNode->GetMesh()->GetMeshBufferCount();
			for (size_t i = 0; i < meshCount; i++)
			{
				MeshBuffer* meshBuffer = meshNode->GetMesh()->GetMeshBuffer(i);
				vkFreeDescriptorSets(logicalDevice, descriptorPool, 1, &meshBuffer->descriptorSet);
				meshBuffer->vertexBuffer.Clear();
				meshBuffer->indexBuffer.Clear();

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->texture->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->texture->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->texture->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->normalMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->normalMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->normalMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->specularMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->specularMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->specularMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->metallicMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->metallicMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->metallicMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->roughnessMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->roughnessMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->roughnessMap->textureImageView, nullptr);
			}
			
		}
	}

	for (SceneNode* node : currentScene->lightsCubesNodes)
	{
		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);

		if (meshNode)
		{
			meshNode->uniformNodeMeshStagingBuffer.Clear();
			meshNode->uniformNodeMeshBuffer.Clear();
			meshNode->uniformNodeMaterialStagingBuffer.Clear();
			meshNode->uniformNodeMaterialBuffer.Clear();

			int meshCount = meshNode->GetMesh()->GetMeshBufferCount();
			for (size_t i = 0; i < meshCount; i++)
			{
				MeshBuffer* meshBuffer = meshNode->GetMesh()->GetMeshBuffer(i);
				vkFreeDescriptorSets(logicalDevice, descriptorPool, 1, &meshBuffer->descriptorSet);
				meshBuffer->vertexBuffer.Clear();
				meshBuffer->indexBuffer.Clear();

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->texture->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->texture->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->texture->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->normalMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->normalMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->normalMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->specularMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->specularMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->specularMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->metallicMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->metallicMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->metallicMap->textureImageView, nullptr);

				vkDestroySampler(logicalDevice, meshNode->GetMesh()->GetMaterial()->roughnessMap->textureSampler, nullptr);
				meshNode->GetMesh()->GetMaterial()->roughnessMap->Clear();
				vkDestroyImageView(logicalDevice, meshNode->GetMesh()->GetMaterial()->roughnessMap->textureImageView, nullptr);
			}

		}
	}


	vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);

	for (size_t i = 0; i < swapChain.imageCount; i++) 
	{
		vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
		vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
		vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
	}

	swapChain.CleanUp(logicalDevice);

	delete currentScene;

	vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
	
	ImGui_ImplVulkan_Shutdown();
	ImGui_ImplGlfw_Shutdown();

	vkDestroyDevice(logicalDevice, nullptr);
	
	vkDestroyDebugReportCallbackEXT(instance, debugCallback, nullptr);

	//vkDestroySurfaceKHR(instance, swapChain.surface, nullptr);
	vkDestroyInstance(instance, nullptr);

	glfwDestroyWindow(window);

	glfwTerminate();
}

void VulkanDriver::CreateInstance()
{
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "LumEngine";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "LumEngine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;


	std::vector<const char*> enabledExtensions = { VK_KHR_SURFACE_EXTENSION_NAME };

	unsigned int glfwExtensionCount = 0;
	auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
		enabledExtensions.push_back(glfwExtensions[i]);

#ifdef VULKAN_ENABLE_VALIDATION
	enabledExtensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
#endif

	VkInstanceCreateInfo instanceCreateInfo = {};
	instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instanceCreateInfo.pApplicationInfo = &appInfo;
	instanceCreateInfo.enabledExtensionCount = enabledExtensions.size();
	instanceCreateInfo.ppEnabledExtensionNames = enabledExtensions.data();
#ifdef VULKAN_ENABLE_VALIDATION
	const char* layerNames[] = { "VK_LAYER_LUNARG_standard_validation" };
	instanceCreateInfo.enabledLayerCount = 1;
	instanceCreateInfo.ppEnabledLayerNames = layerNames;
#endif

	DEBUG_CHECK_VK(vkCreateInstance(&instanceCreateInfo, nullptr, &instance));

	volkLoadInstance(instance);
}

void VulkanDriver::CreateDebugCallback()
{
#ifdef VULKAN_ENABLE_VALIDATION
	LeUTILS::SetupDebugCallback(instance, debugCallback);
#endif
}

void VulkanDriver::CreatePhysicalDevice()
{
	uint32_t gpuCount = 0;

	DEBUG_CHECK_VK(vkEnumeratePhysicalDevices(instance, &gpuCount, nullptr));

	assert(gpuCount > 0);
	std::vector<VkPhysicalDevice> physicalDevices(gpuCount);

	DEBUG_CHECK_VK(vkEnumeratePhysicalDevices(instance, &gpuCount, physicalDevices.data()));
	physicalDevice = physicalDevices[0];

	vulkanDevice = new VulkanDevice(physicalDevice, swapChain.surface);

	vulkanDevice->CreateLogicalDevice();
	this->logicalDevice = vulkanDevice->logicalDevice;

	this->vulkanDevice->msaaSamples = LeUTILS::GetMaxUsableSampleCount(physicalDevice);
}

void VulkanDriver::GetPhysicalDeviceLimitations()
{
	vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
	vkGetPhysicalDeviceFeatures(physicalDevice, &deviceFeatures);
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);
}

void VulkanDriver::InitializeImGui()
{
	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = instance;
	init_info.PhysicalDevice = physicalDevice;
	init_info.Device = logicalDevice;
	init_info.QueueFamily = vulkanDevice->queueFamilyIndices.graphics;
	init_info.Queue = graphicQueue;
	init_info.PipelineCache = pipelineCache;
	init_info.DescriptorPool = descriptorPool;
	init_info.MSAASamples = vulkanDevice->msaaSamples;
	init_info.MinImageCount = 2;
	init_info.ImageCount = swapChain.imageCount;
	init_info.CheckVkResultFn = check_vk_result;

	ImGui_ImplVulkan_Init(&init_info, mainRenderPass);

	{
		VkCommandBuffer imguiFontCommandBuffer = VK_NULL_HANDLE;
		CreateCommandBuffer(imguiFontCommandBuffer, false);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		DEBUG_CHECK_VK(vkBeginCommandBuffer(imguiFontCommandBuffer, &begin_info));

		ImGui_ImplVulkan_CreateFontsTexture(imguiFontCommandBuffer);

		VkSubmitInfo end_info = {};
		end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		end_info.commandBufferCount = 1;
		end_info.pCommandBuffers = &imguiFontCommandBuffer;
		DEBUG_CHECK_VK(vkEndCommandBuffer(imguiFontCommandBuffer));
		DEBUG_CHECK_VK(vkQueueSubmit(graphicQueue, 1, &end_info, VK_NULL_HANDLE));

		DEBUG_CHECK_VK(vkDeviceWaitIdle(logicalDevice));
		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void VulkanDriver::CreateQueues()
{
	vkGetDeviceQueue(logicalDevice, vulkanDevice->queueFamilyIndices.graphics, 0, &graphicQueue);
	vkGetDeviceQueue(logicalDevice, vulkanDevice->queueFamilyIndices.present, 0, &presentationQueue);
}

void VulkanDriver::CreateCommandPool()
{
	VkCommandPoolCreateInfo cmdPoolInfo = LeUTILS::CommandPoolCreateInfoUtils(vulkanDevice->queueFamilyIndices.graphics);
	DEBUG_CHECK_VK(vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &commandPool));
}

void VulkanDriver::SetupDepthStencilFormat()
{
	LeUTILS::GetSupportedDepthFormat(physicalDevice, &depthFormat);
}

void VulkanDriver::CreateCommandBuffer(VkCommandBuffer& cdBuffer, bool shouldStart)
{
	if (cdBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &cdBuffer);
		cdBuffer = VK_NULL_HANDLE;
	}
	
	DEBUG_CHECK_VK(vkAllocateCommandBuffers(logicalDevice, &LeUTILS::CommandBufferAllocateUtils(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1), &cdBuffer));

	if (shouldStart)
	{
		VkCommandBufferBeginInfo cmdBufInfo = {};
		cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		DEBUG_CHECK_VK(vkBeginCommandBuffer(cdBuffer, &LeUTILS::CommandBufferBeginInfoUtils()));
	}
}

void VulkanDriver::CreateCommandBuffer(VkCommandBuffer& cdBuffer, VkCommandBufferUsageFlagBits flags)
{
	if (cdBuffer != VK_NULL_HANDLE)
	{
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &cdBuffer);
		cdBuffer = VK_NULL_HANDLE;
	}

	DEBUG_CHECK_VK(vkAllocateCommandBuffers(logicalDevice, &LeUTILS::CommandBufferAllocateUtils(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1), &cdBuffer));

	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmdBufInfo.flags = flags;
	DEBUG_CHECK_VK(vkBeginCommandBuffer(cdBuffer, &LeUTILS::CommandBufferBeginInfoUtils()));
}

void VulkanDriver::CreateCommandBuffer(VkCommandBuffer* cdBuffer, uint32_t bgCount)
{
	VkCommandBufferAllocateInfo cbAllocateInfo = LeUTILS::CommandBufferAllocateUtils(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, bgCount);
	DEBUG_CHECK_VK(vkAllocateCommandBuffers(logicalDevice, &cbAllocateInfo, cdBuffer));
}

void VulkanDriver::FlushCommanderBuffer(VkCommandBuffer& commandBuffer, VkQueue queue, bool shouldEnd, bool shouldErase)
{
	if (commandBuffer == VK_NULL_HANDLE)
		return;

	if (shouldEnd)
		DEBUG_CHECK_VK(vkEndCommandBuffer(commandBuffer));

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	DEBUG_CHECK_VK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
	DEBUG_CHECK_VK(vkQueueWaitIdle(queue));

	if (shouldErase)
	{
		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
		commandBuffer = VK_NULL_HANDLE;
	}
}

VkShaderModule VulkanDriver::CreateShaderModule(const std::vector<char>& code)
{
	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.size();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

	VkShaderModule shaderModule;
	DEBUG_CHECK_VK(vkCreateShaderModule(logicalDevice, &createInfo, nullptr, &shaderModule));
	return shaderModule;
}

void VulkanDriver::TransitionImageLayout(BufferHandle& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t arrayLayers, uint32_t mipLevels)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	 	CreateCommandBuffer(commandBuffer, true);

	//VkCommandBuffer commandBuffer = beginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image.image;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = arrayLayers;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
	{
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	FlushCommanderBuffer(commandBuffer, graphicQueue, true, true);

	//endSingleTimeCommands(commandBuffer);
}

void VulkanDriver::CreateRenderPass()
{
	std::array<VkAttachmentDescription, 3> attachments = {};
	// Color attachment
	attachments[0].format = swapChain.colorFormat;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription colorAttachmentResolve = {};
	attachments[2].format = swapChain.colorFormat;
	attachments[2].samples = vulkanDevice->msaaSamples;
	attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	attachments[1].format = depthFormat;
	attachments[1].samples = vulkanDevice->msaaSamples;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorReference = {};
	colorReference.attachment = 2;
	colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 1;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 0;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDescription = {};
	subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDescription.colorAttachmentCount = 1;
	subpassDescription.pColorAttachments = &colorReference;
	subpassDescription.pDepthStencilAttachment = &depthReference;
	subpassDescription.inputAttachmentCount = 0;
	subpassDescription.pInputAttachments = nullptr;
	subpassDescription.preserveAttachmentCount = 0;
	subpassDescription.pPreserveAttachments = nullptr;
	subpassDescription.pResolveAttachments = &colorAttachmentResolveRef;

	// Subpass dependencies for layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpassDescription;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();

	DEBUG_CHECK_VK(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &mainRenderPass));
}

void VulkanDriver::CreatePipelineCache()
{
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	DEBUG_CHECK_VK(vkCreatePipelineCache(logicalDevice, &pipelineCacheCreateInfo, nullptr, &pipelineCache));
}

void VulkanDriver::CreateFrameBuffer()
{
	VkImageView attachments[3];

	attachments[2] = msaaRenderTarget.view;

	attachments[1] = msaaDepthStencil.view;
	   
	VkFramebufferCreateInfo frameBufferCreateInfo = {};
	frameBufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	frameBufferCreateInfo.pNext = NULL;
	frameBufferCreateInfo.renderPass = mainRenderPass;
	frameBufferCreateInfo.attachmentCount = 3;
	frameBufferCreateInfo.pAttachments = attachments;
	frameBufferCreateInfo.width = windowWidth;
	frameBufferCreateInfo.height = windowHeight;
	frameBufferCreateInfo.layers = 1;

	// Create frame buffers for every swap chain image
	frameBuffers.resize(swapChain.imageCount);
	for (uint32_t i = 0; i < frameBuffers.size(); i++)
	{
		attachments[0] = swapChain.buffers[i].view;
		DEBUG_CHECK_VK(vkCreateFramebuffer(logicalDevice, &frameBufferCreateInfo, nullptr, &frameBuffers[i]));
	}
}

void VulkanDriver::CreateDescriptorPool()
{
	std::vector<VkDescriptorPoolSize> poolSizes = { LeUTILS::GetDescriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 150), LeUTILS::GetDescriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 150) };

	VkDescriptorPoolCreateInfo descriptorPoolInfo = LeUTILS::DescriptorPoolCreateInfoUtils(poolSizes.size(), poolSizes.data(), 90);
	DEBUG_CHECK_VK(vkCreateDescriptorPool(logicalDevice, &descriptorPoolInfo, nullptr, &descriptorPool));
}

void VulkanDriver::InitilizeRessourcesManager()
{
	ressourcesList.pipelineLayouts = new PipelineLayoutList(vulkanDevice->logicalDevice);
	ressourcesList.pipelines = new PipelineList(vulkanDevice->logicalDevice);
	ressourcesList.descriptorSetLayouts = new DescriptorSetLayoutList(vulkanDevice->logicalDevice);
	ressourcesList.descriptorSets = new DescriptorSetList(vulkanDevice->logicalDevice, descriptorPool);
	//ressourcesList.textures = new TextureList(vulkanDevice->logicalDevice, textureLoader);
}

void VulkanDriver::SetupVertexDescriptions()
{
	verticesDescription.bindingDescriptions = {	LeUTILS::VertexInputBindingDescriptionUtils(0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX) };

	verticesDescription.attributeDescriptions = 
	{
		LeUTILS::VertexInputAttributeDescriptionUtils(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos)),
		LeUTILS::VertexInputAttributeDescriptionUtils(0, 1, VK_FORMAT_R32G32_SFLOAT,    offsetof(Vertex, uv)),
		LeUTILS::VertexInputAttributeDescriptionUtils(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)),
		LeUTILS::VertexInputAttributeDescriptionUtils(0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
	};

	verticesDescription.inputState = LeUTILS::PipelineVertexInputStateCreateInfoUtils();
	verticesDescription.inputState.vertexBindingDescriptionCount = verticesDescription.bindingDescriptions.size();
	verticesDescription.inputState.pVertexBindingDescriptions = verticesDescription.bindingDescriptions.data();
	verticesDescription.inputState.vertexAttributeDescriptionCount = verticesDescription.attributeDescriptions.size();
	verticesDescription.inputState.pVertexAttributeDescriptions = verticesDescription.attributeDescriptions.data();


}

//void VulkanDriver::CreateAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment *attachment, VkCommandBuffer layoutCmd, uint32_t width, uint32_t height)
//{
//	VkImageAspectFlags aspectMask = 0;
//	VkImageLayout imageLayout;
//
//	attachment->format = format;
//
//	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
//	{
//		aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
//		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
//	}
//	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
//	{
//		aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
//		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
//	}
//
//	assert(aspectMask > 0);
//
//	VkImageCreateInfo image = LeUTILS::ImageCreateInfoUtils();
//	image.imageType = VK_IMAGE_TYPE_2D;
//	image.format = format;
//	image.extent.width = width;
//	image.extent.height = height;
//	image.extent.depth = 1;
//	image.mipLevels = 1;
//	image.arrayLayers = 1;
//	image.samples = VK_SAMPLE_COUNT_1_BIT;
//	image.tiling = VK_IMAGE_TILING_OPTIMAL;
//	image.usage = usage | VK_IMAGE_USAGE_SAMPLED_BIT;
//
//	//VkDedicatedAllocationImageCreateInfoNV dedicatedImageInfo{ VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_IMAGE_CREATE_INFO_NV };
//	//if (enableNVDedicatedAllocation)
//	//{
//	//	dedicatedImageInfo.dedicatedAllocation = VK_TRUE;
//	//	image.pNext = &dedicatedImageInfo;
//	//}
//	DEBUG_CHECK_VK(vkCreateImage(logicalDevice, &image, nullptr, &attachment->image));
//
//	VkMemoryAllocateInfo memAlloc = LeUTILS::MemoryAllocateInfoUtils();
//	VkMemoryRequirements memReqs;
//	vkGetImageMemoryRequirements(logicalDevice, attachment->image, &memReqs);
//	memAlloc.allocationSize = memReqs.size;
//	memAlloc.memoryTypeIndex = vulkanDevice->GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
//
//	VkDedicatedAllocationMemoryAllocateInfoNV dedicatedAllocationInfo{ VK_STRUCTURE_TYPE_DEDICATED_ALLOCATION_MEMORY_ALLOCATE_INFO_NV };
//	//if (enableNVDedicatedAllocation)
//	//{
//	//	dedicatedAllocationInfo.image = attachment->image;
//	//	memAlloc.pNext = &dedicatedAllocationInfo;
//	//}
//
//	DEBUG_CHECK_VK(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &attachment->memory));
//	DEBUG_CHECK_VK(vkBindImageMemory(logicalDevice, attachment->image, attachment->memory, 0));
//
//	VkImageViewCreateInfo imageView = LeUTILS::ImageViewCreateInfo();
//	imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
//	imageView.format = format;
//	imageView.subresourceRange = {};
//	imageView.subresourceRange.aspectMask = aspectMask;
//	imageView.subresourceRange.baseMipLevel = 0;
//	imageView.subresourceRange.levelCount = 1;
//	imageView.subresourceRange.baseArrayLayer = 0;
//	imageView.subresourceRange.layerCount = 1;
//	imageView.image = attachment->image;
//	DEBUG_CHECK_VK(vkCreateImageView(logicalDevice, &imageView, nullptr, &attachment->view));
//}

void VulkanDriver::PrepareOffscreenRendering()
{	
	offscreenFramebuffer.SetSize(2048, 2048);

	VkImageCreateInfo image = LeUTILS::ImageCreateInfoUtils();
	image.imageType = VK_IMAGE_TYPE_2D;
	image.extent.width = offscreenFramebuffer.width;
	image.extent.height = offscreenFramebuffer.height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.format = VK_FORMAT_D16_UNORM;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
	DEBUG_CHECK_VK(vkCreateImage(logicalDevice, &image, nullptr, &offscreenFramebuffer.depth.image));
	
	VkMemoryAllocateInfo memAlloc = LeUTILS::MemoryAllocateInfoUtils();
	VkMemoryRequirements memReqs;
	vkGetImageMemoryRequirements(logicalDevice, offscreenFramebuffer.depth.image, &memReqs);
	memAlloc.allocationSize = memReqs.size;
	memAlloc.memoryTypeIndex = vulkanDevice->GetMemoryType(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	DEBUG_CHECK_VK(vkAllocateMemory(logicalDevice, &memAlloc, nullptr, &offscreenFramebuffer.depth.memory));
	DEBUG_CHECK_VK(vkBindImageMemory(logicalDevice, offscreenFramebuffer.depth.image, offscreenFramebuffer.depth.memory, 0));
	
	VkImageViewCreateInfo depthStencilView = LeUTILS::ImageViewCreateInfo();
	depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
	depthStencilView.format = VK_FORMAT_D16_UNORM;
	depthStencilView.subresourceRange = {};
	depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	depthStencilView.subresourceRange.baseMipLevel = 0;
	depthStencilView.subresourceRange.levelCount = 1;
	depthStencilView.subresourceRange.baseArrayLayer = 0;
	depthStencilView.subresourceRange.layerCount = 1;
	depthStencilView.image = offscreenFramebuffer.depth.image;
	DEBUG_CHECK_VK(vkCreateImageView(logicalDevice, &depthStencilView, nullptr, &offscreenFramebuffer.depth.view));
	   
	VkSamplerCreateInfo dSampler = LeUTILS::SamplerCreateInfoUtils();
	dSampler.magFilter = VK_FILTER_LINEAR;
	dSampler.minFilter = VK_FILTER_LINEAR;
	dSampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	dSampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
	dSampler.addressModeV = dSampler.addressModeU;
	dSampler.addressModeW = dSampler.addressModeU;
	dSampler.mipLodBias = 0.0f;
	dSampler.maxAnisotropy = 1.0f;
	dSampler.minLod = 0.0f;
	dSampler.maxLod = 1.0f;
	dSampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &dSampler, nullptr, &depthSampler));

	VkAttachmentDescription attachmentDescription{};
	attachmentDescription.format = VK_FORMAT_D16_UNORM;
	attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
	attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

	VkAttachmentReference depthReference = {};
	depthReference.attachment = 0;
	depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 0;
	subpass.pDepthStencilAttachment = &depthReference;

	// Use subpass dependencies for attachment layout transitions
	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = &attachmentDescription;
	renderPassInfo.attachmentCount = 1;
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
	renderPassInfo.pDependencies = dependencies.data();
	DEBUG_CHECK_VK(vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &offscreenFramebuffer.renderPass));
	
	VkFramebufferCreateInfo fbufCreateInfo = LeUTILS::FramebufferCreateInfoUtils();
	fbufCreateInfo.renderPass = offscreenFramebuffer.renderPass;
	fbufCreateInfo.pAttachments = &offscreenFramebuffer.depth.view;
	fbufCreateInfo.attachmentCount = 1;
	fbufCreateInfo.width = offscreenFramebuffer.width;
	fbufCreateInfo.height = offscreenFramebuffer.height;
	fbufCreateInfo.layers = 1;
	DEBUG_CHECK_VK(vkCreateFramebuffer(logicalDevice, &fbufCreateInfo, nullptr, &offscreenFramebuffer.frameBuffer));
	
}

void VulkanDriver::PrepareSceneUniformBuffer()
{
	sceneUniformBuffer = new UniformBufferHandle();
	VkDeviceSize sceneBufferSize = sizeof(SceneUniformBufferObject);
	vulkanDevice->CreateBuffer(sceneBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, sceneUniformBuffer->stagingBuffer, sceneUniformBuffer->data);
	vkMapMemory(logicalDevice, sceneUniformBuffer->stagingBuffer.memory, 0, sceneBufferSize, 0, &sceneUniformBuffer->data);
	vulkanDevice->CreateBuffer(sceneBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, sceneUniformBuffer->buffers);

	lightUniformBuffer = new UniformBufferHandle();
	VkDeviceSize lightBufferSize = sizeof(LightUniformBufferObject);
	vulkanDevice->CreateBuffer(lightBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightUniformBuffer->stagingBuffer, lightUniformBuffer->data);
	vkMapMemory(logicalDevice, lightUniformBuffer->stagingBuffer.memory, 0, lightBufferSize, 0, &lightUniformBuffer->data);
	vulkanDevice->CreateBuffer(lightBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lightUniformBuffer->buffers);

	ambientUniformBuffer = new UniformBufferHandle();
	VkDeviceSize ambientBufferSize = sizeof(AmbientUniformBufferObject);
	vulkanDevice->CreateBuffer(ambientBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, ambientUniformBuffer->stagingBuffer, ambientUniformBuffer->data);
	vkMapMemory(logicalDevice, ambientUniformBuffer->stagingBuffer.memory, 0, ambientBufferSize, 0, &ambientUniformBuffer->data);
	vulkanDevice->CreateBuffer(ambientBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, ambientUniformBuffer->buffers);

	lightParametersUniformBuffer = new UniformBufferHandle();
	VkDeviceSize lightParamsBufferSize = sizeof(LightParamsUniformBufferObject);
	vulkanDevice->CreateBuffer(lightParamsBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, lightParametersUniformBuffer->stagingBuffer, lightParametersUniformBuffer->data);
	vkMapMemory(logicalDevice, lightParametersUniformBuffer->stagingBuffer.memory, 0, lightParamsBufferSize, 0, &lightParametersUniformBuffer->data);
	vulkanDevice->CreateBuffer(lightParamsBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, lightParametersUniformBuffer->buffers);

	shadowMatrixUniformBuffer = new UniformBufferHandle();
	VkDeviceSize shadowMatrixBufferSize = sizeof(ShadowMatrixUniformBufferObject);
	vulkanDevice->CreateBuffer(shadowMatrixBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, shadowMatrixUniformBuffer->stagingBuffer, shadowMatrixUniformBuffer->data);
	vkMapMemory(logicalDevice, shadowMatrixUniformBuffer->stagingBuffer.memory, 0, shadowMatrixBufferSize, 0, &shadowMatrixUniformBuffer->data);
	vulkanDevice->CreateBuffer(shadowMatrixBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shadowMatrixUniformBuffer->buffers);
	
	skyboxUniformData = new UniformBufferHandle();
	vulkanDevice->CreateBuffer(sceneBufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, skyboxUniformData->stagingBuffer, skyboxUniformData->data);
	vkMapMemory(logicalDevice, skyboxUniformData->stagingBuffer.memory, 0, sceneBufferSize, 0, &skyboxUniformData->data);
	vulkanDevice->CreateBuffer(sceneBufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, skyboxUniformData->buffers);
	
	CreateCommandBuffer(uniformSceneCommandBuffer, true);
	
	VkBufferCopy copyRegionVertexScene = {};
	copyRegionVertexScene.size = sizeof(SceneUniformBufferObject);
	vkCmdCopyBuffer(uniformSceneCommandBuffer, sceneUniformBuffer->stagingBuffer.buffer, sceneUniformBuffer->buffers.buffer, 1, &copyRegionVertexScene);
	
	VkBufferCopy copyRegionVertexLight = {};
	copyRegionVertexLight.size = sizeof(LightUniformBufferObject);
	vkCmdCopyBuffer(uniformSceneCommandBuffer, lightUniformBuffer->stagingBuffer.buffer, lightUniformBuffer->buffers.buffer, 1, &copyRegionVertexLight);

	VkBufferCopy copyRegionVertexAmbient = {};
	copyRegionVertexAmbient.size = sizeof(AmbientUniformBufferObject);
	vkCmdCopyBuffer(uniformSceneCommandBuffer, ambientUniformBuffer->stagingBuffer.buffer, ambientUniformBuffer->buffers.buffer, 1, &copyRegionVertexAmbient);

	VkBufferCopy copyRegionVertexLightParams = {};
	copyRegionVertexLightParams.size = sizeof(LightParamsUniformBufferObject);
	vkCmdCopyBuffer(uniformSceneCommandBuffer, lightParametersUniformBuffer->stagingBuffer.buffer, lightParametersUniformBuffer->buffers.buffer, 1, &copyRegionVertexLightParams);
	
	VkBufferCopy copyRegionVertexShadowMatrix = {};
	copyRegionVertexShadowMatrix.size = sizeof(ShadowMatrixUniformBufferObject);
	vkCmdCopyBuffer(uniformSceneCommandBuffer, shadowMatrixUniformBuffer->stagingBuffer.buffer, shadowMatrixUniformBuffer->buffers.buffer, 1, &copyRegionVertexShadowMatrix);
	
	vkCmdCopyBuffer(uniformSceneCommandBuffer, skyboxUniformData->stagingBuffer.buffer, skyboxUniformData->buffers.buffer, 1, &copyRegionVertexScene);

	DEBUG_CHECK_VK(vkEndCommandBuffer(uniformSceneCommandBuffer));
}

void VulkanDriver::CreateSceneDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding sceneUniformLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding meshNodeUniformLayoutBinding	 = LeUTILS::DescriptorSetLayoutBindingUtils(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding materialUniformLayoutBinding	 = LeUTILS::DescriptorSetLayoutBindingUtils(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding lightUniformLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding ambientUniformLayoutBinding	 = LeUTILS::DescriptorSetLayoutBindingUtils(4, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding lightParamsUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(5, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding samplerLayoutBinding			 = LeUTILS::DescriptorSetLayoutBindingUtils(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding normalMapLayoutBinding			 = LeUTILS::DescriptorSetLayoutBindingUtils(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding specularMapLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding metallicMapLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding roughnessMapLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding shadowMapSamplerLayoutBinding	 = LeUTILS::DescriptorSetLayoutBindingUtils(11, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding skyboxSamplerLayoutBinding		 = LeUTILS::DescriptorSetLayoutBindingUtils(12, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);
		
	std::array<VkDescriptorSetLayoutBinding, 13> bindings = { sceneUniformLayoutBinding, meshNodeUniformLayoutBinding, materialUniformLayoutBinding, lightUniformLayoutBinding, ambientUniformLayoutBinding,
		lightParamsUniformLayoutBinding, samplerLayoutBinding, normalMapLayoutBinding, specularMapLayoutBinding, metallicMapLayoutBinding, roughnessMapLayoutBinding, shadowMapSamplerLayoutBinding, skyboxSamplerLayoutBinding };
		
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	ressourcesList.descriptorSetLayouts->add("main", layoutInfo);
}

void VulkanDriver::CreateLightCubePipeline()
{
	std::vector<char> vertShaderCode = LeUTILS::ReadFile("../Data/Shaders/lightCube.vert.spv");
	std::vector<char> fragShaderCode = LeUTILS::ReadFile("../Data/Shaders/lightCube.frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	/*FIXED PIPELINE FUNCTION*/

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = LeUTILS::InputAssemblyStateUtils(
	1, 
	static_cast<uint32_t>(verticesDescription.attributeDescriptions.size()),
	verticesDescription.bindingDescriptions.data(),
	verticesDescription.attributeDescriptions.data() );

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = LeUTILS::PipelineInputAssemblyStateUtils(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = LeUTILS::PipelineViewportUtils((float)windowWidth, (float)windowHeight);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = LeUTILS::PipelineViewportStateUtils(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = LeUTILS::RasterizationStateUtils(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE);

	VkPipelineMultisampleStateCreateInfo multisampling = LeUTILS::PipelineMultisampleStateUtils(vulkanDevice->msaaSamples, VK_FALSE);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = LeUTILS::PipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = LeUTILS::PipelineColorBlendStateUtils(&colorBlendAttachment, VK_FALSE);

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_LINE_WIDTH };
	VkPipelineDynamicStateCreateInfo dynamicState = LeUTILS::PipelineDynamicStateUtils(dynamicStates, 2);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = LeUTILS::PipelineLayoutInfo(ressourcesList.descriptorSetLayouts->getPtr("lightCube"));

	VkPipelineLayout lightCubePipelineLayout = ressourcesList.pipelineLayouts->add("lightCube", pipelineLayoutInfo);

	/* !! FIXED PIPELINE FUNCTION*/

	VkPipelineDepthStencilStateCreateInfo depthStencil = LeUTILS::PipelineDepthStencilStateUtils(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = lightCubePipelineLayout;
	pipelineInfo.renderPass = mainRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &depthStencil;

	ressourcesList.pipelines->addGraphicsPipeline("lightCube", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void VulkanDriver::CreateLightCubeDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding sceneUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding meshNodeUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding lightParamsUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(2, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
	VkDescriptorSetLayoutBinding materialUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(3, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT);
		   
	std::array<VkDescriptorSetLayoutBinding, 4> bindings = { sceneUniformLayoutBinding, meshNodeUniformLayoutBinding, lightParamsUniformLayoutBinding, materialUniformLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	ressourcesList.descriptorSetLayouts->add("lightCube", layoutInfo);
}

void VulkanDriver::CreateLightCubeBufferDescriptorSet(MeshSceneNode* node, MeshBuffer* buffer, int lightIndex)
{
	if (buffer->descriptorSet == VK_NULL_HANDLE)
	{
		VkDescriptorSetLayout layouts[] = { ressourcesList.descriptorSetLayouts->get("lightCube") };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		DEBUG_CHECK_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &buffer->descriptorSet));
	}

	// Scene buffers
	VkDescriptorBufferInfo bufferSceneVertexInfo = {};
	bufferSceneVertexInfo.buffer = sceneUniformBuffer->buffers.buffer;
	bufferSceneVertexInfo.offset = 0;
	bufferSceneVertexInfo.range = sizeof(SceneUniformBufferObject);

	// Node buffers
	VkDescriptorBufferInfo bufferNodeVertexInfo = {};
	bufferNodeVertexInfo.buffer = node->uniformNodeMeshBuffer.buffer;
	bufferNodeVertexInfo.offset = 0;
	bufferNodeVertexInfo.range = sizeof(UniformNodeVertexBuffer);
	
	// Light Params buffer
	VkDescriptorBufferInfo lightParamsInfo = {};
	lightParamsInfo.buffer = lightParametersUniformBuffer->buffers.buffer;
	lightParamsInfo.offset = 0;
	lightParamsInfo.range = sizeof(LightParamsUniformBufferObject);
	
	// Material buffers
	VkDescriptorBufferInfo bufferMaterialInfo = {};
	bufferMaterialInfo.buffer = node->uniformNodeMaterialBuffer.buffer;
	bufferMaterialInfo.offset = 0;
	bufferMaterialInfo.range = sizeof(UniformMaterialBuffer);
	
	std::array<VkWriteDescriptorSet, 4> descriptorWrites = {};
	
	descriptorWrites[0] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferSceneVertexInfo);

	descriptorWrites[1] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &bufferNodeVertexInfo);

	descriptorWrites[2] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &lightParamsInfo);

	descriptorWrites[3] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &bufferMaterialInfo);

	//descriptorWrites[3] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &lightsInfo);

	vkUpdateDescriptorSets(logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void VulkanDriver::CreateMsaaRessources()
{
	VkFormat colorFormat = swapChain.colorFormat;

	vulkanDevice->CreateImage(windowWidth, windowHeight, 1, vulkanDevice->msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msaaRenderTarget.buffer, 1, 0);
	
	vulkanDevice->CreateImageView(msaaRenderTarget.buffer.image, colorFormat, msaaRenderTarget.view, 1, VK_IMAGE_ASPECT_COLOR_BIT);

	TransitionImageLayout(msaaRenderTarget.buffer, colorFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1, 1);

	vulkanDevice->CreateImage(windowWidth, windowHeight, 1, vulkanDevice->msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, msaaDepthStencil.buffer, 1, 0);

	vulkanDevice->CreateImageView(msaaDepthStencil.buffer.image, depthFormat, msaaDepthStencil.view, 1, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
}

void VulkanDriver::CreateShadowDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding depthMatUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	
	std::array<VkDescriptorSetLayoutBinding, 1> sdBindings = { depthMatUniformLayoutBinding };

	VkDescriptorSetLayoutCreateInfo shadowLayoutInfo = {};
	shadowLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	shadowLayoutInfo.bindingCount = static_cast<uint32_t>(sdBindings.size());
	shadowLayoutInfo.pBindings = sdBindings.data();

	ressourcesList.descriptorSetLayouts->add("shadow", shadowLayoutInfo);
}

void VulkanDriver::CreateGraphicPipeline()
{
	std::vector<char> vertShaderCode = LeUTILS::ReadFile("../Data/Shaders/vert.spv");
	std::vector<char> fragShaderCode = LeUTILS::ReadFile("../Data/Shaders/frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	/*FIXED PIPELINE FUNCTION*/

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = LeUTILS::InputAssemblyStateUtils(
		1, 
		static_cast<uint32_t>(verticesDescription.attributeDescriptions.size()),
		verticesDescription.bindingDescriptions.data(),
		verticesDescription.attributeDescriptions.data());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = LeUTILS::PipelineInputAssemblyStateUtils(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = LeUTILS::PipelineViewportUtils((float)windowWidth, (float)windowHeight);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = LeUTILS::PipelineViewportStateUtils(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = LeUTILS::RasterizationStateUtils(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE, 0);
	
	VkPipelineMultisampleStateCreateInfo multisampling = LeUTILS::PipelineMultisampleStateUtils(vulkanDevice->msaaSamples, VK_FALSE);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = LeUTILS::PipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = LeUTILS::PipelineColorBlendStateUtils(&colorBlendAttachment, VK_FALSE);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = LeUTILS::PipelineLayoutInfo(ressourcesList.descriptorSetLayouts->getPtr("main"));

	VkPipelineLayout mainPipelineLayout = ressourcesList.pipelineLayouts->add("main", pipelineLayoutInfo);

	/* !! FIXED PIPELINE FUNCTION*/

	VkPipelineDepthStencilStateCreateInfo depthStencil = LeUTILS::PipelineDepthStencilStateUtils(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = mainPipelineLayout;
	pipelineInfo.renderPass = mainRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &depthStencil;

	ressourcesList.pipelines->addGraphicsPipeline("main", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);

	// Modify parameters objects to create transparent pipelines

	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	depthStencil.depthWriteEnable = VK_FALSE;
	colorBlendAttachment.blendEnable = VK_TRUE;

	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

	fragShaderCode.clear();
	fragShaderCode = LeUTILS::ReadFile("../Data/Shaders/frag.transparent.spv");
	fragShaderModule = CreateShaderModule(fragShaderCode);
	shaderStages[1].module = fragShaderModule;
	// pipeline cull back faces
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;

	ressourcesList.pipelines->addGraphicsPipeline("transparent_back", pipelineInfo, pipelineCache);

	rasterizer.cullMode = VK_CULL_MODE_FRONT_BIT;
	ressourcesList.pipelines->addGraphicsPipeline("transparent_front", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
}

void VulkanDriver::CreateShadowPipeline()
{
	std::vector<char> vertShaderCode = LeUTILS::ReadFile("../Data/Shaders/offscreen.vert.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo};
	
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = LeUTILS::InputAssemblyStateUtils(
		1, 
		static_cast<uint32_t>(verticesDescription.attributeDescriptions.size()),
		verticesDescription.bindingDescriptions.data(),
		verticesDescription.attributeDescriptions.data());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = LeUTILS::PipelineInputAssemblyStateUtils(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = LeUTILS::PipelineViewportUtils((float)offscreenFramebuffer.width, (float)offscreenFramebuffer.height);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = LeUTILS::PipelineViewportStateUtils(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = LeUTILS::RasterizationStateUtils(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, VK_TRUE);
  	 
	VkPipelineMultisampleStateCreateInfo multisampling = LeUTILS::PipelineMultisampleStateUtils(VK_SAMPLE_COUNT_1_BIT, VK_FALSE);

	VkPipelineColorBlendAttachmentState blendAttachmentState = LeUTILS::PipelineColorBlendAttachmentState(0xf, VK_FALSE);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = LeUTILS::PipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = LeUTILS::PipelineColorBlendStateUtils(&colorBlendAttachment, VK_FALSE);

	VkDescriptorSetLayout descriptorSetLayouts[] = { ressourcesList.descriptorSetLayouts->get("shadow"), ressourcesList.descriptorSetLayouts->get("main") };

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = LeUTILS::PipelineLayoutInfo(descriptorSetLayouts, 2);

	VkPipelineLayout shadowPipelineLayout = ressourcesList.pipelineLayouts->add("shadow", pipelineLayoutInfo);

	VkPipelineDepthStencilStateCreateInfo pipelineDepthStencilStateCreateInfo = LeUTILS::PipelineDepthStencilStateUtils(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	pipelineDepthStencilStateCreateInfo.front = pipelineDepthStencilStateCreateInfo.back;
	pipelineDepthStencilStateCreateInfo.back.compareOp = VK_COMPARE_OP_ALWAYS;

	std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_DEPTH_BIAS };
	VkPipelineDynamicStateCreateInfo pipelineDynamicStateCreateInfo = LeUTILS::PipelineDynamicStateUtils(dynamicStateEnables.data(), dynamicStateEnables.size());

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.pDynamicState = &pipelineDynamicStateCreateInfo;
	pipelineInfo.stageCount = 1;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.layout = shadowPipelineLayout;
	pipelineInfo.renderPass = offscreenFramebuffer.renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &pipelineDepthStencilStateCreateInfo;

	ressourcesList.pipelines->addGraphicsPipeline("shadow", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
}

void VulkanDriver::CreateMeshSceneNodeUniformBuffer(MeshSceneNode* node)
{
	VkDeviceSize bufferSize = sizeof(UniformNodeVertexBuffer);

	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, node->uniformNodeMeshStagingBuffer);
	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, node->uniformNodeMeshBuffer);
	
	VkCommandBuffer nodeCopyCommandBuffer = VK_NULL_HANDLE;
	CreateCommandBuffer(nodeCopyCommandBuffer, true);
	node->_copyCommandBuffer = nodeCopyCommandBuffer;
	VkBufferCopy copyRegion = {};
	copyRegion.size = sizeof(UniformNodeVertexBuffer);
	vkCmdCopyBuffer(node->_copyCommandBuffer, node->uniformNodeMeshStagingBuffer.buffer, node->uniformNodeMeshBuffer.buffer, 1, &copyRegion);

	bufferSize = sizeof(UniformMaterialBuffer);

	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, node->uniformNodeMaterialStagingBuffer);
	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, node->uniformNodeMaterialBuffer);

	copyRegion.size = sizeof(UniformMaterialBuffer);
	vkCmdCopyBuffer(node->_copyCommandBuffer, node->uniformNodeMaterialStagingBuffer.buffer, node->uniformNodeMaterialBuffer.buffer, 1, &copyRegion);
	vkEndCommandBuffer(node->_copyCommandBuffer);
}

void VulkanDriver::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	CreateCommandBuffer(commandBuffer, true);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	FlushCommanderBuffer(commandBuffer, graphicQueue, true, true);
}

void VulkanDriver::UpdateShadowDescriptorSet(MeshSceneNode* node)
{
	VkDescriptorSet sDescriptorSet = ressourcesList.descriptorSets->get("shadow");
	if (sDescriptorSet == VK_NULL_HANDLE)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = ressourcesList.descriptorSetLayouts->getPtr("shadow");
		allocInfo.descriptorSetCount = 1;

		sDescriptorSet = ressourcesList.descriptorSets->add("shadow", allocInfo);
	}
			
	VkDescriptorBufferInfo shadowMatrixBufferInfo = {};
	shadowMatrixBufferInfo.buffer = shadowMatrixUniformBuffer->buffers.buffer;
	shadowMatrixBufferInfo.offset = 0;
	shadowMatrixBufferInfo.range = sizeof(ShadowMatrixUniformBufferObject);

	std::vector<VkWriteDescriptorSet> writeDescriptorSets = { LeUTILS::WriteDescriptorSetUtils(sDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &shadowMatrixBufferInfo) };
	vkUpdateDescriptorSets(logicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);
}

void VulkanDriver::CreateQuadDebugShadowPipeline()
{
	std::vector<char> vertShaderCode = LeUTILS::ReadFile("../Data/Shaders/quad.vert.spv");
	std::vector<char> fragShaderCode = LeUTILS::ReadFile("../Data/Shaders/quad.frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	/*FIXED PIPELINE FUNCTION*/

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = LeUTILS::InputAssemblyStateUtils(1, 
		static_cast<uint32_t>(verticesDescription.attributeDescriptions.size()), 
		verticesDescription.bindingDescriptions.data(),
		verticesDescription.attributeDescriptions.data());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = LeUTILS::PipelineInputAssemblyStateUtils(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = LeUTILS::PipelineViewportUtils((float)windowWidth, (float)windowHeight);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = LeUTILS::PipelineViewportStateUtils(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = LeUTILS::RasterizationStateUtils(VK_POLYGON_MODE_FILL, VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, VK_FALSE);

	VkPipelineMultisampleStateCreateInfo multisampling = LeUTILS::PipelineMultisampleStateUtils(vulkanDevice->msaaSamples, VK_FALSE);

	VkPipelineColorBlendAttachmentState blendAttachmentState = LeUTILS::PipelineColorBlendAttachmentState(0xf, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = LeUTILS::PipelineColorBlendStateUtils(&blendAttachmentState, VK_FALSE);

	VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamicState = LeUTILS::PipelineDynamicStateUtils(dynamicStates, 2);

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = LeUTILS::PipelineLayoutInfo(ressourcesList.descriptorSetLayouts->getPtr("quadDebug"), 1);

	VkPipelineLayout quadDebugPipelineLayout = ressourcesList.pipelineLayouts->add("quadDebug", pipelineLayoutInfo);

	/* !! FIXED PIPELINE FUNCTION*/

	VkPipelineDepthStencilStateCreateInfo depthStencil = LeUTILS::PipelineDepthStencilStateUtils(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);
	depthStencil.front = depthStencil.back;

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = quadDebugPipelineLayout;
	pipelineInfo.renderPass = mainRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &depthStencil;

	ressourcesList.pipelines->addGraphicsPipeline("quadDebug", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
}

void VulkanDriver::CreateQuadDebugShadowDescriptorSetLayout()
{	
	VkDescriptorSetLayoutBinding fragLayoutBinding = {};
	fragLayoutBinding.binding = 0;
	fragLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	fragLayoutBinding.descriptorCount = 1;
	fragLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { fragLayoutBinding };

	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	ressourcesList.descriptorSetLayouts->add("quadDebug", layoutInfo);
}

void VulkanDriver::UpdateQuadDebugShadowDescriptorSet()
{
	VkDescriptorSet sDescriptorSet = ressourcesList.descriptorSets->get("quadDebug");
	if (sDescriptorSet == VK_NULL_HANDLE)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = ressourcesList.descriptorSetLayouts->getPtr("quadDebug");
		allocInfo.descriptorSetCount = 1;

		sDescriptorSet = ressourcesList.descriptorSets->add("quadDebug", allocInfo);
	}
	
	VkDescriptorBufferInfo quadBufferInfo = {};

	//quadBufferInfo.buffer = currentScene->shadowDebugNode->uniformNodeMeshBuffer.buffer;
	//quadBufferInfo.offset = 0;
	//quadBufferInfo.range = sizeof(UniformNodeVertexBuffer);

	VkDescriptorImageInfo shadowMapDescInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, offscreenFramebuffer.depth.view, depthSampler);
	
	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	//descriptorWrites[0] = LeUTILS::WriteDescriptorSetUtils(sDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &quadBufferInfo);
	descriptorWrites[0] = LeUTILS::WriteDescriptorSetUtils(sDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, &shadowMapDescInfo);

	vkUpdateDescriptorSets(logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void VulkanDriver::CreateNodeMeshBufferDescriptorSet(MeshSceneNode* node, MeshBuffer* buffer)
{
	//TODO : https://developer.nvidia.com/vulkan-shader-resource-binding

	if (buffer->descriptorSet == VK_NULL_HANDLE)
	{
		VkDescriptorSetLayout layouts[] = { ressourcesList.descriptorSetLayouts->get("main") };
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = layouts;

		DEBUG_CHECK_VK(vkAllocateDescriptorSets(logicalDevice, &allocInfo, &buffer->descriptorSet));
	}

	// Scene buffers
	VkDescriptorBufferInfo bufferSceneVertexInfo = LeUTILS::DescriptorBufferInfoUtils(sceneUniformBuffer->buffers.buffer, sizeof(SceneUniformBufferObject));

	// Node buffers
	VkDescriptorBufferInfo bufferNodeVertexInfo = LeUTILS::DescriptorBufferInfoUtils(node->uniformNodeMeshBuffer.buffer, sizeof(UniformNodeVertexBuffer));

	// Material buffers
	VkDescriptorBufferInfo bufferMaterialInfo = LeUTILS::DescriptorBufferInfoUtils(node->uniformNodeMaterialBuffer.buffer, sizeof(UniformMaterialBuffer));

	// Light buffer
	VkDescriptorBufferInfo lightsInfo = LeUTILS::DescriptorBufferInfoUtils(lightUniformBuffer->buffers.buffer, sizeof(LightUniformBufferObject));

	// Ambient buffer
	VkDescriptorBufferInfo ambientInfo = LeUTILS::DescriptorBufferInfoUtils(ambientUniformBuffer->buffers.buffer, sizeof(AmbientUniformBufferObject));

	// Light Params buffer
	VkDescriptorBufferInfo lightParamsInfo = LeUTILS::DescriptorBufferInfoUtils(lightParametersUniformBuffer->buffers.buffer, sizeof(LightParamsUniformBufferObject));
	
	// Tex buffer
	VkDescriptorImageInfo imageInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, node->GetMesh()->GetMaterial()->texture->textureImageView, node->GetMesh()->GetMaterial()->texture->textureSampler);

	// Normal map buffer
	VkDescriptorImageInfo normalMapImageInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, node->GetMesh()->GetMaterial()->normalMap->textureImageView, normalMapSampler);

	// Specular map buffer
	VkDescriptorImageInfo specularMapImageInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, node->GetMesh()->GetMaterial()->specularMap->textureImageView, specularMapSampler);
	
	// Metallic map buffer
	VkDescriptorImageInfo metallicMapImageInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, node->GetMesh()->GetMaterial()->metallicMap->textureImageView, metallicMapSampler);
	
	// Roughness map buffer
	VkDescriptorImageInfo roughnessMapImageInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, node->GetMesh()->GetMaterial()->roughnessMap->textureImageView, roughnessMapSampler);

	// ShadowMap
	VkDescriptorImageInfo shadowMapDescInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, offscreenFramebuffer.depth.view, depthSampler);

	// Skybox
	VkDescriptorImageInfo skyboxDescInfo = LeUTILS::DescriptorImageInfoUtils(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, skyboxCubeMap->textureImageView, skyboxMapSampler);

	std::array<VkWriteDescriptorSet, 13> descriptorWrites = {};	
	
	descriptorWrites[0] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &bufferSceneVertexInfo);
	descriptorWrites[1] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, &bufferNodeVertexInfo);
	descriptorWrites[2] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2, &bufferMaterialInfo);
	descriptorWrites[3] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3, &lightsInfo);
	descriptorWrites[4] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4, &ambientInfo);
	descriptorWrites[5] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 5, &lightParamsInfo);
	descriptorWrites[6] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 6, &imageInfo);
	descriptorWrites[7] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 7, &normalMapImageInfo);
	descriptorWrites[8] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 8, &specularMapImageInfo);
	descriptorWrites[9] =  LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 9, &metallicMapImageInfo);
	descriptorWrites[10] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 10, &roughnessMapImageInfo);
	descriptorWrites[11] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 11, &shadowMapDescInfo);
	descriptorWrites[12] = LeUTILS::WriteDescriptorSetUtils(buffer->descriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 12, &skyboxDescInfo);

	vkUpdateDescriptorSets(logicalDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void VulkanDriver::CopyBufferToImage(BufferHandle& srcBuffer, BufferHandle& dstImage, int layerCount, uint32_t width, uint32_t height)
{
	dstImage.SetDevice(logicalDevice);
	VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
	CreateCommandBuffer(commandBuffer, true);

	VkBufferImageCopy region = {};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = layerCount;

	region.imageOffset = { 0, 0, 0 };
	region.imageExtent = { width, height, 1	};

	vkCmdCopyBufferToImage( commandBuffer, srcBuffer.buffer, dstImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region );

	FlushCommanderBuffer(commandBuffer, graphicQueue, true, true);
}

void VulkanDriver::CreateTextureBuffer(Texture* texture)
{
	BufferHandle stagingImage;
	//CreateImage(texture->GetDimensions().x, texture->GetDimensions().y, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_LINEAR, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage);
	vulkanDevice->CreateBuffer(texture->GetMemorySize(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage);
	
	void* data;
	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, stagingImage.memory, 0, texture->GetMemorySize(), 0, &data));
	memcpy(data, texture->GetData(), (size_t)texture->GetMemorySize());
	vkUnmapMemory(logicalDevice, stagingImage.memory);
	
	vulkanDevice->CreateImage(texture->GetDimensions().x, texture->GetDimensions().y, texture->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->buffer, 1, 0);
	
	TransitionImageLayout(texture->buffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, texture->mipLevels);
	CopyBufferToImage(stagingImage, texture->buffer, 1, texture->GetDimensions().x, texture->GetDimensions().y);
	//TransitionImageLayout(texture->buffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture->mipLevels);

	GenerateMipMaps(texture->buffer, VK_FORMAT_R8G8B8A8_UNORM, texture->GetDimensions().x, texture->GetDimensions().y, texture->mipLevels);

	vulkanDevice->CreateImageView(texture->buffer.image, VK_FORMAT_R8G8B8A8_UNORM, texture->textureImageView, texture->mipLevels);

	stagingImage.Clear();
}

void VulkanDriver::GenerateMipMaps(BufferHandle& srcBuffer, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) 
		throw std::runtime_error("texture image format does not support linear blitting!");

	VkCommandBuffer tmpCommandBuffer = VK_NULL_HANDLE;
	CreateCommandBuffer(tmpCommandBuffer, true);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = srcBuffer.image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	int32_t mipWidth = texWidth;
	int32_t mipHeight = texHeight;

	for (uint32_t i = 1; i < mipLevels; i++) 
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(tmpCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(tmpCommandBuffer, srcBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, srcBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(tmpCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

		if (mipWidth > 1) mipWidth /= 2;
		if (mipHeight > 1) mipHeight /= 2;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(tmpCommandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	FlushCommanderBuffer(tmpCommandBuffer, graphicQueue, true, true);
}

void VulkanDriver::CreateCubeMapTextureBuffer(Texture* texture, Texture cubeMapTextureArray[6], size_t singleLayerSize)
{
	BufferHandle stagingImage;
	uint32_t cubeMapImageSize = cubeMapTextureArray[0].GetDimensions().x;

	size_t imageSize = singleLayerSize * 6;

	vulkanDevice->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingImage);

	void* data;
	//uint8_t* data;
	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, stagingImage.memory, 0, imageSize, 0, &data));

	for (size_t i = 0; i < 6; ++i)
		memcpy((static_cast<uint8_t*>(data)) + (singleLayerSize * i), cubeMapTextureArray[i].GetData(), singleLayerSize);

	vkUnmapMemory(logicalDevice, stagingImage.memory);

	vulkanDevice->CreateImage(cubeMapImageSize, cubeMapImageSize, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, texture->buffer, 6, VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);

	TransitionImageLayout(texture->buffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, 1);
	CopyBufferToImage(stagingImage, texture->buffer, 6, cubeMapImageSize, cubeMapImageSize);
	TransitionImageLayout(texture->buffer, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 6, 1);

	//vulkanDevice->CreateImageView(texture->buffer.image, VK_FORMAT_R8G8B8A8_UNORM, texture->textureImageView, VK_IMAGE_VIEW_TYPE_CUBE);

	VkImageViewCreateInfo viewCreateInfo = {};
	viewCreateInfo.image = texture->buffer.image;
	viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
	viewCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
	viewCreateInfo.components = { VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A };
	viewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	viewCreateInfo.subresourceRange.baseMipLevel = 0;
	viewCreateInfo.subresourceRange.levelCount = 1;
	viewCreateInfo.subresourceRange.baseArrayLayer = 0;
	viewCreateInfo.subresourceRange.layerCount = 6;
	DEBUG_CHECK_VK(vkCreateImageView(logicalDevice, &viewCreateInfo, nullptr, &texture->textureImageView));

	stagingImage.Clear();
}

void VulkanDriver::CreateMeshBuffers(MeshBuffer* meshBuffer)
{
	VkDeviceSize bufferSize = sizeof(Vertex) * meshBuffer->vertices.size();
	BufferHandle stagingBuffer;
	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

	void* data;
	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, stagingBuffer.memory, 0, bufferSize, 0, &data));
	memcpy(data, meshBuffer->vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBuffer.memory);

	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, meshBuffer->vertexBuffer);
	CopyBuffer(stagingBuffer.buffer, meshBuffer->vertexBuffer.buffer, bufferSize);
	stagingBuffer.Clear();

	// Indices buffer
	bufferSize = 2 * meshBuffer->indices.size();
	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer);

	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, stagingBuffer.memory, 0, bufferSize, 0, &data));
	memcpy(data, meshBuffer->indices.data(), (size_t)bufferSize);
	vkUnmapMemory(logicalDevice, stagingBuffer.memory);

	vulkanDevice->CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, meshBuffer->indexBuffer);
	CopyBuffer(stagingBuffer.buffer, meshBuffer->indexBuffer.buffer, bufferSize);
	stagingBuffer.Clear();
}

void VulkanDriver::CreateSceneObjectsBuffers()
{
	for (SceneNode* node : currentScene->nodes)
	{
		MeshSceneNode* meshSceneNode = static_cast<MeshSceneNode*>(node);
		CreateMeshSceneNodeUniformBuffer(meshSceneNode);

		Mesh* mesh = meshSceneNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); i++)
		{
			CreateMeshBuffers(mesh->GetMeshBuffer(i));
			CreateTextureBuffer(mesh->GetMaterial()->texture);
					   
			VkSamplerCreateInfo samplerInfo = LeUTILS::VkSamplerCreateInfoUtils();
			samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			samplerInfo.minLod = 0;
			samplerInfo.maxLod = static_cast<float>(mesh->GetMaterial()->texture->mipLevels);
			samplerInfo.mipLodBias = 0;
			DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &mesh->GetMaterial()->texture->textureSampler));

			CreateTextureBuffer(mesh->GetMaterial()->normalMap);
			CreateTextureBuffer(mesh->GetMaterial()->specularMap);
			CreateTextureBuffer(mesh->GetMaterial()->metallicMap);
			CreateTextureBuffer(mesh->GetMaterial()->roughnessMap);
			CreateNodeMeshBufferDescriptorSet(meshSceneNode, mesh->GetMeshBuffer(i));
			UpdateShadowDescriptorSet(meshSceneNode);
		}
	}

	int lightIndex = 0;
	for (SceneNode* node : currentScene->lightsCubesNodes)
	{
		MeshSceneNode* meshSceneNode = static_cast<MeshSceneNode*>(node);
		CreateMeshSceneNodeUniformBuffer(meshSceneNode);

		Mesh* mesh = meshSceneNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); i++)
		{
			CreateMeshBuffers(mesh->GetMeshBuffer(i));

			CreateLightCubeBufferDescriptorSet(meshSceneNode, mesh->GetMeshBuffer(i), lightIndex);
		}

		++lightIndex;
	}

	CreateMeshBuffers(currentScene->skyboxNode->mesh->GetMeshBuffer(0));
	CreateMeshBuffers(currentScene->shadowDebugNode->mesh->GetMeshBuffer(0));
}

void VulkanDriver::CreateSyncObjects()
{
	const int imageCount = swapChain.imageCount;

	imageAvailableSemaphores.resize(imageCount);
	renderFinishedSemaphores.resize(imageCount);
	inFlightFences.resize(imageCount);

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;


	for (size_t i = 0; i < imageCount; i++)
	{
		DEBUG_CHECK_VK(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]));
		DEBUG_CHECK_VK(vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]));
		DEBUG_CHECK_VK(vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]));
	}
}

void VulkanDriver::CreateSkyboxPipeline()
{
	std::vector<char> vertShaderCode = LeUTILS::ReadFile("../Data/Shaders/skybox.vert.spv");
	std::vector<char> fragShaderCode = LeUTILS::ReadFile("../Data/Shaders/skybox.frag.spv");

	VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
	VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";
	vertShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";
	fragShaderStageInfo.pSpecializationInfo = nullptr;

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = LeUTILS::InputAssemblyStateUtils(1,
		static_cast<uint32_t>(verticesDescription.attributeDescriptions.size()),
			verticesDescription.bindingDescriptions.data(),
			verticesDescription.attributeDescriptions.data());

	VkPipelineInputAssemblyStateCreateInfo inputAssembly = LeUTILS::PipelineInputAssemblyStateUtils(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE);

	VkViewport viewport = LeUTILS::PipelineViewportUtils((float)windowWidth, (float)windowHeight);

	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = swapChain.swapchainExtent;

	VkPipelineViewportStateCreateInfo viewportState = LeUTILS::PipelineViewportStateUtils(&viewport, &scissor);

	VkPipelineRasterizationStateCreateInfo rasterizer = LeUTILS::RasterizationStateUtils(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE, VK_TRUE);

	VkPipelineMultisampleStateCreateInfo multisampling = LeUTILS::PipelineMultisampleStateUtils(vulkanDevice->msaaSamples, VK_FALSE);

	VkPipelineColorBlendAttachmentState blendAttachmentState = LeUTILS::PipelineColorBlendAttachmentState(0xf, VK_FALSE);

	VkPipelineColorBlendAttachmentState colorBlendAttachment = LeUTILS::PipelineColorBlendAttachmentState(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT, VK_FALSE);

	VkPipelineColorBlendStateCreateInfo colorBlending = LeUTILS::PipelineColorBlendStateUtils(&colorBlendAttachment, VK_FALSE);

	VkDescriptorSetLayout descriptorSetLayouts[] = { ressourcesList.descriptorSetLayouts->get("skybox"), ressourcesList.descriptorSetLayouts->get("main") };
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = LeUTILS::PipelineLayoutInfo(descriptorSetLayouts, 2);

	VkPipelineLayout skyboxPipelineLayout = ressourcesList.pipelineLayouts->add("skybox", pipelineLayoutInfo);

	VkPipelineDepthStencilStateCreateInfo depthStencil = LeUTILS::PipelineDepthStencilStateUtils(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);

	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr;
	pipelineInfo.layout = skyboxPipelineLayout;
	pipelineInfo.renderPass = mainRenderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
	pipelineInfo.basePipelineIndex = -1;
	pipelineInfo.pDepthStencilState = &depthStencil;

	ressourcesList.pipelines->addGraphicsPipeline("skybox", pipelineInfo, pipelineCache);

	vkDestroyShaderModule(logicalDevice, fragShaderModule, nullptr);
	vkDestroyShaderModule(logicalDevice, vertShaderModule, nullptr);
}

void VulkanDriver::LoadCubeMap()
{
	std::vector<std::string> textPath = { "posx.jpg",  "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg", "negz.jpg" };
	Texture textArray[6];

	void* texData[6];
	for (size_t i = 0; i < 6; ++i)
	{
		textArray[i].LoadFile("../Data/Textures/Maskonaive2/" + textPath[i]);
		texData[i] = textArray[i].GetData();
	}

	const VkDeviceSize imageSize = textArray[0].GetDimensions().x * textArray[0].GetDimensions().y * 4 * 6;
	const VkDeviceSize layerSize = imageSize / 6;

	skyboxCubeMap = new Texture();

	CreateCubeMapTextureBuffer(skyboxCubeMap, textArray, layerSize);
}

void VulkanDriver::CreateSkyboxSampler()
{
	VkSamplerCreateInfo sampler = {};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.magFilter = VK_FILTER_LINEAR;
	sampler.minFilter = VK_FILTER_LINEAR;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.compareOp = VK_COMPARE_OP_NEVER;
	sampler.minLod = 0.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	sampler.maxAnisotropy = 1.0f;
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &sampler, nullptr, &skyboxMapSampler));
}

void VulkanDriver::UpdateSkyboxDescriptorSet()
{
	VkDescriptorSet sDescriptorSet = ressourcesList.descriptorSets->get("skybox");

	VkDescriptorImageInfo textureDescriptor = {};
	textureDescriptor.sampler = skyboxMapSampler;
	textureDescriptor.imageView = skyboxCubeMap->textureImageView;
	textureDescriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;


	if (sDescriptorSet == VK_NULL_HANDLE)
	{
		VkDescriptorSetAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = ressourcesList.descriptorSetLayouts->getPtr("skybox");
		allocInfo.descriptorSetCount = 1;

		sDescriptorSet = ressourcesList.descriptorSets->add("skybox", allocInfo);
	}

	std::vector<VkWriteDescriptorSet> writeDescriptorSets;

	VkDescriptorBufferInfo skyboxBufferInfo = {};

	skyboxBufferInfo.buffer = skyboxUniformData->buffers.buffer;
	skyboxBufferInfo.offset = 0;
	skyboxBufferInfo.range = sizeof(SceneUniformBufferObject);

	VkWriteDescriptorSet vertex = LeUTILS::WriteDescriptorSetUtils(sDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &skyboxBufferInfo);
	vertex.pImageInfo = &textureDescriptor;
	VkWriteDescriptorSet fragment = LeUTILS::WriteDescriptorSetUtils(sDescriptorSet, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &skyboxBufferInfo);
	fragment.pImageInfo = &textureDescriptor;
	writeDescriptorSets = { vertex , fragment };

	vkUpdateDescriptorSets(logicalDevice, writeDescriptorSets.size(), writeDescriptorSets.data(), 0, NULL);

	currentScene->skyboxNode->GetMesh()->GetMeshBuffer(0)->descriptorSet = sDescriptorSet;

}

void VulkanDriver::CreateSkyboxDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding skyboxVertexUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
	VkDescriptorSetLayoutBinding skyboxFragmentUniformLayoutBinding = LeUTILS::DescriptorSetLayoutBindingUtils(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT);

	std::array<VkDescriptorSetLayoutBinding, 2> skyboxBindings = { skyboxVertexUniformLayoutBinding, skyboxFragmentUniformLayoutBinding };
	VkDescriptorSetLayoutCreateInfo skyboxLayoutInfo = {};
	skyboxLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	skyboxLayoutInfo.bindingCount = static_cast<uint32_t>(skyboxBindings.size());
	skyboxLayoutInfo.pBindings = skyboxBindings.data();

	ressourcesList.descriptorSetLayouts->add("skybox", skyboxLayoutInfo);
}

void VulkanDriver::CreateTextureSampler()
{
	VkSamplerCreateInfo samplerInfo = LeUTILS::VkSamplerCreateInfoUtils();
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &normalMapSampler));
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &specularMapSampler));
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &metallicMapSampler));
	DEBUG_CHECK_VK(vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &roughnessMapSampler));
}

void VulkanDriver::UpdateSceneUniformBuffer()
{
	SceneUniformBufferObject ubo = {};

	ubo.view = camera.ReturnViewMatrix();

	ubo.proj = glm::perspective(glm::radians(45.0f), swapChain.swapchainExtent.width / (float)swapChain.swapchainExtent.height, 0.1f, 1000.0f);

	ubo.proj[1][1] *= -1;

	ubo.depthVP = shadowMatrixUniformBufferObject.depthVP;

	memcpy(sceneUniformBuffer->data, &ubo, sizeof(SceneUniformBufferObject));
	memcpy(lightUniformBuffer->data, &lightUniformBufferObject, sizeof(LightUniformBufferObject));
	memcpy(ambientUniformBuffer->data, &ambientUniformBufferObject, sizeof(AmbientUniformBufferObject));
	memcpy(lightParametersUniformBuffer->data, &lightParamsUniformBufferObject, sizeof(LightParamsUniformBufferObject)); 
	memcpy(skyboxUniformData->data, &ubo, sizeof(SceneUniformBufferObject));

	FlushCommanderBuffer(uniformSceneCommandBuffer, graphicQueue, false, false);
}

void VulkanDriver::UpdateShadowUniformBuffer(LeLight light, int lightType)
{
	glm::mat4 depthProjectionMatrix(1.0);
	if (light.isVisible)
	{
		if (lightType == 1)
		{
			depthProjectionMatrix = glm::perspective(glm::radians(light.outerAngle), 1.0f, 0.1f, 1000.0f);
		}
		else if (lightType == 2)
			depthProjectionMatrix = glm::ortho(-20.f, 20.f, -20.f, 20.f, 0.1f, 1000.0f);
	}
	
	depthProjectionMatrix[1][1] *= -1;

	glm::vec3 lightFront;

	lightFront.x = cos(glm::radians(light.rotation.x)) * sin(glm::radians(light.rotation.y));
	lightFront.y = sin(glm::radians(light.rotation.x));
	lightFront.z = cos(glm::radians(light.rotation.x)) * cos(glm::radians(light.rotation.y));
	lightFront = glm::normalize(lightFront);
	lightFront.z *= -1.f;

	if (lightType == 2)
	{
		light.position.x = -10.f;
		light.position.y = 9.f;
		light.position.z = 12.f;
		
	}
	
	 glm::mat4 depthViewMatrix = glm::lookAt(glm::vec3(light.position.x, light.position.y, light.position.z), glm::vec3(light.position.x, light.position.y, light.position.z) + lightFront, glm::vec3(0, 1, 0));


	shadowMatrixUniformBufferObject.depthVP = depthProjectionMatrix * depthViewMatrix;
	shadowMatrixUniformBufferObject.lightType = lightType;

	memcpy(shadowMatrixUniformBuffer->data, &shadowMatrixUniformBufferObject, sizeof(ShadowMatrixUniformBufferObject));
}

void VulkanDriver::UpdateMeshUniformBuffer(MeshSceneNode* node)
{
	UniformNodeVertexBuffer nodeVertexData;
	nodeVertexData.model = node->GetTransformation();

	void* data;
	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, node->uniformNodeMeshStagingBuffer.memory, 0, sizeof(UniformNodeVertexBuffer), 0, &data));
	memcpy(data, &nodeVertexData, sizeof(UniformNodeVertexBuffer));
	vkUnmapMemory(logicalDevice, node->uniformNodeMeshStagingBuffer.memory);

	//FlushCommanderBuffer(node->_copyCommandBuffer, graphicQueue, false);
}

void VulkanDriver::UpdateMaterialUniformBuffer(MeshSceneNode* node)
{
	//UniformMaterialBuffer materialData;
	//nodeVertexData.model = node->GetTransformation();

	void* data;
	DEBUG_CHECK_VK(vkMapMemory(logicalDevice, node->uniformNodeMaterialStagingBuffer.memory, 0, sizeof(UniformMaterialBuffer), 0, &data));
	memcpy(data, node->GetMesh()->GetMaterial(), sizeof(UniformMaterialBuffer));
	vkUnmapMemory(logicalDevice, node->uniformNodeMaterialStagingBuffer.memory);

	FlushCommanderBuffer(node->_copyCommandBuffer, graphicQueue, false);
}

void VulkanDriver::PrepareSceneDrawing()
{
	if (!objectBuffersCreated)
	{
		CreateSceneObjectsBuffers();
		UpdateSkyboxDescriptorSet();
		UpdateQuadDebugShadowDescriptorSet();
		objectBuffersCreated = true;
	}

	if (camera.type == camera.MOUSE && InputManager::instance->GetKeyInputDown(GLFW_KEY_ESCAPE))
	{		
		hideGUI = false;
		camera.ExitCameraMode(window);
		cameraButtonValue = 1;
	}

	
	vkWaitForFences(logicalDevice, 1, &inFlightFences[syncIndex], VK_TRUE, UINT64_MAX);

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	CreateImGuiInterface();
	ImGui::Render();

	DEBUG_CHECK_VK(vkAcquireNextImageKHR(logicalDevice, swapChain.GetSwapChain(), UINT64_MAX, imageAvailableSemaphores[syncIndex], VK_NULL_HANDLE, &currentBuffer));
}

void VulkanDriver::PrepareDrawing()
{
	for (auto node : currentScene->nodes)
	{
		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		UpdateMeshUniformBuffer(meshNode);
		UpdateMaterialUniformBuffer(meshNode);
	}

	currentScene->UpdateLightsCubesTransform();

	for (auto node : currentScene->lightsCubesNodes)
	{
		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		UpdateMeshUniformBuffer(meshNode);
		UpdateMaterialUniformBuffer(meshNode);
	}
	
	UpdateShadowUniformBuffer(lightUniformBufferObject.light[0], currentScene->lightProperty[0].lightType);
	UpdateSceneUniformBuffer();

	VkCommandBuffer drawCmdBuf = VK_NULL_HANDLE;
	CreateCommandBuffer(drawCmdBuf, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
	drawCommandBuffer[currentBuffer] = drawCmdBuf;

	// Shadow Pass
	VkExtent2D shadowExtent = {};
	shadowExtent.height = offscreenFramebuffer.height;
	shadowExtent.width = offscreenFramebuffer.width;
	std::array<VkClearValue, 1> shadowClearValues = {};
	shadowClearValues[0].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo shadowRenderPassBeginInfo = LeUTILS::VkRenderPassBeginInfoUtils(offscreenFramebuffer.renderPass, offscreenFramebuffer.frameBuffer, shadowExtent);;
	shadowRenderPassBeginInfo.clearValueCount = 1;
	shadowRenderPassBeginInfo.pClearValues = shadowClearValues.data();

	vkCmdBeginRenderPass(drawCommandBuffer[currentBuffer], &shadowRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
	VkViewport viewport = {};
	viewport.height = (float)offscreenFramebuffer.width;
	viewport.width = (float)offscreenFramebuffer.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	vkCmdSetViewport(drawCommandBuffer[currentBuffer], 0, 1, &viewport);
	
	VkRect2D sScissor = {};
	sScissor.extent.width = offscreenFramebuffer.width;
	sScissor.extent.height = offscreenFramebuffer.height;
	sScissor.offset.x = 0;
	sScissor.offset.y = 0;
	
	vkCmdSetScissor(drawCommandBuffer[currentBuffer], 0, 1, &sScissor);

	vkCmdSetDepthBias(drawCommandBuffer[currentBuffer], 1.25f, 0.0f, 1.75f);

	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("shadow"));
	vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("shadow"), 0, 1, ressourcesList.descriptorSets->getPtr("shadow"), 0, NULL);
	   
	for (auto node : currentScene->nodes)
	{
		if (!node->isVisible)
			continue;

		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		Mesh* mesh = meshNode->GetMesh();
				
		for (size_t i = 0; i < mesh->GetMeshBufferCount(); ++i)
		{
			MeshBuffer* buffer = mesh->GetMeshBuffer(i);

			VkBuffer vertexBuffers[] = { buffer->vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };
			
			vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], buffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("shadow"), 1, 1, &buffer->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], buffer->indices.size(), 1, 0, 0, 0);
		}
	}

	vkCmdEndRenderPass(drawCommandBuffer[currentBuffer]);
	// !!Shadow Pass

	VkRenderPassBeginInfo renderPassInfo = LeUTILS::VkRenderPassBeginInfoUtils(mainRenderPass, frameBuffers[currentBuffer], swapChain.swapchainExtent);
	std::array<VkClearValue, 3> mainClearValues = {};
	mainClearValues[0].color = { (161.f / 255.f), (209.f / 255.f), (72.f / 255.f), 1.0f };
	mainClearValues[1].depthStencil = { 1.0f, 0 };
	mainClearValues[2].color = { (161.f / 255.f), (209.f / 255.f), (72.f / 255.f), 1.0f };
	//mainClearValues[3].depthStencil = { 1.0f, 0 };

	renderPassInfo.clearValueCount = static_cast<uint32_t>(mainClearValues.size());
	renderPassInfo.pClearValues = mainClearValues.data();

	vkCmdBeginRenderPass(drawCommandBuffer[currentBuffer], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	VkClearAttachment clearAttachments[1] = {};

	clearAttachments[0].aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	clearAttachments[0].clearValue.color = { (161.f / 255.f), (209.f / 255.f), (72.f / 255.f), 1.0f };
	clearAttachments[0].colorAttachment = 0;

	VkClearRect clearRect = {};
	clearRect.layerCount = 1;
	clearRect.rect.offset = { 0, 0 };
	clearRect.rect.extent = { windowWidth, windowHeight};

	vkCmdClearAttachments( drawCommandBuffer[currentBuffer], 1, clearAttachments, 1, &clearRect);
}

void VulkanDriver::PrepareMeshDrawing()
{
	// Bind the graphic pipeline
	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("main"));

	for (auto node : currentScene->nodes)
	{
		if (!node->isVisible || node->isTransparent)
			continue;

		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		
		Mesh* mesh = meshNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); ++i)
		{
			MeshBuffer* buffer = mesh->GetMeshBuffer(i);

			VkBuffer vertexBuffers[] = { buffer->vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], buffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("main"), 0, 1, &buffer->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], buffer->indices.size(), 1, 0, 0, 0);
		}
	}

	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("lightCube"));

	int lightIndex = 0;
	for (auto node : currentScene->lightsCubesNodes)
	{
		LeLight* lightData = currentScene->lightProperty[lightIndex].lightData;
		++lightIndex;

		if (lightData->position.w == 0.0f || !lightData->isVisible)
			continue;

		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);

		Mesh* mesh = meshNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); ++i)
		{
			MeshBuffer* buffer = mesh->GetMeshBuffer(i);

			VkBuffer vertexBuffers[] = { buffer->vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], buffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("lightCube"), 0, 1, &buffer->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], buffer->indices.size(), 1, 0, 0, 0);
		}

	}

	VkDeviceSize offsets[] = { 0 };

	if (showShadowMapDebug && !hideGUI)
	{
		MeshBuffer* quadBuffer = currentScene->shadowDebugNode->GetMesh()->GetMeshBuffer(0);
		VkBuffer quadVertexBuffers[] = { quadBuffer->vertexBuffer.buffer };
		vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("quadDebug"), 0, 1, ressourcesList.descriptorSets->getPtr("quadDebug"), 0, NULL);
		vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("quadDebug"));
		vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, quadVertexBuffers, offsets);
		vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], quadBuffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], quadBuffer->indices.size(), 1, 0, 0, 0);
	}

	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("skybox"));

	MeshBuffer* sbBuffer = currentScene->skyboxNode->GetMesh()->GetMeshBuffer(0);

	VkBuffer vertexBuffers[] = { sbBuffer->vertexBuffer.buffer };

	vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], sbBuffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("skybox"), 0, 1, &sbBuffer->descriptorSet, 0, nullptr);

	vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], sbBuffer->indices.size(), 1, 0, 0, 0);

	// We draw first the back faces
	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("transparent_front"));

	for (auto node : currentScene->nodes)
	{
		if (!node->isVisible || !node->isTransparent)
			continue;

		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		
		Mesh* mesh = meshNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); ++i)
		{
			MeshBuffer* buffer = mesh->GetMeshBuffer(i);

			VkBuffer vertexBuffers[] = { buffer->vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], buffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("main"), 0, 1, &buffer->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], buffer->indices.size(), 1, 0, 0, 0);
		}
	}

	vkCmdBindPipeline(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelines->get("transparent_back"));

	for (auto node : currentScene->nodes)
	{
		if (!node->isVisible || !node->isTransparent)
			continue;

		MeshSceneNode* meshNode = static_cast<MeshSceneNode*>(node);
		Mesh* mesh = meshNode->GetMesh();

		for (size_t i = 0; i < mesh->GetMeshBufferCount(); ++i)
		{
			MeshBuffer* buffer = mesh->GetMeshBuffer(i);

			VkBuffer vertexBuffers[] = { buffer->vertexBuffer.buffer };
			VkDeviceSize offsets[] = { 0 };

			vkCmdBindVertexBuffers(drawCommandBuffer[currentBuffer], 0, 1, vertexBuffers, offsets);

			vkCmdBindIndexBuffer(drawCommandBuffer[currentBuffer], buffer->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT16);

			vkCmdBindDescriptorSets(drawCommandBuffer[currentBuffer], VK_PIPELINE_BIND_POINT_GRAPHICS, ressourcesList.pipelineLayouts->get("main"), 0, 1, &buffer->descriptorSet, 0, nullptr);

			vkCmdDrawIndexed(drawCommandBuffer[currentBuffer], buffer->indices.size(), 1, 0, 0, 0);
		}
	}
}

void VulkanDriver::SubmitDrawing()
{

	ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), drawCommandBuffer[currentBuffer]);

	vkCmdEndRenderPass(drawCommandBuffer[currentBuffer]);
	   
	// Commands are ready
	DEBUG_CHECK_VK(vkEndCommandBuffer(drawCommandBuffer[currentBuffer]));

	// Submit the draw commands
	VkSubmitInfo submitDrawInfos = {};
	submitDrawInfos.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitDrawInfos.pCommandBuffers = &drawCommandBuffer[currentBuffer];
	submitDrawInfos.commandBufferCount = 1;

	VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[syncIndex] };
	submitDrawInfos.pSignalSemaphores = signalSemaphores;
	submitDrawInfos.signalSemaphoreCount = 1;

	VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[syncIndex] };
	submitDrawInfos.pWaitSemaphores = waitSemaphores;
	submitDrawInfos.waitSemaphoreCount = 1;

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitDrawInfos.pWaitDstStageMask = waitStages;
		
	vkResetFences(logicalDevice, 1, &inFlightFences[syncIndex]);

	DEBUG_CHECK_VK(vkQueueSubmit(graphicQueue, 1, &submitDrawInfos, inFlightFences[syncIndex]));
	DEBUG_CHECK_VK(vkQueueWaitIdle(graphicQueue));
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain.GetSwapChain() };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &currentBuffer;
	DEBUG_CHECK_VK(vkQueuePresentKHR(presentationQueue, &presentInfo));

	syncIndex = (syncIndex + 1) % swapChain.imageCount;
}