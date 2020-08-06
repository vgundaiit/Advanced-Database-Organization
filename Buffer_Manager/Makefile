CC=gcc
CFLAGS=-I.

test_1: test_assign2_1.c storage_mgr.c dberror.c

	gcc -o test_1 buffer_mgr.c buffer_mgr_stat.c test_assign2_1.c storage_mgr.c dberror.c  -I.


test_2: test_assign2_2.c storage_mgr.c dberror.c

	gcc -o test_2 buffer_mgr.c buffer_mgr_stat.c test_assign2_2.c storage_mgr.c dberror.c  -I.

clean:
	rm -rf *.o test_1 test_2

all: test_1 test_2
