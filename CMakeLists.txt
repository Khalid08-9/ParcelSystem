cmake_minimum_required(VERSION 3.16)
project(ParcelSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parcels
        src/main.cpp
        src/ParcelQueue.cpp
        src/ParcelRegistry.cpp
        src/Status.cpp
)
target_include_directories(parcels PRIVATE include)

enable_testing()

add_executable(tree_tests
        tests/tree_test.cpp
        src/tree.cpp
)
target_include_directories(tree_tests PRIVATE include)
add_test(NAME tree_tests COMMAND tree_tests)
