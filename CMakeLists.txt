# 手动列出所有的 .c 源文件
set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTest.c
)

# 添加库
add_library(CuTest SHARED ${SRC})

# 添加包含目录
target_include_directories(CuTest PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
