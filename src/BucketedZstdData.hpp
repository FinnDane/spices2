#ifndef BUCKETEDZSTDDATA_H
#define BUCKETEDZSTDDATA_H

#include <cstdint>
#include <vector>
#include <optional>
#include <fstream>

class BucketedZstdData {
	public:
	BucketedZstdData(std::ifstream &file);
	std::optional<std::vector<char>> getDatasetWithId(std::uint32_t id);
	std::optional<std::vector<std::vector<char>>> getEntriesByID(std::uint32_t id);
	private:
	std::ifstream &file;
};

#endif