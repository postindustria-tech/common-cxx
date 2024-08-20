cmake_minimum_required(VERSION 3.24)
set (CMAKE_C_STANDARD 11)
set (CMAKE_CXX_STANDARD 17)

project(CommonCXX LANGUAGES CXX C)

if (NOT MSVC)
    add_compile_options(-fPIC)
endif()

if (MSVC)
    if (CMAKE_GENERATOR_PLATFORM)
        set (GEN_PLATFORM ${CMAKE_GENERATOR_PLATFORM})
    else()
        set (GEN_PLATFORM ${CMAKE_VS_PLATFORM_NAME_DEFAULT})
    endif()

    if (${GEN_PLATFORM} MATCHES .*64.* AND NOT 32bit)
        set(ARCH x64)
    else()
        set(ARCH x86)
    endif()

else()
    message(STATUS "System Processor: ${CMAKE_SYSTEM_PROCESSOR}")
    if (${CMAKE_SYSTEM_PROCESSOR} MATCHES ^armhf)
        set(ARCH armhf)
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES ^aarch64)
        set(ARCH aarch64)
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES ^arm64)
        set(ARCH aarch64)
    elseif (${CMAKE_SYSTEM_PROCESSOR} MATCHES 64)
        if (32bit)
            message(STATUS "32bit: ${32bit}")
            set (ARCH x86)
        else()
            message(STATUS "32bit: ${32bit}")
            set (ARCH x64)
        endif()
    else()
        set (ARCH x86)
    endif()
endif()
message(STATUS "Arch name: ${ARCH}")
message(STATUS "Generator Platform: ${CMAKE_GENERATOR_PLATFORM}")
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

if (MSVC)
    set (OS windows)
elseif (UNIX AND NOT APPLE)
    set (OS linux)
elseif (APPLE)
    set (OS macos)
endif()
message(STATUS "OS name: ${OS}")

# include CMakeLists from common-cxx library
option(BUILD_TESTING "" OFF)
include("${CMAKE_CURRENT_LIST_DIR}/../../CMakeLists.txt" NO_POLICY_SCOPE)

set(NAMESPACE "FiftyoneDegrees.Common")
set(DLLNAME "${NAMESPACE}.Native")
set(CXXWRAP "${CMAKE_CURRENT_LIST_DIR}/../../commonCSHARP_wrap.cxx")

find_package(SWIG 4.0 COMPONENTS csharp)
if(SWIG_FOUND)
    message("-- Found SWIG ${SWIG_EXECUTABLE}, running codegen of the wrappers:")
    execute_process(COMMAND ${SWIG_EXECUTABLE} -c++ -csharp -namespace "${NAMESPACE}" -dllimport "${DLLNAME}.dll" -outdir ${CMAKE_CURRENT_LIST_DIR}/FiftyoneDegrees.Common -o "${CXXWRAP}" ${CMAKE_CURRENT_LIST_DIR}/../../common.i
        COMMAND_ECHO STDOUT)
endif()

if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio")
    set(OUTDIR ".")
else()
    set(OUTDIR "${CMAKE_BUILD_TYPE}")
endif()

add_library(common-cxx-dotnet SHARED "${CXXWRAP}")
target_link_libraries(common-cxx-dotnet fiftyone-common-cxx)
set_target_properties(common-cxx-dotnet PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${OUTDIR}"
    LIBRARY_OUTPUT_DIRECTORY "${OUTDIR}"
    OUTPUT_NAME "${DLLNAME}" 
    PREFIX "" 
    SUFFIX ".dll")

if (MSVC)
    # /wd4100 needed to disable "unreferenced formal parameter" which comes from the SWIG file
    target_compile_options(common-cxx-dotnet PRIVATE "/D_CRT_SECURE_NO_WARNINGS" "/W4" "/WX" "/wd4100" "/EHsc")
else()
    add_compile_options(-fPIC)
    target_compile_options(common-cxx-dotnet INTERFACE "-static-libgcc -static-libstdc++")
    target_link_options(common-cxx-dotnet INTERFACE "-static-libgcc -static-libstdc++")
endif()