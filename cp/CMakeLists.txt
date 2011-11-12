set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DVERSION=\\\"${HVF_VERSION}\\\"")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ${PROJECT_SOURCE_DIR}/cp/include/")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${PROJECT_SOURCE_DIR}/cp/include/nucleus.h")

set(CMAKE_EXE_LINKER_FLAGS "-T ${PROJECT_SOURCE_DIR}/cp/scripts/linker-hvf.script")

set(SUBDIRS nucleus mm fs drivers shell guest)

set(SRCFILES "")
foreach(SUBDIR ${SUBDIRS})
	set(FILES "")
	include(${SUBDIR}/objs.cmake)
	foreach(FILE ${FILES})
		set(SRCFILES ${SRCFILES} ${SUBDIR}/${FILE})
	endforeach()
endforeach()

add_executable(hvf ${SRCFILES})

target_link_libraries(hvf clock)
target_link_libraries(hvf digest)
target_link_libraries(hvf string)
target_link_libraries(hvf ebcdic)
target_link_libraries(hvf errno)
