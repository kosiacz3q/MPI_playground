cmake_minimum_required(VERSION 3.5)
project(Contest)

SET(CMAKE_C_COMPILER mpicc)
SET(CMAKE_CXX_COMPILER mpicxx)

find_package(MPI REQUIRED)
include_directories(
        MPI_INCLUDE_PATH
        src
        core)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 ${MPI_COMPILE_FLAGS}")
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")

set(SOURCE_FILES
        src/Messages.cpp
        src/BeautyAgent.cpp
        src/MessageBroker.cpp
        src/LamportVectorClock.cpp
        src/Logger.cpp
        )

add_executable(Contest ${SOURCE_FILES} src/main.cpp)

add_executable(Tests ${SOURCE_FILES} tests/main_tests.cpp tests/tests.cpp)


