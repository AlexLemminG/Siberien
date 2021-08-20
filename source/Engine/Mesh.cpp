#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "Mesh.h"
#include "Resources.h"
#include "Serialization.h"
#include "System.h"
#include "MeshRenderer.h" //TODO just animator



MeshVertexLayout& MeshVertexLayout::Begin() {
	attributes.clear();
	bgfxLayout = bgfx::VertexLayout();
	bgfxLayout.begin();
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddPosition() {
	bgfxLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
	attributes.push_back(bgfx::Attrib::Position);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddNormal() {
	bgfxLayout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
	attributes.push_back(bgfx::Attrib::Normal);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddTexCoord() {
	bgfxLayout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
	attributes.push_back(bgfx::Attrib::TexCoord0);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddTangent() {
	bgfxLayout.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float);
	attributes.push_back(bgfx::Attrib::Tangent);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddIndices() {
	bgfxLayout.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8);
	attributes.push_back(bgfx::Attrib::Indices);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::AddWeights() {
	bgfxLayout.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
	attributes.push_back(bgfx::Attrib::Weight);
	return *this;
}

MeshVertexLayout& MeshVertexLayout::End() {
	bgfxLayout.end();
	return *this;
}

std::vector<uint8_t> MeshVertexLayout::CreateBuffer(aiMesh* mesh) const {
	int numVerts = mesh->mNumVertices;
	int stride = bgfxLayout.m_stride;
	std::vector<uint8_t> buffer;
	buffer.resize(bgfxLayout.getSize(numVerts));

	std::vector<float> weights;
	weights.resize(numVerts * Mesh::bonesPerVertex, 0.f);

	std::vector<uint8_t> indices;
	indices.resize(numVerts * Mesh::bonesPerVertex, 0);

	for (int iBone = 0; iBone < mesh->mNumBones; iBone++) {
		for (int j = 0; j < mesh->mBones[iBone]->mNumWeights; j++) {
			int vertexID = mesh->mBones[iBone]->mWeights[j].mVertexId;
			float weight = mesh->mBones[iBone]->mWeights[j].mWeight;

			for (int i = 0; i < Mesh::bonesPerVertex; i++) {
				if (weights[vertexID * Mesh::bonesPerVertex + i] == 0.f) {
					weights[vertexID * Mesh::bonesPerVertex + i] = weight;
					indices[vertexID * Mesh::bonesPerVertex + i] = iBone;
					break;
				}
			}
		}
	}

	for (int i = 0; i < numVerts; i++) {
		float w = 0.f;
		for (int j = 0; j < Mesh::bonesPerVertex; j++) {
			w += weights[i * Mesh::bonesPerVertex + j];
		}
		weights[i * Mesh::bonesPerVertex] += 1.f - w;
	}

	for (int i = 0; i < attributes.size(); i++) {
		auto attribute = attributes[i];
		int offset = bgfxLayout.getOffset(attribute);
		if (attribute == bgfx::Attrib::Position) {
			aiVector3D* srcBuffer = mesh->mVertices;
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += 1;
				dstBuffer += stride;
			}
		}
		else if (attribute == bgfx::Attrib::Normal) {
			aiVector3D* srcBuffer = mesh->mNormals;
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += 1;
				dstBuffer += stride;
			}
		}
		else if (attribute == bgfx::Attrib::TexCoord0) {
			aiVector3D* srcBuffer = mesh->mTextureCoords[0];
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 2);
				srcBuffer += 1;
				dstBuffer += stride;
			}
		}
		else if (attribute == bgfx::Attrib::Tangent) {
			aiVector3D* srcBuffer = mesh->mTangents;
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += 1;
				dstBuffer += stride;
			}
		}
		else if (attribute == bgfx::Attrib::Indices) {
			uint8_t* srcBuffer = &indices[0];
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(uint8_t) * Mesh::bonesPerVertex);
				srcBuffer += Mesh::bonesPerVertex;
				dstBuffer += stride;
			}
		}
		else if (attribute == bgfx::Attrib::Weight) {
			float* srcBuffer = &weights[0];
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * Mesh::bonesPerVertex);
				srcBuffer += Mesh::bonesPerVertex;
				dstBuffer += stride;
			}
		}
	}
	return std::move(buffer);
}

std::vector<uint16_t> MeshVertexLayout::CreateIndices(aiMesh* mesh) {
	std::vector<uint16_t> indices;
	for (int i = 0; i < mesh->mNumFaces; i++) {
		ASSERT(mesh->mFaces[i].mNumIndices == 3);
		indices.push_back(mesh->mFaces[i].mIndices[0]);
		indices.push_back(mesh->mFaces[i].mIndices[1]);
		indices.push_back(mesh->mFaces[i].mIndices[2]);
	}
	return std::move(indices);
}


//TODO optimize systemManager to exclude empty Update functions
class VertexLayoutSystem : public System<VertexLayoutSystem> {
public:
	bool Init() override {
		layout_pos.Begin().AddPosition().AddNormal().AddTangent().AddTexCoord().AddIndices().AddWeights().End();
		return true;
	}
	PriorityInfo GetPriorityInfo() const override { return PriorityInfo{ -1 }; }
	MeshVertexLayout layout_pos;
};

REGISTER_SYSTEM(VertexLayoutSystem);

class MeshAssetImporter : public AssetImporter2 {
public:
	virtual bool ImportAll(AssetDatabase2::BinaryImporterHandle& databaseHandle) override {
		//TODO importer could be heavy
		Assimp::Importer importer{};
		auto importFlags =
			aiProcess_ValidateDataStructure |
			aiProcess_CalcTangentSpace |
			aiProcess_PopulateArmatureData |
			aiProcess_Triangulate |
			aiProcess_ConvertToLeftHanded;

		auto* scene = importer.ReadFile(databaseHandle.GetAssetPath().c_str(), importFlags);
		//load from memory does not work with glb + blender for some reason

		//std::vector<uint8_t> buffer;
		//if (!databaseHandle.ReadAssetAsBinary(buffer)) {
		//	return false;
		//}
		//auto* scene = importer.ReadFileFromMemory(&buffer[0], buffer.size(), importFlags);

		if (!scene) {
			//TODO error
			return false;
		}
		scene = importer.GetOrphanedScene();
		auto fullAsset = std::make_shared<FullMeshAsset>();
		fullAsset->scene.reset(scene);
		//TODO pass separate database handle
		databaseHandle.AddAssetToLoaded("FullMeshAsset", fullAsset);

		std::unordered_map < std::string, std::shared_ptr<MeshAnimation>> animations;
		const std::string armaturePrefix = "Armature|";
		for (int iAnim = 0; iAnim < scene->mNumAnimations; iAnim++) {
			auto anim = scene->mAnimations[iAnim];
			auto animation = std::make_shared<MeshAnimation>();
			animation->assimAnimation = anim;
			std::string animName = anim->mName.C_Str();
			if (animName.find(armaturePrefix.c_str()) == 0) {
				animName = animName.substr(armaturePrefix.size(), animName.size() - armaturePrefix.size());
			}
			databaseHandle.AddAssetToLoaded(animName, animation);
			animations[animName] = animation;
		}

		int importedMeshesCount = 0;
		for (int iMesh = 0; iMesh < scene->mNumMeshes; iMesh++) {
			auto* aiMesh = scene->mMeshes[iMesh];
			auto mesh = std::make_shared<Mesh>();
			mesh->name = aiMesh->mName.C_Str();
			mesh->originalMeshPtr = aiMesh;
			databaseHandle.AddAssetToLoaded(mesh->name.c_str(), mesh);

			auto vertices = VertexLayoutSystem::Get()->layout_pos.CreateBuffer(aiMesh);
			mesh->vertexBuffer = bgfx::createVertexBuffer(bgfx::copy(&vertices[0], vertices.size()), VertexLayoutSystem::Get()->layout_pos.bgfxLayout);
			bgfx::setName(mesh->vertexBuffer, mesh->name.c_str());

			auto indices = VertexLayoutSystem::Get()->layout_pos.CreateIndices(aiMesh);
			mesh->indexBuffer = bgfx::createIndexBuffer(bgfx::copy(&indices[0], indices.size() * sizeof(uint16_t)));
			bgfx::setName(mesh->indexBuffer, mesh->name.c_str());
			mesh->tPoseAnim = animations["TPose"];
			mesh->Init();

			importedMeshesCount++;
			//bgfx::createVertexBuffer(bgfx::makeRef(aiMesh->mVertices, aiMesh->mNumVertices), VertexLayouts::);
		}

		if (importedMeshesCount == 0) {
			//TODO send error to handler
			LogError("failed to import: no meshes");
		}

		return true;
	}
};



void Mesh::Init() {
	if (!originalMeshPtr) {
		return;
	}
	bones.resize(originalMeshPtr->mNumBones);
	std::unordered_map<std::string, int> boneMapping;
	for (int iBone = 0; iBone < originalMeshPtr->mNumBones; iBone++) {
		bones[iBone].idx = iBone;
		bones[iBone].name = originalMeshPtr->mBones[iBone]->mName.C_Str();

		bones[iBone].offset = *(Matrix4*)(void*)(&(originalMeshPtr->mBones[iBone]->mOffsetMatrix.a1));
		bones[iBone].offset = bones[iBone].offset.Transpose();
		//bones[iBone].offset = bones[iBone].offset.Inverse();
		boneMapping[bones[iBone].name] = iBone;
	}

	for (int iBone = 0; iBone < originalMeshPtr->mNumBones; iBone++) {
		auto parentNode = originalMeshPtr->mBones[iBone]->mNode->mParent;
		auto it = boneMapping.find(parentNode->mName.C_Str());
		if (it != boneMapping.end()) {
			bones[iBone].parentBoneIdx = it->second;
		}
		else {
			bones[iBone].parentBoneIdx = -1;
		}
		bones[iBone].initialLocal = *(Matrix4*)(void*)(&(originalMeshPtr->mBones[iBone]->mNode->mTransformation.a1));
		bones[iBone].initialLocal = bones[iBone].initialLocal.Transpose();
	}

	for (int iBone = 0; iBone < originalMeshPtr->mNumBones; iBone++) {
		if (tPoseAnim) {
			auto transform = tPoseAnim->GetTransform(bones[iBone].name, 0.f);
			bones[iBone].inverseTPoseRotation = GetRot(bones[iBone].initialLocal) * transform.rotation.Inverse();
		}
	}
}


Mesh::~Mesh() {
	if (bgfx::isValid(vertexBuffer)) {
		bgfx::destroy(vertexBuffer);
	}
	if (bgfx::isValid(indexBuffer)) {
		bgfx::destroy(indexBuffer);
	}
}
DECLARE_BINARY_ASSET2(Mesh, MeshAssetImporter);
