cmake_minimum_required(VERSION 3.4.1)

project(FMO)

# find dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED)

if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /WX /fp:fast")
    add_definitions(-D_SCL_SECURE_NO_WARNINGS)
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -ffast-math")
endif()

if(DEFINED ANDROID_NATIVE_API_LEVEL)
	set(FMO_BUILD_ANDROID_DEFAULT YES)
	set(FMO_BUILD_DESKTOP_DEFAULT NO)
	set(FMO_BUILD_TESTS_DEFAULT NO)
else()
	set(FMO_BUILD_ANDROID_DEFAULT NO)
	set(FMO_BUILD_DESKTOP_DEFAULT YES)
	set(FMO_BUILD_TESTS_DEFAULT YES)
endif()

set(FMO_BUILD_ANDROID ${FMO_BUILD_ANDROID_DEFAULT} CACHE BOOL "Build the shared library for the Android app")
set(FMO_BUILD_DESKTOP ${FMO_BUILD_DESKTOP_DEFAULT} CACHE BOOL "Build the desktop executable")
set(FMO_BUILD_TESTS ${FMO_BUILD_TESTS_DEFAULT} CACHE BOOL "Build the tests executable")

add_subdirectory("fmo" "")

if(FMO_BUILD_ANDROID)
    add_subdirectory("android" "")
endif()

if(FMO_BUILD_DESKTOP)
    add_subdirectory("desktop" "")
endif()

if(FMO_BUILD_TESTS)
    add_subdirectory("tests" "")
endif()
