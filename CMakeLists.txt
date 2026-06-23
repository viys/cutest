option(CUTEST_ENABLE_COVERAGE "Enable coverage instrumentation for GCC/Clang builds" OFF)

# 手动列出所有的 .c 源文件
set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTest.c
)

# 添加静态库
add_library(CuTest_static STATIC ${SRC})

# 添加包含目录
target_include_directories(CuTest_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CUTEST_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(CuTest_static PUBLIC -O0 -g --coverage)
    else()
        message(WARNING "CUTEST_ENABLE_COVERAGE only supports GNU and Clang compilers")
    endif()
endif()

# 创建接口库
add_library(CuTest INTERFACE)

# 将静态库作为接口库的依赖
target_link_libraries(CuTest INTERFACE CuTest_static)
