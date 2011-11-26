include(CheckFunctionExists)
configure_file(config.h.in config.h)

check_function_exists(getpagesize HAVE_GETPAGESIZE)
check_function_exists(memset HAVE_MEMSET)
check_function_exists(mmap HAVE_MMAP)
check_function_exists(munmap HAVE_MUNMAP)
check_function_exists(strdup HAVE_STRDUP)
check_function_exists(strndup HAVE_STRNDUP)

add_definitions(-DHAVE_CONFIG_H -I. -g -O2)

add_executable(re2c code.cc dfa.cc main.cc parser.cc actions.cc substr.cc translate.cc scanner.cc mbo_getopt.cc)
