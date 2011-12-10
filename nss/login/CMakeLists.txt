set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ${CMAKE_CURRENT_SOURCE_DIR}")

set(CMAKE_EXE_LINKER_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.script")

add_executable(login entry.s psa.s int.s die.s main.c io.c mch.c prg.c)
