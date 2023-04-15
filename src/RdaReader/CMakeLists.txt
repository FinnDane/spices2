project(RdaReader)

set(CMAKE_CXX_STANDARD 20)

add_library(RdaReader src/RdaReader src/SharedIndex.cpp src/BucketedZstdData.cpp)
target_include_directories(RdaReader PUBLIC include)
target_link_libraries(RdaReader zstd)