#include "Buffer.hpp"
#include <cstring>

Buffer::Buffer()
	: size(0) {}

Buffer::Buffer(const Buffer& src)
	: size(src.size) {
		std::memcpy(buf, src.buf, size);
	}

Buffer::~Buffer() {}

Buffer& Buffer::operator=(const Buffer &src) {
	size = src.size;
	std::memcpy(buf, src.buf, size);
	return *this;
}

const char* Buffer::getBuf(void) const {
	return buf;
}

size_t Buffer::getSize(void) const {
	return size;
}

size_t Buffer::getSafeSize(const size_t demandedSize) const {
	if (demandedSize < capacity)
		return demandedSize;
	return capacity;
}

void Buffer::setSize(const size_t newSize) {
	size = newSize;
}

char* Buffer::begin(void) {
	return buf;
}

const char* Buffer::begin(void) const {
	return buf;
}

char* Buffer::end(void) {
	return buf + size;
}

const char* Buffer::end(void) const {
	return buf + size;
}