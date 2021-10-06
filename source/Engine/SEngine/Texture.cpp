

#include "Texture.h"
#include "Resources.h"
#include "bimg/bimg.h"
#include "bx/allocator.h"
#include "bgfx/bgfx.h"
#include <windows.h>
#include "Serialization.h"
#include "bimg/decode.h"
#include "yaml-cpp/yaml.h"
#include "Compression.h"
#include "DbgVars.h"

static bx::DefaultAllocator s_bxAllocator = bx::DefaultAllocator();

//TODO streaming
DBG_VAR_BOOL(dbg_lowTextures, "low res textures", false);

class TextureImporter : public AssetImporter {
public:

	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override
	{
		int importerVersion = 9;
		YAML::Node metaYaml;
		if (!databaseHandle.ReadMeta(metaYaml)) {
			metaYaml = YAML::Node();
		}
		//TODO is normal
		//TODO is linear
		bool hasMips = metaYaml["mips"].IsDefined() ? metaYaml["mips"].as<bool>() : true;
		auto formatStr = metaYaml["format"].IsDefined() ? metaYaml["format"].as<std::string>() : "BC1";
		auto flags = metaYaml["flags"].IsDefined() ? metaYaml["flags"].as<int>() : BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC;

		bool isNormalMap;
		if (metaYaml["isNormal"].IsDefined()) {
			isNormalMap = metaYaml["isNormal"].as<bool>();
		}
		else {
			auto name = databaseHandle.GetAssetFileName();
			std::transform(name.begin(), name.end(), name.begin(),
				[](unsigned char c) { return std::tolower(c); });

			isNormalMap = name.find("normal") != std::string::npos;
		}

		//TODO return default invalid texture
		auto txBin = LoadTexture(importerVersion, databaseHandle, hasMips, formatStr, isNormalMap, dbg_lowTextures);
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


		auto tx = bgfx::createTexture2D(
			imageContainer.m_width,
			imageContainer.m_height,
			imageContainer.m_numMips > 1,
			imageContainer.m_numLayers,
			bgfx::TextureFormat::Enum(imageContainer.m_format),
			flags,
			mem);

		if (!bgfx::isValid(tx)) {
			bimg::imageFree(pImageContainer);
			return false;
		}

		auto texture = std::make_shared<Texture>();
		texture->handle = tx;

		bgfx::setName(tx, databaseHandle.GetAssetPath().c_str());
		//TODO keep memeory buffer ?

		databaseHandle.AddAssetToLoaded("texture", texture);

		return true;
	}

	std::vector<uint8_t> LoadTexture(int importerVersion, AssetDatabase_BinaryImporterHandle& databaseHandle, bool mips, std::string format, bool isNormalMap, bool loadLow) {
		//std::string assetPath = databaseHandle.;
		//std::string metaAssetPath = databaseHandle.GetLibraryPathFromId("meta");
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

		if (lastChange == 0) {
			needRebuild = false;
		}

		std::string libraryFileName = loadLow ? "texture_low" : "texture";

		std::vector<uint8_t> buffer;
		bool bufferLoaded = false;
		if (!needRebuild) {
			bufferLoaded = databaseHandle.ReadFromLibraryFile(libraryFileName, buffer);
			if (bufferLoaded) {
				BinaryBuffer from(std::move(buffer));
				BinaryBuffer to;
				bool decompressed = Compression::Decompress(from, to);
				ASSERT(decompressed);
				buffer = to.ReleaseData();
				bufferLoaded = decompressed;
			}
		}

		if (bufferLoaded) {
			return std::move(buffer);
		}

		databaseHandle.EnsureForderForLibraryFileExists("texture");

		std::string convertedAssetPath = databaseHandle.GetLibraryPathFromId("texture");
		std::string convertedAssetPathLow = databaseHandle.GetLibraryPathFromId("texture_low");

		bool converted = ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPath, mips, format, isNormalMap, 0);

		//TODO just load mip from full size texture
		bool convertedLow = ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPathLow, mips, format, isNormalMap, 4);

		if (!converted || !convertedLow) {
			return buffer;
		}

		auto metaNode = YAML::Node();
		metaNode["lastChange"] = lastChange;
		metaNode["lastMetaChange"] = lastChangeMeta;
		metaNode["importerVersion"] = importerVersion;

		databaseHandle.WriteToLibraryFile("meta", metaNode);

		BinaryBuffer binBuffer;

		std::vector<uint8_t> tempBuffer;
		databaseHandle.ReadFromLibraryFile("texture", tempBuffer);

		std::vector<uint8_t> tempBuffer2;
		databaseHandle.ReadFromLibraryFile("texture_low", tempBuffer2);

		buffer = loadLow ? tempBuffer2 : tempBuffer;

		binBuffer = BinaryBuffer(std::move(tempBuffer));
		Compression::Compress(binBuffer);
		databaseHandle.WriteToLibraryFile("texture", binBuffer.GetData());


		binBuffer = BinaryBuffer(std::move(tempBuffer2));
		Compression::Compress(binBuffer);
		databaseHandle.WriteToLibraryFile("texture_low", binBuffer.GetData());


		return buffer;
	}

	bool ConvertTextureFile(AssetDatabase_BinaryImporterHandle& databaseHandle,
		const std::string& inFile,
		const std::string& outFile,
		bool isMips,
		const std::string& format,
		bool isNormalMap,
		int skippedMips)
	{
		std::string params = "";
		params += " -f " + inFile;
		params += " -o " + outFile;
		params += " --mips ";
		params += (isMips ? "true" : "false");
		params += " --mipskip ";
		params += FormatString("%d", skippedMips);
		if (isNormalMap) {
			params += " --normalmap ";
		}
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

		//TODO use compressonator and multiple threads if needed
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


		if (!result)
		{
			LogError("Failed to compile texure '%s'", databaseHandle.GetAssetPath().c_str());
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

};
DECLARE_BINARY_ASSET(Texture, TextureImporter);


Texture::~Texture() {
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
	}
}