#pragma once
#define VK_NO_PROTOTYPES
#include <volk.h>
#include <vector>

namespace LeUTILS
{
	std::string								GetErrorString(VkResult errorCode);
	void									SetupDebugCallback(VkInstance instance, VkDebugReportCallbackEXT& callback);
	VkSampleCountFlagBits					GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
	VkSemaphoreCreateInfo					SemaphoreCreateInfoUtils();
	VkCommandPoolCreateInfo					CommandPoolCreateInfoUtils(uint32_t queueFamilyIndex);
	VkCommandBufferAllocateInfo				CommandBufferAllocateUtils(VkCommandPool commandPool, VkCommandBufferLevel level, uint32_t bufferCount);
	VkCommandBufferBeginInfo				CommandBufferBeginInfoUtils();
	VkBool32								GetSupportedDepthFormat(VkPhysicalDevice physicalDevice, VkFormat* depthFormat);
	VkDescriptorPoolSize					GetDescriptorPoolSize(VkDescriptorType type, uint32_t descriptorCount);
	VkDescriptorPoolCreateInfo				DescriptorPoolCreateInfoUtils(uint32_t poolSizeCount, VkDescriptorPoolSize* pPoolSizes, uint32_t maxSets);
	VkVertexInputBindingDescription			VertexInputBindingDescriptionUtils(uint32_t binding, uint32_t stride, VkVertexInputRate inputRate);
	VkVertexInputAttributeDescription		VertexInputAttributeDescriptionUtils(uint32_t binding, uint32_t location, VkFormat format, uint32_t offset);
	VkPipelineVertexInputStateCreateInfo	PipelineVertexInputStateCreateInfoUtils();
	VkImageCreateInfo						ImageCreateInfoUtils();
	VkImageViewCreateInfo					ImageViewCreateInfo();
	VkMemoryAllocateInfo					MemoryAllocateInfoUtils();
	VkFramebufferCreateInfo					FramebufferCreateInfoUtils();
	VkSamplerCreateInfo						SamplerCreateInfoUtils();
	VkBufferCreateInfo						BufferCreateInfoUtils(VkBufferUsageFlags usage, VkDeviceSize size);
	std::vector<char>						ReadFile(const std::string & filename);
	VkRenderPassBeginInfo					VkRenderPassBeginInfoUtils(VkRenderPass renderPass, VkFramebuffer frameBuffer, VkExtent2D& extents);
	VkSamplerCreateInfo						VkSamplerCreateInfoUtils();
	VkWriteDescriptorSet					WriteDescriptorSetUtils(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorBufferInfo* bufferInfo, uint32_t descriptorCount = 1);
	VkWriteDescriptorSet					WriteDescriptorSetUtils(VkDescriptorSet dstSet, VkDescriptorType type, uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t descriptorCount = 1);
	VkDescriptorBufferInfo					DescriptorBufferInfoUtils(VkBuffer buffer, VkDeviceSize range, VkDeviceSize offset = 0);
	VkDescriptorImageInfo					DescriptorImageInfoUtils(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler);
	VkDescriptorSetLayoutBinding			DescriptorSetLayoutBindingUtils(int binding, VkDescriptorType descType, VkShaderStageFlags shaderStage);
	VkPipelineVertexInputStateCreateInfo	InputAssemblyStateUtils(int bindingCount, int attributeCount, VkVertexInputBindingDescription* bindingDesc, VkVertexInputAttributeDescription* attribDesc);
	VkPipelineRasterizationStateCreateInfo  RasterizationStateUtils(VkPolygonMode polygonMode, VkCullModeFlags cullMode, VkFrontFace frontFace, VkBool32 depthBiasEnable, VkPipelineRasterizationStateCreateFlags flags = 0);
	VkPipelineMultisampleStateCreateInfo	PipelineMultisampleStateUtils(VkSampleCountFlagBits sampleCount, VkBool32 sampleShading);
	VkPipelineColorBlendAttachmentState		PipelineColorBlendAttachmentState(VkColorComponentFlags colorWriteMask, VkBool32 enableBlend);
	VkPipelineColorBlendStateCreateInfo		PipelineColorBlendStateUtils(VkPipelineColorBlendAttachmentState* attachement, VkBool32 enableLogicOp);
	VkPipelineDynamicStateCreateInfo		PipelineDynamicStateUtils(VkDynamicState* dynamicStateArray, int stateCount);
	VkPipelineLayoutCreateInfo				PipelineLayoutInfo(VkDescriptorSetLayout* layout, int layoutCount = 1);
	VkPipelineDepthStencilStateCreateInfo	PipelineDepthStencilStateUtils(VkBool32 enableDepthTest, VkBool32 enableDepthWrite, VkCompareOp compareOp);
	VkPipelineInputAssemblyStateCreateInfo	PipelineInputAssemblyStateUtils(VkPrimitiveTopology primitiveTopology, VkBool32 enablePrimitiveRestart);
	VkViewport								PipelineViewportUtils(float windowWidth, float windowHeight);
	VkPipelineViewportStateCreateInfo		PipelineViewportStateUtils(VkViewport* viewport, VkRect2D* scissor);
}