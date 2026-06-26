cmake_minimum_required(VERSION 3.10)

project(CuTest LANGUAGES C)

option(CUTEST_ENABLE_COVERAGE "Enable coverage instrumentation for GCC/Clang builds" OFF)

add_subdirectory(src)
