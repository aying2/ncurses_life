cmake_minimum_required(VERSION 3.26)

project(life VERSION 1.0.0 LANGUAGES C)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(life
    life.c
)

if (MSVC)
    # warning level 4
    add_compile_options(/W4)
else()
    # additional warnings
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package( Curses REQUIRED )
include_directories( ${CURSES_INCLUDE_DIRS} )
target_link_libraries( life ${CURSES_LIBRARIES} )
