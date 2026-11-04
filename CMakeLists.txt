cmake_minimum_required(VERSION 3.16)
project(ParcelSystem LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(parcels
        src/main.cpp
        src/ParcelQueue.cpp
        src/ParcelRegistry.cpp
        src/Status.cpp
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

add_executable(activity_tests
        tests/activity_test.cpp
        src/Activity.cpp
        src/ActivityStack.cpp
        src/ActivityHistory.cpp
        src/Resource.cpp
        src/ResourceQueue.cpp
        src/ResourceManager.cpp
        src/ParcelQueue.cpp
        src/ParcelRegistry.cpp
        src/Status.cpp
)
target_include_directories(activity_tests PRIVATE include)
add_test(NAME activity_tests COMMAND activity_tests)

add_executable(service_tests
        tests/service_test.cpp
        src/ServiceRequest.cpp
        src/RequestPriorityList.cpp
        src/ServiceManager.cpp
        src/Activity.cpp
        src/ActivityStack.cpp
        src/ActivityHistory.cpp
        src/Resource.cpp
        src/ResourceQueue.cpp
        src/ResourceManager.cpp
        src/ParcelQueue.cpp
        src/ParcelRegistry.cpp
        src/Status.cpp
)
target_include_directories(service_tests PRIVATE include)
add_test(NAME service_tests COMMAND service_tests)
