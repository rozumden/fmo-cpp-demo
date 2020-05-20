cmake_minimum_required(VERSION 3.4.1)

project(FMO)

# find dependencies
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenCV 3.0.0 REQUIRED)

### Begin Added from issue thread (uncomment for some macos devices)
# SET (BOOST_ROOT "/usr/local/Cellar/boost/1.72.0_2")
# SET (BOOST_INCLUDEDIR "/usr/local/Cellar/boost/1.72.0_2/include")
# SET (BOOST_LIBRARYDIR "/usr/local/Cellar/boost/1.72.0_2/lib")

# SET (BOOST_MIN_VERSION "1.72.0")
# set (Boost_NO_BOOST_CMAKE ON)
# FIND_PACKAGE(Boost ${BOOST_MIN_VERSION} REQUIRED)
# if (NOT Boost_FOUND)
# message(FATAL_ERROR "Fatal error: Boost (version >= 1.72) required.")
# else()
# message(STATUS "Setting up BOOST")
# message(STATUS " Includes - ${Boost_INCLUDE_DIRS}")
# message(STATUS " Library - ${Boost_LIBRARY_DIRS}")
# include_directories(${Boost_INCLUDE_DIRS})
# link_directories(${Boost_LIBRARY_DIRS})
# endif (NOT Boost_FOUND)
### 

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

