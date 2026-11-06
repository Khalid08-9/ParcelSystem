cmake_minimum_required(VERSION 3.16)
project(ParcelSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parcels
        src/main.cpp
        src/ParcelQueue.cpp
        src/ParcelRegistry.cpp
        src/Status.cpp
        src/tree.cpp
        src/Resource.cpp
        src/ResourceQueue.cpp
        src/ResourceManager.cpp
        src/Activity.cpp
        src/ActivityStack.cpp
        src/ActivityHistory.cpp
        src/ServiceRequest.cpp
        src/RequestPriorityList.cpp
        src/ServiceManager.cpp
)
target_include_directories(parcels PRIVATE include)

enable_testing()

add_executable(tree_tests
        tests/tree_test.cpp
        src/tree.cpp
)
target_include_directories(tree_tests PRIVATE include)
add_test(NAME tree_tests COMMAND tree_tests)

add_executable(status_tests tests/StatusTests.cpp src/Status.cpp)
target_include_directories(status_tests PRIVATE include)
add_test(NAME status_tests COMMAND status_tests)

add_executable(registry_tests
        tests/RegistryTests.cpp
        src/ParcelRegistry.cpp
        src/ParcelQueue.cpp
        src/Status.cpp
)
target_include_directories(registry_tests PRIVATE include)
add_test(NAME registry_tests COMMAND registry_tests)

add_executable(task1_integration_tests
        tests/Task1IntegrationTests.cpp
        src/ParcelRegistry.cpp
        src/ParcelQueue.cpp
        src/Status.cpp
        src/tree.cpp
)
target_include_directories(task1_integration_tests PRIVATE include)
add_test(NAME task1_integration_tests COMMAND task1_integration_tests)
