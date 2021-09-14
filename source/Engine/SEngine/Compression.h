#pragma once

#include "Common.h"

namespace Compression {
	bool Compress(const BinaryBuffer& from, BinaryBuffer& to);
	bool Decompress(const BinaryBuffer& from, BinaryBuffer& to);

	bool Compress(BinaryBuffer& fromTo);
	bool Decompress(BinaryBuffer& fromTo);
}