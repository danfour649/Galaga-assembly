cmake_minimum_required(VERSION 3.16)
project(galaga-assembly C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(GALAGA_IS_EMSCRIPTEN OFF)
if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
    set(GALAGA_IS_EMSCRIPTEN ON)
endif()

# --- Assembly (NASM x86-64) ---
set(GALAGA_ASM_SOURCES asm/collision.asm asm/entities.asm asm/player.asm asm/enemies.asm asm/score.asm asm/game_state.asm)
option(GALAGA_USE_ASM "Link NASM gameplay modules (auto-disabled on Emscripten)" ON)

if(GALAGA_IS_EMSCRIPTEN)
    set(GALAGA_USE_ASM OFF)
    message(STATUS "Emscripten build: C fallbacks + SDL2 port")
elseif(GALAGA_USE_ASM)
    find_program(NASM_EXECUTABLE nasm)
    if(NASM_EXECUTABLE AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|x86"))
        enable_language(ASM_NASM)
        set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -Wall -I${CMAKE_SOURCE_DIR}")
        set(GALAGA_ASM_LIB galaga_asm)
        add_library(${GALAGA_ASM_LIB} OBJECT ${GALAGA_ASM_SOURCES})
        message(STATUS "NASM found: assembly modules enabled")
    else()
        set(GALAGA_USE_ASM OFF)
        message(STATUS "NASM unavailable or non-x86 target: using C fallbacks")
    endif()
else()
    message(STATUS "GALAGA_USE_ASM=OFF: using C fallbacks")
endif()

if(NOT GALAGA_IS_EMSCRIPTEN)
    find_package(SDL2 REQUIRED)
endif()

# --- Main executable ---
set(GALAGA_C_SOURCES
    src/main.c
    src/input.c
    src/render.c
    src/game.c
    src/sprites.c
    src/effects.c
    src/highscore.c
)

if(NOT GALAGA_USE_ASM)
    list(APPEND GALAGA_C_SOURCES src/game_fallback.c)
endif()

add_executable(galaga ${GALAGA_C_SOURCES})

target_include_directories(galaga PRIVATE ${CMAKE_SOURCE_DIR}/include)

if(GALAGA_USE_ASM)
    target_sources(galaga PRIVATE $<TARGET_OBJECTS:${GALAGA_ASM_LIB}>)
    target_compile_definitions(galaga PRIVATE GALAGA_USE_ASM=1)
else()
    target_compile_definitions(galaga PRIVATE GALAGA_USE_ASM=0)
endif()

if(GALAGA_IS_EMSCRIPTEN)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    target_compile_options(galaga PRIVATE -sUSE_SDL=2)
    target_link_options(galaga PRIVATE
        -sUSE_SDL=2
        -sALLOW_MEMORY_GROWTH=1
        --shell-file "${CMAKE_SOURCE_DIR}/web/shell.html"
    )
elseif(WIN32 AND TARGET SDL2::SDL2main)
    target_link_libraries(galaga PRIVATE SDL2::SDL2main)
endif()

if(NOT GALAGA_IS_EMSCRIPTEN)
    if(TARGET SDL2::SDL2)
        target_link_libraries(galaga PRIVATE SDL2::SDL2)
    else()
        target_include_directories(galaga PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(galaga PRIVATE ${SDL2_LIBRARIES})
    endif()
endif()

if(UNIX AND NOT APPLE AND NOT GALAGA_IS_EMSCRIPTEN)
    target_link_libraries(galaga PRIVATE m)
endif()

if(WIN32 AND NOT GALAGA_IS_EMSCRIPTEN)
    # Put SDL2.dll next to galaga.exe so the build dir is runnable from
    # Explorer without MSYS2/vcpkg on PATH.
    if(TARGET SDL2::SDL2)
        add_custom_command(TARGET galaga POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:SDL2::SDL2> $<TARGET_FILE_DIR:galaga>
        )
    endif()
endif()

if(NOT GALAGA_IS_EMSCRIPTEN)
    install(TARGETS galaga RUNTIME DESTINATION bin)
endif()
