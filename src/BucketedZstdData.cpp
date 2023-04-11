#include <iostream>

#include "BucketedZstdData.hpp"

#include <zstd.h>
#include <cstring>

constexpr int headerSize = sizeof(uint32_t);
constexpr int indexEntrySize = sizeof(uint64_t)*2;

BucketedZstdData::BucketedZstdData(std::ifstream &file) : file(file) {}

std::optional<std::vector<char>> BucketedZstdData::getDatasetWithId(std::uint32_t id) {
	file.seekg(0, std::ios::beg);

	uint32_t indexSize;
	file.read((char *)&indexSize, sizeof(indexSize));
	if(indexSize < id) return {};

	//seek to index entry
	file.seekg(sizeof(uint32_t) + indexEntrySize*id, std::ios::beg);

	uint64_t offset, length;
	file.read((char *)&offset, sizeof(offset));
	file.read((char *)&length, sizeof(length));
	if(length == 0) return {};

	file.seekg(offset + headerSize + indexSize * indexEntrySize, std::ios::beg);
	std::vector<char> inBuf(length);
	file.read(inBuf.data(), inBuf.size());

	std::vector<char> output(ZSTD_getFrameContentSize(inBuf.data(), inBuf.size()));
	if(!ZSTD_isError(ZSTD_decompress(output.data(), output.size(), inBuf.data(), inBuf.size()))) {
		return output;
	}

	return {};
}

std::optional<std::vector<std::vector<char>>> BucketedZstdData::getEntriesByID(std::uint32_t id) {
	std::optional<std::vector<char>> rawData = getDatasetWithId(id);
	if(!rawData.has_value()) return {};

	const char *fileIndex = rawData.value().data();

	std::vector<std::vector<char>> output;
	uint32_t readId;
	while(fileIndex < rawData.value().data() + rawData.value().size()) {
		const uint32_t *readId = (uint32_t *)fileIndex;
		fileIndex += sizeof(uint32_t);

		if(*readId == id) {
			std::vector<char> &object = output.emplace_back(std::vector<char>(*(uint32_t *)fileIndex));

			memcpy(object.data(), fileIndex + sizeof(uint32_t), object.size());
		}
		fileIndex += *(uint32_t *)fileIndex + sizeof(uint32_t);
	}
	return output;
}