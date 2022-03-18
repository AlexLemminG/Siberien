#include "Compression.h"
#include "zlib.h"
#include "lz4.h"
#include "System.h"
#include "Cmd.h"
#include <fstream>

//TODO different algo for big files
bool Compression::Compress(const BinaryBuffer& from, BinaryBuffer& to) {
	const auto& data = from.GetData();
	std::vector<uint8_t> compressBuff;

	const int max_dst_size = LZ4_compressBound(data.size());
	ResizeVectorNoInit(compressBuff, max_dst_size + sizeof(uint64_t));
	const int compressed_data_size = LZ4_compress_default((const char*)data.data(), (char*)compressBuff.data(), data.size(), max_dst_size);
	if (compressed_data_size <= 0) {
		LogError("Failed to compress data");
		return false;
	}

	ResizeVectorNoInit(compressBuff, compressed_data_size + sizeof(uint64_t));
	uint64_t uncompressedSize = data.size();
	memcpy(compressBuff.data() + compressed_data_size, &uncompressedSize, sizeof(uint64_t));
	compressBuff.shrink_to_fit();
	to = BinaryBuffer(std::move(compressBuff));

	return true;
}

bool Compression::Decompress(const BinaryBuffer& from, BinaryBuffer& to) {
	const auto& data = from.GetData();
	std::vector<uint8_t> uncopressBuff;
	uint64_t size;
	memcpy(&size, data.data() + data.size() - sizeof(uint64_t), sizeof(uint64_t));
	uLongf decompressed_size = (uLongf)size;
	ResizeVectorNoInit(uncopressBuff, decompressed_size);

	int final_decompressed_size = LZ4_decompress_safe((const char*)data.data(), (char*)uncopressBuff.data(), data.size() - sizeof(uint64_t), decompressed_size);
	if (final_decompressed_size < 0) {
		LogError("Failed to decompress data");
		return false;
	}
	ASSERT(final_decompressed_size == decompressed_size);
	to = BinaryBuffer(std::move(uncopressBuff));
	return true;
}

bool Compression::Compress(BinaryBuffer& fromTo) {
	BinaryBuffer to;
	if (!Compress(fromTo, to)) {
		return false;
	}
	fromTo = BinaryBuffer(to.ReleaseData());
	return true;
}

bool Compression::Decompress(BinaryBuffer& fromTo) {
	BinaryBuffer to;
	if (!Decompress(fromTo, to)) {
		return false;
	}
	fromTo = BinaryBuffer(to.ReleaseData());
	return true;
}


class CompressionSystem : public System<CompressionSystem> {
	class Handler : public Cmd::Handler {
		// Inherited via Handler
		virtual void Handle(std::string command, const std::vector<std::string>& args) override
		{
			bool decompress = command == "decompress";

			if (args.size() < 2) {
				LogError("'%s' command expected at least 2 arguments (got %d)", command.c_str(), (int)args.size());
				return;
			}

			std::ifstream in(args[0], std::ios::binary | std::ios::ate);
			std::ofstream out(args[1], std::ios::binary);

			if (!in) {
				LogError("'%s' command input file '%s' not found", command.c_str(), args[0].c_str());
				return;
			}
			if (!out) {
				LogError("'%s' command failed to open output file '%s'", command.c_str(), args[1].c_str());
				return;
			}
			std::vector<uint8_t> charBuffer;

			std::streamsize size = in.tellg();
			in.seekg(0, std::ios::beg);

			ResizeVectorNoInit(charBuffer, size);
			if (!in.read((char*)charBuffer.data(), size)) {
				LogError("'%s' command error while reading input file '%s'", command.c_str(), args[0].c_str());
				return;
			}

			BinaryBuffer buffer{ std::move(charBuffer) };
			bool ok = false;
			if (decompress) {
				ok = Compression::Decompress(buffer);
			}
			else {
				ok = Compression::Compress(buffer);
			}
			if (!ok) {
				LogError("'%s' command failed to %s buffer", command.c_str(), command.c_str());
				return;
			}
			charBuffer = buffer.ReleaseData();
			if (charBuffer.size() > 0) {
				out.write((char*)&charBuffer[0], charBuffer.size());
			}

			Log("'%s' success", command.c_str());
		}
	};

	bool Init() {
		auto handler = std::make_shared<Handler>();
		Cmd::Get()->AddHandler("decompress", handler);
		Cmd::Get()->AddHandler("compress", handler);
		return true;
	}
};

REGISTER_SYSTEM(CompressionSystem);