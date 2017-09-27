set(BINARY fmo-taxonomy-v1)

add_library(${BINARY} STATIC
    algorithm-taxonomy.cpp
    algorithm-taxonomy.hpp
    components.cpp
    output.cpp
    visualization.cpp
)

set_property(TARGET ${BINARY} PROPERTY CXX_EXTENSIONS OFF)
set_property(TARGET ${BINARY} PROPERTY CXX_STANDARD 14)
target_include_directories(${BINARY} PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${BINARY} PUBLIC fmo-core PRIVATE ${OpenCV_LIBS})
