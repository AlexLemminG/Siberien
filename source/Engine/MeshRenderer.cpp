#include "bx/allocator.h"

#include "MeshRenderer.h"
#include "assimp/Importer.hpp"
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include "bgfx/bgfx.h"
#include <windows.h>
#include "assimp/scene.h"
#include "bimg/bimg.h"
#include "bimg/decode.h"
#include "Time.h"
#include "GameObject.h"
#include "Dbg.h"
//#include "bgfx_utils.h"

class VertexLayout {
public:
	bgfx::VertexLayout bgfxLayout;

	VertexLayout& Begin() {
		bgfxLayout.begin();
		return *this;
	}

	VertexLayout& AddPosition() {
		bgfxLayout.add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::Position);
		return *this;
	}

	VertexLayout& AddNormal() {
		bgfxLayout.add(bgfx::Attrib::Normal, 3, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::Normal);
		return *this;
	}

	VertexLayout& AddTexCoord() {
		bgfxLayout.add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::TexCoord0);
		return *this;
	}

	VertexLayout& AddTangent() {
		bgfxLayout.add(bgfx::Attrib::Tangent, 3, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::Tangent);
		return *this;
	}

	VertexLayout& AddIndices() {
		bgfxLayout.add(bgfx::Attrib::Indices, 4, bgfx::AttribType::Uint8);
		attributes.push_back(bgfx::Attrib::Indices);
		return *this;
	}

	VertexLayout& AddWeights() {
		bgfxLayout.add(bgfx::Attrib::Weight, 4, bgfx::AttribType::Float);
		attributes.push_back(bgfx::Attrib::Weight);
		return *this;
	}

	VertexLayout& End() {
		bgfxLayout.end();
		return *this;
	}
	std::vector<bgfx::Attrib::Enum> attributes;

	std::vector<uint8_t> CreateBuffer(std::shared_ptr<Mesh> gameMesh) {
		auto* mesh = gameMesh->originalMeshPtr;
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
					*((float*)dstBuffer) = srcBuffer->x;
					*(((float*)dstBuffer) + 1) = -srcBuffer->y;
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
};

static VertexLayout layout_pos;
void InitVertexLayouts() {
	layout_pos.Begin().AddPosition().AddNormal().AddTangent().AddTexCoord().AddIndices().AddWeights().End();
}

void PrintHierarchy(aiNode* node, int offset) {
	std::string off = "";
	for (int i = 0; i < offset; i++) {
		off += " ";
	}
	LogError(off + node->mName.C_Str());
	for (int i = 0; i < node->mNumChildren; i++) {
		PrintHierarchy(node->mChildren[i], offset + 1);
	}
}

class MeshAssetImporter : public AssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		//TODO importer could be heavy
		Assimp::Importer importer{};
		//TODO return void for all importers
		auto fullPath = AssetDatabase::assetsFolderPrefix + path; //TODO pass full path or file as argument
		auto* scene = importer.ReadFile(fullPath, aiProcess_ValidateDataStructure | aiProcess_CalcTangentSpace | aiProcess_PopulateArmatureData);
		if (!scene) {
			LogError("failed to import '%s': failed to open mesh", path.c_str());
			return nullptr;
		}
		scene = importer.GetOrphanedScene();
		auto fullAsset = std::make_shared<FullMeshAsset>();
		fullAsset->scene.reset(scene);
		//TODO pass separate database handle
		database.AddAsset(fullAsset, path, "FullMeshAsset");

		PrintHierarchy(scene->mRootNode, 0);
		int importedMeshesCount = 0;
		for (int iMesh = 0; iMesh < scene->mNumMeshes; iMesh++) {
			auto* aiMesh = scene->mMeshes[iMesh];
			auto mesh = std::make_shared<Mesh>();
			mesh->originalMeshPtr = aiMesh;
			database.AddAsset(mesh, path, aiMesh->mName.C_Str());

			//TODO should keep buffer ?
			mesh->buffer = layout_pos.CreateBuffer(mesh);
			mesh->vertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(&mesh->buffer[0], mesh->buffer.size()), layout_pos.bgfxLayout);
			bgfx::setName(mesh->vertexBuffer, mesh->originalMeshPtr->mName.C_Str());
			mesh->indices = CreateIndices(aiMesh);
			mesh->indexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(&mesh->indices[0], mesh->indices.size() * sizeof(uint16_t)));
			bgfx::setName(mesh->indexBuffer, mesh->originalMeshPtr->mName.C_Str());
			mesh->Init();

			importedMeshesCount++;
			//bgfx::createVertexBuffer(bgfx::makeRef(aiMesh->mVertices, aiMesh->mNumVertices), VertexLayouts::);
		}

		for (int iAnim = 0; iAnim < scene->mNumAnimations; iAnim++) {
			auto anim = scene->mAnimations[iAnim];
			auto animation = std::make_shared<MeshAnimation>();
			animation->assimAnimation = anim;
			database.AddAsset(animation, path, anim->mName.C_Str());
		}

		if (importedMeshesCount == 0) {
			LogError("failed to import '%s': no meshes", path.c_str());
		}


		return nullptr;
	}
private:
	std::vector<uint16_t> CreateIndices(aiMesh* mesh) {
		std::vector<uint16_t> indices;
		for (int i = 0; i < mesh->mNumFaces; i++) {
			ASSERT(mesh->mFaces[i].mNumIndices == 3);
			indices.push_back(mesh->mFaces[i].mIndices[0]);
			indices.push_back(mesh->mFaces[i].mIndices[1]);
			indices.push_back(mesh->mFaces[i].mIndices[2]);
		}
		return std::move(indices);
	}

};

class ShaderAssetImporter : public TextAssetImporter {
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) override {
		auto vsShaderPath = node["vs"].as<std::string>();
		auto fsShaderPath = node["fs"].as<std::string>();

		//TODO return default invalid shader
		auto vsBin = LoadShader(database, vsShaderPath, true);
		if (!vsBin) {
			return nullptr;
		}
		auto vs = bgfx::createShader(bgfx::makeRef(&vsBin->buffer[0], vsBin->buffer.size()));
		if (vs.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}
		auto fsBin = LoadShader(database, fsShaderPath, false);
		if (!fsBin) {
			return nullptr;
		}
		auto fs = bgfx::createShader(bgfx::makeRef(&fsBin->buffer[0], fsBin->buffer.size()));
		if (fs.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}
		auto program = bgfx::createProgram(vs, fs, false);
		if (program.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}

		auto shader = std::make_shared<Shader>();
		shader->buffers.push_back(vsBin);
		shader->buffers.push_back(fsBin);
		shader->program = program;
		bgfx::setName(vs, vsShaderPath.c_str());
		bgfx::setName(fs, vsShaderPath.c_str());

		return shader;
		//return nullptr;
	}

	std::shared_ptr<BinaryAsset> LoadShader(AssetDatabase& database, std::string path, bool isVertex) {
		std::string textAssetPath = path;
		std::string binAssetPath = path;

		binAssetPath += "\\";

		const char* shaderPath = "???";

		switch (bgfx::getRendererType())
		{
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:  shaderPath = "dx9";   break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12: shaderPath = "dx11";  break;
		case bgfx::RendererType::Gnm:        shaderPath = "pssl";  break;
		case bgfx::RendererType::Metal:      shaderPath = "metal"; break;
		case bgfx::RendererType::Nvn:        shaderPath = "nvn";   break;
		case bgfx::RendererType::OpenGL:     shaderPath = "glsl";  break;
		case bgfx::RendererType::OpenGLES:   shaderPath = "essl";  break;
		case bgfx::RendererType::Vulkan:     shaderPath = "spirv"; break;
		case bgfx::RendererType::WebGPU:     shaderPath = "spirv"; break;

		case bgfx::RendererType::Count:
			ASSERT(false);//TODO "You should not be here!"
			break;
		}

		binAssetPath += shaderPath;
		binAssetPath += ".bin";

		std::string metaPath = binAssetPath + ".meta";

		bool needRebuild = false;
		auto meta = database.LoadTextAssetFromLibrary(metaPath);
		long lastChange = database.GetLastModificationTime(textAssetPath);
		if (meta == nullptr) {
			needRebuild = true;
		}
		else {
			long lastChangeRecorded = meta->GetYamlNode()["lastChange"].as<long>();
			if (lastChange != lastChangeRecorded) {
				needRebuild = true;
			}
		}

		std::shared_ptr<BinaryAsset> binaryAsset;
		if (!needRebuild) {
			binaryAsset = database.LoadBinaryAssetFromLibrary(binAssetPath);
		}

		if (binaryAsset) {
			return binaryAsset;
		}

		std::string params = "";
		params += " -f " + database.GetAssetPath(textAssetPath);
		params += " -o " + database.GetLibraryPath(binAssetPath);
		params += " --type ";
		params += (isVertex ? "v" : "f");
		params += " --platform ";
		params += "windows";
		params += " -i assets\\shaders\\include";
		params += " -p ";
		params += (isVertex ? "v" : "p");
		params += "s_5_0";

		STARTUPINFOA si;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		std::vector<char> buffer(params.begin(), params.end());
		buffer.push_back(0);
		LPSTR ccc = &buffer[0];

		std::string binAssetFolder = database.GetLibraryPath(path) + "\\";

		database.CreateFolders(binAssetFolder);

		auto result = CreateProcessA(
			database.GetToolsPath("shaderc.exe").c_str()
			, &buffer[0]
			, NULL
			, NULL
			, false
			, 0
			, NULL
			, NULL
			, &si
			, &pi);
		//TODO error checks

		if (!result)
		{
			LogError("Failed to compile shader '%s'", path.c_str());
			return nullptr;
		}
		else
		{
			// Successfully created the process.  Wait for it to finish.
			WaitForSingleObject(pi.hProcess, INFINITE);

			// Close the handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		auto metaNode = YAML::Node();
		metaNode["lastChange"] = lastChange;

		std::ofstream fout(database.GetLibraryPath(metaPath));
		fout << metaNode;

		binaryAsset = database.LoadBinaryAssetFromLibrary(binAssetPath);
		if (binaryAsset) {
			return binaryAsset;
		}
		else {
			return nullptr;
		}

	}
};


class TextureImporter : public AssetImporter {
public:
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const std::string& path) override
	{
		int importerVersion = 5;

		std::string metaPath = path + ".meta";
		auto meta = database.LoadTextAsset(metaPath);
		YAML::Node metaYaml;
		if (!meta || !meta->GetYamlNode()) {
			metaYaml = YAML::Node();
		}
		else {
			metaYaml = meta->GetYamlNode();

		}
		bool hasMips = metaYaml["mips"].IsDefined() ? metaYaml["mips"].as<bool>() : false;
		auto formatStr = metaYaml["format"].IsDefined() ? metaYaml["format"].as<std::string>() : "BC1";

		//TODO return default invalid shader
		auto txBin = LoadTexture(importerVersion, database, path, hasMips, formatStr);
		if (!txBin) {
			return nullptr;
		}

		auto allocator = bx::DefaultAllocator();
		auto pImageContainer = bimg::imageParse(&allocator, &txBin->buffer[0], txBin->buffer.size());
		//TODO delete

		//bimg::ImageContainer* imageContainer = bimg::imageParse(entry::getAllocator(), data, size);
		if (!pImageContainer) {
			return nullptr;
		}

		const bgfx::Memory* mem = bgfx::makeRef(
			pImageContainer->m_data
			, pImageContainer->m_size
		);
		//TODO delete
		auto imageContainer = *pImageContainer;
		//const bgfx::Memory* memory = bgfx::makeRef(&txBin->buffer[0] + imageContainer.m_offset, txBin->buffer.size() - imageContainer.m_offset);
		auto tx = bgfx::createTexture2D(
			imageContainer.m_width,
			imageContainer.m_height,
			imageContainer.m_numMips > 1,
			imageContainer.m_numLayers,
			bgfx::TextureFormat::Enum(imageContainer.m_format),
			BGFX_TEXTURE_NONE | BGFX_SAMPLER_NONE,
			mem);
		if (!bgfx::isValid(tx)) {
			return nullptr;
		}

		auto texture = std::make_shared<Texture>();
		texture->handle = tx;
		texture->bin = txBin;

		bgfx::setName(tx, path.c_str());
		//TODO keep memeory buffer ?

		database.AddAsset(texture, path, "");

		return texture;
	}

	std::shared_ptr<BinaryAsset> LoadTexture(int importerVersion, AssetDatabase& database, std::string path, bool mips, std::string format) {
		std::string assetPath = path;
		std::string metaAssetPath = path + ".meta";
		std::string convertedAssetPath = path;

		convertedAssetPath += ".bin";

		std::string metaPath = convertedAssetPath + ".meta";

		bool needRebuild = false;
		auto meta = database.LoadTextAssetFromLibrary(metaPath);
		long lastChangeMeta = database.GetLastModificationTime(metaAssetPath);
		long lastChange = database.GetLastModificationTime(assetPath);
		if (meta == nullptr) {
			needRebuild = true;
		}
		else {
			long lastChangeRecorded = meta->GetYamlNode()["lastChange"].as<long>();
			long lastMetaChangeRecorded = meta->GetYamlNode()["lastMetaChange"].as<long>();
			long importerVersionRecorded = meta->GetYamlNode()["importerVersion"].IsDefined() ? meta->GetYamlNode()["importerVersion"].as<long>() : -1;
			if (lastChange != lastChangeRecorded || lastMetaChangeRecorded != lastChangeMeta || importerVersionRecorded != importerVersion) {
				needRebuild = true;
			}
		}

		std::shared_ptr<BinaryAsset> binaryAsset;
		if (!needRebuild) {
			binaryAsset = database.LoadBinaryAssetFromLibrary(convertedAssetPath);
		}

		if (binaryAsset) {
			return binaryAsset;
		}

		std::string params = "";
		params += " -f " + database.GetAssetPath(assetPath);
		params += " -o " + database.GetLibraryPath(convertedAssetPath);
		params += " --mips ";
		params += (mips ? "true" : "false");
		params += " -t ";
		params += format;
		params += " --as ";
		params += "dds";

		STARTUPINFOA si;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		std::vector<char> buffer(params.begin(), params.end());
		buffer.push_back(0);
		LPSTR ccc = &buffer[0];

		std::string binAssetFolder = database.GetLibraryPath(path) + "\\";

		database.CreateFolders(binAssetFolder);

		auto result = CreateProcessA(
			database.GetToolsPath("texturec.exe").c_str()
			, &buffer[0]
			, NULL
			, NULL
			, false
			, 0
			, NULL
			, NULL
			, &si
			, &pi);
		//TODO error checks

		if (!result)
		{
			LogError("Failed to compile texure '%s'", path.c_str());
			return nullptr;
		}
		else
		{
			// Successfully created the process.  Wait for it to finish.
			WaitForSingleObject(pi.hProcess, INFINITE);

			// Close the handles.
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}

		auto metaNode = YAML::Node();
		metaNode["lastChange"] = lastChange;
		metaNode["lastMetaChange"] = lastChangeMeta;
		metaNode["importerVersion"] = importerVersion;

		std::ofstream fout(database.GetLibraryPath(metaPath));
		fout << metaNode;

		binaryAsset = database.LoadBinaryAssetFromLibrary(convertedAssetPath);
		if (binaryAsset) {
			return binaryAsset;
		}
		else {
			return nullptr;
		}

	}

};

DECLARE_BINARY_ASSET(Mesh, MeshAssetImporter);
DECLARE_TEXT_ASSET(Material);
DECLARE_CUSTOM_TEXT_ASSET(Shader, ShaderAssetImporter);
DECLARE_BINARY_ASSET(Texture, TextureImporter);
DECLARE_TEXT_ASSET(MeshRenderer);
DECLARE_TEXT_ASSET(DirLight);
DECLARE_TEXT_ASSET(PointLight);

std::vector<MeshRenderer*> MeshRenderer::enabledMeshRenderers;

void PointLight::OnEnable() {
	pointLights.push_back(this);
}

void PointLight::OnDisable() {
	pointLights.erase(std::find(pointLights.begin(), pointLights.end(), this));
}
std::vector<PointLight*> PointLight::pointLights;

Vector3 aiConvert(const aiVector3D& v) {
	return Vector3(v.x, v.y, v.z);
}
Quaternion aiConvert(const aiQuaternion& v) {
	return Quaternion(v.x, v.y, v.z, v.w);
}

template<typename OUT_T, typename IN_T>
static bool GetAnimValue(OUT_T& out, IN_T* keys, int numKeys, float t) {
	if (numKeys <= 0) {
		return false;
	}
	float tPos = Mathf::Repeat(t * 1000.f, keys[numKeys - 1].mTime);

	for (int iPos = 0; iPos < numKeys; iPos++) {
		if (iPos == numKeys - 1 || keys[iPos + 1].mTime >= tPos) {
			auto thisKey = keys[iPos];
			auto nextKey = iPos == numKeys - 1 ? thisKey : keys[iPos + 1];
			tPos = (tPos - thisKey.mTime) / Mathf::Max(nextKey.mTime - thisKey.mTime, 0.001f);

			out = Mathf::Lerp(aiConvert(thisKey.mValue), aiConvert(nextKey.mValue), tPos);
			return true;
		}
	}
	return false;
}
void AnimationTransform::ToMatrix(Matrix4& matrix) {
	matrix = Matrix4::Transform(position, rotation.ToMatrix(), scale);
}

AnimationTransform MeshAnimation::GetTransform(const std::string& bone, float t) {
	for (int iNode = 0; iNode < assimAnimation->mNumChannels; iNode++) {
		auto* channel = assimAnimation->mChannels[iNode];
		if (bone != channel->mNodeName.C_Str()) {
			continue;
		}

		AnimationTransform transform;
		GetAnimValue(transform.position, channel->mPositionKeys, channel->mNumPositionKeys, t);
		GetAnimValue(transform.rotation, channel->mRotationKeys, channel->mNumRotationKeys, t);
		GetAnimValue(transform.scale, channel->mScalingKeys, channel->mNumScalingKeys, t);

		return transform;
	}
	ASSERT(false);
	return AnimationTransform();
}

AnimationTransform AnimationTransform::Lerp(const AnimationTransform& a, const AnimationTransform& b, float t) {
	AnimationTransform l;
	l.position = Mathf::Lerp(a.position, b.position, t);
	l.rotation = Mathf::Lerp(a.rotation, b.rotation, t);
	l.scale = Mathf::Lerp(a.scale, b.scale, t);
	return l;
}

void Animator::Update() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	if (currentAnimation != nullptr) {
		currentTime += Time::deltaTime() * speed;
		for (auto& bone : mesh->bones) {
			auto transform = currentAnimation->GetTransform(bone.name, currentTime);
			transform.ToMatrix(meshRenderer->bonesLocalMatrices[bone.idx]);
			SetRot(meshRenderer->bonesLocalMatrices[bone.idx], GetRot(bone.initialLocal));//TODO f this shit
		}
	}
	UpdateWorldMatrices();


	for (auto bone : meshRenderer->bonesWorldMatrices) {
		//Dbg::Draw(bone, 5);
	}
}

void Animator::OnEnable() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	for (auto& bone : mesh->bones) {
		meshRenderer->bonesLocalMatrices[bone.idx] = bone.initialLocal;
	}
	UpdateWorldMatrices();

	currentAnimation = defaultAnimation;
}

void Animator::UpdateWorldMatrices() {
	auto meshRenderer = gameObject()->GetComponent<MeshRenderer>();
	if (!meshRenderer) {
		return;
	}
	auto mesh = meshRenderer->mesh;
	if (!mesh) {
		return;
	}

	for (auto& bone : mesh->bones) {
		if (bone.parentBoneIdx != -1) {
			auto& parentBoneMatrix = meshRenderer->bonesWorldMatrices[bone.parentBoneIdx];
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		else {
			auto& parentBoneMatrix = gameObject()->transform()->matrix;
			meshRenderer->bonesWorldMatrices[bone.idx] = parentBoneMatrix * meshRenderer->bonesLocalMatrices[bone.idx];
		}
		auto rot = Quaternion::FromAngleAxis(Mathf::pi, Vector3_forward).ToMatrix4();
		//meshRenderer->bonesWorldMatrices[bone.idx] = meshRenderer->bonesWorldMatrices[bone.idx] * rot;
		//meshRenderer->bonesWorldMatrices[bone.idx] = meshRenderer->bonesWorldMatrices[bone.idx];
		meshRenderer->bonesFinalMatrices[bone.idx] = meshRenderer->bonesWorldMatrices[bone.idx] * bone.offset;
	}
}

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
}

DECLARE_TEXT_ASSET(Animator);
DECLARE_TEXT_ASSET(MeshAnimation);
DECLARE_TEXT_ASSET(SphericalHarmonics);

void MeshRenderer::OnEnable() {
	this->worldMatrix = Matrix4::Identity();
	enabledMeshRenderers.push_back(this);
	if (mesh && mesh->originalMeshPtr && mesh->originalMeshPtr->HasBones()) {
		bonesWorldMatrices.resize(mesh->originalMeshPtr->mNumBones);
		bonesLocalMatrices.resize(mesh->originalMeshPtr->mNumBones);
		bonesFinalMatrices.resize(mesh->originalMeshPtr->mNumBones);
	}
	if (material->randomColorTextures.size() > 0) {
		randomColorTextureIdx = Random::Range(0, material->randomColorTextures.size());
	}
}
