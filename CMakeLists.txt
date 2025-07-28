cmake_minimum_required(VERSION 3.15)

project(ProxyDLL LANGUAGES CXX)

include("VC-LTL helper for cmake.cmake")

# Flavor options
option(LOGGING "" OFF)

add_compile_definitions(
  "$<$<BOOL:${LOGGING}>:LOGGING>"
)

# Compiler options
if(!MSVC)
  message(FATAL_ERROR "Current doesn't support compiler other than MSVC.")
endif()

add_compile_options(
  /Gy
  /utf-8
  /experimental:deterministic
)

add_link_options(
  /OPT:REF
  /OPT:ICF=10
)

foreach(flag_var CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
                 CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
  string(REPLACE /GR /GR- "${flag_var}" "${${flag_var}}")
endforeach()

add_compile_definitions(
  NOMINMAX
  WIN32_LEAN_AND_MEAN
)

# Targets

macro(add_dll name)
    add_library(${name} SHARED ${ARGN})
    target_compile_features(${name} PUBLIC cxx_std_20)
    target_include_directories(${name} PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")
    set_property(TARGET ${name} PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endmacro()

add_dll("version" "dll2.cpp" "version.def" "version.hpp" "Utils.hpp")
