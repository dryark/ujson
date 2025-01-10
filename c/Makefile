all: test

test: ujsonin.h test.c red_black_tree.c sds.c string-tree.c ujsonin.c
	gcc -o test test.c red_black_tree.c sds.c string-tree.c ujsonin.c

testc: ujsonin.h test.c red_black_tree.c sds.c string-tree.c ujsonin.c
	clang -o testc test.c red_black_tree.c sds.c string-tree.c ujsonin.c

clean:
	rm test
