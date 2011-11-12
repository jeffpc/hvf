cmake_minimum_required(VERSION 2.8)

# we need to compile the build tools
include(ExternalProject)
ExternalProject_add(build
	DOWNLOAD_COMMAND ""
	SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/build
	CONFIGURE_COMMAND cmake <SOURCE_DIR>
	INSTALL_COMMAND ""
	BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/build
)

include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME Linux)
enable_language(ASM-ATT)

set(CMAKE_LINKER "s390x-linux-ld")
set(CMAKE_OBJCOPY "s390x-linux-objcopy")
set(CMAKE_ASM "s390x-linux-as")
CMAKE_FORCE_C_COMPILER(s390x-linux-gcc GNU)
set(CMAKE_ASM-ATT_COMPILE_OBJECT
	"${CMAKE_ASM} ${ASM_SYS_FLAGS} <FLAGS> -o <OBJECT> <SOURCE>")

set(CMAKE_C_FLAGS "-g -fno-strict-aliasing -fno-builtin -nostdlib -nostdinc -Wall -m64 -I ${PROJECT_SOURCE_DIR}/include/ -include ${PROJECT_SOURCE_DIR}/include/types.h -O2")

set(HVF_VERSION "0.16-rc3")

add_subdirectory(lib)
add_subdirectory(cp)
add_subdirectory(nss)
add_subdirectory(loader)
add_subdirectory(installer)

add_custom_command(
	OUTPUT installer/archive.cpio
	COMMAND ${CMAKE_CURRENT_BINARY_DIR}/build/mkarchive
		${CMAKE_CURRENT_SOURCE_DIR}/cp/config/hvf.directory text 80
		${CMAKE_CURRENT_SOURCE_DIR}/cp/config/system.config text 80
		${CMAKE_CURRENT_SOURCE_DIR}/cp/config/local-3215.txt text 80
		${CMAKE_CURRENT_BINARY_DIR}/cp/hvf bin
		${CMAKE_CURRENT_BINARY_DIR}/loader/eckd.rto bin
		${CMAKE_CURRENT_BINARY_DIR}/loader/loader.rto bin
		${CMAKE_CURRENT_SOURCE_DIR}/doc/installed_files.txt text 80
		${CMAKE_CURRENT_BINARY_DIR}/nss/8ball/8ball bin
		> installer/archive.cpio
	DEPENDS hvf 8ball loader-rto
)
add_custom_command(
	OUTPUT installer.bin
	COMMAND ./build/padcat installer/rdr.rto installer/installer.rto installer/archive.cpio > installer.bin
	DEPENDS installer-rto installer/archive.cpio
)

add_custom_target(distro ALL DEPENDS installer.bin)
