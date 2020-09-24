#include "LeMaterial.h"

// source = https://github.com/google/filament/blob/master/docs/Material%20Properties.pdf
LeMaterial const LeMaterial::Silver = LeMaterial("Silver", glm::vec4(250.f, 249.f, 245.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Aluminum = LeMaterial("Aluminum", glm::vec4(244.f, 245.f, 245.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Platinum = LeMaterial("Platinum", glm::vec4(214.f, 209.f, 200.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Iron = LeMaterial("Iron", glm::vec4(192.f, 189.f, 186.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Titanium = LeMaterial("Titanium", glm::vec4(206.f, 200.f, 194.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Copper = LeMaterial("Copper", glm::vec4(251.f, 216.f, 184.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Gold = LeMaterial("Gold", glm::vec4(255.f, 220.f, 157.f, 1.0f) / 255.f, 0.1f, 1.0f);
LeMaterial const LeMaterial::Brass = LeMaterial("Brass", glm::vec4(244.f, 228.f, 173.f, 1.0f) / 255.f, 0.1f, 1.0f);

LeMaterial const LeMaterial::Coal = LeMaterial("Coal", glm::vec4(50.f, 50.f, 50.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Rubber = LeMaterial("Rubber", glm::vec4(53.f, 53.f, 53.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Mud = LeMaterial("Mud", glm::vec4(85.f, 61.f, 49.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Wood = LeMaterial("Wood", glm::vec4(135.f, 92.f, 60.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Vegetation = LeMaterial("Vegetation", glm::vec4(123.f, 130.f, 78.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Brick = LeMaterial("Brick", glm::vec4(148.f, 125.f, 117.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Sand = LeMaterial("Sand", glm::vec4(177.f, 168.f, 132.f, 1.0f) / 255.f);
LeMaterial const LeMaterial::Concrete = LeMaterial("Concrete", glm::vec4(192.f, 191.f, 187.f, 1.0f) / 255.f);

LeMaterial::LeMaterial()
	: name("Default")
{
	params.color			= glm::vec4(1.f, 1.f, 1.f, 1.f);
	params.roughness		= 0.5f;
	params.metallic			= 0.f;
	params.reflectance		= 0.5f;
};

LeMaterial::LeMaterial(std::string _name, glm::vec4 color, float roughness, float metallic, float reflectance)
	: name(_name)
{
	params.color			= color;
	params.roughness		= roughness;
	params.metallic			= metallic;
	params.reflectance		= reflectance;
};

void LeMaterial::CopyMaterial(LeMaterial materialToCopy)
{
	name	= materialToCopy.name;
	params	= materialToCopy.params;
}

LeMaterial const LeMaterial::GetMaterialTemplate(LeMaterialTemplate materialTemplate)
{
	switch (materialTemplate)
	{
		case LeMaterialTemplate::Silver:		return LeMaterial::Silver;		break;
		case LeMaterialTemplate::Aluminum:		return LeMaterial::Aluminum;	break;
		case LeMaterialTemplate::Platinum:		return LeMaterial::Platinum;	break;
		case LeMaterialTemplate::Iron:			return LeMaterial::Iron;		break;
		case LeMaterialTemplate::Titanium:		return LeMaterial::Titanium;	break;
		case LeMaterialTemplate::Copper:		return LeMaterial::Copper;		break;
		case LeMaterialTemplate::Gold:			return LeMaterial::Gold;		break;
		case LeMaterialTemplate::Brass:			return LeMaterial::Brass;		break;

		case LeMaterialTemplate::Coal:			return LeMaterial::Coal;		break;
		case LeMaterialTemplate::Rubber:		return LeMaterial::Rubber;		break;
		case LeMaterialTemplate::Mud:			return LeMaterial::Mud;			break;
		case LeMaterialTemplate::Wood:			return LeMaterial::Wood;		break;
		case LeMaterialTemplate::Vegetation:	return LeMaterial::Vegetation;	break;
		case LeMaterialTemplate::Brick:			return LeMaterial::Brick;		break;
		case LeMaterialTemplate::Sand:			return LeMaterial::Sand;		break;
		case LeMaterialTemplate::Concrete:		return LeMaterial::Concrete;	break;
		default:								return LeMaterial();
	}
}