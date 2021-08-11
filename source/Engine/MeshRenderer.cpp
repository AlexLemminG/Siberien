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

	VertexLayout& End() {
		bgfxLayout.end();
		return *this;
	}
	std::vector<bgfx::Attrib::Enum> attributes;

	std::vector<uint8_t> CreateBuffer(aiMesh* mesh) {
		int numVerts = mesh->mNumVertices;
		int stride = bgfxLayout.m_stride;
		std::vector<uint8_t> buffer;
		buffer.resize(bgfxLayout.getSize(numVerts));
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
		}
		return std::move(buffer);
	}
};

static VertexLayout layout_pos;
void InitVertexLayouts() {
	layout_pos.Begin().AddPosition().AddNormal().AddTangent().AddTexCoord().End();
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
		auto* scene = importer.ReadFile(fullPath, aiProcess_ValidateDataStructure | aiProcess_CalcTangentSpace);
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
			mesh->buffer = layout_pos.CreateBuffer(aiMesh);
			mesh->vertexBuffer = bgfx::createVertexBuffer(bgfx::makeRef(&mesh->buffer[0], mesh->buffer.size()), layout_pos.bgfxLayout);
			bgfx::setName(mesh->vertexBuffer, mesh->originalMeshPtr->mName.C_Str());
			mesh->indices = CreateIndices(aiMesh);
			mesh->indexBuffer = bgfx::createIndexBuffer(bgfx::makeRef(&mesh->indices[0], mesh->indices.size() * sizeof(uint16_t)));
			bgfx::setName(mesh->indexBuffer, mesh->originalMeshPtr->mName.C_Str());

			importedMeshesCount++;
			//bgfx::createVertexBuffer(bgfx::makeRef(aiMesh->mVertices, aiMesh->mNumVertices), VertexLayouts::);
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
	pointLights.erase(std::find(pointLights.begin(), pointLights.end(), this), pointLights.end());
}
std::vector<PointLight*> PointLight::pointLights;
