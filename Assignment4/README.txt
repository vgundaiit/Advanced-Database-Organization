Assignment 4 - B tree
------------------------------------------------------------------
Submitted by Group No. 16

1. Deepa Bada (dbada@hawk.iit.edu)
2. Krishnachaitanya Kilari (kkilari@hawk.iit.edu)
3. Varun Gunda (vgunda2@hawk.iit.edu)

------------------------------------------------------------------
The following files were there in this assignment : 
1. btree_helper.h
2. btree_mgr.c
3. btree_mgr.h 
4. buffer_mgr.c
5. buffer_mgr.h
6. storage_mgr.c
7. storage_mgr.h
8. buffer_mgr_stat.c
9. buffer_mgr_stat.h
10. dt.h
11. record_mgr.c
12. record_mgr.h
13. test_assign4_1.c
14. test_helper.h
15. dberror.c
16. dberror.h
17. Makefile
18. README
19. extra_tests

---------------------------------Makefile--------------------------

To create the executables, you can execute the below command:
1. make all

The above command creates output files "test_assign4_1" and "extra_tests". This can be then executed using "./test_assign4_1"  and "./extra_tests"

To clean the output files created use the command:"make clean"

--------------------------------Helper Header file------------------------
btree_helper.h is created to define the set of functions that are implented in this assignment. Functions defined in this header file are:
a) readPageData & readFileMetaData: These functions read page data and meta data of the
	index page file
b) writePageData, prepareWritablePageData, prepareWritableMetaData: These functions are used 
	to write data to index pagefile. Last 2 function are helper functions that create the writable 
	content
c) findPageToInsertNewEntry: This finds the right leaf page insert the new entry
d) splitNode: This splits the given node into two nodes
e) addNewKeyAndPointerToLeaf & addNewKeyAndPointerToNonLeaf : These add keys and pointers to
	leaf and non leaf nodes respectively
f) insertPropagateUp & updateParentInDownChildNodes: These are used in insertion algorithm. These help in propagating
	insertion effect upstream
g) deleteEntryFromNode: This deletes a key and pointer from a node
h) findLeafPageNumOfKey: This finds the leaf page number of given key
i) findLeafPages: This finds the leaf page numbers
j) allocateSpaceForData & deallocateSpace: These are used to allocate and deallocate memory
k) keyPointerFormattedData: This formats key and pointer values to writable to page form
l) getDataBeforeSeparatorForFloat, getDataBeforeSeparatorForInt : Helper functions
	that give data before the separators


---------------------------------Funtions Implemented-------------------

Table and manager:
a) initIndexManager(void *mgmtData):
	- In this method, we are Initializing the Indexes to perform our b-tree operations in further.

b) shutdownIndexManager():
	- This method is used to kill the Index manager that is initiated earlier.

c) createBtree(char *idxId, DataType keyType, int n):
	- This method is used to create a Btree to store the Index, key type and the empty space in it.

d) openBtree(BTreeHandle **tree, char *idxId):
	- This method plays a role in initializing the btree to fetch the data and it then reads the schema from the file that is fetched.

e) closeBtree(BTreeHandle *tree):
	- It is used to close the btree and note it if there is any key accessed from it.

f) deleteBtree(char *idxId):
	- This method is used to delete the Btree in the page, provided if the deleteKey is returned the code RC_OK . If not, the tree will not be deleted.

Accessing record about the b-tree:
a) getNumNodes(BTreeHandle *tree, int *result):
	- In this method, it is helpful to access the count of nodes in the give b-tree.

b) getNumEntries (BTreeHandle *tree, int *result):
	- This method is used to get the count of entries in the given b-tree by using the method openBtree to open the binary tree.

c) getKeyType (BTreeHandle *tree, DataType *result):
	- This method is used to get the type of the key as the key values were in different datatypes for leaf and non-leaf nodes.

Index Access :
a) findKey (BTreeHandle *tree, Value *key, RID *result):
	- This method is used to find the key in the given binary tree by using the helper funcitons from the b-tree helper file and thus Key and recordId is returned.

b) insertKey (BTreeHandle *tree, Value *key, RID rid);
	- This methd is used to insert the key in the given b-tree provided the key is passed with this method. It takes help of the b-tree handle to insert the key in b-tree.

c) deleteKey (BTreeHandle *tree, Value *key);
	- This method is used to delete a key from the given b-tree provided the key for the particular page is returned with this method and thus the key is removed from tree.

d) openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle):
	- This method is used to open the b-tree that is passed to this method and gets the tree scanned by using the datastructures BT_ScanHandle and BTreeHandle.

e) nextEntry (BT_ScanHandle *handle, RID *result):
	- This method helps in finding the next entry in the tree by taking the use of BT_ScanHandle and thus returns the result out of this method by RID.

f) closeTreeScan (BT_ScanHandle *handle):
	- This method is called when the tree scanning needs to be closed in proper by returning the values to the record and thus the scanning is performed by using one of the datastructures BT_ScanHandle.

-----------------DATA STRUCTURES USED IN RECORD MANAGER------------------------------------------------------------------

1. BTreeHandle - Comprises information about the datatype of the keys from the different nodes of the tree, it also has the page index amnd management data accessed.

2. fileMetaData- Comprises information about the root page number of the given binary tree. It also comprises of maximum entries per page and count of nodes in the tree and then the entries in the tree.

3. ScanMgmtData - This data structure helps in scanning the pages and getting information about the current page, current position in page, To get to know whether the current page is loaded, also to calculate the num of leaf pages in the treeconditin for scanning, number of matched records etc.,

4. pageData - This will hold information related to a page in index pagefile

5. keyData - This will be used to store key and pointers.
--------------------------------------------------------------------------------------------------------------------------

------------------------Extra tests------------------------------------------------------------
The file extra_tests.c has extra tests to test the B Tree manager. These tests test the B+ tree manager
functions with higher node capacity.
