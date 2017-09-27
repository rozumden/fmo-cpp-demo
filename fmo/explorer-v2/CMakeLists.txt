set(BINARY fmo-explorer-v2)

add_library(${BINARY} STATIC
    clusters.cpp
    components.cpp
    explorer.cpp
    explorer.hpp
    object.cpp
    preprocess.cpp
    strips.cpp
    visualize.cpp
)

set_property(TARGET ${BINARY} PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${BINARY} PROPERTY CXX_STANDARD 14)
target_include_directories(${BINARY} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${BINARY} PUBLIC fmo-core PRIVATE ${OpenCV_LIBS})
