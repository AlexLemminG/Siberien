

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
#include "Compressonator.h"
#include <bx\file.h>
#include "SMath.h"

static bx::DefaultAllocator s_bxAllocator = bx::DefaultAllocator();
static constexpr bool compressTextures = false;
static constexpr int skippedMipsForLow = 4;
static constexpr int importerVersion = 11;

//TODO streaming
DBG_VAR_BOOL(dbg_lowTextures, "low res textures", false);

class TextureImporter : public AssetImporter {
public:
	static int WrapModeStrToFlags(std::string& str) {
		if (str == "repeat") {
			return 0;
		}
		else if (str == "clamp") {
			return BGFX_SAMPLER_UVW_CLAMP;
		}
		else if (str == "mirror") {
			return BGFX_SAMPLER_UVW_MIRROR;
		}
		else {
			LogError("Unknown wrapMode '%s'", str.c_str()); //TODO texture file path
			return 0;
		}
	}
	virtual bool ImportAll(AssetDatabase_BinaryImporterHandle& databaseHandle) override
	{
		YAML::Node metaYaml;
		if (!databaseHandle.ReadMeta(metaYaml)) {
			metaYaml = YAML::Node();
		}
		//TODO is normal
		//TODO is linear
		bool hasMips = metaYaml["mips"].IsDefined() ? metaYaml["mips"].as<bool>() : true;
		auto formatStr = metaYaml["format"].IsDefined() ? metaYaml["format"].as<std::string>() : "BC1";
		auto wrapModeStr = metaYaml["wrapMode"].IsDefined() ? metaYaml["wrapMode"].as<std::string>() : "repeat";
		auto flags = metaYaml["flags"].IsDefined() ? metaYaml["flags"].as<int>() : BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC;
		flags |= WrapModeStrToFlags(wrapModeStr);

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

		std::string libraryFileName = loadLow ? "texture_low.dds" : "texture.dds";

		std::vector<uint8_t> buffer;
		bool bufferLoaded = false;
		if (!needRebuild) {
			bufferLoaded = databaseHandle.ReadFromLibraryFile(libraryFileName, buffer);
			if (bufferLoaded && compressTextures) {
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

		std::string convertedAssetPath = databaseHandle.GetLibraryPathFromId("texture.dds");
		std::string convertedAssetPathLow = databaseHandle.GetLibraryPathFromId("texture_low.dds");

		bool converted = ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPath, convertedAssetPathLow, mips, format, isNormalMap, skippedMipsForLow);

		//TODO
		//TODO just load mip from full size texture
		bool convertedLow = converted;// ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPathLow, mips, format, isNormalMap, 4);

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
		databaseHandle.ReadFromLibraryFile("texture.dds", tempBuffer);

		std::vector<uint8_t> tempBuffer2;
		databaseHandle.ReadFromLibraryFile("texture_low.dds", tempBuffer2);

		buffer = tempBuffer;// loadLow ? tempBuffer2 : tempBuffer;

		binBuffer = BinaryBuffer(std::move(tempBuffer));
		if (compressTextures) {
			Compression::Compress(binBuffer);
		}
		databaseHandle.WriteToLibraryFile("texture.dds", binBuffer.GetData());


		binBuffer = BinaryBuffer(std::move(tempBuffer2));
		if (compressTextures) {
			Compression::Compress(binBuffer);
		}
		databaseHandle.WriteToLibraryFile("texture_low.dds", binBuffer.GetData());


		return buffer;
	}

	bool ConvertTextureFile(AssetDatabase_BinaryImporterHandle& databaseHandle,
		const std::string& inFile,
		const std::string& outFile,
		const std::string& outFileLow,
		bool isMips,
		const std::string& format,
		bool isNormalMap,
		int lowSkippedMips)
	{
		//TODO convert in memory
		//TODO formats suppoert
		//TODO is normal map
		//TODO isMips

		CMP_MipSet srcTexture;

		if (CMP_LoadTexture(inFile.c_str(), &srcTexture) != CMP_ERROR::CMP_OK) {
			return false;
		}

		CMP_MipSet destTexture;

		CMP_CompressOptions options = { 0 };
		options.dwSize = sizeof(options);
		options.fquality = 1.0f;
		options.DestFormat = CMP_ParseFormat((char*)format.c_str());//TODO check error
		options.dwnumThreads = 0;


		if (CMP_GenerateMIPLevels(&srcTexture, CMP_CalcMinMipSize(srcTexture.m_nWidth, srcTexture.m_nHeight, srcTexture.m_nMaxMipLevels)) != CMP_ERROR::CMP_OK) {
			return false;
		}

		CMP_Feedback_Proc p = nullptr;
		if (CMP_ConvertMipTexture(&srcTexture, &destTexture, &options, p) != CMP_ERROR::CMP_OK) {
			return false;
		}
		Log("coverting texture '%s'", inFile.c_str());
		CMP_SaveTexture(outFile.c_str(), &destTexture);
		CMP_FreeMipSet(&destTexture);

		auto srcTexture2 = srcTexture;
		CMP_MipLevel* tempLevels[32];
		lowSkippedMips = Mathf::Min(lowSkippedMips, srcTexture.m_nMipLevels - 1);
		if (lowSkippedMips > 0) {
			for (int i = 0; i < srcTexture.m_nMipLevels - lowSkippedMips; i++) {
				tempLevels[i] = srcTexture.m_pMipLevelTable[i];
				srcTexture.m_pMipLevelTable[i] = srcTexture.m_pMipLevelTable[i + lowSkippedMips];
			}
			srcTexture.m_nWidth = srcTexture.m_pMipLevelTable[0]->m_nWidth;
			srcTexture.m_nHeight = srcTexture.m_pMipLevelTable[0]->m_nHeight;
		}
		srcTexture.m_nMipLevels = srcTexture.m_nMipLevels - lowSkippedMips;

		if (CMP_ConvertMipTexture(&srcTexture, &destTexture, &options, p) != CMP_ERROR::CMP_OK) {
			return false;
		}
		CMP_SaveTexture(outFileLow.c_str(), &destTexture);

		if (lowSkippedMips > 0) {
			srcTexture.m_nMipLevels = srcTexture2.m_nMipLevels;
			srcTexture.m_nHeight = srcTexture2.m_nHeight;
			srcTexture.m_nWidth = srcTexture2.m_nWidth;

			for (int i = 0; i < srcTexture.m_nMipLevels - lowSkippedMips; i++) {
				srcTexture.m_pMipLevelTable[i] = tempLevels[i];
			}
		}

		CMP_FreeMipSet(&destTexture);
		CMP_FreeMipSet(&srcTexture);

		return true;

		//bimg::imageWriteDds()


		bx::FileWriter fr;
		bx::Error err;
		if (!fr.open(bx::FilePath(outFile.c_str()), false, &err)) {
			return false;
		}
		bimg::ImageContainer imageContainer{};
		imageContainer.m_width = destTexture.m_nWidth;
		imageContainer.m_height = destTexture.m_nHeight;
		imageContainer.m_format = bimg::TextureFormat::Enum::BC1;
		imageContainer.m_numMips = destTexture.m_nMipLevels;
		imageContainer.m_offset = 0;
		imageContainer.m_allocator = nullptr;
		imageContainer.m_depth = destTexture.m_nDepth;
		imageContainer.m_numLayers = 1;
		imageContainer.m_hasAlpha = destTexture.m_nChannels == 4;
		imageContainer.m_srgb = true;

		if (!bimg::imageWriteDds(&fr, imageContainer, destTexture.pData, destTexture.dwDataSize, &err)) {
			return false;
		}

		return true;


		std::string params = "";
		params += " -f " + inFile;
		params += " -o " + outFile;
		params += " --mips ";
		params += (isMips ? "true" : "false");
		params += " --mipskip ";
		params += FormatString("%d", lowSkippedMips);
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