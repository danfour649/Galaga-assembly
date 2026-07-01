cmake_minimum_required(VERSION 3.16)
project(galaga-assembly C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(SDL2 REQUIRED)

# --- Assembly (NASM x86-64) ---
set(GALAGA_ASM_SOURCES asm/collision.asm asm/entities.asm asm/player.asm)
set(GALAGA_USE_ASM 1)

find_program(NASM_EXECUTABLE nasm)
if(NASM_EXECUTABLE AND (CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|i686|x86"))
    enable_language(ASM_NASM)
    set(CMAKE_ASM_NASM_FLAGS "${CMAKE_ASM_NASM_FLAGS} -Wall -I${CMAKE_SOURCE_DIR}")
    set(GALAGA_ASM_LIB galaga_asm)
    add_library(${GALAGA_ASM_LIB} OBJECT ${GALAGA_ASM_SOURCES})
    message(STATUS "NASM found: assembly modules enabled")
else()
    set(GALAGA_USE_ASM 0)
    message(STATUS "NASM unavailable or non-x86 target: using C fallbacks")
endif()

# --- Main executable ---
set(GALAGA_C_SOURCES
    src/main.c
    src/input.c
    src/render.c
    src/game.c
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

if(TARGET SDL2::SDL2)
    target_link_libraries(galaga PRIVATE SDL2::SDL2)
else()
    target_include_directories(galaga PRIVATE ${SDL2_INCLUDE_DIRS})
    target_link_libraries(galaga PRIVATE ${SDL2_LIBRARIES})
endif()

if(UNIX AND NOT APPLE)
    target_link_libraries(galaga PRIVATE m)
endif()

install(TARGETS galaga RUNTIME DESTINATION bin)
