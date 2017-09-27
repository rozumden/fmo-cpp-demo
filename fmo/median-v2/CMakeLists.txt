set(BINARY fmo-median-v2)

add_library(${BINARY} STATIC
    algorithm-median.cpp
    algorithm-median.hpp
    components.cpp
    objects-find.cpp
    objects-match.cpp
    objects-select.cpp
    output.cpp
    visualization.cpp
)

set_property(TARGET ${BINARY} PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${BINARY} PROPERTY CXX_STANDARD 14)
target_include_directories(${BINARY} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${BINARY} PUBLIC fmo-core PRIVATE ${OpenCV_LIBS})
