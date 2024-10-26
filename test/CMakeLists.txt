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

# 将 myclass 库链接到可执行文件
target_link_libraries(cutest PRIVATE CuTest)