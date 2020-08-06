CC=gcc
CFLAGS= -lm -I.

test_assign4_1: btree_mgr.c  buffer_mgr.c  buffer_mgr_stat.c  dberror.c  rm_serializer.c  storage_mgr.c  test_assign4_1.c

	gcc -o test_assign4_1 btree_mgr.c  buffer_mgr.c  buffer_mgr_stat.c  dberror.c  rm_serializer.c  storage_mgr.c  test_assign4_1.c -lm -I.

extra: btree_mgr.c  buffer_mgr.c  buffer_mgr_stat.c  dberror.c  rm_serializer.c  storage_mgr.c  extra_tests.c

	gcc -o extra_tests btree_mgr.c  buffer_mgr.c  buffer_mgr_stat.c  dberror.c  rm_serializer.c  storage_mgr.c  extra_tests.c -lm -I.

clean:
	rm -rf *.o test_assign4_1 extra_tests

all: test_assign4_1 extra