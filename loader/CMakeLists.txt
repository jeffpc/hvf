add_executable(loader setmode.s loader_c.c loader_asm.s)
set_target_properties(loader PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker.script")

add_custom_command(
	OUTPUT loader.rto
	COMMAND ${CMAKE_OBJCOPY} -O binary -j .text -j .data -j .rodata -j .rodata.str1.2 loader loader.rto
	DEPENDS loader
)

add_executable(eckd eckd.s)
set_target_properties(eckd PROPERTIES LINK_FLAGS "-T ${CMAKE_CURRENT_SOURCE_DIR}/linker-eckd.script")

add_custom_command(
	OUTPUT eckd.rto
	COMMAND ${CMAKE_OBJCOPY} -O binary -j .text -j .data -j .rodata -j .rodata.str1.2 eckd eckd.rto
	DEPENDS eckd
)

add_custom_target(loader-rto ALL DEPENDS eckd.rto loader.rto)
