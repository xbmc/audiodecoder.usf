project(audiodecoder.usf)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(xbmc REQUIRED)

include_directories(${XBMC_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/lazyusf
                    ${PROJECT_SOURCE_DIR}/lib/psflib)

add_subdirectory(lib/lazyusf)
add_subdirectory(lib/psflib)

set(USF_SOURCES src/USFCodec.cpp)

set(DEPLIBS lazyusf psflib)

add_definitions(-DARCH_MIN_SSE2)

build_addon(audiodecoder.usf USF DEPLIBS)

include(CPack)
