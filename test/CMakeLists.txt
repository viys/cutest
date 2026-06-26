cmake_minimum_required(VERSION 3.10)

# 定义项目名称和使用的语言
project(cutest C)

# 设置 C99 标准
set(CMAKE_C_STANDARD 99)

include(CTest)

set(CUTEST_TEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTestTest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AllTests.c
)

set(CUTEST_TEST_TARGET_NAME cutest)
set(CUTEST_TEST_OUTPUT_NAME cutest)
set(CUTEST_CTEST_NAME cutest)

if(CUTEST_USE_MEMORY_MIDDLEWARE)
    set(CUTEST_TEST_OUTPUT_NAME cutest_middleware)
    set(CUTEST_CTEST_NAME cutest_middleware)
endif()

# 添加子目录
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/lib_build)

add_executable(${CUTEST_TEST_TARGET_NAME} ${CUTEST_TEST_SOURCES})
set_target_properties(${CUTEST_TEST_TARGET_NAME} PROPERTIES
    OUTPUT_NAME ${CUTEST_TEST_OUTPUT_NAME}
)

target_include_directories(${CUTEST_TEST_TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(${CUTEST_TEST_TARGET_NAME} PUBLIC
    CuTest
)

if(CUTEST_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${CUTEST_TEST_TARGET_NAME} PRIVATE -O0 -g --coverage)
        set_property(TARGET ${CUTEST_TEST_TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS " --coverage")
    else()
        message(WARNING "CUTEST_ENABLE_COVERAGE only supports GNU and Clang compilers")
    endif()
endif()

if(BUILD_TESTING)
    add_test(NAME ${CUTEST_CTEST_NAME} COMMAND ${CUTEST_TEST_TARGET_NAME})
endif()
