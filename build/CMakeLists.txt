cmake_minimum_required(VERSION 2.6)

add_executable(ccw_gen ccw_gen.c)
add_executable(padcat padcat.c)
add_executable(mkarchive mkarchive.c)

add_subdirectory(byacc)
add_subdirectory(re2c)
