#include "Texture.h"
#include "Resources.h"
#include "bimg/bimg.h"
#include "bx/allocator.h"
#include "bgfx/bgfx.h"
#include <windows.h>
#include "Serialization.h"
#include "bimg/decode.h"

static bx::DefaultAllocator s_bxAllocator = bx::DefaultAllocator();

class TextureImporter : public AssetImporter {
public:
	// Inherited via AssetImporter
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override
	{
		int importerVersion = 5;
		YAML::Node metaYaml;
		if (!databaseHandle.ReadMeta(metaYaml)) {
			metaYaml = YAML::Node();
		}
		bool hasMips = metaYaml["mips"].IsDefined() ? metaYaml["mips"].as<bool>() : false;
		auto formatStr = metaYaml["format"].IsDefined() ? metaYaml["format"].as<std::string>() : "BC1";

		//TODO return default invalid shader
		auto txBin = LoadTexture(importerVersion, databaseHandle, hasMips, formatStr);
		if (txBin.size() == 0) {
			return false;
		}

		auto pImageContainer = bimg::imageParse(&s_bxAllocator, &txBin[0], txBin.size());
		
		if (!pImageContainer) {
			return false;
		}

		//TODO slow?
		auto mem = bgfx::copy(
			pImageContainer->m_data
			, pImageContainer->m_size
		);

		//TODO delete
		auto imageContainer = *pImageContainer;
		bimg::imageFree(pImageContainer);
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
			bimg::imageFree(pImageContainer);
			return false;
		}

		auto texture = std::make_shared<Texture>();
		texture->handle = tx;
		//texture->bin = txBin;
		//texture->pImageContainer = pImageContainer;

		bgfx::setName(tx, databaseHandle.GetAssetPath().c_str());
		//TODO keep memeory buffer ?

		databaseHandle.AddAssetToLoaded("texture", texture);

		return true;
	}

	std::vector<uint8_t> LoadTexture(int importerVersion, AssetDatabase_BinaryImporterHandle& databaseHandle, bool mips, std::string format) {
		//std::string assetPath = databaseHandle.;
		//std::string metaAssetPath = databaseHandle.GetLibraryPathFromId("meta");
		std::string convertedAssetPath = databaseHandle.GetLibraryPathFromId("texture");
		std::string metaPath = databaseHandle.GetLibraryPathFromId("meta");


		bool needRebuild = false;
		YAML::Node libraryMeta;
		databaseHandle.ReadFromLibraryFile("meta", libraryMeta);
		long lastChangeMeta;
		long lastChange;
		databaseHandle.GetLastModificationTime(lastChange, lastChangeMeta);
		if (!libraryMeta.IsDefined()) {
			needRebuild = true;
		}
		else {
			long lastChangeRecorded = libraryMeta["lastChange"].as<long>();
			long lastMetaChangeRecorded = libraryMeta["lastMetaChange"].as<long>();
			long importerVersionRecorded = libraryMeta["importerVersion"].IsDefined() ? libraryMeta["importerVersion"].as<long>() : -1;
			if (lastChange != lastChangeRecorded || lastMetaChangeRecorded != lastChangeMeta || importerVersionRecorded != importerVersion) {
				needRebuild = true;
			}
		}

		std::vector<uint8_t> buffer;
		bool bufferLoaded = false;
		if (!needRebuild) {
			bufferLoaded = databaseHandle.ReadFromLibraryFile("texture", buffer);
		}

		if (bufferLoaded) {
			return std::move(buffer);
		}

		databaseHandle.EnsureForderForLibraryFileExists("texture");

		std::string params = "";
		params += " -f " + databaseHandle.GetAssetPath();
		params += " -o " + convertedAssetPath;
		params += " --mips ";
		params += (mips ? "true" : "false");
		params += " -t ";
		params += format;
		params += " --as ";
		params += "dds";

		//TODO move to platform independent stuff
		STARTUPINFOA si;
		memset(&si, 0, sizeof(STARTUPINFOA));
		si.cb = sizeof(STARTUPINFOA);

		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(PROCESS_INFORMATION));

		std::vector<char> paramsBuffer(params.begin(), params.end());
		paramsBuffer.push_back(0);
		LPSTR ccc = &paramsBuffer[0];

		auto result = CreateProcessA(
			databaseHandle.GetToolPath("texturec.exe").c_str()
			, &paramsBuffer[0]
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
			LogError("Failed to compile texure '%s'", databaseHandle.GetAssetPath().c_str());
			return buffer;
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

		databaseHandle.WriteToLibraryFile("meta", metaNode);
		databaseHandle.ReadFromLibraryFile("texture", buffer);
		return buffer;
	}

};
DECLARE_BINARY_ASSET(Texture, TextureImporter);


Texture::~Texture() {
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
	}
}