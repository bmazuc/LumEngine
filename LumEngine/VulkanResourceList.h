#pragma once

#define VK_NO_PROTOTYPES
#include "volk.h"
#include <string>
#include <unordered_map>
#include <iostream>
#include <fstream>

#ifdef _DEBUG
	#define DEBUG_CHECK_VK(x) if (VK_SUCCESS != (x)) { std::cout << (#x) << std::endl; __debugbreak(); }
#else
	#define DEBUG_CHECK_VK(x) 
#endif

template <typename T>
class VulkanResourceList
{
public:
	VkDevice &device;
	std::unordered_map<std::string, T> resources;
	VulkanResourceList(VkDevice &dev) : device(dev) {};
	const T get(std::string name)
	{
		return resources[name];
	}
	T *getPtr(std::string name)
	{
		return &resources[name];
	}
	bool present(std::string name)
	{
		return resources.find(name) != resources.end();
	}
};

class PipelineLayoutList : public VulkanResourceList<VkPipelineLayout>
{
public:
	PipelineLayoutList(VkDevice &dev) : VulkanResourceList(dev) {};

	~PipelineLayoutList()
	{
		for (auto& pipelineLayout : resources)
		{
			vkDestroyPipelineLayout(device, pipelineLayout.second, nullptr);
		}
	}

	VkPipelineLayout add(std::string name, VkPipelineLayoutCreateInfo &createInfo)
	{
		VkPipelineLayout pipelineLayout;
		DEBUG_CHECK_VK(vkCreatePipelineLayout(device, &createInfo, nullptr, &pipelineLayout));
		resources[name] = pipelineLayout;
		return pipelineLayout;
	}
};

class PipelineList : public VulkanResourceList<VkPipeline>
{
public:
	PipelineList(VkDevice &dev) : VulkanResourceList(dev) {};

	~PipelineList()
	{
		for (auto& pipeline : resources)
		{
			vkDestroyPipeline(device, pipeline.second, nullptr);
		}
	}

	VkPipeline addGraphicsPipeline(std::string name, VkGraphicsPipelineCreateInfo &pipelineCreateInfo, VkPipelineCache &pipelineCache)
	{
		VkPipeline pipeline;
		DEBUG_CHECK_VK(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline));
		resources[name] = pipeline;
		return pipeline;
	}
};

class DescriptorSetLayoutList : public VulkanResourceList<VkDescriptorSetLayout>
{
public:
	DescriptorSetLayoutList(VkDevice &dev) : VulkanResourceList(dev) {};

	~DescriptorSetLayoutList()
	{
		for (auto& descriptorSetLayout : resources)
		{
			vkDestroyDescriptorSetLayout(device, descriptorSetLayout.second, nullptr);
		}
	}

	VkDescriptorSetLayout add(std::string name, VkDescriptorSetLayoutCreateInfo createInfo)
	{
		VkDescriptorSetLayout descriptorSetLayout;
		DEBUG_CHECK_VK(vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descriptorSetLayout));
		resources[name] = descriptorSetLayout;
		return descriptorSetLayout;
	}
};

class DescriptorSetList : public VulkanResourceList<VkDescriptorSet>
{
private:
	VkDescriptorPool descriptorPool;
public:
	DescriptorSetList(VkDevice &dev, VkDescriptorPool pool) : VulkanResourceList(dev), descriptorPool(pool) {};

	~DescriptorSetList()
	{
		for (auto& descriptorSet : resources)
		{
			vkFreeDescriptorSets(device, descriptorPool, 1, &descriptorSet.second);
		}
	}

	VkDescriptorSet add(std::string name, VkDescriptorSetAllocateInfo allocInfo)
	{
		VkDescriptorSet descriptorSet;
		DEBUG_CHECK_VK(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		resources[name] = descriptorSet;
		return descriptorSet;
	}
};