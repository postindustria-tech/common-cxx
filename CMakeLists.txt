cmake_minimum_required(VERSION 3.10)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 11)

project(51DegreesCommon VERSION 4.0.1 LANGUAGES CXX C)

include(CTest)

find_package(Threads REQUIRED)

if(NOT CMAKE_BUILD_TYPE AND NOT MSVC)
	message("-- Setting default CMAKE_BUILD_TYPE  to Release")
	set(CMAKE_BUILD_TYPE "Release")
endif()
  
option(MemoryOnly "MemoryOnly" OFF)
option(NoThreading "NoThreading" OFF)
option(ExceptionsDisabled "ExceptionsDisabled" OFF)

if (MemoryOnly)
	message("-- Memory only compilation (FIFTYONE_DEGREES_MEMORY_ONLY) is enabled")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIFTYONE_DEGREES_MEMORY_ONLY")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFIFTYONE_DEGREES_MEMORY_ONLY")
endif()

if (NoThreading)
	message("-- No Threading compilation (FIFTYONE_DEGREES_NO_THREADING) is enabled")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIFTYONE_DEGREES_NO_THREADING")
endif()

if (ExceptionsDisabled)
	message("-- Exceptions disable compilation (FIFTYONE_DEGREES_EXCEPTIONS_DISABLED) is enabled")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFIFTYONE_DEGREES_EXCEPTIONS_DISABLED")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DFIFTYONE_DEGREES_EXCEPTIONS_DISABLED")
endif()

if (MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /D _UNICODE /D UNICODE")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Common

FILE(GLOB COMC_SRC ${CMAKE_CURRENT_LIST_DIR}/*.c)
FILE(GLOB COMC_H ${CMAKE_CURRENT_LIST_DIR}/*.h)
add_library(fiftyone-common-c ${COMC_SRC} ${COMC_H})
target_link_libraries(fiftyone-common-c	${CMAKE_THREAD_LIBS_INIT})

FILE(GLOB COMCPP_SRC ${CMAKE_CURRENT_LIST_DIR}/*.cpp)
FILE(GLOB COMCPP_H ${CMAKE_CURRENT_LIST_DIR}/*.hpp)
add_library(fiftyone-common-cxx ${COMCPP_SRC} ${COMCPP_H})
target_link_libraries(fiftyone-common-cxx fiftyone-common-c)

set_target_properties(fiftyone-common-c fiftyone-common-cxx	PROPERTIES FOLDER "Common") 
if (MSVC)
	target_compile_options(fiftyone-common-c PRIVATE "/D_CRT_SECURE_NO_WARNINGS")
	target_compile_options(fiftyone-common-cxx PRIVATE "/D_CRT_SECURE_NO_WARNINGS")
endif()

# Examples

add_executable(CachePerf ${CMAKE_CURRENT_LIST_DIR}/performance/CachePerf.c)
target_link_libraries(CachePerf	fiftyone-common-c)
if (MSVC)
	target_compile_options(CachePerf PRIVATE "/D_CRT_SECURE_NO_WARNINGS")
endif()
set_target_properties(CachePerf	PROPERTIES FOLDER "Examples/Common") 

# Download and unpack googletest at configure time
configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeLists.txt.in googletest-download/CMakeLists.txt)
execute_process(COMMAND ${CMAKE_COMMAND} -G "${CMAKE_GENERATOR}" .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
	message(FATAL_ERROR "-- CMake step for googletest failed: ${result}")
endif()
execute_process(COMMAND ${CMAKE_COMMAND} --build .
	RESULT_VARIABLE result
	WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/googletest-download)
if(result)
	message(FATAL_ERROR "-- Build step for googletest failed: ${result}")
endif()

# Prevent overriding the parent project's compiler/linker
# settings on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# Add googletest directly to our build. This defines
# the gtest and gtest_main targets.
add_subdirectory(${CMAKE_CURRENT_BINARY_DIR}/googletest-src
	${CMAKE_CURRENT_BINARY_DIR}/googletest-build
	EXCLUDE_FROM_ALL)

# The gtest/gtest_main targets carry header search path
# dependencies automatically when using CMake 2.8.11 or
# later. Otherwise we have to add them here ourselves.
if (CMAKE_VERSION VERSION_LESS 2.8.11)
	include_directories("${gtest_SOURCE_DIR}/include")
endif()

include(GoogleTest)

# Tests

set(COM_TEST ${CMAKE_CURRENT_LIST_DIR}/tests)
FILE(GLOB COM_TEST_SRC ${COM_TEST}/*.cpp)
FILE(GLOB COM_TEST_H ${COM_TEST}/*.hpp)
add_executable(CommonTests ${COM_TEST_SRC} ${COM_TEST_H})
if (MSVC)
	target_compile_options(CommonTests PRIVATE "/D_CRT_SECURE_NO_WARNINGS")
endif()

target_link_libraries(CommonTests fiftyone-common-cxx gtest_main)
gtest_discover_tests(CommonTests)
set_target_properties(CommonTests PROPERTIES FOLDER "Tests") 

if (CMAKE_COMPILER_IS_GNUCC)
    target_compile_options(CommonTests PRIVATE "-Wall" "-Wno-unused-variable" "-Wno-unused-result")
endif()
