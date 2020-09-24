#pragma once

#include <assimp\Importer.hpp>
#include <assimp\postprocess.h>
#include <assimp/scene.h>
#include "Mesh.h"
#include <iostream>
#include "MeshBuffer.h"
#include <regex>

const std::vector<Vertex> cubeVertex =
{
	{{-0.5f, -0.5f, 0.5f},	 {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{0.5f, -0.5f, 0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}},
	{{0.5f, 0.5f, 0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f, 0.5f, 0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f},  {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},
	{{-0.5f, 0.5f, -0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	{{0.5f, 0.5f, -0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}},
	{{0.5f, -0.5f, -0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}},

	{{-0.5f, -0.5f, -0.5f},	 {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}},
	{{0.5f, -0.5f, 0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},
	{{-0.5f, -0.5f, 0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}},

	{{0.5f, 0.5f, -0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f, -0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}},
	{ {-0.5f, 0.5f, 0.5f},	 {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},
	{{0.5f, 0.5f, 0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}},

	{ {0.5f, -0.5f, 0.5f},	 {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, -0.5f, -0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
	{ {0.5f, 0.5f, -0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f, 0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}},

	{ {-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
	{{-0.5f, -0.5f, 0.5f},	 {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}},
	{ {-0.5f, 0.5f, 0.5f},	 {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}},
	{{-0.5f, 0.5f, -0.5f},	 {1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}}
};

const std::vector<uint16_t> cubeIndices =
{
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4,
	8, 9, 10, 10, 11, 8,
	12, 13, 14, 14, 15, 12,
	16, 17, 18, 18, 19, 16,
	20, 21, 22, 22, 23, 20
};

class MeshLoader
{
public:
	MeshLoader() = delete;
	~MeshLoader() = delete;

	static std::vector<std::string> split(const std::string& input, const std::string& regex) 
	{
		// passing -1 as the submatch index parameter performs splitting
		std::regex re(regex);
		std::sregex_token_iterator first{ input.begin(), input.end(), re, -1 };
		std::sregex_token_iterator last;
		return { first, last };
	}

	static Mesh* LoadMesh(std::string filename)
	{
		Assimp::Importer importer;
		const aiScene* assimpScene = importer.ReadFile(filename.c_str(), aiProcess_Triangulate | aiProcess_GenSmoothNormals /*| aiProcess_FlipUVs*/ | aiProcess_PreTransformVertices);

		if (!assimpScene)
			return nullptr;

		size_t pos = filename.find_last_of("/\\");
		std::string folderPath = "";
		if (pos != std::string::npos)
			folderPath = filename.substr(0, pos);

		if (folderPath.size() > 0)
		{
			char lastChar = folderPath[folderPath.size() - 1];
			if (lastChar != '/' && lastChar != '\\')
				folderPath.push_back('/');
		}

		Mesh* mesh = new Mesh();
		mesh->CreateMaterial();
		std::vector<std::string> splitFileName = split(filename, "/");
		mesh->name = splitFileName[splitFileName.size()-1];

		for (unsigned int i = 0; i < assimpScene->mNumMeshes; ++i)
		{
			aiMesh* assimpMesh = assimpScene->mMeshes[i];

			MeshBuffer* buffer = mesh->AddMeshBuffer();
			buffer->vertices.resize(assimpMesh->mNumVertices);
			for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
			{
				buffer->vertices[i].pos = ToVec3GLM(assimpMesh->mVertices[i]);
				buffer->vertices[i].color = glm::vec3(1.0f, 1.0f, 1.0f);
				buffer->vertices[i].normal = ToVec3GLM(assimpMesh->mNormals[i]);

				if (assimpMesh->GetNumUVChannels() > 0)
				{
					const aiVector3D uv = assimpMesh->mTextureCoords[0][i];
					buffer->vertices[i].uv = ToVec2GLM(uv);
				}
				else
				{
					buffer->vertices[i].uv = glm::vec2(0.f, 0.f);
				}
			}

			buffer->indices.resize(assimpMesh->mNumFaces * 3);
			for (unsigned int i = 0; i < assimpMesh->mNumFaces; i++)
			{
				const aiFace face = assimpMesh->mFaces[i];

				if (face.mNumIndices != 3)
				{
					std::cout << "Error : Face != 3 indices !" << std::endl;
					delete mesh;
					return nullptr;
				}

				buffer->indices[i * 3 + 0] = face.mIndices[0];
				buffer->indices[i * 3 + 1] = face.mIndices[1];
				buffer->indices[i * 3 + 2] = face.mIndices[2];
			}

			mesh->GetMaterial()->texture = new Texture();
			mesh->GetMaterial()->normalMap = new Texture();
			mesh->GetMaterial()->specularMap = new Texture();
			mesh->GetMaterial()->metallicMap = new Texture();
			mesh->GetMaterial()->roughnessMap = new Texture();

			aiMaterial* mat = assimpScene->mMaterials[assimpMesh->mMaterialIndex];
			if (mat->GetTextureCount(aiTextureType_DIFFUSE) > 0)
			{
				aiString path;
				mat->GetTexture(aiTextureType_DIFFUSE, 0, &path);
							   
				std::cout << "Texture = " << folderPath + path.C_Str() << std::endl;
				mesh->GetMaterial()->texture->LoadFile(folderPath + path.C_Str(), true);// tryToLoadTexture(buffer, folderPath + path.C_Str());
			}

			// normal map is of type height in ironman
			if (mat->GetTextureCount(aiTextureType_HEIGHT) > 0)
			{
				aiString path;
				mat->GetTexture(aiTextureType_HEIGHT, 0, &path);

				std::cout << "Normal map = " << folderPath + path.C_Str() << std::endl;
				mesh->GetMaterial()->normalMap->LoadFile(folderPath + path.C_Str());// tryToLoadTexture(buffer, folderPath + path.C_Str());
			}

			if (mat->GetTextureCount(aiTextureType_SPECULAR) > 0)
			{
				aiString path;
				mat->GetTexture(aiTextureType_SPECULAR, 0, &path);

				std::cout << "Specular map = " << folderPath + path.C_Str() << std::endl;
				mesh->GetMaterial()->specularMap->LoadFile(folderPath + path.C_Str());// tryToLoadTexture(buffer, folderPath + path.C_Str());
			}
		}

		return mesh;
	}

	static Mesh* LoadDefaultCube()
	{
		Mesh* mesh = new Mesh();
		mesh->name = "DefaultCube";
		mesh->CreateMaterial();

		MeshBuffer* buffer = mesh->AddMeshBuffer();

		buffer->vertices = cubeVertex;
		buffer->indices = cubeIndices;

		mesh->GetMaterial()->texture = new Texture();
		mesh->GetMaterial()->normalMap = new Texture();
		mesh->GetMaterial()->specularMap = new Texture();
		mesh->GetMaterial()->metallicMap = new Texture();
		mesh->GetMaterial()->roughnessMap = new Texture();

		return mesh;
	}

	static Mesh* LoadDefaultQuad()
	{
		Mesh* mesh = new Mesh();
		mesh->name = "DefaultCube";
		mesh->CreateMaterial();

		MeshBuffer* buffer = mesh->AddMeshBuffer();

		std::vector<Vertex> vertexBuffer =
		{
			{ { 1.0f, 1.0f, 0.0f }, { 1.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
			{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
			{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }},
			{ { 1.0f, 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 1.0f }}
		};

		std::vector<uint16_t> indexBuffer = { 0,1,2, 2,3,0 };

		buffer->vertices = vertexBuffer;
		buffer->indices = indexBuffer;

		mesh->GetMaterial()->texture = new Texture();
		mesh->GetMaterial()->normalMap = new Texture();
		mesh->GetMaterial()->specularMap = new Texture();
		mesh->GetMaterial()->metallicMap = new Texture();
		mesh->GetMaterial()->roughnessMap = new Texture();

		return mesh;
	}

	static glm::vec2 ToVec2GLM(aiVector3D vector)
	{
		return glm::vec2(vector.x, vector.y);
	}

	static glm::vec3 ToVec3GLM(aiVector3D vector)
	{
		return glm::vec3(vector.x, vector.y, vector.z);
	}
};

