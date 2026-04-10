cmake_minimum_required(VERSION 3.10)
project(json VERSION 1.0.0 LANGUAGES C)

# C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Options
option(JSON_BUILD_TESTS "Build tests" ON)
option(JSON_BUILD_BENCHMARKS "Build benchmarks" ON)

# Header-only library target
add_library(json INTERFACE)
target_include_directories(json INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Installation
include(GNUInstallDirs)
install(TARGETS json
    EXPORT json-targets
)
install(EXPORT json-targets
    FILE json-targets.cmake
    NAMESPACE json::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/json
)
install(FILES include/json.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Tests
if(JSON_BUILD_TESTS)
    enable_testing()
    
    add_executable(test_json
        tests/test_json.c
    )
    target_link_libraries(test_json PRIVATE json)
    target_include_directories(test_json PRIVATE . tests)
    
    # Copy test files to build directory
    file(COPY tests/testfiles DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    
    add_test(NAME json_tests COMMAND test_json)
endif()

# Benchmarks
if(JSON_BUILD_BENCHMARKS)
    add_executable(bench_json
        tests/bench.c
    )
    target_link_libraries(bench_json PRIVATE json m)
    target_include_directories(bench_json PRIVATE . tests)
    
    # Comparison benchmark: json.h (SIMD)
    add_executable(bench_jsonh_compare
        tests/bench_compare.c
    )
    target_link_libraries(bench_jsonh_compare PRIVATE json m)
    target_include_directories(bench_jsonh_compare PRIVATE . tests)
    
    # Comparison benchmark: tidwall/json.c (original)
    add_executable(bench_tidwall_compare
        tests/bench_tidwall.c
        third_party/tidwall_json.c
    )
    target_link_libraries(bench_tidwall_compare PRIVATE m)
    # third_party must come BEFORE . so tidwall_json.c finds tidwall_json.h
    target_include_directories(bench_tidwall_compare PRIVATE third_party tests .)
    
    # Copy test files to build directory if not already done
    if(NOT JSON_BUILD_TESTS)
        file(COPY tests/testfiles DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    endif()
endif()

# Summary
message(STATUS "")
message(STATUS "json configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Tests: ${JSON_BUILD_TESTS}")
message(STATUS "  Benchmarks: ${JSON_BUILD_BENCHMARKS}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
