#include "Shader.h"
#include "Resources.h"
#include <windows.h>
#include "Serialization.h"


class ShaderAssetImporter : public TextAssetImporter {
	virtual std::shared_ptr<Object> Import(AssetDatabase& database, const YAML::Node& node) override {
		auto vsShaderPath = node["vs"].as<std::string>();
		auto fsShaderPath = node["fs"].as<std::string>();

		//TODO return default invalid shader
		auto vsBin = LoadShader(database, vsShaderPath, true);
		if (!vsBin) {
			return nullptr;
		}
		auto vs = bgfx::createShader(bgfx::copy(&vsBin->buffer[0], vsBin->buffer.size()));
		if (vs.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}
		auto fsBin = LoadShader(database, fsShaderPath, false);
		if (!fsBin) {
			return nullptr;
		}
		auto fs = bgfx::createShader(bgfx::copy(&fsBin->buffer[0], fsBin->buffer.size()));
		if (fs.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}
		auto program = bgfx::createProgram(vs, fs, false);
		if (program.idx == bgfx::kInvalidHandle) {
			return nullptr;
		}

		auto shader = std::make_shared<Shader>();
		//shader->buffers.push_back(vsBin);
		//shader->buffers.push_back(fsBin);
		shader->program = program;
		shader->name = std::string(vsShaderPath.c_str()) + " " + fsShaderPath.c_str();
		bgfx::setName(vs, vsShaderPath.c_str());
		bgfx::setName(fs, fsShaderPath.c_str());

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
DECLARE_CUSTOM_TEXT_ASSET(Shader, ShaderAssetImporter);
Shader::~Shader() {
	if (bgfx::isValid(program)) {
		bgfx::destroy(program);
	}
}