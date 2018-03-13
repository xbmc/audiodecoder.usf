project(lazyusf)

enable_language(C)

cmake_minimum_required(VERSION 2.6)

set(SOURCES audio.c
            cpu.c
            dma.c
            exception.c
            interpreter_cpu.c
            interpreter_ops.c
            main.c
            memory.c
            pif.c
            registers.c
            tlb.c
            usf.c
            rsp/rsp.c
            rsp_hle/alist_audio.c
            rsp_hle/alist.c
            rsp_hle/alist_naudio.c
            rsp_hle/alist_nead.c
            rsp_hle/audio.c
            rsp_hle/cicx105.c
            rsp_hle/hle.c
            rsp_hle/jpeg.c
            rsp_hle/memory.c
            rsp_hle/mp3.c
            rsp_hle/musyx.c
            rsp_hle/plugin.c)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-msse HAS_SSE)
if(HAS_SSE)
  add_definitions(-DARCH_MIN_SSE2)
elseif(NOT WIN32)
  add_definitions(-DARCH_MIN_ARM_NEON)
endif()

set(CMAKE_POSITION_INDEPENDENT_CODE 1)
add_library(lazyusf STATIC ${SOURCES})
