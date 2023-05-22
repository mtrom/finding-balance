cmake_minimum_required(VERSION 3.18.0)

project(ddh_unbalanced_psi)

# equivalent to -std=c++17
set(CMAKE_CXX_STANDARD 17)

# places executable in bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

if(APPLE)
    set(OSNAME "osx")
else()
    set(OSNAME "linux")
endif()

set(
    CMAKE_PREFIX_PATH
    ${PROJECT_SOURCE_DIR}/vcpkg/packages/
    ${PROJECT_SOURCE_DIR}/vcpkg/packages/ms-gsl_x64-${OSNAME}/share/
)

# import cryptoTools library
find_package(coproto)
find_package(cryptoTools)
find_package(APSI CONFIG REQUIRED)

# generate random data for inputs
add_executable(datagen
    ${PROJECT_SOURCE_DIR}/src/datagen.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(datagen oc::cryptoTools)
target_link_libraries(datagen APSI::apsi)

# run the offline portions of the protocol
add_executable(oprf
    ${PROJECT_SOURCE_DIR}/src/oprf.cc
    ${PROJECT_SOURCE_DIR}/src/client.cc
    ${PROJECT_SOURCE_DIR}/src/server.cc
    ${PROJECT_SOURCE_DIR}/src/cuckoo.cc
    ${PROJECT_SOURCE_DIR}/src/hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(oprf oc::cryptoTools)
target_link_libraries(oprf APSI::apsi)

# test c++ library
add_executable(tests
    ${PROJECT_SOURCE_DIR}/src/tests/test_all.cc
    ${PROJECT_SOURCE_DIR}/src/tests/test_cuckoo.cc
    ${PROJECT_SOURCE_DIR}/src/tests/test_hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/tests/test_utils.cc
    ${PROJECT_SOURCE_DIR}/src/cuckoo.cc
    ${PROJECT_SOURCE_DIR}/src/hashtable.cc
    ${PROJECT_SOURCE_DIR}/src/utils.cc
)
target_link_libraries(tests oc::cryptoTools)
target_link_libraries(tests APSI::apsi)
