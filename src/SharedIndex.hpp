#ifndef SHAREDINDEX_HPP
#define SHAREDINDEX_HPP

#include <cstdint>
#include <vector>
#include <optional>

class SharedIndex {
	public:
	SharedIndex(const std::vector<char> &data);
	std::optional<std::uint64_t> getID(const std::vector<char> &datasetName);
	private:
	const std::vector<char> &data;
};

#endif