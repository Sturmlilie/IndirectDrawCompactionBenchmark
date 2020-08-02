cmake_minimum_required(VERSION 3.14)
project(IndirectDrawCompactionBenchmark C)

find_package(PkgConfig REQUIRED)

pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(GLEW REQUIRED glew)

add_executable(IndirectDrawBenchmark main.c)

target_link_libraries(IndirectDrawBenchmark ${SDL2_LIBRARIES} ${GLEW_LIBRARIES})
target_include_directories(IndirectDrawBenchmark PUBLIC ${SDL2_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS})
target_compile_options(IndirectDrawBenchmark PUBLIC ${SDL2_CFLAGS_OTHER} ${GLEW_CFLAGS_OTHER})
