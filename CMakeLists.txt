project(spices2)

set(CMAKE_CXX_STANDARD 20)

add_executable(spices src/spices.cpp src/SharedIndex.cpp src/BucketedZstdData.cpp)
target_link_libraries(spices zstd)