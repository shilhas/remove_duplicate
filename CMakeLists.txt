
cmake_minimum_required(VERSION 3.8)

SET (CMAKE_CXX_STANDARD 17)

project(Remove_Duplicate)

OPTION(EN_DEBUG "Enable Debugging" OFF)

IF(EN_DEBUG)
    ADD_DEFINITIONS(-DDEBUG)
ENDIF(EN_DEBUG)

find_package(OpenSSL REQUIRED)

add_subdirectory(argparse)

add_executable(${PROJECT_NAME} 
                src/remove_dup.cc 
                src/main.cc)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/hdr)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL argparse)