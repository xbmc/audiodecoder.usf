project(audiodecoder.usf)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR})

enable_language(CXX)

find_package(kodi REQUIRED)

include_directories(${KODI_INCLUDE_DIR}
                    ${PROJECT_SOURCE_DIR}/lib/lazyusf
                    ${PROJECT_SOURCE_DIR}/lib/psflib)

add_subdirectory(lib/lazyusf)
add_subdirectory(lib/psflib)

set(USF_SOURCES src/USFCodec.cpp)

set(DEPLIBS lazyusf psflib)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-msse HAS_SSE)
if(HAS_SSE)
  add_definitions(-DARCH_MIN_SSE2)
else()
  add_definitions(-DARCH_MIN_ARM_NEON)
endif()

build_addon(audiodecoder.usf USF DEPLIBS)

include(CPack)
