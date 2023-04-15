#include "SharedIndex.hpp"

#include <cstring>
#include <iostream>

SharedIndex::SharedIndex(const std::vector<char> &data) : data(data) {}

std::optional<std::uint64_t> SharedIndex::getID(const std::vector<char> &datasetName) {
	const char *fileIndex = data.data();
	fileIndex += sizeof(uint32_t);

	for(uint64_t entryN = 0;; ++entryN ) {
		if(fileIndex >= data.data() + data.size()) return {};

		if(*fileIndex == datasetName.size()) {
			if(!std::memcmp(fileIndex + 1, datasetName.data(), datasetName.size())) return entryN;
		}

		fileIndex += *fileIndex + 1;
	}
}