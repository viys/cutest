# 手动列出所有的 .c 源文件
set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTest.c
)

# 添加静态库
add_library(CuTest_static SHARED ${SRC})

# 添加包含目录
target_include_directories(CuTest_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 创建接口库
add_library(CuTest INTERFACE)

# 将静态库作为依赖
target_link_libraries(CuTest INTERFACE CuTest_static)