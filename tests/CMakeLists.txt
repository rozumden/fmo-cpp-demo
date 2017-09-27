# fmo-test

add_executable(fmo-test
    ../catch/catch.hpp
    test-algebra.cpp
    test-convert.cpp
    test-data.cpp
    test-data.hpp
    test-load.cpp
    test-main.cpp
    test-processing.cpp
    test-region.cpp
    test-retainer.cpp
    test-tools.hpp
)

set_property(TARGET fmo-test PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET fmo-test PROPERTY CXX_STANDARD 14)

target_compile_definitions(fmo-test PRIVATE CATCH_CONFIG_FAST_COMPILE)
target_link_libraries(fmo-test ${FMO_LIBS})
install(TARGETS fmo-test DESTINATION bin)

# fmo-test assets
set(ASSETS_SRC "${CMAKE_CURRENT_SOURCE_DIR}/assets")
install(DIRECTORY "${ASSETS_SRC}" DESTINATION bin)

if(MSVC)
    # copy assets to the MSVC default launch directory
    set(ASSETS_DST "${CMAKE_CURRENT_BINARY_DIR}/assets")
else()
    # copy assets next to the test executable in the build tree
    set(ASSETS_DST "$<TARGET_FILE_DIR:fmo-test>/assets")
endif()

add_custom_command(TARGET fmo-test POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${ASSETS_SRC}" "${ASSETS_DST}")

# fmo-benchmark

add_executable(fmo-benchmark
	benchmark-main.cpp)

set_property(TARGET fmo-benchmark PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET fmo-benchmark PROPERTY CXX_STANDARD 14)

target_link_libraries(fmo-benchmark ${FMO_LIBS})
install(TARGETS fmo-benchmark DESTINATION bin)
