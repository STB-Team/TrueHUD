cmake_minimum_required(VERSION 3.22)

project(
	TrueHUD
	VERSION 1.1.10
	LANGUAGES CXX
)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if("${PROJECT_SOURCE_DIR}" STREQUAL "${PROJECT_BINARY_DIR}")
	message(FATAL_ERROR "in-source builds are not allowed")
endif()

macro(set_from_environment VARIABLE)
	if(NOT DEFINED ${VARIABLE} AND DEFINED ENV{${VARIABLE}})
		set(${VARIABLE} $ENV{${VARIABLE}})
	endif()
endmacro()

option(COPY_OUTPUT "Copy the output of build operations to the game directory" OFF)
set_from_environment(CompiledPluginsPath)
if(COPY_OUTPUT AND NOT DEFINED CompiledPluginsPath)
	message(FATAL_ERROR "CompiledPluginsPath is required when COPY_OUTPUT is enabled")
endif()
option(ENABLE_SKYRIM_SE "Enable support for Skyrim SE in the dynamic runtime feature." ON)
option(ENABLE_SKYRIM_AE "Enable support for Skyrim AE in the dynamic runtime feature." OFF)
option(ENABLE_SKYRIM_VR "Enable support for Skyrim VR in the dynamic runtime feature." OFF)
set(BUILD_TESTS OFF)
option(TRUEHUD_PROFILE "Record alternating baseline/optimized diagnostic windows" OFF)
option(TRUEHUD_BUILD_TESTS "Build packet cache regression tests" ON)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(src)
include(cmake/packaging.cmake)

if(TRUEHUD_BUILD_TESTS)
	enable_testing()
	add_executable(TrueHUDUpdateCacheTests tests/UpdateCacheTests.cpp)
	target_compile_features(TrueHUDUpdateCacheTests PRIVATE cxx_std_20)
	target_include_directories(TrueHUDUpdateCacheTests PRIVATE "${PROJECT_SOURCE_DIR}/src")
	add_test(NAME TrueHUDUpdateCache COMMAND TrueHUDUpdateCacheTests)
endif()
