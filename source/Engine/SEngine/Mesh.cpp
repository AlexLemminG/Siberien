

#include "assimp/scene.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "Mesh.h"
#include "Resources.h"
#include "Serialization.h"
#include "System.h"
#include "Animation.h"
#include "PhysicsSystem.h"//TODO looks weird
#include "ryml.hpp"
#include "Compression.h"
#include "shlwapi.h"
#include <filesystem>


class ModelAnimationsMeta {
public:
	bool deformBonesOnly = true;

	REFLECT_BEGIN(ModelAnimationsMeta);
	REFLECT_VAR(deformBonesOnly);
	REFLECT_END()
};
class ModelMeta {
public:
	ModelAnimationsMeta animations{};

	REFLECT_BEGIN(ModelMeta);
	REFLECT_VAR(animations);
	REFLECT_END()
};

class LibraryMeshMeta :public Object {
public:
	uint64_t lastAssetChangeTime;
	uint64_t lastMetaChangeTime;
	int importerVersion;
	REFLECT_BEGIN(LibraryMeshMeta);
	REFLECT_VAR(lastAssetChangeTime);
	REFLECT_VAR(lastMetaChangeTime);
	REFLECT_VAR(importerVersion);
	REFLECT_END();
};

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


static std::vector<uint16_t> CalcIndicesFromAiMesh(aiMesh* mesh) {
	std::vector<uint16_t> indices;
	for (int i = 0; i < mesh->mNumFaces; i++) {
		if (mesh->mFaces[i].mNumIndices == 3) {
			indices.push_back(mesh->mFaces[i].mIndices[0]);
			indices.push_back(mesh->mFaces[i].mIndices[1]);
			indices.push_back(mesh->mFaces[i].mIndices[2]);
		}
	}
	return std::move(indices);
}

static std::vector<RawVertexData> CalcVerticesFromAiMesh(aiMesh* mesh) {
	std::vector<RawVertexData> result;
	if (!mesh) {
		return result;
	}
	int numVerts = mesh->mNumVertices;
	ResizeVectorNoInit(result, numVerts);

	memset(result.data(), 0, result.size() * sizeof(RawVertexData));
	ASSERT(mesh->mNumBones < 256);
	for (int iBone = 0; iBone < mesh->mNumBones; iBone++) {
		for (int j = 0; j < mesh->mBones[iBone]->mNumWeights; j++) {
			int vertexID = mesh->mBones[iBone]->mWeights[j].mVertexId;
			float weight = mesh->mBones[iBone]->mWeights[j].mWeight;
			auto& vertex = result[vertexID];

			bool found = false;
			for (int i = 0; i < RawVertexData::bonesPerVertex; i++) {
				if (vertex.boneWeights[i] == 0.f) {
					vertex.boneWeights[i] = weight;
					vertex.boneIndices[i] = iBone;
					found = true;
					break;
				}
			}
			if (!found) {
				ASSERT(false);
			}
		}
	}

	for (int i = 0; i < numVerts; i++) {
		float w = 0.f;
		auto& vertex = result[i];
		for (int j = 0; j < RawVertexData::bonesPerVertex; j++) {
			w += vertex.boneWeights[j];
		}
		vertex.boneWeights[0] += 1.f - w;
	}

	for (int i = 0; i < numVerts; i++) {
		auto& vertex = result[i];
		{
			const auto& aiPos = mesh->mVertices[i];
			vertex.pos.x = aiPos.x;
			vertex.pos.y = aiPos.y;
			vertex.pos.z = aiPos.z;
		}
		{
			const auto& aiNormal = mesh->mNormals[i];
			vertex.normal.x = aiNormal.x;
			vertex.normal.y = aiNormal.y;
			vertex.normal.z = aiNormal.z;
		}
		if (mesh->mTextureCoords[0]) {
			const auto& aiTexCoord = mesh->mTextureCoords[0][i];
			vertex.uv.x = aiTexCoord.x;
			vertex.uv.y = aiTexCoord.y;
		}
		else {
			vertex.uv.x = 0.f;
			vertex.uv.y = 0.f;
		}
		if (mesh->mTangents) {
			const auto& aiTangent = mesh->mTangents[i];
			vertex.tangent.x = aiTangent.x;
			vertex.tangent.y = aiTangent.y;
			vertex.tangent.z = aiTangent.z;
		}
		else {
			vertex.tangent.x = 0.f;
			vertex.tangent.y = 0.f;
			vertex.tangent.z = 1.f;
		}
	}
	return std::move(result);
}

std::vector<uint8_t> MeshVertexLayout::CreateBuffer(const std::vector<RawVertexData>& rawVertices) const {
	int numVerts = rawVertices.size();
	int strideSrc = sizeof(RawVertexData);
	int strideDst = bgfxLayout.m_stride;
	std::vector<uint8_t> buffer;
	ResizeVectorNoInit(buffer, bgfxLayout.getSize(numVerts));

	for (int i = 0; i < attributes.size(); i++) {
		auto attribute = attributes[i];
		int offset = bgfxLayout.getOffset(attribute);
		if (attribute == bgfx::Attrib::Position) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].pos);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
		else if (attribute == bgfx::Attrib::Normal) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].normal);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
		else if (attribute == bgfx::Attrib::TexCoord0) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].uv);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 2);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
		else if (attribute == bgfx::Attrib::Tangent) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].tangent);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * 3);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
		else if (attribute == bgfx::Attrib::Indices) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].boneIndices[0]);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(uint8_t) * RawVertexData::bonesPerVertex);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
		else if (attribute == bgfx::Attrib::Weight) {
			const uint8_t* srcBuffer = (uint8_t*)&(rawVertices[0].boneWeights[0]);
			uint8_t* dstBuffer = &buffer[offset];
			for (int iVertex = 0; iVertex < numVerts; iVertex++) {
				memcpy(dstBuffer, srcBuffer, sizeof(float) * RawVertexData::bonesPerVertex);
				srcBuffer += strideSrc;
				dstBuffer += strideDst;
			}
		}
	}
	return std::move(buffer);
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

class MeshAssetImporter : public AssetImporter {
public:
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override {
		auto meshAsset = Import(databaseHandle);

		if (!meshAsset) {
			return false;
		}
		databaseHandle.AddAssetToLoaded("FullMeshAsset", meshAsset);
		for (auto& mesh : meshAsset->meshes) {
			auto vertices = VertexLayoutSystem::Get()->layout_pos.CreateBuffer(mesh->rawVertices);
			mesh->vertexBuffer = bgfx::createVertexBuffer(bgfx::copy(&vertices[0], vertices.size()), VertexLayoutSystem::Get()->layout_pos.bgfxLayout);
			ASSERT(bgfx::isValid(mesh->vertexBuffer));
			bgfx::setName(mesh->vertexBuffer, mesh->name.c_str());
			auto& indices = mesh->rawIndices;
			//TODO makeRef + release func instead of copy 
			mesh->indexBuffer = bgfx::createIndexBuffer(bgfx::copy(&indices[0], indices.size() * sizeof(uint16_t)));
			ASSERT(bgfx::isValid(mesh->indexBuffer));
			bgfx::setName(mesh->indexBuffer, mesh->name.c_str());

			databaseHandle.AddAssetToLoaded(mesh->name.c_str(), mesh);
		}
		for (auto& animation : meshAsset->animations) {
			databaseHandle.AddAssetToLoaded(animation->name.c_str(), animation);
		}

		return true;
	}

	bool ConvertBlendToGlb(AssetDatabase_BinaryImporterHandle& databaseHandle, const std::string& inFile, const std::string& outFile, const ModelMeta& meta) {
		DWORD blenderLocSize = 255;
		char blenderLoc[255];
		memset(blenderLoc, 0, 255);
		auto findBlendResult = AssocQueryStringA(ASSOCF_VERIFY, ASSOCSTR_EXECUTABLE, ".blend", nullptr, blenderLoc, &blenderLocSize);
		if (findBlendResult != 0) {
			//TODO log error
			return false;
		}
		std::string blenderApp = blenderLoc;
		int launcherStrIdx = blenderApp.find("blender-launcher.exe");
		if (launcherStrIdx != -1) {
			blenderApp.replace(launcherStrIdx, strlen("blender-launcher.exe"), "blender.exe"); // *-launcher.exe does not wait
		}

		//TODO less hardcode
		auto pythonConverterScript = databaseHandle.GetToolPath("BlenderToGlb.py");//TODO make sure file exist
		std::string params = " ";
		params += inFile;
		params += " -b";
		params += " --python \"" + pythonConverterScript + "\"";

		//TODO move to platform independent stuff
		STARTUPINFOA si;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		std::vector<char> paramsBuffer(params.begin(), params.end());
		paramsBuffer.push_back(0);
		LPSTR ccc = &paramsBuffer[0];

		SetEnvironmentVariableA("SENGINE_BLENDER_EXPORTER_ANIMATIONS_DEFORM_BONES_ONLY", meta.animations.deformBonesOnly ? "True" : "False");
		SetEnvironmentVariableA("SENGINE_BLENDER_EXPORTER_OUTPUT_FILE", outFile.c_str());

		//TODO delete outFile before converting

		auto result = CreateProcessA(
			blenderApp.c_str()
			, &paramsBuffer[0]
			, NULL
			, NULL
			, false
			, 0
			, NULL
			, NULL
			, &si
			, &pi);

		if (!result)
		{
			LogError("Failed to open blender for exporting '%s'", databaseHandle.GetAssetPath().c_str());
			return false;
		}
		else
		{
			// Successfully created the process.  Wait for it to finish.
			WaitForSingleObject(pi.hProcess, INFINITE);

			// Close the handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		return true;
	}

	std::shared_ptr<FullMeshAsset> Import(AssetDatabase_BinaryImporterHandle& databaseHandle) {
		int importerVersion = 1;//TODO move somewhere
		std::string convertedAssetPath = databaseHandle.GetLibraryPathFromId("MeshAsset");
		std::string metaPath = databaseHandle.GetLibraryPathFromId("meta");


		bool needRebuild = true;
		LibraryMeshMeta expectedMeta;
		expectedMeta.importerVersion = importerVersion;
		databaseHandle.GetLastModificationTime(expectedMeta.lastAssetChangeTime, expectedMeta.lastMetaChangeTime);

		std::unique_ptr<ryml::Tree> libraryMetaYaml;
		databaseHandle.ReadFromLibraryFile("meta", libraryMetaYaml);
		if (libraryMetaYaml) {
			LibraryMeshMeta meta;
			SerializationContext c{ libraryMetaYaml->rootref() };
			::Deserialize(c, meta);
			if (meta.lastAssetChangeTime == expectedMeta.lastAssetChangeTime &&
				meta.lastMetaChangeTime == expectedMeta.lastMetaChangeTime &&
				meta.importerVersion == expectedMeta.importerVersion) {
				needRebuild = false;
			}
		}

		std::vector<uint8_t> buffer;
		bool bufferLoaded = false;
		if (!needRebuild) {
			bufferLoaded = databaseHandle.ReadFromLibraryFile("MeshAsset", buffer);
			if (bufferLoaded) {
				BinaryBuffer from = BinaryBuffer(std::move(buffer));
				BinaryBuffer to;
				bufferLoaded = Compression::Decompress(from, to);
				if (bufferLoaded) {
					buffer = to.ReleaseData();
				}
			}
		}
		if (bufferLoaded) {
			auto meshAsset = DeserializeFromBuffer(buffer);
			return meshAsset;
		}


		std::unique_ptr<ryml::Tree> metaYamlTree;
		databaseHandle.ReadMeta(metaYamlTree);
		ModelMeta meta{};
		if (metaYamlTree) {
			SerializationContext c = SerializationContext(metaYamlTree->rootref());
			::Deserialize(c, meta);
		}

		//TODO some pipelining for blender
		auto originalAssetPathReal = databaseHandle.GetAssetPathReal();
		bool isBlendFile = databaseHandle.GetFileExtension() == "blend";

		if (isBlendFile) {
			//TODO less hardcode
			databaseHandle.EnsureForderForTempFileExists("blenderToGlb.glb");
			std::string tempFile = databaseHandle.GetTempPathFromFileName("blenderToGlb.glb");
			if (!ConvertBlendToGlb(databaseHandle, originalAssetPathReal, tempFile, meta)) {
				return false;
			}
			originalAssetPathReal = tempFile;
		}

		auto meshAsset = ImportUsingAssimp(originalAssetPathReal);
		if (!meshAsset) {
			return false;
		}

		buffer = SerializeToBuffer(meshAsset);
		SerializationContext c{};
		::Serialize(c, expectedMeta);

		databaseHandle.EnsureForderForLibraryFileExists("meta");
		databaseHandle.EnsureForderForLibraryFileExists("MeshAsset");

		databaseHandle.WriteToLibraryFile("meta", c.GetYamlNode());

		{
			BinaryBuffer from = BinaryBuffer(std::move(buffer));
			BinaryBuffer to;
			bool compressed = Compression::Compress(from, to);
			ASSERT(compressed);
			databaseHandle.WriteToLibraryFile("MeshAsset", to.GetData());
		}
		Log("Converted mesh '%s'", databaseHandle.GetAssetPath().c_str());
		return meshAsset;
	}


	void DeserializeFromBuffer(BinaryBuffer& buffer, const std::shared_ptr<FullMeshAsset>& meshAsset, FullMeshAsset_Node& node) {
		int meshIdx = -1;
		buffer.Read(meshIdx);
		if (meshIdx != -1) {
			if (meshIdx < meshAsset->meshes.size()) {
				node.mesh = meshAsset->meshes[meshIdx];
			}
			else {
				ASSERT(false);
			}
		}

		buffer.Read((uint8_t*)&node.localTransformMatrix, sizeof(Matrix4));

		int numChildren;
		buffer.Read(numChildren);
		ResizeVectorNoInit(node.childNodes, numChildren);
		for (int i = 0; i < numChildren; i++) {
			DeserializeFromBuffer(buffer, meshAsset, node.childNodes[i]);
		}
	}

	//TODO typedef buffer
	std::shared_ptr<FullMeshAsset> DeserializeFromBuffer(std::vector<uint8_t>& rawBuffer) {
		BinaryBuffer buffer{ std::move(rawBuffer) };//WARN dangaros
		auto fullMeshAsset = std::make_shared<FullMeshAsset>();

		int numMeshes;
		buffer.Read(numMeshes);
		for (int i = 0; i < numMeshes; i++) {
			std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>();
			fullMeshAsset->meshes.push_back(mesh);
			buffer.Read(mesh->name);

			buffer.Read(mesh->assetMaterialName);

			int numIndices;
			buffer.Read(numIndices);
			ResizeVectorNoInit(mesh->rawIndices, numIndices);
			if (numIndices) {
				buffer.Read((uint8_t*)&mesh->rawIndices[0], numIndices * sizeof(decltype(mesh->rawIndices[0])));
			}

			int numVertices;
			buffer.Read(numVertices);
			ResizeVectorNoInit(mesh->rawVertices, numVertices);
			if (numVertices) {
				buffer.Read((uint8_t*)&mesh->rawVertices[0], numVertices * sizeof(decltype(mesh->rawVertices[0])));
			}

			int numBones;
			buffer.Read(numBones);
			ResizeVectorNoInit(mesh->bones, numBones);
			for (int i = 0; i < numBones; i++) {
				buffer.Read(mesh->bones[i].name);
				buffer.Read(mesh->bones[i].idx);
				buffer.Read(mesh->bones[i].parentBoneIdx);
				buffer.Read(mesh->bones[i].offset);
				buffer.Read(mesh->bones[i].initialLocal);
			}

			buffer.Read(mesh->aabb);
			buffer.Read(mesh->boundingSphere);
		}
		PhysicsSystem::Get()->DeserializeMeshPhysicsDataFromBuffer(fullMeshAsset->meshes, buffer);

		int numAnimations;
		buffer.Read(numAnimations);
		for (int i = 0; i < numAnimations; i++) {
			auto animation = std::make_shared<MeshAnimation>();
			fullMeshAsset->animations.push_back(animation);
			buffer.Read(animation->name);

			int numBones;
			buffer.Read(numBones);
			for (int i = 0; i < numBones; i++) {
				std::string boneName;
				buffer.Read(boneName);
				animation->channelNames.push_back(boneName);
				int numKeyframes;
				buffer.Read(numKeyframes);
				auto& keyframes = animation->channelKeyframes.emplace_back();
				if (numKeyframes) {
					ResizeVectorNoInit(keyframes, numKeyframes);
					buffer.Read((uint8_t*)&keyframes[0], sizeof(decltype(keyframes[0])) * numKeyframes);
				}
			}
		}

		DeserializeFromBuffer(buffer, fullMeshAsset, fullMeshAsset->rootNode);

		return fullMeshAsset;
	}

	void SerializeToBuffer(BinaryBuffer& buffer, const std::shared_ptr<FullMeshAsset>& meshAsset, const FullMeshAsset_Node& node) {
		int meshIdx = -1;
		if (node.mesh) {
			auto it = std::find(meshAsset->meshes.begin(), meshAsset->meshes.end(), node.mesh);
			if (it != meshAsset->meshes.end()) {
				meshIdx = it - meshAsset->meshes.begin();
			}
			else {
				ASSERT(false);
			}
		}
		buffer.Write(meshIdx);
		buffer.Write((uint8_t*)&node.localTransformMatrix, sizeof(Matrix4));
		int numChildren = node.childNodes.size();
		buffer.Write(numChildren);
		for (int i = 0; i < numChildren; i++) {
			SerializeToBuffer(buffer, meshAsset, node.childNodes[i]);
		}
	}

	std::vector<uint8_t> SerializeToBuffer(const std::shared_ptr<FullMeshAsset>& meshAsset) {
		BinaryBuffer buffer;
		int numMeshes = meshAsset->meshes.size();
		buffer.Write(numMeshes);

		for (int i = 0; i < numMeshes; i++) {
			auto mesh = meshAsset->meshes[i];
			buffer.Write(mesh->name);

			buffer.Write(mesh->assetMaterialName);

			int numIndices = mesh->rawIndices.size();
			buffer.Write(numIndices);
			if (numIndices) {
				buffer.Write((uint8_t*)&mesh->rawIndices[0], numIndices * sizeof(decltype(mesh->rawIndices[0])));
			}

			int numVertices = mesh->rawVertices.size();
			buffer.Write(numVertices);
			if (numVertices) {
				buffer.Write((uint8_t*)&mesh->rawVertices[0], numVertices * sizeof(decltype(mesh->rawVertices[0])));
			}

			int numBones = mesh->bones.size();
			buffer.Write(numBones);
			for (int i = 0; i < numBones; i++) {
				buffer.Write(mesh->bones[i].name);
				buffer.Write(mesh->bones[i].idx);
				buffer.Write(mesh->bones[i].parentBoneIdx);
				buffer.Write(mesh->bones[i].offset);
				buffer.Write(mesh->bones[i].initialLocal);
			}

			buffer.Write(mesh->aabb);
			buffer.Write(mesh->boundingSphere);
		}
		PhysicsSystem::Get()->SerializeMeshPhysicsDataToBuffer(meshAsset->meshes, buffer);

		int numAnimations = meshAsset->animations.size();
		buffer.Write(numAnimations);
		for (const auto& animation : meshAsset->animations) {
			std::string name = animation->name;
			buffer.Write(name);

			int numBones = animation->channelNames.size();
			buffer.Write(numBones);
			for (int iBone = 0; iBone < numBones; iBone++) {
				const std::string& boneName = animation->channelNames[iBone];
				buffer.Write(boneName);
				const auto& keyframes = animation->channelKeyframes[iBone];
				int numKeyframes = keyframes.size();
				buffer.Write(numKeyframes);
				if (numKeyframes) {
					buffer.Write((uint8_t*)&keyframes[0], sizeof(decltype(keyframes[0])) * numKeyframes);
				}
			}
		}

		SerializeToBuffer(buffer, meshAsset, meshAsset->rootNode);

		return std::move(buffer.ReleaseData());
	}
	AABB CalcAABB(std::vector<RawVertexData>& vertices) {
		if (vertices.size() == 0) {
			return AABB{ Vector3_zero, Vector3_zero };
		}
		AABB aabb;
		aabb.min = vertices[0].pos;
		aabb.max = vertices[0].pos;

		for (auto& v : vertices) {
			aabb.Expand(v.pos);
		}
		return aabb;
	}
	std::shared_ptr<FullMeshAsset> ImportUsingAssimp(std::string assetPath) {
		//TODO importer could be heavy
		Assimp::Importer importer{};
		auto importFlags =
			aiProcess_ValidateDataStructure |
			aiProcess_CalcTangentSpace |
			aiProcess_PopulateArmatureData |
			aiProcess_Triangulate |
			aiProcess_ImproveCacheLocality |
			aiProcess_ConvertToLeftHanded;

		auto* scene = importer.ReadFile(assetPath.c_str(), importFlags);
		//load from memory does not work with glb + blender for some reason

		//std::vector<uint8_t> buffer;
		//if (!databaseHandle.ReadAssetAsBinary(buffer)) {
		//	return false;
		//}
		//auto* scene = importer.ReadFileFromMemory(&buffer[0], buffer.size(), importFlags);

		if (!scene) {
			//TODO error
			return nullptr;
		}
		auto fullAsset = std::make_shared<FullMeshAsset>();
		//TODO animation and mesh names may not be unique

		std::unordered_map<std::string, std::shared_ptr<MeshAnimation>> animations;
		const std::string armaturePrefix = "Armature|";
		for (int iAnim = 0; iAnim < scene->mNumAnimations; iAnim++) {
			auto anim = scene->mAnimations[iAnim];
			auto animation = std::make_shared<MeshAnimation>();
			animation->DeserializeFromAssimp(anim);
			fullAsset->animations.push_back(animation);
			animations[animation->name] = animation;//TODO remove
		}

		int importedMeshesCount = 0;
		for (int iMesh = 0; iMesh < scene->mNumMeshes; iMesh++) {
			auto* aiMesh = scene->mMeshes[iMesh];
			auto mesh = std::make_shared<Mesh>();

			mesh->name = aiMesh->mName.C_Str();

			if (aiMesh->mMaterialIndex < scene->mNumMaterials) {
				mesh->assetMaterialName = scene->mMaterials[aiMesh->mMaterialIndex]->GetName().C_Str();
			}

			mesh->rawVertices = CalcVerticesFromAiMesh(aiMesh);
			mesh->rawIndices = CalcIndicesFromAiMesh(aiMesh);

			mesh->aabb = CalcAABB(mesh->rawVertices);
			mesh->boundingSphere.pos = mesh->aabb.GetCenter();
			mesh->boundingSphere.radius = mesh->aabb.GetSize().Length() / 2.f;

			ResizeVectorNoInit(mesh->bones, aiMesh->mNumBones);
			std::unordered_map<std::string, int> boneMapping;
			for (int iBone = 0; iBone < aiMesh->mNumBones; iBone++) {
				mesh->bones[iBone].idx = iBone;
				mesh->bones[iBone].name = aiMesh->mBones[iBone]->mName.C_Str();

				mesh->bones[iBone].offset = *(Matrix4*)(void*)(&(aiMesh->mBones[iBone]->mOffsetMatrix.a1));
				mesh->bones[iBone].offset = mesh->bones[iBone].offset.Transpose();
				boneMapping[mesh->bones[iBone].name] = iBone;
			}

			for (int iBone = 0; iBone < aiMesh->mNumBones; iBone++) {
				auto parentNode = aiMesh->mBones[iBone]->mNode->mParent;
				auto it = boneMapping.find(parentNode->mName.C_Str());
				if (it != boneMapping.end()) {
					mesh->bones[iBone].parentBoneIdx = it->second;
				}
				else {
					mesh->bones[iBone].parentBoneIdx = -1;
				}
				mesh->bones[iBone].initialLocal = *(Matrix4*)(void*)(&(aiMesh->mBones[iBone]->mNode->mTransformation.a1));
				mesh->bones[iBone].initialLocal = mesh->bones[iBone].initialLocal.Transpose();
			}

			PhysicsSystem::Get()->CalcMeshPhysicsDataFromBuffer(mesh);

			fullAsset->meshes.push_back(mesh);

			importedMeshesCount++;
		}

		CopyNodesHierarchy(scene, scene->mRootNode, fullAsset, fullAsset->rootNode);
		if (importedMeshesCount == 0) {
			//TODO send error to handler
			LogError("failed to import: no meshes");
		}

		return fullAsset;
	}

	void CopyNodesHierarchy(const aiScene* srcScene, aiNode* srcNode, const std::shared_ptr<FullMeshAsset>& dstScene, FullMeshAsset_Node& dstNode) {
		if (srcNode == nullptr) {
			return;
		}
		if (srcNode->mNumMeshes > 0) {
			//TODO multiple meshes
			dstNode.mesh = dstScene->meshes[srcNode->mMeshes[0]];
		}
		dstNode.localTransformMatrix = *(Matrix4*)&(srcNode->mTransformation);
		dstNode.localTransformMatrix = dstNode.localTransformMatrix.Transpose();
		ResizeVectorNoInit(dstNode.childNodes, srcNode->mNumChildren);
		for (int iChild = 0; iChild < srcNode->mNumChildren; iChild++) {
			CopyNodesHierarchy(srcScene, srcNode->mChildren[iChild], dstScene, dstNode.childNodes[iChild]);
		}
	}
};



void Mesh::Init() {
}


Mesh::~Mesh() {
	if (bgfx::isValid(vertexBuffer)) {
		bgfx::destroy(vertexBuffer);
	}
	if (bgfx::isValid(indexBuffer)) {
		bgfx::destroy(indexBuffer);
	}
}
DECLARE_BINARY_ASSET(Mesh, MeshAssetImporter);