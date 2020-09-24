#pragma once

#include "VulkanDevice.h"
#include "LeSwapChain.h"
#include "VulkanResourceList.h"
#include "Scene.h"
#include "FrameBuffer.h"

#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include <imgui_impl_glfw.h>
#include "LeCamera.h"
#include "InputManager.h"
#include "UniformBufferHandle.h"

struct Resources
{
	PipelineLayoutList*		 pipelineLayouts;
	PipelineList*			 pipelines;
	DescriptorSetLayoutList* descriptorSetLayouts;
	DescriptorSetList*		 descriptorSets;
};

struct VerticesDescription
{
	VkPipelineVertexInputStateCreateInfo inputState;
	std::vector<VkVertexInputBindingDescription> bindingDescriptions;
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
};

// Ubo stuct created on the fly
struct SceneUniformBufferObject
{
	glm::mat4 view;
	glm::mat4 proj;
	glm::mat4 depthVP;
};

struct ShadowMatrixUniformBufferObject
{
	alignas(16) glm::mat4 depthVP = { glm::mat4() };
	alignas(16) int lightType = { 0 };
};

class VulkanDriver
{
public:
	VulkanDriver(unsigned int width, unsigned int height);
	~VulkanDriver();
	
	Scene* CreateEmptyInitialScene();

	void PrepareSceneDrawing();
	void PrepareDrawing();
	void PrepareMeshDrawing();
	void SubmitDrawing();

	GLFWwindow*	window = nullptr;

private:
	unsigned int	windowWidth = 0;
	unsigned int	windowHeight = 0;

	VkDebugReportCallbackEXT debugCallback;

	VulkanDevice* vulkanDevice;
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkInstance instance;
	
	Resources ressourcesList;
	Scene* currentScene;		

	struct
	{
		BufferHandle buffer;
		VkImageView view;
	} msaaRenderTarget;

	struct
	{
		BufferHandle buffer;
		VkImageView view;
	} msaaDepthStencil;
	VkFormat depthFormat;
	
	VkPhysicalDeviceProperties		 deviceProperties;
	VkPhysicalDeviceFeatures		 deviceFeatures;
	VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
	
	ShadowMatrixUniformBufferObject shadowMatrixUniformBufferObject;

	LeSwapChain					swapChain;
	std::vector<VkFramebuffer>	frameBuffers;

	VkQueue							graphicQueue		= VK_NULL_HANDLE;
	VkQueue							presentationQueue	= VK_NULL_HANDLE;
	VkCommandPool					commandPool			= VK_NULL_HANDLE;
	VkCommandBuffer					setupCommandBuffer	= VK_NULL_HANDLE;
	std::vector<VkCommandBuffer>	drawCommandBuffer;
	VkRenderPass					mainRenderPass		= VK_NULL_HANDLE;
	VkPipelineCache					pipelineCache		= VK_NULL_HANDLE;
	VkDescriptorPool				descriptorPool		= VK_NULL_HANDLE;

	VerticesDescription				verticesDescription;

	LeCamera camera;
	
	// Synchronization Objects
	std::vector<VkSemaphore>	imageAvailableSemaphores;
	std::vector<VkSemaphore>	renderFinishedSemaphores;
	std::vector<VkFence>		inFlightFences;
	size_t syncIndex = 0;

	// Run time configuration variables
	bool		hideGUI = false;
	bool		objectBuffersCreated = false;
	bool		openLightSetting = false;
	bool		openMeshSetting = false;
	bool		showShadowMapDebug = false;
	int			cameraButtonValue = 1;
	uint32_t	currentBuffer = 0;

	// Image Sampler
	VkSampler normalMapSampler		= VK_NULL_HANDLE;
	VkSampler specularMapSampler	= VK_NULL_HANDLE;
	VkSampler metallicMapSampler	= VK_NULL_HANDLE;
	VkSampler roughnessMapSampler	= VK_NULL_HANDLE;
	VkSampler depthSampler			= VK_NULL_HANDLE;
	VkSampler skyboxMapSampler		= VK_NULL_HANDLE;

	// OFFSCREEN RENDERING
	OffscreenFrameBuffer offscreenFramebuffer;

	// UBO BUFFERS HANDLE
	UniformBufferHandle* sceneUniformBuffer;
	UniformBufferHandle* lightUniformBuffer;
	UniformBufferHandle* ambientUniformBuffer;
	UniformBufferHandle* lightParametersUniformBuffer;
	UniformBufferHandle* shadowMatrixUniformBuffer;
	UniformBufferHandle* skyboxUniformData;
	VkCommandBuffer		 uniformSceneCommandBuffer;
		
    LightUniformBufferObject        lightUniformBufferObject;
    AmbientUniformBufferObject      ambientUniformBufferObject;
    LightParamsUniformBufferObject  lightParamsUniformBufferObject;

	Texture*		skyboxCubeMap;

	// Initialize
	void drvCreateWindow();
	void CreateInstance();
	void CreateDebugCallback();
	void CreatePhysicalDevice();
	void GetPhysicalDeviceLimitations();
	void InitilizeRessourcesManager();
	void CreateImGuiInterface();
	void CreateQueues();
	void CreateCommandPool();
	void CreateDescriptorPool();
	void SetupSampleValues();
	void CreateTextureSampler();
	void CreatePipelineCache();
	void InitializeImGui();
	void CreateSyncObjects(); 
	void LoadCubeMap();

	void SetupDepthStencilFormat();
	void CreateFrameBuffer();

	void SetupVertexDescriptions();

	// Create and manage UBOs
	void PrepareSceneUniformBuffer();
	void CreateMeshSceneNodeUniformBuffer(MeshSceneNode* node);
	void UpdateSceneUniformBuffer();
	void UpdateShadowUniformBuffer(LeLight light, int lightType);
	void UpdateMeshUniformBuffer(MeshSceneNode* node);
	void UpdateMaterialUniformBuffer(MeshSceneNode* node);

	// Create Scene Rendering Objects
	void CreateMsaaRessources();
	void CreateRenderPass();
	void CreateGraphicPipeline();
	void CreateSceneDescriptorSetLayout();
	void CreateNodeMeshBufferDescriptorSet(MeshSceneNode* node, MeshBuffer* buffer);
	
	// Create Skybox Rendering Objects
	void CreateSkyboxPipeline();
	void CreateSkyboxSampler();
	void UpdateSkyboxDescriptorSet();
	void CreateSkyboxDescriptorSetLayout();

	// Create Light Cube Objects
	void CreateLightCubePipeline();
	void CreateLightCubeDescriptorSetLayout();
	void CreateLightCubeBufferDescriptorSet(MeshSceneNode* node, MeshBuffer* buffer, int lightIndex);

	// Create Shadow Rendering Objects
	void PrepareOffscreenRendering();
	void CreateShadowPipeline();
	void CreateShadowDescriptorSetLayout();
	void UpdateShadowDescriptorSet(MeshSceneNode* node);

	// Create quad for light shadow map debug
	void CreateQuadDebugShadowPipeline();
	void CreateQuadDebugShadowDescriptorSetLayout();
	void UpdateQuadDebugShadowDescriptorSet();

	// Drawing Preparation
	void CreateSceneObjectsBuffers();
	void CreateMeshBuffers(MeshBuffer* meshBuffer);

	// Buffer Management
	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
	void CreateTextureBuffer(Texture* texture);
	void GenerateMipMaps(BufferHandle& srcBuffer, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
	void CreateCubeMapTextureBuffer(Texture* texture, Texture* cubeMapTextureArray, size_t singleLayerSize);
	void CopyBufferToImage(BufferHandle& srcBuffer, BufferHandle& dstImage, int layerCount, uint32_t width, uint32_t height);
	
	// Command Buffer Management
	void CreateCommandBuffer(VkCommandBuffer& cdBuffer, bool shouldStart = false);
	void CreateCommandBuffer(VkCommandBuffer& cdBuffer, VkCommandBufferUsageFlagBits flags);
	void CreateCommandBuffer(VkCommandBuffer* cdBuffer, uint32_t bgCount);
	void FlushCommanderBuffer(VkCommandBuffer& commandBuffer, VkQueue queue, bool shouldEnd = false, bool shouldErase = false);

	// Other
	void			CleanUp();
	VkShaderModule	CreateShaderModule(const std::vector<char>& code);
	void			TransitionImageLayout(BufferHandle& image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t arrayLayers, uint32_t mipLevels);
	
	//void CreateAttachment(VkFormat format, VkImageUsageFlagBits usage, FrameBufferAttachment* attachment, VkCommandBuffer layoutCmd, uint32_t width, uint32_t height);
};

