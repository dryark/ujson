all: test

test: ujson.h test.c red_black_tree.c sds.c string-tree.c ujson.c
	gcc -o test test.c red_black_tree.c sds.c string-tree.c ujson.c

testc: ujson.h test.c red_black_tree.c sds.c string-tree.c ujson.c
	clang -o testc test.c red_black_tree.c sds.c string-tree.c ujson.c

clean:
	rm test
