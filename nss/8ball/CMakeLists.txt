set(CMAKE_EXE_LINKER_FLAGS "-melf_s390 -T ${CMAKE_CURRENT_SOURCE_DIR}/linker-8ball.script")
set(CMAKE_ASM-ATT_FLAGS "-m31")

add_executable(8ball 8ball.s)
