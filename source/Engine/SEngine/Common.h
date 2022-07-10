#pragma once

#include "Defines.h"

#include <cassert>
#include <iostream>
#include "optick.h"
#include "Asserts.h"
#include <vector>
#include <string>
#include "StringUtils.h"

//TODO rename file

#define LOG_ERROR(msg) \
std::cout << msg << std::endl;

#define SAFE_DELETE(var) \
if(var) \
{delete var; var = nullptr;}

template<typename ... Args>
void LogCritical(const std::string & format, Args ... args) {
	std::cerr << "Critical: " << FormatString(format, args...) << std::endl;
}

template<typename ... Args>
void LogError(const std::string& format, Args ... args) {
	std::cerr << "Error: " << FormatString(format, args...) << std::endl;
}

template<typename ... Args>
void LogWarning(const std::string& format, Args ... args) {
	std::cout << "Warning: " << FormatString(format, args...) << std::endl;
}

template<typename ... Args>
void Log(const std::string& format, Args ... args) {
	std::cout << FormatString(format, args...) << std::endl;
}


class BinaryBuffer {
public:
	BinaryBuffer() {}
	BinaryBuffer(std::vector<uint8_t>&& readBuffer) : buffer(std::move(readBuffer)) {}

	std::vector<uint8_t>&& ReleaseData() {
		return std::move(buffer);
	}
	const std::vector<uint8_t>& GetData() const {
		return buffer;
	}

	template<typename T>
	void Read(T& val) {
		memcpy(&val, &buffer[currentReadOffset], sizeof(T));
		currentReadOffset += sizeof(T);
	}
	template<>
	void Read<std::string>(std::string& val) {
		int length;
		Read(length);
		if (length) {
			val = std::string((char*)&buffer[currentReadOffset], length);
		}
		else {
			val = "";
		}
		currentReadOffset += length;
	}
	template<typename T>
	void Write(const T& val) {
		auto offset = buffer.size();
		buffer.resize(buffer.size() + sizeof(T));
		memcpy(&buffer[offset], &val, sizeof(T));
	}

	template<>
	void Write<std::string>(const std::string& val) {
		Write((int)val.size());
		if (val.size()) {
			Write((uint8_t*)&val[0], val.size());
		}
	}

	void Write(const uint8_t* buf, int size) {
		auto offset = buffer.size();
		buffer.resize(buffer.size() + size);
		memcpy(&buffer[offset], buf, size);
	}
	void Read(uint8_t* buf, int size) {
		memcpy(buf, &buffer[currentReadOffset], size);
		currentReadOffset += size;
	}
private:
	int currentReadOffset = 0;
	std::vector<uint8_t> buffer;
};

template<typename V>
void ResizeVectorNoInit(V& v, size_t newSize)
{
#if SE_DEBUG
	//fater in debug
	v.resize(newSize);
#else
	struct vt { typename V::value_type v; vt() {} };
	static_assert(sizeof(vt[10]) == sizeof(typename V::value_type[10]), "alignment error");
	typedef std::vector<vt, typename std::allocator_traits<typename V::allocator_type>::template rebind_alloc<vt>> V2;
	reinterpret_cast<V2&>(v).resize(newSize);
#endif
}