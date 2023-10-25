cmake_minimum_required(VERSION 3.10)

project(asrtbflash)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

add_compile_options($<$<CONFIG:Release>:/MT> # Runtime library: Multi-threaded
                    $<$<CONFIG:RelWithDebInfo>:/MT> # Runtime library: Multi-threaded
                    $<$<CONFIG:Debug>:/MTd> # Runtime library: Multi-threaded Debug
                    )

#===============================================================================
# Third Party Dependencies
#===============================================================================
include(cmake/CPM.cmake)
CPMAddPackage("gh:fmtlib/fmt#10.1.1")
CPMAddPackage(
  NAME spdlog
  GIT_TAG v1.12.0
  GITHUB_REPOSITORY gabime/spdlog
  OPTIONS "SPDLOG_INSTALL YES"
  "SPDLOG_FMT_EXTERNAL YES"
)
add_dependencies(spdlog fmt)
CPMAddPackage("gh:jarro2783/cxxopts@3.1.1")
CPMAddPackage(
  NAME lzma
  VERSION 2201
  URL https://7-zip.org/a/lzma2201.7z
)

#===============================================================================
# Build Libraries
#===============================================================================
add_library(7zcompress
  ${lzma_SOURCE_DIR}/C/LzmaDec.c
  ${lzma_SOURCE_DIR}/C/LzmaEnc.c
  ${lzma_SOURCE_DIR}/C/LzFind.c
  ${lzma_SOURCE_DIR}/C/CpuArch.c
  ${CMAKE_SOURCE_DIR}/src/lzma.cc
)
target_include_directories(7zcompress PUBLIC ${lzma_SOURCE_DIR}/C)
target_compile_definitions(7zcompress PUBLIC _7ZIP_ST)

add_library(bufio
  ${CMAKE_SOURCE_DIR}/src/stacker.cc
  ${CMAKE_SOURCE_DIR}/src/destacker.cc
)
target_link_libraries(bufio PUBLIC fmt::fmt spdlog::spdlog 7zcompress)
set_target_properties(bufio PROPERTIES
  LINKER_LANGUAGE CXX
  CXX_STANDARD 17
)

#===============================================================================
# Build Executables
#===============================================================================
add_executable(asrtbflash
  ${CMAKE_SOURCE_DIR}/src/asrtbflash.cc
  ${CMAKE_SOURCE_DIR}/src/smbios.cc
  ${CMAKE_SOURCE_DIR}/src/console.cpp
)
target_link_libraries(asrtbflash PRIVATE bufio cxxopts)
if(${CMAKE_GENERATOR} MATCHES "^Visual Studio*")
  message(STATUS "USE MSVC APPEND requireAdministrator privilege")
  set_target_properties(asrtbflash PROPERTIES LINK_FLAGS "/level='requireAdministrator' /uiAccess='false'")
  # target_compile_options(asrtbflash PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/W4 > # /WX
endif()

add_executable(test_cmd ${CMAKE_SOURCE_DIR}/src/test_cmd.cc)
target_link_libraries(test_cmd PRIVATE fmt::fmt spdlog::spdlog)

add_executable(tbflash ${CMAKE_SOURCE_DIR}/src/tbflash_test.cc)
target_link_libraries(tbflash PRIVATE fmt::fmt spdlog::spdlog)
