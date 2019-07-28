set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os")

add_executable(installer setmode.s loader_c.c loader_asm.s cpio.c malloc.c
	edf.c)
target_link_libraries(installer string)
target_link_libraries(installer ebcdic)
set_target_properties(installer PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.script")

add_custom_command(
	OUTPUT installer.rto
	COMMAND ${CMAKE_OBJCOPY} -O binary -j .text -j .data -j .rodata -j .rodata.str1.2 installer installer.rto
	DEPENDS installer
)

add_custom_command(
	OUTPUT rdr.s
	COMMAND ../build/ccw_gen 00080000 80100000 installer.rto 100000 080000 > rdr.s
	DEPENDS installer.rto
)

add_executable(rdr rdr.s)
set_target_properties(rdr PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker-rdr.script")
add_custom_command(
	OUTPUT rdr.rto
	COMMAND ${CMAKE_OBJCOPY} -O binary -j .text -j .data -j .rodata -j .rodata.str1.2 rdr rdr.rto
	DEPENDS rdr
)

add_custom_target(installer-rto ALL DEPENDS rdr.rto installer.rto)
