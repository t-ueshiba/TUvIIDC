cmake_minimum_required(VERSION 3.5)
project(TUvIIDC)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

enable_language(CXX)
set(CMAKE_CXX_STANDARD		17)
set(CMAKE_CXX_STANDARD_REQUIRED	ON)
set(CMAKE_CXX_EXTENSIONS	OFF)
set(CMAKE_CXX_FLAGS_RELEASE	"-Wall -O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG	"-g")

if(APPLE)
  set(MACOSX_RPATH TRUE)
endif()

find_package(PkgConfig REQUIRED)
find_package(X11 REQUIRED)
find_package(Boost REQUIRED)
pkg_check_modules(LIBUSB-1.0 REQUIRED libusb-1.0)
pkg_check_modules(YAML-CPP REQUIRED yaml-cpp)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${CMAKE_INSTALL_PREFIX}/include
    ${X11_INCLUDE_DIRS}
    ${Boost_INCLUDE_DIRS})

link_directories(
    ${CMAKE_INSTALL_PREFIX}/lib
    ${LIBUSB-1.0_LIBRARY_DIRS}
    ${YAML-CPP_LIBRARY_DIRS}
)

file(GLOB sources *.cc)
add_library(${PROJECT_NAME} SHARED ${sources})
target_link_libraries(${PROJECT_NAME} TUIIDC TUv)

install(TARGETS ${PROJECT_NAME} LIBRARY DESTINATION lib)

add_subdirectory(TU)
