#ifndef RDAREADER_HPP
#define RDAREADER_HPP

#include <optional>
#include <iostream>
#include <vector>
#include <mutex>
#include <functional>
#include <filesystem>

class RdaReader {
	public:
	RdaReader();
	RdaReader(std::function<void(const std::string &)> logger);

	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::filesystem::path &rootDir,
		const std::function<void(const char *s, size_t n)> output
	);
	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::filesystem::path &rootDir,
		const std::function<void(const char *s, size_t n)> output,
		std::mutex &outputMutex
	);

	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::vector<char> &sharedIndex,
		const std::vector<std::istream *> &rdas,
		const std::function<void(const char *s, size_t n)> output
	);
	size_t readDataset(
		const std::vector<char> &datasetName,
		const std::vector<char> &sharedIndex,
		const std::vector<std::istream *> &rdas,
		const std::function<void(const char *s, size_t n)> output,
		std::mutex &outputMutex
	);

	size_t readRda(
		std::istream &rda,
		uint64_t id,
		const std::function<void(const char *s, size_t n)> output
	);
	size_t readRda(
		std::istream &rda,
		uint64_t id,
		const std::function<void(const char *s, size_t n)> output,
		std::mutex &outputMutex
	);
	private:
	const std::function<void(const std::string &)> logger;
	void log(const std::string &string);
};

#endif