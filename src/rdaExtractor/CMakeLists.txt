project(rdaExtractor)

set(CMAKE_CXX_STANDARD 20)

add_executable(RdaReaderExec src/rdaExtractor.cpp)
target_link_libraries(RdaReaderExec RdaReader)