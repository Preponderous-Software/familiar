cmake_minimum_required(VERSION 3.19)
project(familiar VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(FAMILIAR_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(FAMILIAR_BUILD_TESTS "Build the test suite" ON)

function(familiar_warnings target)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4)
    if(FAMILIAR_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
    if(FAMILIAR_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
endfunction()

# The simulation: the town, walking and climbing, the clock. No engine, no
# GPU, so it builds and tests in seconds. Never fuse multiply-adds: the rules
# must give bit-identical results on every compiler.
add_library(familiar_sim STATIC
  src/sim/town.cpp
  src/sim/world.cpp)
target_include_directories(familiar_sim PUBLIC src)
familiar_warnings(familiar_sim)
if(MSVC)
  target_compile_options(familiar_sim PRIVATE /fp:precise)
else()
  target_compile_options(familiar_sim PRIVATE -ffp-contract=off)
endif()

if(FAMILIAR_BUILD_TESTS)
  enable_testing()
  foreach(t movement town)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE familiar_sim)
    target_include_directories(test_${t} PRIVATE tests)
    familiar_warnings(test_${t})
    add_test(NAME ${t} COMMAND test_${t} "${CMAKE_CURRENT_BINARY_DIR}")
  endforeach()
endif()
