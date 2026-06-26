set(CUTEST_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CuTest.c
)

if(CUTEST_USE_MEMORY_MIDDLEWARE)
    list(APPEND CUTEST_SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/memory/CuMemory.c
    )
endif()

add_library(CuTest_static STATIC ${CUTEST_SOURCES})

target_include_directories(CuTest_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(CUTEST_USE_MEMORY_MIDDLEWARE)
    target_compile_definitions(CuTest_static PUBLIC CUTEST_USE_MEMORY_MIDDLEWARE=1)

    if(NOT "${CUTEST_MEMORY_HEAP_SIZE}" STREQUAL "")
        target_compile_definitions(CuTest_static PUBLIC
            CUTEST_MEMORY_HEAP_SIZE=${CUTEST_MEMORY_HEAP_SIZE}
        )
    endif()

    if(NOT "${CUTEST_MEMORY_ALIGNMENT}" STREQUAL "")
        target_compile_definitions(CuTest_static PUBLIC
            CUTEST_MEMORY_ALIGNMENT=${CUTEST_MEMORY_ALIGNMENT}
        )
    endif()
endif()

if(CUTEST_ENABLE_COVERAGE)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(CuTest_static PUBLIC -O0 -g --coverage)
    else()
        message(WARNING "CUTEST_ENABLE_COVERAGE only supports GNU and Clang compilers")
    endif()
endif()

add_library(CuTest INTERFACE)
target_link_libraries(CuTest INTERFACE CuTest_static)
