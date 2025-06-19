
cmake_minimum_required(VERSION 3.8)

SET (CMAKE_CXX_STANDARD 17)

project(FindDup)

find_package(OpenSSL REQUIRED)

add_executable(${PROJECT_NAME} src/remove_dup.cc)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/hdr)

target_link_libraries(${PROJECT_NAME} OpenSSL::SSL)