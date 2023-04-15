#include <iostream>
#include <fstream>
#include <filesystem>
#include <execution>
#include <algorithm>
#include <mutex>
#include <atomic>

#include "SharedIndex.hpp"
#include "BucketedZstdData.hpp"

#ifdef _WIN32
#include <io.h>
#define IS_REDIRECTED !(_isatty(_fileno(stdout)))

#elif __unix__
#include <unistd.h>
#define IS_REDIRECTED !(isatty(fileno(stdout)))

#else
#warning "Redirection cannot be checked, will always asume to be redirected"
#define IS_REDIRECTED true

#endif

std::mutex cerrMutex;

#define CERRLOG(...) \
	{ \
		std::lock_guard lock(cerrMutex); \
		fprintf(stderr, __VA_ARGS__); \
	}


std::optional<std::vector<char>> readSharedIndex(const std::filesystem::path &filePath) {
	std::ifstream sharedIndexFile(filePath, std::ios::binary | std::ios::ate);
	if(!sharedIndexFile.good()) {
		return {};
	}
	std::vector<char> sharedIndexData(sharedIndexFile.tellg());
	sharedIndexFile.seekg(0, std::ios::beg);

	sharedIndexFile.read(sharedIndexData.data(), sharedIndexData.size());
	return sharedIndexData;
}

void processRDA(const std::filesystem::directory_entry &file, std::atomic_size_t &totalEntries, uint64_t id, std::mutex &outputMutex) {
	if(file.path().extension() == ".rda") {
		std::ifstream fileStream(file.path(), std::ios::binary);
		CERRLOG("Reading %s\n", std::string(file.path().filename()).c_str());
		BucketedZstdData bucket(fileStream);

		if(std::optional<std::vector<std::vector<char>>> data = bucket.getEntriesByID(id)) {
			totalEntries += data.value().size();
			const std::lock_guard lock(outputMutex);
			CERRLOG("Writing %s\n", std::string(file.path().filename()).c_str());
			for(const auto &entry : data.value()) {
				std::cout.write(entry.data(), entry.size()) << '\n';
			}
		}
	}
}

int main(int argc, char **argv) {
	if(argc != 3 && argc != 4) {
		std::cerr << "usage: subreddit rootdirectory [force write to terminal(true | false)]" << std::endl;
		return 1;
	}

	if(!IS_REDIRECTED && (argc != 4 || std::string(argv[3]) != "true")) {
		std::cerr << "output is not redirected, specify you want to write to the terminal" << std::endl;
		return 1;
	}

	const std::filesystem::path rootDirectory(argv[2]);
	const std::filesystem::path sharedIndexPath(rootDirectory / "sharedindex.shi");

	std::cerr << "Loading shared index..." << std::flush;
	std::vector<char> sharedIndexData;
	if(auto data = readSharedIndex(sharedIndexPath)) {
		sharedIndexData = data.value();
	} else {
		std::cerr << "cannot find '" << sharedIndexPath << "'" << std::endl;
		return 2;
	}
	std::cerr << "Loaded shared index" << std::endl;

	std::string datesetString = argv[1];
	std::vector<char> datasetName(datesetString.begin(), datesetString.end());

	SharedIndex sharedIndex(sharedIndexData);

	std::cerr << "Fetching ID from shared index... " << std::flush;
	std::atomic_size_t totalEntries(0);
	if(std::optional<std::uint64_t> id = sharedIndex.getID(datasetName)) {
		std::cerr << "Found ID: " << id.value() << std::endl;

		std::mutex outputMutex;
		std::for_each(
			std::execution::par,
			std::filesystem::begin(std::filesystem::directory_iterator(rootDirectory)),
			std::filesystem::end(std::filesystem::directory_iterator()),
			[&totalEntries, &id, &outputMutex](const auto& file) {processRDA(file, totalEntries, id.value(), outputMutex);}
		);

		std::cerr << "Found a total of " << totalEntries << " entries" << std::endl;
	} else {
		std::cerr << "Cannot find '" << argv[1] << "' in the shared index" << std::endl;
		return 2;
	}
}