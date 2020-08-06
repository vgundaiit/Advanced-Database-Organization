CC=gcc
CFLAGS=-I.

test_assign1_1: test_assign1_1.c storage_mgr.c dberror.c

	gcc -o test_assign1_1 test_assign1_1.c storage_mgr.c dberror.c -I.


test_assign1_2: test_assign1_2.c storage_mgr.c dberror.c

	gcc -o test_assign1_2 test_assign1_2.c storage_mgr.c dberror.c -I.

clean:
	rm -rf *.o test_assign1_1 test_assign1_2

all: test_assign1_1 test_assign1_2
