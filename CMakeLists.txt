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
option(FAMILIAR_WITH_STRATA "Build the playable game on the Strata engine" ON)
option(FAMILIAR_WEB "Build the browser game (Emscripten + Strata's WebGL 2 backend)" OFF)

# Emscripten: the rules and tests build as they are and the tests run under
# Node; FAMILIAR_WEB adds the browser game. Strata compiles C++ exceptions as
# wasm exceptions, and everything in one .wasm must match.
if(EMSCRIPTEN)
  cmake_minimum_required(VERSION 3.24)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:-fwasm-exceptions>")
  add_link_options(-fwasm-exceptions -sALLOW_MEMORY_GROWTH=1)
  if(FAMILIAR_WITH_STRATA)
    message(FATAL_ERROR "FAMILIAR_WITH_STRATA builds the desktop game: use -DFAMILIAR_WITH_STRATA=OFF -DFAMILIAR_WEB=ON")
  endif()
endif()
if(FAMILIAR_WEB AND NOT EMSCRIPTEN)
  message(FATAL_ERROR "FAMILIAR_WEB needs Emscripten (emcmake cmake ...)")
endif()

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
  src/sim/autopilot.cpp
  src/sim/cat.cpp
  src/sim/craft.cpp
  src/sim/doors.cpp
  src/sim/notebook.cpp
  src/sim/npc.cpp
  src/sim/town.cpp
  src/sim/world.cpp)
target_include_directories(familiar_sim PUBLIC src)
familiar_warnings(familiar_sim)
if(MSVC)
  target_compile_options(familiar_sim PRIVATE /fp:precise)
else()
  target_compile_options(familiar_sim PRIVATE -ffp-contract=off)
endif()

# Saves: the World as checksummed bytes, written atomically with the previous
# save kept (src/save/save.h).
add_library(familiar_save STATIC src/save/save.cpp src/save/sha256.cpp)
target_link_libraries(familiar_save PUBLIC familiar_sim)
familiar_warnings(familiar_save)

if(FAMILIAR_BUILD_TESTS)
  enable_testing()
  foreach(t movement town doors cat craft autopilot saves)
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE familiar_sim familiar_save)
    target_compile_definitions(test_${t} PRIVATE FAMILIAR_FIXTURES="${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures")
    target_include_directories(test_${t} PRIVATE tests)
    target_compile_definitions(test_${t} PRIVATE FAMILIAR_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    familiar_warnings(test_${t})
    if(EMSCRIPTEN)
      target_link_options(test_${t} PRIVATE -sNODERAWFS=1)
    endif()
    add_test(NAME ${t} COMMAND test_${t} "${CMAKE_CURRENT_BINARY_DIR}")
  endforeach()
  # Writes a new format's fixtures (never an old one's): see tools/make_save_fixtures.cpp.
  add_executable(make_save_fixtures tools/make_save_fixtures.cpp)
  target_link_libraries(make_save_fixtures PRIVATE familiar_save)
  familiar_warnings(make_save_fixtures)
  if(EMSCRIPTEN)
    target_link_options(make_save_fixtures PRIVATE -sNODERAWFS=1)
  endif()
endif()

# ---------------------------------------------------------------- the game on Strata
# Strata is fetched at FAMILIAR_STRATA_REF (the v0.7.0 tag: ABI 1.12), or
# taken from a local checkout with -DFETCHCONTENT_SOURCE_DIR_STRATA=<path>.
# The desktop game (FAMILIAR_WITH_STRATA) and the browser game (FAMILIAR_WEB)
# share everything but their main().
if(FAMILIAR_WITH_STRATA OR FAMILIAR_WEB)
  cmake_minimum_required(VERSION 3.24)
  include(FetchContent)
  set(FAMILIAR_STRATA_REF "20827c3d67e71c1f942c29e7cf8adfa4801188a8" CACHE STRING "Strata git ref (a full commit SHA)")
  set(STRATA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(STRATA_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
  set(STRATA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(STRATA_WITH_SIM OFF CACHE BOOL "" FORCE)
  if(FAMILIAR_WEB)
    set(STRATA_WITH_USD OFF CACHE BOOL "" FORCE)
  endif()
  FetchContent_Declare(strata
    GIT_REPOSITORY https://github.com/Preponderous-Software/strata.git
    GIT_TAG ${FAMILIAR_STRATA_REF})
  FetchContent_MakeAvailable(strata)

  add_library(familiar_play STATIC src/bridge/geometry.cpp src/bridge/scene.cpp src/play/session.cpp src/play/view.cpp)
  target_include_directories(familiar_play PUBLIC src)
  target_link_libraries(familiar_play PUBLIC familiar_sim familiar_save strata::static)
  familiar_warnings(familiar_play)
endif()

if(FAMILIAR_WITH_STRATA)
  add_executable(familiar app/main.cpp)
  target_link_libraries(familiar PRIVATE familiar_play)
  target_compile_definitions(familiar PRIVATE FAMILIAR_VERSION="${PROJECT_VERSION}")
  familiar_warnings(familiar)
endif()

# emcmake cmake -S . -B build-web -DFAMILIAR_WITH_STRATA=OFF -DFAMILIAR_WEB=ON:
# the page is familiar.html + .js + .wasm.
if(FAMILIAR_WEB)
  add_executable(familiar-web app/web_main.cpp)
  target_link_libraries(familiar-web PRIVATE familiar_play)
  familiar_warnings(familiar-web)
  set_target_properties(familiar-web PROPERTIES OUTPUT_NAME familiar SUFFIX ".html")
  target_link_options(familiar-web PRIVATE
    -sSTACK_SIZE=1MB
    "-sEXPORTED_FUNCTIONS=_main,_familiar_look,_familiar_stick,_familiar_press,_familiar_captured,_familiar_save,_familiar_restart,_familiar_to_doorstep,_familiar_to_spot,_familiar_to_place,_familiar_notebook,_familiar_cover,_familiar_help,_familiar_resize,_familiar_status"
    -lidbfs.js
    -sEXPORTED_RUNTIME_METHODS=ccall,cwrap,UTF8ToString,FS,IDBFS,addRunDependency,removeRunDependency
    "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  set_property(TARGET familiar-web APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/web/shell.html")
  # The loading shell (web/strata-kit, vendored from Strata) and its bridge go
  # inline into the page, with the download's size (web/strata_page.cmake).
  set_property(TARGET familiar-web APPEND PROPERTY LINK_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/web/strata-kit/strata-loader.js"
    "${CMAKE_CURRENT_SOURCE_DIR}/web/strata-boot.js")
  add_custom_command(TARGET familiar-web POST_BUILD
    COMMAND "${CMAKE_COMMAND}" "-DHTML=$<TARGET_FILE:familiar-web>"
            "-DWASM=$<TARGET_FILE_DIR:familiar-web>/$<TARGET_FILE_BASE_NAME:familiar-web>.wasm"
            "-DWEB=${CMAKE_CURRENT_SOURCE_DIR}/web" -P "${CMAKE_CURRENT_SOURCE_DIR}/web/strata_page.cmake"
    VERBATIM)
endif()
