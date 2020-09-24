#pragma once

#include <string>
#include "glm/vec4.hpp"
#include <vector>
#include "Texture.h"

enum class LeMaterialTemplate : int
{
	Default = 0,
	Silver = 1,
	Aluminum = 2,
	Platinum = 3,
	Iron = 4,
	Titanium = 5,
	Copper = 6,
	Gold = 7,
	Brass = 8,

	Coal = 9,
	Rubber = 10,
	Mud = 11,
	Wood = 12,
	Vegetation = 13,
	Brick = 14,
	Sand = 15,
	Concrete = 16
};

struct UniformMaterialBuffer
{
	alignas(16) glm::vec4	color;
	alignas(4) float		reflectance;
	alignas(4) float		roughness;
	alignas(4) float		metallic;
};

struct LeMaterial
{
	UniformMaterialBuffer params;

	Texture* texture;
	Texture* normalMap;
	Texture* specularMap;
	Texture* metallicMap;
	Texture* roughnessMap;

	std::string name;

	LeMaterial();
	LeMaterial(std::string _name, glm::vec4 color, float roughness = 0.5f, float metallic = 0.f, float reflectance = 0.5f);

	void CopyMaterial(LeMaterial materialToCopy);

	// Materials template
	static LeMaterial const Silver;
	static LeMaterial const Aluminum;
	static LeMaterial const Platinum;
	static LeMaterial const Iron;
	static LeMaterial const Titanium;
	static LeMaterial const Copper;
	static LeMaterial const Gold;
	static LeMaterial const Brass;

	static LeMaterial const Coal;
	static LeMaterial const Rubber;
	static LeMaterial const Mud;
	static LeMaterial const Wood;
	static LeMaterial const Vegetation;
	static LeMaterial const Brick;
	static LeMaterial const Sand;
	static LeMaterial const Concrete;

	// For debug
	int selectedMaterialTemplate = 0;
	static LeMaterial const GetMaterialTemplate(LeMaterialTemplate materialTemplate);
};