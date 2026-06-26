set(SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTest.c
)

add_library(CuTest_static STATIC ${SRC})

target_include_directories(CuTest_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CUTEST_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(CuTest_static PUBLIC -O0 -g --coverage)
    else()
        message(WARNING "CUTEST_ENABLE_COVERAGE only supports GNU and Clang compilers")
    endif()
endif()

add_library(CuTest INTERFACE)
target_link_libraries(CuTest INTERFACE CuTest_static)
