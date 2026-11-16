# Copyright (c) 2007-2014, Lloyd Hilaiel <me@lloyd.io>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

cmake_minimum_required(VERSION 4.0)

project(YetAnotherJSONParser C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(YAJL_MAJOR 2)
set(YAJL_MINOR 1)
set(YAJL_MICRO 1)
set(YAJL_VERSION "${YAJL_MAJOR}.${YAJL_MINOR}.${YAJL_MICRO}" CACHE INTERNAL "Yajl Full Version")

set(ENABLE_ASAN OFF CACHE INTERNAL "Build with ASan+UBSan+LSan")
set(ENABLE_CFISAN OFF CACHE INTERNAL "Build with CFISan")
set(YAJL_BUILD_SHARED ON CACHE INTERNAL "Build the shared yajl library")

if (ENABLE_ASAN)
  set(AFL_ASAN_ENV
    AFL_USE_ASAN=1
    AFL_USE_UBSAN=1
    AFL_LLVM_DICT2FILE=${CMAKE_SOURCE_DIR}/llvm.dict)

  set(AFL_LAUNCHER ${CMAKE_COMMAND} -E env ${AFL_ASAN_ENV})

  # Globally (set before project()/targets are created):
  set(CMAKE_C_COMPILER_LAUNCHER "${AFL_LAUNCHER}")
endif ()

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif ()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")

if (WIN32)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4")
  add_definitions(-DWIN32)
  set(linkFlags "/PDB:NONE /INCREMENTAL:NO /OPT:NOREF /OPT:NOICF")
  set(CMAKE_EXE_LINKER_FLAGS    "${linkFlags}"
      CACHE STRING "YAJL linker flags" FORCE)
  set(CMAKE_EXE_LINKER_FLAGS_DEBUG ""
      CACHE STRING "YAJL debug linker flags" FORC E)
  set(CMAKE_EXE_LINKER_FLAGS_RELEASE
      CACHE STRING "YAJL release linker flags" FORCE)
  set(CMAKE_SHARED_LINKER_FLAGS "${linkFlags}"
      CACHE STRING "YAJL shared linker flags" FORCE)
  set(CMAKE_MODULE_LINKER_FLAGS "${linkFlags}"
      CACHE STRING "YAJL module linker flags" FORCE)

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4996 /wd4255 /wd4130 /wd4100 /wd4711")
  set(CMAKE_C_FLAGS_DEBUG "/D DEBUG /Od /Z7")
  set(CMAKE_C_FLAGS_RELEASE "/D NDEBUG /O2")
else ()
  if (CMAKE_COMPILER_IS_GNUCC)
    INCLUDE(CheckCCompilerFlag)
    CHECK_C_COMPILER_FLAG(-fvisibility=hidden HAVE_GCC_VISIBILITY)
    if (HAVE_GCC_VISIBILITY)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
    endif ()
  endif ()

  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -Wpointer-arith -Wno-format-y2k -Wstrict-prototypes -Wmissing-declarations -Wnested-externs -Wextra	-Wundef -Wwrite-strings -Wold-style-definition -Wredundant-decls -Wno-unused-parameter -Wno-sign-compare -Wmissing-prototypes")

  set(CMAKE_C_FLAGS_DEBUG "-DDEBUG -g -glldb -O0 -gcolumn-info -fPIC -Wall")
  set(CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -g -gcolumn-info -Wuninitialized")
endif ()

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(reformatter)
add_subdirectory(verify)
add_subdirectory(example)
add_subdirectory(perf)

include(YAJLDoc.cmake)

# a test target
add_custom_target(test
                  ./run_tests.sh ${CMAKE_CURRENT_BINARY_DIR}/test/parsing/yajl_test
                  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/test/parsing)

add_custom_target(test-api ${CMAKE_CURRENT_SOURCE_DIR}/test/api/run_tests.sh
                  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/test/api)
