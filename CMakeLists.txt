cmake_minimum_required(VERSION 3.4)
project(lockfree)

configure_file(CMakeLists.txt.in googletest-download/CMakeLists.txt)

add_library(lockfree INTERFACE)
target_compile_features(lockfree INTERFACE cxx_std_17)
target_include_directories(lockfree INTERFACE include)

enable_testing()
add_subdirectory(tests)
