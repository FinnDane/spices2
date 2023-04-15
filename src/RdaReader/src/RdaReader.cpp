#include "RdaReader.hpp"
#include "SharedIndex.hpp"
#include "BucketedZstdData.hpp"

#include <mutex>
#include <execution>
#include <algorithm>
#include <atomic>

void RdaReader::log(const std::string &string) {
	if(logger) logger(string);
}

RdaReader::RdaReader() : logger(nullptr) {}

RdaReader::RdaReader(std::function<void(const std::string &)> logger) : logger(logger) {}

size_t RdaReader::readRda(std::istream &rda, uint64_t id, std::ostream &output, std::mutex &outputMutex) {
	log("Reading an rda\n");
	BucketedZstdData bucket(rda);

	if(std::optional<std::vector<std::vector<char>>> data = bucket.getEntriesByID(id)) {
		const std::lock_guard lock(outputMutex);
		log("Writing " + std::to_string(data.value().size()) + " entries to output\n");
		for(const auto &entry : data.value()) {
			output.write(entry.data(), entry.size()) << '\n';
		}
		log("Done writing entries\n");
		return data.value().size();
	}
	log("No entries found\n");
	return 0;
}

size_t RdaReader::readRda(std::istream &rda, uint64_t id, std::ostream &output) {
	std::mutex dummyMutex;
	return readRda(rda, id, output, dummyMutex);
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::vector<char> &sharedIndex, const std::vector<std::istream *> &rdas, std::ostream &output, std::mutex &outputMutex) {
	log("Reading shared index... ");
	SharedIndex sharedIndexReader(sharedIndex);

	if(std::optional<std::uint64_t> id = sharedIndexReader.getID(datasetName)) {
		log("Found ID: " + std::to_string(id.value()) + '\n');
		std::atomic_size_t totalEntries(0);
		std::for_each(
			std::execution::par,
			rdas.begin(),
			rdas.end(),
			[this, &totalEntries, &id, &output, &outputMutex](std::istream * const &rda) {totalEntries += readRda(*rda, id.value(), output, outputMutex);}
		);
		return totalEntries;
	}
	log("No entries found\n");
	return 0;
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::vector<char> &sharedIndex, const std::vector<std::istream *> &rdas, std::ostream &output) {
	std::mutex dummyMutex;
	return readDataset(datasetName, sharedIndex, rdas, output, dummyMutex);
}