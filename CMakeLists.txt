cmake_minimum_required(VERSION 3.10)

project(CuTest LANGUAGES C)

option(CUTEST_ENABLE_COVERAGE "Enable coverage instrumentation for GCC/Clang builds" OFF)
option(CUTEST_USE_MEMORY_MIDDLEWARE "Build CuTest with the memory middleware enabled" OFF)
set(CUTEST_MEMORY_HEAP_SIZE "" CACHE STRING "Override the memory middleware heap size in bytes")
set(CUTEST_MEMORY_ALIGNMENT "" CACHE STRING "Override the memory middleware alignment in bytes")

add_subdirectory(src)
