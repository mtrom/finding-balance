cmake_minimum_required(VERSION 3.18.0)

project(ddh_unbalanced_psi)

# equivalent to -std=c++17
set(CMAKE_CXX_STANDARD 17)

# places executable in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# import cryptoTools library
find_package(coproto)
find_package(cryptoTools)

# generate random data for inputs
add_executable(datagen
    ${PROJECT_SOURCE_DIR}/src/datagen.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(datagen oc::cryptoTools)

# run the offline portions of the protocol
add_executable(offline
    ${PROJECT_SOURCE_DIR}/src/offline.cc
    ${PROJECT_SOURCE_DIR}/src/client.cc
    ${PROJECT_SOURCE_DIR}/src/hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/server.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(offline oc::cryptoTools)

# run the online portions of the protocol
add_executable(online
    ${PROJECT_SOURCE_DIR}/src/online.cc
    ${PROJECT_SOURCE_DIR}/src/client.cc
    ${PROJECT_SOURCE_DIR}/src/hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/server.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(online oc::cryptoTools)

# test c++ library
add_executable(tests
    ${PROJECT_SOURCE_DIR}/src/tests/test_all.cc
    ${PROJECT_SOURCE_DIR}/src/tests/test_hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/tests/test_utils.cc
    ${PROJECT_SOURCE_DIR}/src/hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(tests oc::cryptoTools)
