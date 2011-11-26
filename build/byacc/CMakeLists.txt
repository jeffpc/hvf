add_definitions(-O2 -DNDEBUG)

add_executable(yacc closure.c error.c graph.c lalr.c lr0.c main.c mkpar.c
	output.c reader.c skeleton.c symtab.c verbose.c warshall.c)
