#include "Compression.h"
#include "zlib.h"
#include "lz4.h"

//resize vector without initialization
//TODO to common

//TODO different algo for big files
bool Compression::Compress(const BinaryBuffer& from, BinaryBuffer& to) {
    const auto& data = from.GetData();
    std::vector<uint8_t> compressBuff;
    
    const int max_dst_size = LZ4_compressBound(data.size());
    ResizeVectorNoInit(compressBuff, max_dst_size + sizeof(uint64_t));
    const int compressed_data_size = LZ4_compress_default((const char*)data.data(), (char*)compressBuff.data(), data.size(), max_dst_size);
    if (compressed_data_size <= 0) {
        //TODO
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
    if (final_decompressed_size < 0)
    {
        //TODO
        //fprintf(stderr, "uncompress(...) failed, res = %d\n", res);
        //exit(1);
        ASSERT(false);
        return false;
    }
    ASSERT(final_decompressed_size == decompressed_size);
    to = BinaryBuffer(std::move(uncopressBuff));
    return true;
}
