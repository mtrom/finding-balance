cmake_minimum_required(VERSION 3.25.0)

project(ddh_unbalanced_psi)

# equivalent to -std=c++17
set(CMAKE_CXX_STANDARD 17)

# places executable in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# import cryptoTools library
find_package(coproto)
find_package(cryptoTools)

add_executable(main src/main.cc src/client.cc src/server.cc src/utils.cc src/hashtable.cc)
target_link_libraries(main oc::cryptoTools)

add_executable(tests src/tests/test_all.cc src/tests/test_utils.cc src/utils.cc)
target_link_libraries(tests oc::cryptoTools)
