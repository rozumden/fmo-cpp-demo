# fmo-core

add_library(fmo-core STATIC
    "../include/fmo/agglomerator.hpp"
    "../include/fmo/agglomerator-impl.hpp"
    "../include/fmo/algebra.hpp"
    "../include/fmo/algorithm.hpp"
    "../include/fmo/allocator.hpp"
    "../include/fmo/assert.hpp"
    "../include/fmo/subsampler.hpp"
    "../include/fmo/differentiator.hpp"
    "../include/fmo/benchmark.hpp"
    "../include/fmo/common.hpp"
    "../include/fmo/exchange.hpp"
    "../include/fmo/image.hpp"
    "../include/fmo/pointset.hpp"
    "../include/fmo/processing.hpp"
    "../include/fmo/region.hpp"
    "../include/fmo/retainer.hpp"
    "../include/fmo/stats.hpp"
    "../include/fmo/strip.hpp"
    agglomerator.cpp
    algorithm.cpp
    assert.cpp
    benchmark.cpp
    subsampler.cpp
    differentiator.cpp
    image.cpp
    image-util.cpp
    image-util.hpp
    include-opencv.hpp
    include-simd.hpp
    processing-basic.cpp
    processing-median3.cpp
    processing-median5.cpp
    processing-fitting.cpp
    region.cpp
    stats.cpp
    strip.cpp
)

set_property(TARGET fmo-core PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET fmo-core PROPERTY CXX_STANDARD 14)

target_include_directories(fmo-core
    PRIVATE ${OpenCV_INCLUDE_DIRS}
    PUBLIC "../include")

target_link_libraries(fmo-core PRIVATE ${OpenCV_LIBS})

# subdirectories

add_subdirectory(explorer-v1)
add_subdirectory(explorer-v2)
add_subdirectory(explorer-v3)
add_subdirectory(median-v1)
add_subdirectory(median-v2)
add_subdirectory(taxonomy-v1)

# fmo

set(FMO_LIBS "fmo-core;fmo-explorer-v1;fmo-explorer-v2;fmo-explorer-v3;fmo-median-v1;fmo-median-v2;fmo-taxonomy-v1" CACHE INTERNAL "List of FMO libraries to link")
