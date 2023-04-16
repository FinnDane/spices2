#include <RdaReader.hpp>

#include <filesystem>
#include <iostream>
#include <fstream>

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

void threadedLog(const std::string &input) {
	std::lock_guard lock(cerrMutex);
	std::cerr << input << std::flush;
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

int main(int argc, char **argv) {
	if(argc != 3 && argc != 4) {
		std::cerr << "usage: datesetname rootdirectory [force write to terminal(true | false)]" << std::endl;
		return 1;
	}

	if(!IS_REDIRECTED && (argc != 4 || std::string(argv[3]) != "true")) {
		std::cerr << "output is not redirected, specify you want to write to the terminal" << std::endl;
		return 1;
	}

	std::string datesetString = argv[1];
	std::vector<char> datasetName(datesetString.begin(), datesetString.end());

	const std::filesystem::path rootDirectory(argv[2]);
	RdaReader rdaReader(threadedLog);
	rdaReader.readDataset(datasetName, rootDirectory, [](const char *s, size_t n){std::cout.write(s, n);});
}