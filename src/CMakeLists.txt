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

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

configure_file(
  include/yajl/yajl_version.h.in
  ${PROJECT_BINARY_DIR}/include/yajl/yajl_version.h
  @ONLY)

add_library(yajl_obj
            OBJECT yajl.c
                   yajl_lex.c
                   yajl_parser.c
                   yajl_buf.c
                   yajl_encode.c
                   yajl_gen.c
                   yajl_alloc.c
                   yajl_tree.c
                   yajl_version.c)

list(APPEND yajl_targets_list yajl_s)

if (ENABLE_CFISAN)
  set(YAJL_BUILD_SHARED OFF CACHE INTERNAL "Build the shared yajl library")
  # Flags needed at both compile and link time

  list(APPEND cfisan_common_flags -fsanitize=cfi
                                  -flto
                                  -fvisibility=hidden
  )

  # Compile-only flags
  list(APPEND cfisan_compile_flags
      -fno-omit-frame-pointer
  )
  
  set_property(SOURCE yajl_alloc.c 
               APPEND PROPERTY COMPILE_OPTIONS "-fno-sanitize=cfi")

  target_compile_options(yajl_obj PRIVATE ${cfisan_compile_flags} ${cfisan_common_flags})
  target_link_options(yajl_obj PRIVATE ${cfisan_common_flags})
endif ()

if (ENABLE_ASAN)
  # Flags needed at both compile and link time
  set(asan_common_flags -fsanitize=address,undefined)

  # Compile-only flags
  list(APPEND asan_compile_flags
      -fno-omit-frame-pointer
      -fno-optimize-sibling-calls
      -fno-common
  )

  # AppleClang doesn't have lsan or local-bounds; ID is "AppleClang", so this is plain Clang only
  if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
      list(APPEND asan_compile_flags -fsanitize=leak)
      list(APPEND asan_compile_flags -fsanitize=local-bounds)
  endif ()

  if (CMAKE_C_COMPILER_ID MATCHES "Clang")
      list(APPEND asan_compile_flags
          -fsanitize-address-use-after-scope
          -fsanitize-address-use-after-return=always)
  endif ()

  target_compile_options(yajl_obj PUBLIC ${asan_compile_flags} ${asan_common_flags})
  target_link_options(yajl_obj PUBLIC ${asan_common_flags})
endif ()

target_sources(yajl_obj
  PUBLIC FILE_SET headers
    TYPE HEADERS
    BASE_DIRS include ${CMAKE_BINARY_DIR}/include
    FILES
      include/yajl/yajl_parse.h
      include/yajl/yajl_gen.h
      include/yajl/yajl_common.h
      include/yajl/yajl_tree.h
      ${CMAKE_BINARY_DIR}/include/yajl/yajl_version.h
  PRIVATE FILE_SET private_headers
    TYPE HEADERS
    FILES
      yajl_parser.h
      yajl_lex.h
      yajl_buf.h
      yajl_encode.h
      yajl_alloc.h)

target_compile_definitions(yajl_obj PRIVATE YAJL_BUILD)
add_library(yajl_s STATIC $<TARGET_OBJECTS:yajl_obj>)

install(TARGETS yajl_s LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Public headers (yajl_parse.h, yajl_gen.h, yajl_common.h, yajl_tree.h, ...)
install(TARGETS yajl_obj
  FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (YAJL_BUILD_SHARED)
  list(APPEND yajl_targets_list yajl)

  add_library(yajl SHARED $<TARGET_OBJECTS:yajl_obj>)

  install(TARGETS yajl LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

  #### setup shared library version number
  set_target_properties(yajl PROPERTIES
                        DEFINE_SYMBOL YAJL_BUILD
                        DEFINE_SYMBOL YAJL_SHARED
                        SOVERSION ${YAJL_MAJOR}
                        VERSION ${YAJL_MAJOR}.${YAJL_MINOR}.${YAJL_MICRO})
endif ()


foreach(tgt ${yajl_targets_list})
  target_include_directories(${tgt} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

  target_link_options(${tgt} PUBLIC
    "$<BUILD_INTERFACE:$<TARGET_PROPERTY:yajl_obj,INTERFACE_LINK_OPTIONS>>")
endforeach()

configure_package_config_file(
  ${PROJECT_SOURCE_DIR}/cmake/YAJLConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/YAJLConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/YAJL)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/YAJLConfigVersion.cmake
  VERSION 2.1.1
  COMPATIBILITY SameMajorVersion)
  
install(TARGETS ${yajl_targets_list}
  EXPORT YAJLTargets
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT YAJLTargets
  FILE YAJLTargets.cmake
  NAMESPACE YAJL::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/YAJL)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/YAJLConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/YAJLConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/YAJL)

foreach(p INTERFACE_LINK_LIBRARIES
          INTERFACE_COMPILE_OPTIONS INTERFACE_LINK_OPTIONS
          INTERFACE_COMPILE_DEFINITIONS INTERFACE_SOURCES)
  get_target_property(v yajl_s ${p})
  message(STATUS "yajl_s ${p} = ${v}")
endforeach()
