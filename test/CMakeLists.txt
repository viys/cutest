cmake_minimum_required(VERSION 3.10)

# 定义项目名称和使用的语言
project(cutest C)

# 设置 C99 标准
set(CMAKE_C_STANDARD 99)

# 添加子目录
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_BINARY_DIR}/lib_build)

# 添加可执行文件，并指定源文件
add_executable(cutest 
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTestTest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/AllTests.c
)

# 让测试工程继续直接包含迁移后的公共头文件。
target_include_directories(cutest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

# 将 CuTest 库链接到可执行文件
target_link_libraries(cutest PUBLIC
    CuTest
)

if(CUTEST_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(cutest PRIVATE -O0 -g --coverage)
        set_property(TARGET cutest APPEND_STRING PROPERTY LINK_FLAGS " --coverage")
    else()
        message(WARNING "CUTEST_ENABLE_COVERAGE only supports GNU and Clang compilers")
    endif()
endif()
