#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "storage_mgr.h"
#include "dberror.h"
#include "test_helper.h"

// test name
char *testName;

/* test output files */
#define TESTPF "test_pagefile.bin"

/* prototypes for test functions */
static void extraTestCases(void);
\
/* main function running all tests */
int
main (void)
{
  testName = "";

  initStorageManager();

  extraTestCases();

  return 0;
}


/* check a return code. If it is not RC_OK then output a message, error description, and exit */
/* Try to create, open, and close a page file */
void extraTestCases(void) {

  SM_FileHandle fh;
  SM_PageHandle ph;
  int i;

  testName = "Testing the contents from single page";

  ph = (SM_PageHandle) malloc(PAGE_SIZE);

  TEST_CHECK(createPageFile (TESTPF));

  TEST_CHECK(openPageFile (TESTPF, &fh));
  printf("Created and Opened a new file\n");

  // read first page into handle and the page should be empty (zero bytes)
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == 0), "expected zero byte in first page of freshly initialized page");
  printf("first block was empty\n");

  // change ph to be a string and write that one to disk
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  TEST_CHECK(writeBlock (0, &fh, ph));
  printf("writing first block\n");

  // read back the page containing the string and check that it is correct
  TEST_CHECK(readFirstBlock (&fh, ph));
  for (i=0; i < PAGE_SIZE; i++)
    ASSERT_TRUE((ph[i] == (i % 10) + '0'), "character in page read from disk is the one we expected.");
  printf("reading first block\n");

  //Appending new Block
  TEST_CHECK(appendEmptyBlock(&fh));
  ASSERT_TRUE((fh.totalNumPages==2),"It has only 2 pages after appending the new page.");

  // change ph to be a string and write that one to disk for second block
  fh.curPagePos = 1;
  for (i=0; i < PAGE_SIZE; i++)
    ph[i] = (i % 10) + '0';
  //TEST_CHECK(writeBlock (1, &fh, ph));
  TEST_CHECK(writeCurrentBlock (&fh, ph));
  printf("writing Second block\n");

  //Reading the current(Second) block
  TEST_CHECK(readCurrentBlock(&fh,ph));
  for(i=0;i<PAGE_SIZE;i++)
    ASSERT_TRUE((ph[i]==(i%10)+'0'),"character in page read from disk is the one we expected.");
  printf("reading the current(Second) block\n");

  //reading the previous Block
  TEST_CHECK(readPreviousBlock(&fh,ph));
  for(i=0;i<PAGE_SIZE;i++)
    ASSERT_TRUE((ph[i] ==(i%10) + '0'), "character in page read from disk is the one we expected");
  printf("Reading the second(previous) block\n");

  //Appending new Block
  TEST_CHECK(appendEmptyBlock(&fh));
  ASSERT_TRUE((fh.totalNumPages==3),"It has only 3 pages after appending the new page.");

  TEST_CHECK(appendEmptyBlock(&fh));
  ASSERT_TRUE((fh.totalNumPages==4),"It has only 4 pages after appending the new page.");

  //reading the next Block
   
  fh.curPagePos = 2;
  TEST_CHECK(readNextBlock(&fh,ph));
  //This page shoud be empty, as it is newly appended
  for(i=0;i<PAGE_SIZE;i++)
    ASSERT_TRUE((ph[i] == 0), "Expected 0 byte in newly appended(third) page");
  printf("Reading the next(third) block\n");

  //Ensuring the capacity.
  TEST_CHECK(ensureCapacity(6, &fh));
  printf("%d\n",fh.totalNumPages);
  ASSERT_TRUE((fh.totalNumPages == 6), "Expected 6 pages after ensuring the capacity");

  //To check whether the new pages were added after ensuring the capacity by checking the current page pos.
  ASSERT_TRUE((fh.curPagePos == 3), "After appending new block to match the range of ensure capacity, the page position is pointing to 3. pointing to the last page i.e 4 ");

  //reading the last Block
  TEST_CHECK(readLastBlock(&fh,ph));
  for(i=0 ; i < PAGE_SIZE ; i++)
    ASSERT_TRUE((ph[i] == 0), "Expected 0 bytes in the last block");
  printf("Last block, which was appended earlier was read and that was expected empty\n");


  TEST_CHECK(destroyPageFile (TESTPF));

  // after destruction trying to open the file should cause an error
  ASSERT_TRUE((openPageFile(TESTPF, &fh) != RC_OK), "opening non-existing file should return an error.");


  TEST_DONE();
}
