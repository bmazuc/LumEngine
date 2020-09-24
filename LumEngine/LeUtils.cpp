#include "LeUtils.h"
#include <Windows.h>
#include <iostream>
#include <fstream>

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanReportFunc(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{
	std::cout << "[VULKAN VALIDATION]" << msg << std::endl;
#ifdef _WIN32
	if (IsDebuggerPresent()) {
		OutputDebugStringA("[VULKAN VALIDATION] ");
		OutputDebugStringA(msg);
		OutputDebugStringA("\n");
	}
#endif
	return VK_FALSE;
}

namespace LeUTILS
{

std::string GetErrorString(VkResult errorCode)
{
	switch (errorCode)
	{
#define STR(r) case VK_ ##r: return #r
		STR(NOT_READY);
		STR(TIMEOUT);
		STR(EVENT_SET);
		STR(EVENT_RESET);
		STR(INCOMPLETE);
		STR(ERROR_OUT_OF_HOST_MEMORY);
		STR(ERROR_OUT_OF_DEVICE_MEMORY);
		STR(ERROR_INITIALIZATION_FAILED);
		STR(ERROR_DEVICE_LOST);
		STR(ERROR_MEMORY_MAP_FAILED);
		STR(ERROR_LAYER_NOT_PRESENT);
		STR(ERROR_EXTENSION_NOT_PRESENT);
		STR(ERROR_FEATURE_NOT_PRESENT);
		STR(ERROR_INCOMPATIBLE_DRIVER);
		STR(ERROR_TOO_MANY_OBJECTS);
		STR(ERROR_FORMAT_NOT_SUPPORTED);
		STR(ERROR_SURFACE_LOST_KHR);
		STR(ERROR_NATIVE_WINDOW_IN_USE_KHR);
		STR(SUBOPTIMAL_KHR);
		STR(ERROR_OUT_OF_DATE_KHR);
		STR(ERROR_INCOMPATIBLE_DISPLAY_KHR);
		STR(ERROR_VALIDATION_FAILED_EXT);
		STR(ERROR_INVALID_SHADER_NV);
#undef STR
	default:
		return "UNKNOWN_ERROR";
	}
}
void SetupDebugCallback(VkInstance instance, VkDebugReportCallbackEXT& callback)
{
	VkDebugReportCallbackCreateInfoEXT debugCallbackInfo = {};
	debugCallbackInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
	debugCallbackInfo.flags = VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT |
		VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_DEBUG_BIT_EXT | VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
	debugCallbackInfo.pfnCallback = VulkanReportFunc;
	vkCreateDebugReportCallbackEXT(instance, &debugCallbackInfo, nullptr, &callback);
}

VkSampleCountFlagBits GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
	VkSampleCountFlags counts = min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

VkSemaphoreCreateInfo SemaphoreCreateInfoUtils()
{
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	semaphoreCreateInfo.pNext = NULL;
	semaphoreCreateInfo.flags = 0;
	return semaphoreCreateInfo;
}

VkCommandPoolCreateInfo CommandPoolCreateInfoUtils(uint32_t queueFamilyIndex)
{
	VkCommandPoolCreateInfo cmdPoolInfo = {};
	cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
	cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	return cmdPoolInfo;
}

VkCommandBufferAllocateInfo CommandBufferAllocateUtils(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount)
{
	VkCommandBufferAllocateInfo commandBufferAllocateInfo = {};
	commandBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocateInfo.commandPool = commandPool;
	commandBufferAllocateInfo.level = level;
	commandBufferAllocateInfo.commandBufferCount = bufferCount;
	return commandBufferAllocateInfo;
}

VkCommandBufferBeginInfo CommandBufferBeginInfoUtils()
{
	VkCommandBufferBeginInfo cmdBufInfo = {};
	cmdBufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	return cmdBufInfo;
}

VkBool32 GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat)
{
	std::vector<VkFormat> depthFormats = {
	VK_FORMAT_D32_SFLOAT_S8_UINT,
	VK_FORMAT_D32_SFLOAT,
	VK_FORMAT_D24_UNORM_S8_UINT,
	VK_FORMAT_D16_UNORM_S8_UINT,
	VK_FORMAT_D16_UNORM
	};

	for (auto& format : depthFormats)
	{
		VkFormatProperties formatProps;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &formatProps);
		if (formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			*depthFormat = format;
			return true;
		}
	}

	return false;
}

VkDescriptorPoolSize GetDescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount)
{
	VkDescriptorPoolSize descriptorPoolSize = {};
	descriptorPoolSize.type = type;
	descriptorPoolSize.descriptorCount = descriptorCount;
	return descriptorPoolSize;
}

VkDescriptorPoolCreateInfo DescriptorPoolCreateInfoUtils(uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets)
{
	VkDescriptorPoolCreateInfo descriptorPoolInfo = {};
	descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolInfo.pNext = NULL;
	descriptorPoolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolInfo.poolSizeCount = poolSizeCount;
	descriptorPoolInfo.pPoolSizes = pPoolSizes;
	descriptorPoolInfo.maxSets = maxSets;
	return descriptorPoolInfo;
}
VkVertexInputBindingDescription VertexInputBindingDescriptionUtils(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate)
{
	VkVertexInputBindingDescription vInputBindDescription = {};
	vInputBindDescription.binding = binding;
	vInputBindDescription.stride = stride;
	vInputBindDescription.inputRate = inputRate;
	return vInputBindDescription;
}

VkVertexInputAttributeDescription VertexInputAttributeDescriptionUtils(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset)
{
	VkVertexInputAttributeDescription vInputAttribDescription = {};
	vInputAttribDescription.location = location;
	vInputAttribDescription.binding = binding;
	vInputAttribDescription.format = format;
	vInputAttribDescription.offset = offset;
	return vInputAttribDescription;
}

VkPipelineVertexInputStateCreateInfo PipelineVertexInputStateCreateInfoUtils()
{
	VkPipelineVertexInputStateCreateInfo pipelineVertexInputStateCreateInfo = {};
	pipelineVertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	pipelineVertexInputStateCreateInfo.pNext = NULL;
	return pipelineVertexInputStateCreateInfo;
}

VkImageCreateInfo ImageCreateInfoUtils()
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.pNext = NULL;
	return imageCreateInfo;
}

VkImageViewCreateInfo ImageViewCreateInfo()
{
	VkImageViewCreateInfo imageViewCreateInfo = {};
	imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	imageViewCreateInfo.pNext = NULL;
	return imageViewCreateInfo;
}

	VkMemoryAllocateInfo MemoryAllocateInfoUtils()
{
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.pNext = NULL;
	memAllocInfo.allocationSize = 0;
	memAllocInfo.memoryTypeIndex = 0;
	return memAllocInfo;
}

VkFramebufferCreateInfo FramebufferCreateInfoUtils()
{
	VkFramebufferCreateInfo framebufferCreateInfo = {};
	framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	framebufferCreateInfo.pNext = NULL;
	return framebufferCreateInfo;
}

VkSamplerCreateInfo SamplerCreateInfoUtils()
{
	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.pNext = NULL;
	return samplerCreateInfo;
}

VkBufferCreateInfo BufferCreateInfoUtils(VkBufferUsageFlags usage, VkDeviceSize size)
{
	VkBufferCreateInfo bufCreateInfo = {};
	bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufCreateInfo.pNext = NULL;
	bufCreateInfo.usage = usage;
	bufCreateInfo.size = size;
	bufCreateInfo.flags = 0;
	return bufCreateInfo;
}
std::vector<char> ReadFile(const std::string & filename)
{
	std::ifstream file(filename, std::ios::ate | std::ios::binary);

	if (!file.is_open())
		throw std::runtime_error(std::string{ "échec de l'ouverture du fichier " } +filename + "!");

	size_t fileSize = (size_t)file.tellg();
	std::vector<char> buffer(fileSize);

	file.seekg(0);
	file.read(buffer.data(), fileSize);
	file.close();

	return buffer;
}

VkRenderPassBeginInfo VkRenderPassBeginInfoUtils(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D& extent)
{
	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;
	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = extent;

	return renderPassInfo;
}

VkSamplerCreateInfo VkSamplerCreateInfoUtils()
{
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	samplerInfo.anisotropyEnable = VK_TRUE;
	samplerInfo.maxAnisotropy = 16;
	samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerInfo.unnormalizedCoordinates = VK_FALSE;
	samplerInfo.compareEnable = VK_FALSE;
	samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;

	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.minLod = 0.0f;
	samplerInfo.maxLod = 0.0f;

	return samplerInfo;
}

VkWriteDescriptorSet WriteDescriptorSetUtils(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding,
	VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pBufferInfo = bufferInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	return writeDescriptorSet;
}

VkWriteDescriptorSet WriteDescriptorSetUtils(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding,
	VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount)
{
	VkWriteDescriptorSet writeDescriptorSet{};
	writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	writeDescriptorSet.dstSet = dstSet;
	writeDescriptorSet.descriptorType = type;
	writeDescriptorSet.dstBinding = binding;
	writeDescriptorSet.pImageInfo = imageInfo;
	writeDescriptorSet.descriptorCount = descriptorCount;
	return writeDescriptorSet;
}

VkDescriptorBufferInfo DescriptorBufferInfoUtils(VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset)
{
	VkDescriptorBufferInfo bufferSceneVertexInfo = {};
	bufferSceneVertexInfo.buffer = buffer;
	bufferSceneVertexInfo.offset = offset;
	bufferSceneVertexInfo.range = range;

	return bufferSceneVertexInfo;
}

VkDescriptorImageInfo DescriptorImageInfoUtils(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler)
{
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = imageLayout;
	imageInfo.imageView = imageView;
	imageInfo.sampler = sampler;
	return imageInfo;
}

VkDescriptorSetLayoutBinding DescriptorSetLayoutBindingUtils(int binding, VkDescriptorType descType,
	VkShaderStageFlags shaderStage)
{
	VkDescriptorSetLayoutBinding sceneUniformLayoutBinding = {};
	sceneUniformLayoutBinding.binding = binding;
	sceneUniformLayoutBinding.descriptorType = descType;
	sceneUniformLayoutBinding.descriptorCount = 1;
	sceneUniformLayoutBinding.stageFlags = shaderStage;
	sceneUniformLayoutBinding.pImmutableSamplers = nullptr;
	return sceneUniformLayoutBinding;
}

VkPipelineVertexInputStateCreateInfo InputAssemblyStateUtils(int bindingCount, int attributeCount,
	VkVertexInputBindingDescription* bindingDesc, VkVertexInputAttributeDescription* attribDesc)
{
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = bindingCount;
	vertexInputInfo.vertexAttributeDescriptionCount = attributeCount;
	vertexInputInfo.pVertexBindingDescriptions = bindingDesc;
	vertexInputInfo.pVertexAttributeDescriptions = attribDesc;
	return vertexInputInfo;
}

VkPipelineRasterizationStateCreateInfo RasterizationStateUtils(VkPolygonMode polygonMode, VkCullModeFlags cullMode,
	VkFrontFace frontFace, VkBool32 depthBiasEnable, VkPipelineRasterizationStateCreateFlags flags)
{
	VkPipelineRasterizationStateCreateInfo pipelineRasterizationStateCreateInfo{};
	pipelineRasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	pipelineRasterizationStateCreateInfo.polygonMode = polygonMode;
	pipelineRasterizationStateCreateInfo.cullMode = cullMode;
	pipelineRasterizationStateCreateInfo.frontFace = frontFace;
	pipelineRasterizationStateCreateInfo.flags = flags;
	pipelineRasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
	pipelineRasterizationStateCreateInfo.lineWidth = 1.0f;
	pipelineRasterizationStateCreateInfo.depthBiasEnable = depthBiasEnable;
	pipelineRasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasClamp = 0.0f;
	pipelineRasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;
	return pipelineRasterizationStateCreateInfo;
}

VkPipelineMultisampleStateCreateInfo PipelineMultisampleStateUtils(VkSampleCountFlagBits sampleCount, VkBool32 sampleShading)
{
	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.rasterizationSamples = sampleCount;
	multisampling.sampleShadingEnable = sampleShading;
	multisampling.minSampleShading = 1.0f;
	multisampling.pSampleMask = nullptr;
	multisampling.alphaToCoverageEnable = VK_FALSE;
	multisampling.alphaToOneEnable = VK_FALSE;
	return multisampling;
}

VkPipelineColorBlendAttachmentState PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 enableBlend)
{
	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = colorWriteMask;
	colorBlendAttachment.blendEnable = enableBlend;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	return colorBlendAttachment;
}

VkPipelineColorBlendStateCreateInfo PipelineColorBlendStateUtils(VkPipelineColorBlendAttachmentState* attachement, VkBool32 enableLogicOp)
{
	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = enableLogicOp;
	colorBlending.logicOp = VK_LOGIC_OP_COPY;
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = attachement;
	colorBlending.blendConstants[0] = 0.0f;
	colorBlending.blendConstants[1] = 0.0f;
	colorBlending.blendConstants[2] = 0.0f;
	colorBlending.blendConstants[3] = 0.0f;
	return colorBlending;
}

VkPipelineDynamicStateCreateInfo PipelineDynamicStateUtils(VkDynamicState* dynamicStateArray, int stateCount)
{
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = stateCount;
	dynamicState.pDynamicStates = dynamicStateArray;
	return dynamicState;
}

VkPipelineLayoutCreateInfo PipelineLayoutInfo(VkDescriptorSetLayout* layout, int layoutCount)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = layoutCount;
	pipelineLayoutInfo.pSetLayouts = layout;
	pipelineLayoutInfo.pushConstantRangeCount = 0;
	pipelineLayoutInfo.pPushConstantRanges = nullptr;
	return pipelineLayoutInfo;
}

VkPipelineDepthStencilStateCreateInfo PipelineDepthStencilStateUtils(VkBool32 enableDepthTest, VkBool32 enableDepthWrite, VkCompareOp compareOp)
{
	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = enableDepthTest;
	depthStencil.depthWriteEnable = enableDepthWrite;
	depthStencil.depthCompareOp = compareOp;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f;
	depthStencil.maxDepthBounds = 1.0f;
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {};
	depthStencil.back = {};
	return depthStencil;
}

VkPipelineInputAssemblyStateCreateInfo PipelineInputAssemblyStateUtils(VkPrimitiveTopology primitiveTopology, VkBool32 enablePrimitiveRestart)
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = primitiveTopology;
	inputAssembly.primitiveRestartEnable = enablePrimitiveRestart;
	return inputAssembly;
}

VkViewport PipelineViewportUtils(float windowWidth, float windowHeight)
{
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = windowWidth;
	viewport.height = windowHeight;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	return viewport;
}

VkPipelineViewportStateCreateInfo PipelineViewportStateUtils(VkViewport* viewport, VkRect2D* scissor)
{
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = scissor;
	return viewportState;
}
}
