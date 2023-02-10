cmake_minimum_required(VERSION 3.25.0)

project(ddh_unbalanced_psi)

# equivalent to -std=c++17
set(CMAKE_CXX_STANDARD 17)

# places executable in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# import cryptoTools library
find_package(cryptoTools)

add_executable(main src/main.cc src/server.cc src/utils.cc)
target_link_libraries(main oc::cryptoTools)
