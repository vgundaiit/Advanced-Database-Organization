#include <stdio.h>
#include "buffer_mgr.h"
#include "btree_mgr.h"
#include "storage_mgr.h"

//Struct to hold a key and right and left pointers
typedef struct keyData{
    int key;
    float left;
    float right;
}keyData;

//Struct to hold a page data
typedef struct pageData{
    //Meta data
    int pageNumber;
    int parentNode;
    int numEntries;
    int leaf;

    //Keys and Pointers : Every page has n entries and n+1 pointers
    int *keys;
    float *pointers; //Selecting float for pointer cause we may have 4.1 etc., at leaf nodes, pointing to the records
}pageData;

//Holds content of page 0;
typedef struct fileMetaData{
    int rootPageNumber;
    int maxEntriesPerPage;
    int numNodes; ///Number of pages minus 1 since page 0 will be used for metadata
    int numEntries; //Stores total number of entries in this index file
    int keyType; //Optional for this assignment cause we are only dealing with integer kind of keys
}fileMetaData;

//Struct to hold book keeping (mgmt data)
typedef struct treeMgmtData{
    fileMetaData fmd;
    //Pointers
    BM_BufferPool* bm;
    BM_PageHandle* ph;
    SM_FileHandle fh;
}treeMgmtData;

typedef struct scanMgmtData{
    int *leafPages; //0 th entry in this and the below one matches
    //Adding -1 to the last of the leaf pages array will help in detecting 
    //if we ran out of pages
    //int *numEntriesInEachPage; 
    int currentPage;//Page Number of current page
    int currentPosInPage;
    int isCurrentPageLoaded;
    int nextPagePosInLeafPages;
    int numOfLeafPages;
    pageData currentPageData;
}scanMgmtData;

//Functions to read page data from a page
RC readPageData(BM_BufferPool* bm,BM_PageHandle* ph,pageData* pd,int pageNumber);
RC readFileMetaData(BM_BufferPool* bm,BM_PageHandle* ph,fileMetaData* fmd,int pageNumber);

//Functions to write page data from a page
RC writePageData(BM_BufferPool* bm,BM_PageHandle* ph,char* content,int pageNumber);
RC prepareWritablePageData(pageData* pd,char* content);
RC prepareWritableMetaData(fileMetaData* fmd,char* content);

//Function to split nodes and return left and right childs
//Input: The node to be split, which is just page data
//Output: tow page datas which represet left page and right page
pageData findPageToInsertNewEntry(BM_BufferPool*,BM_PageHandle*,pageData,int);
//int is the key, left and right are children of the key
RC splitNode(pageData* old,pageData* newLeft,pageData *newRight);
RC addNewKeyAndPointerToLeaf(pageData*,int,RID);
RC addNewKeyAndPointerToNonLeaf(pageData*,keyData);

//Last 3: page number, key, left page number, right page number
RC insertPropagateUp(BTreeHandle*,int,keyData);
RC updateParentInDownChildNodes(BTreeHandle*,pageData);

//Delete won't be required that much as testing is not done for random deletes
RC deleteEntryFromNode(pageData* oldPageData, pageData* newPageData);

//Scanning
RC findLeafPageNumOfKey(pageData* root);
RC findLeafPages(pageData,BM_BufferPool*,BM_PageHandle*,int*);

//Helper functions  
int getDataBeforeSeparatorForInt(char**, char);
float getDataBeforeSeparatorForFloat(char**,char);
RC keyPointerFormattedData(pageData*, char *);

RC allocateSpaceForData(char **);
RC deallocateSpace(char **);

void printNodeContent(BM_BufferPool *bm, BM_PageHandle* ph, pageData root);