

#include "Shader.h"
#include "Resources.h"
#include "Serialization.h"
#include "Render.h"
#include "ryml.hpp"
#include <windows.h>
#include "Config.h"

static constexpr int importerVersion = 6;

class AssetWithModificationDate {
public:
	std::string assetPath;
	uint64_t modificationDate;

	AssetWithModificationDate() :assetPath(""), modificationDate(0) { }

	AssetWithModificationDate(const std::string& assetPath, uint64_t modificationDate) :assetPath(assetPath), modificationDate(modificationDate) { }

	REFLECT_BEGIN(AssetWithModificationDate);
	REFLECT_VAR(assetPath);
	REFLECT_VAR(modificationDate);
	REFLECT_END();
};
class ShaderLibraryMeta : public Object {
public:
	std::vector<AssetWithModificationDate> modificationDates;
	int importerVersion;

	REFLECT_BEGIN(ShaderLibraryMeta);
	REFLECT_VAR(modificationDates);
	REFLECT_VAR(importerVersion);
	REFLECT_END();
};

class BasicShaderAssetImporter : public AssetImporter {
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override
	{
		bool isDebug = Render::IsDebugMode();

		bool isVertex = false;
		std::vector<uint8_t> buffer;
		auto extention = databaseHandle.GetFileExtension();
		if (extention == "vs") {
			isVertex = true;
			if (!LoadShader(buffer, databaseHandle, true, isDebug)) {
				return false;
			}
		}
		else if (extention == "fs") {
			if (!LoadShader(buffer, databaseHandle, false, isDebug)) {
				return false;
			}
		}
		else {
			return false;
		}


		auto basicShader = bgfx::createShader(bgfx::copy(&buffer[0], buffer.size()));
		if (basicShader.idx == bgfx::kInvalidHandle) {
			return false;
		}
		bgfx::setName(basicShader, databaseHandle.GetAssetPath().c_str());


		if (isVertex) {
			std::shared_ptr <VertexShader> shader = std::make_shared<VertexShader>();
			shader->handle = basicShader;
			databaseHandle.AddAssetToLoaded("shader", shader);
		}
		else {
			std::shared_ptr <PixelShader> shader = std::make_shared<PixelShader>();
			shader->handle = basicShader;
			databaseHandle.AddAssetToLoaded("shader", shader);
		}
		return true;
	}

	bool LoadShader(std::vector<uint8_t>& buffer, AssetDatabase_BinaryImporterHandle& databaseHandle, bool isVertex, bool isDebug) {
		//TODO propper fix for loading without sources + check other assets for same mistakes
		std::string textAssetPath = databaseHandle.GetAssetPath();
		std::string binAssetPathId = "";

		const char* shaderPath = "???";
		std::string compilerProfile = "";
		switch (bgfx::getRendererType())
		{
		case bgfx::RendererType::Noop:
		case bgfx::RendererType::Direct3D9:  shaderPath = "dx9";   compilerProfile = ""; break;
		case bgfx::RendererType::Direct3D11:
		case bgfx::RendererType::Direct3D12: shaderPath = "dx11";  compilerProfile = std::string(isVertex ? "v" : "p") + "s_5_0";  break;
		case bgfx::RendererType::Gnm:        shaderPath = "pssl";  break;
		case bgfx::RendererType::Metal:      shaderPath = "metal"; break;
		case bgfx::RendererType::Nvn:        shaderPath = "nvn";   break;
		case bgfx::RendererType::OpenGL:     shaderPath = "glsl";  break;
		case bgfx::RendererType::OpenGLES:   shaderPath = "essl";  break;
		case bgfx::RendererType::Vulkan:     shaderPath = "spirv"; compilerProfile = "spirv15-12"; break;
		case bgfx::RendererType::WebGPU:     shaderPath = "spirv"; compilerProfile = "spirv15-12"; break;

		case bgfx::RendererType::Count:
			ASSERT(false);//TODO "You should not be here!"
			break;
		}


		binAssetPathId += shaderPath;

		if (isDebug) {
			binAssetPathId += "_debug";
		}

		std::string binAssetPath = databaseHandle.GetLibraryPathFromId(binAssetPathId);

		std::string metaId = binAssetPathId + ".meta";

		bool needRebuild = false;
		std::unique_ptr<ryml::Tree> metaYaml;
		bool hasMeta = databaseHandle.ReadFromLibraryFile(metaId, metaYaml);
		if (!hasMeta) {
			needRebuild = true;
		}
		else {
			ShaderLibraryMeta meta;
			SerializationContext c = SerializationContext(metaYaml->rootref());
			::Deserialize(c, meta);
			uint64_t lastFileChange;
			if (meta.importerVersion != importerVersion) {
				needRebuild = true;
			}
			else {
				for (auto& record : meta.modificationDates) {
					uint64_t lastFileChange = databaseHandle.GetLastModificationTime(record.assetPath);
					if (lastFileChange != record.modificationDate && lastFileChange != 0) {
						needRebuild = true;
						break;
					}
				}
			}
		}
		std::string dependenciesAssetPathId = binAssetPathId + ".d";
		//TODO check modification of all included ones

		bool hasBinary = false;
		if (!needRebuild) {
			hasBinary = databaseHandle.ReadFromLibraryFile(binAssetPathId, buffer);
		}

		if (hasBinary) {
			return true;
		}
		std::string params = "";
		params += " -f " + databaseHandle.GetAssetPathReal();
		params += " -o " + databaseHandle.GetLibraryPathFromId(binAssetPathId);
		params += " --type";
		params += (isVertex ? " v" : " f");
		params += " --platform";
		params += " windows";
		params += " -i engine\\assets\\shaders\\include";//TODO not like this
		params += " -i engine\\assets\\engine\\shaders\\include";//TODO not like this
		params += " -p ";
		params += compilerProfile;
		params += " --depends";

		if (isDebug) {
			//params += " --raw ";
			params += " --debug";
		}
		else {
			params += " -O 3";
		}


		STARTUPINFOA si;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		std::vector<char> cmdBuffer(params.begin(), params.end());
		cmdBuffer.push_back(0);
		LPSTR ccc = &cmdBuffer[0];

		databaseHandle.EnsureForderForLibraryFileExists(binAssetPathId);

		auto result = CreateProcessA(
			databaseHandle.GetToolPath("shaderc.exe").c_str()
			, &cmdBuffer[0]
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
			LogError("Failed to compile shader '%s'", databaseHandle.GetAssetPath().c_str());
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
		const char* paramscstr = params.c_str();

		std::vector<std::string> files;
		files.push_back(databaseHandle.GetAssetPath());
		{
			std::vector<uint8_t> depBuffer;
			databaseHandle.ReadFromLibraryFile(dependenciesAssetPathId, depBuffer);
			bool hadFirst = false;
			int start = 0;
			//TODO less hacky pleeeeease
			for (int i = 0; i < depBuffer.size(); i++) {
				if (depBuffer[i] == '\n') {
					if (hadFirst) {
						std::string fileName;
						if (depBuffer[i - 1] == '\\') {
							fileName = (std::string((char*)&depBuffer[start], i - start - 2));
						}
						else {
							fileName = (std::string((char*)&depBuffer[start], i - start));
						}
						files.push_back(AssetDatabase::Get()->GetVirtualPath(fileName));
					}
					else {
						hadFirst = true;
					}
					start = i + 2;
				}
			}
		}
		ShaderLibraryMeta expectedMeta;
		expectedMeta.importerVersion = importerVersion;
		for (auto& file : files) {
			uint64_t lastModificationDate = databaseHandle.GetLastModificationTime(file);
			ASSERT(lastModificationDate != 0);
			expectedMeta.modificationDates.push_back(AssetWithModificationDate(file, lastModificationDate));
		}

		SerializationContext c{};
		::Serialize(c, expectedMeta);
		databaseHandle.WriteToLibraryFile(metaId, c.GetYamlNode());

		hasBinary = databaseHandle.ReadFromLibraryFile(binAssetPathId, buffer);
		if (hasBinary) {
			Log("Converted shader '%s'", databaseHandle.GetAssetPath().c_str());
			return true;
		}
		else {
			LogError("Failed to convert shader '%s'", databaseHandle.GetAssetPath().c_str());
			return false;
		}

	}
};
//TODO dont pass Shader and others to DECLARE
DECLARE_BINARY_ASSET(Shader, BasicShaderAssetImporter);

DECLARE_TEXT_ASSET(Shader);
Shader::~Shader() {
	if (bgfx::isValid(program)) {
		bgfx::destroy(program);
	}
}

void Shader::OnAfterDeserializeCallback(const SerializationContext& context) {
	if (!vs || !bgfx::isValid(vs->handle)) {
		return;
	}
	if (!fs || !bgfx::isValid(fs->handle)) {
		return;
	}
	program = bgfx::createProgram(vs->handle, fs->handle, false);
	if (program.idx == bgfx::kInvalidHandle) {
		return;
	}
	//TODO 
	//name = std::string(vsShaderPath.c_str()) + " " + fsShaderPath.c_str();

}
