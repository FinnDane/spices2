#ifndef RDAREADER_HPP
#define RDAREADER_HPP

#include <optional>
#include <iostream>
#include <vector>
#include <mutex>
#include <functional>

class RdaReader {
	public:
	RdaReader();
	RdaReader(std::function<void(const std::string &)> logger);
	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::vector<char> &sharedIndex,
		const std::vector<std::istream *> &rdas,
		std::ostream &output
	);
	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::vector<char> &sharedIndex,
		const std::vector<std::istream *> &rdas,
		std::ostream &output,
		std::mutex &outputMutex
	);

	size_t readRda(
		std::istream &rda,
		uint64_t id,
		std::ostream &output
	);
	size_t readRda(
		std::istream &rda,
		uint64_t id,
		std::ostream &output,
		std::mutex &outputMutex
	);
	private:
	const std::function<void(const std::string &)> logger;
	void log(const std::string &string);
};

#endif