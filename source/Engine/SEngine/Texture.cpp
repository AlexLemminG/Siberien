

#include "Texture.h"
#include "Resources.h"
#include "bimg/bimg.h"
#include "bx/allocator.h"
#include "bgfx/bgfx.h"
#include <windows.h>
#include "Serialization.h"
#include "bimg/decode.h"
#include "Compression.h"
#include "DbgVars.h"
#include "Compressonator.h"
#include <bx\file.h>
#include "SMath.h"
#include "ryml.hpp"
#include "Config.h"

static bx::DefaultAllocator s_bxAllocator = bx::DefaultAllocator();
static constexpr bool compressTextures = false;
static constexpr int skippedMipsForLow = 4;
static constexpr int importerVersion = 12;

class TextureMeta {
public:
	bool hasMips = true;
	std::string format = "BC1";//TODO enums
	std::string wrapMode = "repeat";//TODO enums
	long flags = BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIN_ANISOTROPIC;//TODO unsigned int
	bool isNormalMap = false;
	bool isLoaded = false;

	REFLECT_BEGIN(TextureMeta);
	REFLECT_VAR(hasMips);
	REFLECT_VAR(format);
	REFLECT_VAR(wrapMode);
	REFLECT_VAR(flags);
	REFLECT_VAR(isNormalMap);
	REFLECT_END()
};

class TextureLibraryMeta :public Object {
public:
	int importerVersion = 0;
	uint64_t changeDate = 0;
	uint64_t metaChangeDate = 0;
	REFLECT_BEGIN(TextureLibraryMeta);
	REFLECT_VAR(importerVersion);
	REFLECT_VAR(changeDate);
	REFLECT_VAR(metaChangeDate);
	REFLECT_END()
};

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
		std::unique_ptr<ryml::Tree> metaYamlTree;
		databaseHandle.ReadMeta(metaYamlTree);
		TextureMeta meta{};
		if (metaYamlTree) {
			SerializationContext c = SerializationContext(metaYamlTree->rootref());
			::Deserialize(c, meta);
			meta.isLoaded = true;
		}

		//TODO is normal
		//TODO is linear
		meta.flags |= WrapModeStrToFlags(meta.wrapMode);

		if (meta.isLoaded) {
			auto name = databaseHandle.GetAssetFileName();
			std::transform(name.begin(), name.end(), name.begin(),
				[](unsigned char c) { return std::tolower(c); });

			meta.isNormalMap = name.find("normal") != std::string::npos;
		}

		//TODO return default invalid texture
		auto txBin = LoadTexture(importerVersion, databaseHandle, meta.hasMips, meta.format, meta.isNormalMap, dbg_lowTextures);
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
			(unsigned int)meta.flags,
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

		bool convertionAllowed = databaseHandle.ConvertionAllowed();
		//TODO dont load if version differs!!!

		TextureLibraryMeta expectedLibMeta;
		bool needRebuild = false;
		if (convertionAllowed) {
			needRebuild = true;
			databaseHandle.GetLastModificationTime(expectedLibMeta.changeDate, expectedLibMeta.metaChangeDate);
			expectedLibMeta.importerVersion = importerVersion;

			std::unique_ptr<ryml::Tree> libraryMetaYaml;
			databaseHandle.ReadFromLibraryFile("meta", libraryMetaYaml);
			if (libraryMetaYaml) {
				TextureLibraryMeta meta;
				SerializationContext c{ libraryMetaYaml->rootref() };
				::Deserialize(c, meta);

				if (meta.changeDate == expectedLibMeta.changeDate &&
					meta.metaChangeDate == expectedLibMeta.metaChangeDate &&
					meta.importerVersion == expectedLibMeta.importerVersion) {
					needRebuild = false;
				}
			}
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
		else if (!convertionAllowed) {
			//TODO error
			return buffer;
		}

		databaseHandle.EnsureForderForLibraryFileExists("texture");

		std::string convertedAssetPath = databaseHandle.GetLibraryPathFromId("texture.dds");
		std::string convertedAssetPathLow = databaseHandle.GetLibraryPathFromId("texture_low.dds");

		bool converted = ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPath, convertedAssetPathLow, mips, format, isNormalMap, skippedMipsForLow);

		//TODO
		//TODO just load mip from full size texture
		bool convertedLow = converted;// ConvertTextureFile(databaseHandle, databaseHandle.GetAssetPath(), convertedAssetPathLow, mips, format, isNormalMap, 4);

		if (!converted || !convertedLow) {
			//TODO error
			return buffer;
		}

		SerializationContext c{};
		::Serialize(c, expectedLibMeta);
		databaseHandle.WriteToLibraryFile("meta", c.GetYamlNode());

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
		Log("coverting texture '%s'", inFile.c_str());
		//TODO convert in memory
		//TODO formats suppoert
		//TODO is normal map
		//TODO isMips

		CMP_MipSet srcTexture;

		std::vector<uint8_t> buffer;
		if (!databaseHandle.ReadAssetAsBinary(buffer)) {
			LogError("Failed to convert texture '%s' : failed to read file", databaseHandle.GetAssetPath().c_str());
			return false;
		}
		if (CMP_LoadTextureFromMemory(buffer.data(), buffer.size(), databaseHandle.GetFileExtension().c_str(), &srcTexture) != CMP_ERROR::CMP_OK) {
			LogError("Failed to convert texture '%s' : failed to load texture to compressinator", databaseHandle.GetAssetPath().c_str());
			return false;
		}

		CMP_MipSet destTexture;

		CMP_CompressOptions options = { 0 };
		options.dwSize = sizeof(options);
		options.fquality = 1.0f;
		options.DestFormat = CMP_ParseFormat((char*)format.c_str());//TODO check error
		options.dwnumThreads = 0;


		if (CMP_GenerateMIPLevels(&srcTexture, CMP_CalcMinMipSize(srcTexture.m_nWidth, srcTexture.m_nHeight, srcTexture.m_nMaxMipLevels)) != CMP_ERROR::CMP_OK) {
			LogError("Failed to convert texture '%s' : failed to generate mip levels", databaseHandle.GetAssetPath().c_str());
			return false;
		}

		CMP_Feedback_Proc p = nullptr;
		if (CMP_ConvertMipTexture(&srcTexture, &destTexture, &options, p) != CMP_ERROR::CMP_OK) {
			LogError("Failed to convert texture '%s' : failed to convert mip texture", databaseHandle.GetAssetPath().c_str());
			return false;
		}
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
			LogError("Failed to convert texture '%s' : failed to convert mip texture to low mips", databaseHandle.GetAssetPath().c_str());
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

		Log("texture converted '%s'", inFile.c_str());
		return true;
	}

};
DECLARE_BINARY_ASSET(Texture, TextureImporter);


Texture::~Texture() {
	// TODO way to manually destroy textures or something like that
	// so we can be sure all texture handles are destroyed before terminating bgfx itself
	if (bgfx::isValid(handle)) {
		bgfx::destroy(handle);
	}
}