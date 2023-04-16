#include "RdaReader.hpp"
#include "SharedIndex.hpp"
#include "BucketedZstdData.hpp"

#include <mutex>
#include <execution>
#include <algorithm>
#include <atomic>
#include <optional>
#include <thread>

void RdaReader::log(const std::string &string) {
	if(logger) logger(string);
}

RdaReader::RdaReader() : logger(nullptr) {}

RdaReader::RdaReader(std::function<void(const std::string &)> logger) : logger(logger) {}

size_t RdaReader::readRda(std::istream &rda, uint64_t id, const std::function<void(const char *s, size_t n)> output, std::mutex &outputMutex) {
	log("Reading an rda\n");
	BucketedZstdData bucket(rda);

	if(std::optional<std::vector<std::vector<char>>> data = bucket.getEntriesByID(id)) {
		const std::lock_guard lock(outputMutex);
		log("Writing " + std::to_string(data.value().size()) + " entries to output\n");
		for(const auto &entry : data.value()) {
			output(entry.data(), entry.size());
			output("\n", 1);
		}
		log("Done writing entries\n");
		return data.value().size();
	}
	log("No entries found\n");
	return 0;
}

size_t RdaReader::readRda(std::istream &rda, uint64_t id, const std::function<void(const char *s, size_t n)> output) {
	std::mutex dummyMutex;
	return readRda(rda, id, output, dummyMutex);
}

std::optional<std::istream *> getVictimIfAvailable(std::vector<std::istream *> &victims, std::mutex &victimMutex) {
	std::lock_guard lock(victimMutex);
	if(victims.size() > 0) {
		std::istream *tmp = victims.back();
		victims.pop_back();
		return tmp;
	}
	return {};
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::vector<char> &sharedIndex, const std::vector<std::istream *> &rdas, const std::function<void(const char *s, size_t n)> output, std::mutex &outputMutex) {
	log("Reading shared index... ");
	SharedIndex sharedIndexReader(sharedIndex);

	std::vector<std::istream *> victims = rdas;
	std::mutex victimMutex;

	if(std::optional<std::uint64_t> id = sharedIndexReader.getID(datasetName)) {
		log("Found ID: " + std::to_string(id.value()) + '\n');
		std::atomic_size_t totalEntries(0);
		std::vector<std::thread> threads(std::thread::hardware_concurrency());
		for(int i = 0; i < std::thread::hardware_concurrency(); ++i) {
			threads[i] = std::thread(
				[&victims, &victimMutex, this, &totalEntries, &id, &output, &outputMutex](){
					for(std::optional<std::istream *> victim; victim = getVictimIfAvailable(victims, victimMutex); ) {
						totalEntries += readRda(*victim.value(), id.value(), output, outputMutex);
					}
				}
			);
		}
		for(std::thread &thread : threads) {
			thread.join();
		}

		return totalEntries;
	}
	log("No entries found\n");
	return 0;
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::vector<char> &sharedIndex, const std::vector<std::istream *> &rdas, const std::function<void(const char *s, size_t n)> output) {
	std::mutex dummyMutex;
	return readDataset(datasetName, sharedIndex, rdas, output, dummyMutex);
}

inline std::optional<std::vector<char>> readSharedIndex(const std::filesystem::path &filePath) {
	std::ifstream sharedIndexFile(filePath, std::ios::binary | std::ios::ate);
	if(!sharedIndexFile.good()) {
		return {};
	}
	std::vector<char> sharedIndexData(sharedIndexFile.tellg());
	sharedIndexFile.seekg(0, std::ios::beg);

	sharedIndexFile.read(sharedIndexData.data(), sharedIndexData.size());
	return sharedIndexData;
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::filesystem::path &rootDir, const std::function<void(const char *s, size_t n)> output, std::mutex &outputMutex) {
	const std::filesystem::path sharedIndexPath(rootDir / "sharedindex.shi");
	std::vector<char> sharedIndexData;
	if(auto data = readSharedIndex(sharedIndexPath)) {
		sharedIndexData = data.value();
	} else {
		log("cannot find '" + sharedIndexPath.string() + "'\n");
		return 0;
	}

	std::vector<std::ifstream> rdasIfstreams;
	for(const std::filesystem::directory_entry &file : std::filesystem::directory_iterator(rootDir)) {
		if(file.path().extension() == ".rda") rdasIfstreams.emplace_back(std::ifstream(file.path(), std::ios::binary));
	}

	std::vector<std::istream *> rdaRefs;
	for(std::ifstream &stream : rdasIfstreams) {
		rdaRefs.push_back(&stream);
	}

	if(size_t totalRead = readDataset(datasetName, sharedIndexData, rdaRefs, [](const char *s, size_t n){std::cout.write(s, n);}, outputMutex)) {
		log("Found a total of " + std::to_string(totalRead) + " entries\n");
		return totalRead;
	} else {
		log("Cannot find '" + std::string(datasetName.data()) + "' in the shared index\n");
		return 0;
	}
}

size_t RdaReader::readDataset(const std::vector<char> &datasetName, const std::filesystem::path &rootDir, const std::function<void(const char *s, size_t n)> output) {
	std::mutex dummyMutex;
	return readDataset(datasetName, rootDir, output, dummyMutex);
}