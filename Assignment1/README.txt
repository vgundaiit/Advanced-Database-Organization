Assignment 1 - Storage Manager
------------------------------------------------------------------
Submitted by Group No. 16

1. Deepa Bada (dbada@hawk.iit.edu)
2. Krishnachaitanya Kilari (kkilari@hawk.iit.edu)
3. Varun Gunda (vgunda2@hawk.iit.edu)

------------------------------------------------------------------
The following files were there in this assignment :  
1. storage_mgr.c
2. storage_mgr.h
3. test_assign1_1.c
4. test_assign1_2.c
5. test_helper.h
6. dberror.c
7. dberror.h
8. Makefile
9. README

---------------------------------Makefile--------------------------

To run the files, you can execute any one of the below commands:
1. make 
2. make all

The above commands create an output file "test_assign1_1" and "test_assign1_2". This can be then executed using "./test_1" and "./test_2"

To clean the output files created use the command:"make clean"

---------------------------------About test Cases--------------------------

About Test Cases:

We have implemented one function named extraTestCases() in the file test_assign1_2.c which contain the extra test cases for the methods that were not tested in the default test case file, and the functions that are tested in the new test cases are :
readBlock()
readFirstBlock() 
readCurrentBlock()
readPreviousBlock()
readNextBlock()
readLastBlock()
writeBlock ()
writeCurrentBlock()
appendEmptyBlock()
ensureCapacity ()

--------------------------------storage_mgr.c---------------------------------
The implementation of each function in storage_mgr.c is described as follows and then these are the interfaces that are declared in storage_mgr.h :

void initStorageManager (): 
	As of now,there is no need to initialize any variable in "init" block. But, We can declare all the global variables that can be used in our project to this block.

Function Description :

1. Functions to manipulate page file: 

 (a). createPageFile (): 
	A new file is created in write mode. An empty block is written with '\0' bytes..

 (b). openPageFile ():
	Checks if the input file is present. If file exists,then opens the existing file or else throws an error RC_FILE_NOT_FOUND.If file exists,Open the file in read+write mode and then we are updating the file handle with file name, total number of pages and current page position.
	
 (c). closePageFile ():
	Close the file using fclose()

 (d). destroyPageFile ():
	Delete the file from the directory handled by file handler.

2. Read Functions:

 (a). readBlock ():
	Checks if the page exists using the totalNumPages In case the page exists, seek the file pointer until that page using built-in function "fseek". Read the file into PageHandler from the seeked location for the next 4096 bytes using built-in function "fread".In case the page does not exists return "NON_EXISTING_PAGE" error

 (b). getBlockPos ():
	returns the current block position using curPagePos variable of the file handler.

 (c). readFirstBlock ():
	function reads the first block by sending fseek to the position of first byte of the first page.

 (d). readPreviousBlock ():
	function reads the previous block by sending fseek to the position of first byte of the previous page. Current page position is found using file handle's currPagePos. This function returns RC_READ_NON_EXISTING_PAGE error if the previous page number is less than zero. 
	
 (e). readCurrentBlock ():
	Calls the readBlock function with pageNum=curPagePos

 (f). readNextBlock ():
	function reads the next block by sending fseek to the position of first byte of the next page. Current page position is found using file handle's currPagePos. This function returns RC_READ_NON_EXISTING_PAGE error if the previous page number is greater than the existing number of pages.

 (g). readLastBlock ():
	finding the last block using in-built function called 'fseek' and then Calls the readBlock function.

3. Write Functions:


 (a). writeBlock ():
	Checks if the page number is properly existing. In case the above condition is true, seek the file pointer until that page using built-in function "fseek". Write the data from the PageHandler to the seeked location for the next 4096 bytes using built-in function "fwrite".In case the page does not exists return "NON-EXISTING PAGE" error.

 (b). writeCurrentBlock ():
	Seek the file pointer until that page using built-in function "fseek".Then increment the file pointer and Write the data from the PageHandler to the seeked location for the next 4096 bytes using built-in function "fwrite".

 (c). appendEmptyBlock ():
	Seek the pointer to the end using 'fseek' and add new block using 'fputc' and increment the file handle's total number of pages. 

 (d). ensureCapacity ():
	Check the capacity and the existing pages, if they are less, do the same process as like appendEmptyBlock and reach the capacity. 


In all the above functions, file handles properties like currPagePos and totalNumOfPages etc., are updated as when required.


--------------------------------------About Metadata (Extra)--------------------------------------------------------------------------
Metadata is used to store the number of pages in the file. In the pagefile, the first few bytes defined by sizeof(int) are used to store the number of pages in the current file. This value is initialized when the file is created and added a page and later on this will be updated whenever you make any changes like appending pages to the file and close the file. The functionality of updating the number of pages is added in closePageFile function.
