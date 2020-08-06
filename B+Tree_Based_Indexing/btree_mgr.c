#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "btree_mgr.h"
#include "buffer_mgr.h"
#include "buffer_mgr_stat.h"
#include "dberror.h"
#include "storage_mgr.h"
#include "btree_helper.h"

//Global Btree Manager
BTreeHandle* treeHandle; 
treeMgmtData* btreeMgmtData;
BT_ScanHandle* sHandle;
scanMgmtData* sMgmtData;
static int count = 0;

// init and shutdown index manager
RC initIndexManager (void *mgmtData){

    printf("\n================Index Manager is Initialized===============\n");

    return RC_OK;
}

//Free up all resources. May be call deleteBtree method
RC shutdownIndexManager (){
    return RC_OK;
}

// create a Btree with the name idxId, for the key of type keyType and
//max number of records that can fit in a note is given by n
RC createBtree (char *idxId, DataType keyType, int n){

    treeHandle = (BTreeHandle*)malloc(sizeof(BTreeHandle));
    btreeMgmtData = (treeMgmtData*)malloc(sizeof(treeMgmtData));
    sHandle = (BT_ScanHandle*)malloc(sizeof(BT_ScanHandle));

    btreeMgmtData->bm = MAKE_POOL();
    btreeMgmtData->ph = MAKE_PAGE_HANDLE();
    treeHandle->mgmtData = btreeMgmtData;

    sMgmtData = (scanMgmtData*)malloc(sizeof(scanMgmtData));
    sHandle->mgmtData = sMgmtData;


    //Create a page file with the given idxId name
    createPageFile(idxId);

    //Open the given page file and assign it to fh
    int returnVal = openPageFile (idxId, &(btreeMgmtData->fh));
    
    btreeMgmtData->fmd.rootPageNumber = 1;
    btreeMgmtData->fmd.maxEntriesPerPage = n;
    btreeMgmtData->fmd.numEntries = 0;
    btreeMgmtData->fmd.numNodes = 1; 

    initBufferPool(btreeMgmtData->bm,idxId,10,RS_FIFO,NULL);

    //Ensure that there are two pages in the pagefile
    ensureCapacity (2, &(btreeMgmtData->fh));

    char *dataString;

    allocateSpaceForData(&dataString);
    //Prepare writable meta data for the root
    prepareWritableMetaData(&(btreeMgmtData->fmd),dataString);
    //Update metadata in page 0
    writePageData(btreeMgmtData->bm,btreeMgmtData->ph,dataString,0);
    deallocateSpace(&dataString);

    allocateSpaceForData(&dataString);
    //Prepare writable data for page 1
    //Create data for the page
    pageData root;
    root.leaf = 1;
    root.numEntries = 0;
    root.parentNode = -1;
    root.pageNumber = 1;
    allocateSpaceForData(&dataString);
    prepareWritablePageData(&root,dataString);
    //Update data in page 1
    writePageData(btreeMgmtData->bm,btreeMgmtData->ph,dataString,btreeMgmtData->fmd.rootPageNumber);
    deallocateSpace(&dataString);

    //shut down the buffer pool
    shutdownBufferPool(btreeMgmtData->bm);

    //close the page file
    return RC_OK;
}

RC openBtree (BTreeHandle **tree, char *idxId){
    
    //Initialize storage manager and buffer manager
    //Open the given page file and assign it to fh
    int returnVal = openPageFile (idxId, &(btreeMgmtData->fh));
    
    btreeMgmtData->bm = MAKE_POOL();
    btreeMgmtData->ph = MAKE_PAGE_HANDLE();
    initBufferPool(btreeMgmtData->bm,idxId,10,RS_FIFO,NULL);

    //Read file meta data from page 0

    fileMetaData fmd;
    readFileMetaData(btreeMgmtData->bm,btreeMgmtData->ph,&fmd,0);

    //Fill Btree managers data
    treeHandle->idxId = idxId;
    treeHandle->keyType = fmd.keyType;

    btreeMgmtData->fmd.numNodes = fmd.numNodes;
    btreeMgmtData->fmd.maxEntriesPerPage = fmd.maxEntriesPerPage;
    btreeMgmtData->fmd.rootPageNumber = fmd.rootPageNumber;
    btreeMgmtData->fmd.numEntries = fmd.numEntries;

    treeHandle->mgmtData = btreeMgmtData;

    *tree = treeHandle;

    return RC_OK;
}

//Write all pages to disk
RC closeBtree (BTreeHandle *tree){
    //int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;

    
    //Update Page Metadata with new root number , new number of nodes and new number of entries
    //maxEntriesPerPage and KeyType being the same
    
    char *dataString;

    allocateSpaceForData(&dataString);
    //Prepare writable meta data for the root
    prepareWritableMetaData(&(btreeMgmtData->fmd),dataString);
    //Update metadata in page 0
    writePageData(btreeMgmtData->bm,btreeMgmtData->ph,dataString,0);
    deallocateSpace(&dataString);

    shutdownBufferPool(bm);
    free(btreeMgmtData->bm);
    free(btreeMgmtData->ph);
    free(tree->mgmtData);
    free(tree);
    return RC_OK;
    

}

RC deleteBtree (char *idxId){

    if(remove(idxId) == 0)
    {
        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }


}

// access information about a b-tree
RC getNumNodes (BTreeHandle *tree, int *result){
     
    *result = ((treeMgmtData*)tree->mgmtData)->fmd.numNodes;
    return RC_OK;
    
}

RC getNumEntries (BTreeHandle *tree, int *result){
    // *result = ((btreeMgmtData*)tree->mgmtData)->totalNumEntries;
    *result = ((treeMgmtData*)tree->mgmtData)->fmd.numEntries;
    return RC_OK;
}

RC getKeyType (BTreeHandle *tree, DataType *result){
    *result = ((treeMgmtData*)tree->mgmtData)->fmd.keyType;
    return RC_OK;
}

// index access
//Search operation!
//Even this should be iterative search
RC findKey (BTreeHandle *tree, Value *key, RID *result){
    int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;
    
    //Read root page data
    pageData rootPageData;
    readPageData(bm,ph,&rootPageData,rootPageNumber);

    pageData leafPageData = findPageToInsertNewEntry(bm,ph,rootPageData,key->v.intV);

    for (size_t i = 0; i < leafPageData.numEntries; i++)
    {
        if(leafPageData.keys[i] == key->v.intV){
            //Return corresponding child
            float child = leafPageData.pointers[i];
            int childValue = round(child*10);
            int slot =  childValue % 10;
            int pageNum =  childValue / 10;
            result->page = pageNum;
            result->slot = slot;
            return RC_OK;
        }
    }
    return RC_IM_KEY_NOT_FOUND;
}

RC deleteKeyAndPointerFromLeaf(pageData* page,int key){
    int newKeys[5] = {0};
    float newChildren[5] = {0};
    int i = 0;
    int count = 0;
    int found = 0;
    while(i < page->numEntries){
        if(key == page->keys[i] && i < page->numEntries){
            i++;
            found = 1;
            continue;
        }
        newKeys[count] = page->keys[i];
        newChildren[count] = page->pointers[i];
        i++;
        count++;
    }
    if(!found){
        return RC_IM_KEY_NOT_FOUND;
    }
    newChildren[count] = -1;
    //free(page->keys);
    //free(page->children);
    page->numEntries -= 1;
    for (size_t i = 0; i < count; i++)
    {
        page->keys[i] = newKeys[i];
        page->pointers[i] = newChildren[i];
    }
    page->pointers[count] = newChildren[count];
    return RC_OK;
    
}

RC insertKey (BTreeHandle *tree, Value *key, RID rid){
    int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    int maxEntries = ((treeMgmtData*)tree->mgmtData)->fmd.maxEntriesPerPage;
    
    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;
    
    int currentNumberOfNodes = ((treeMgmtData*)tree->mgmtData)->fmd.numNodes;

    //Read root page data
    pageData rootPageData;
    readPageData(bm,ph,&rootPageData,rootPageNumber);

    //Find the correct page/node to insert this btree_mgr.c
    pageData pageToInsert;
    pageToInsert = findPageToInsertNewEntry(bm,ph,rootPageData,key->v.intV);

    //Now that you found the leaf page to insert
    //First Insert key and pointer (rid) in the leaf node in sorted fashion
    int success = addNewKeyAndPointerToLeaf(&pageToInsert,key->v.intV,rid);
    if(success == RC_IM_KEY_ALREADY_EXISTS){
        return RC_IM_KEY_ALREADY_EXISTS;
    }

    //Then see if the leaf node is having max entries + 1, 
    if(pageToInsert.numEntries > maxEntries){
        //If so, split it and paste the content to two new page data
        //Keys For New Node => Keys for right child & Old Node => Left Child
        //Setting Up New Node First
        int *keysForNewNode = (int *)malloc(10*sizeof(int));
        float *childrenForNewNode = (float *)malloc(10*sizeof(float));
        int count = 0;
        for (size_t i = (int)ceil((pageToInsert.numEntries)/2)+1; i < pageToInsert.numEntries; i++)
        {
            keysForNewNode[count] = pageToInsert.keys[i];
            childrenForNewNode[count] = pageToInsert.pointers[i];
            count++;
        }
        childrenForNewNode[count] = -1;

        int *keysForOldNode = (int *)malloc(10*sizeof(int));
        float *childrenForOldNode = (float *)malloc(10*sizeof(float));
        count = 0;
        for (size_t i = 0; i <= (int)ceil((pageToInsert.numEntries)/2); i++)
        {
            keysForOldNode[count] = pageToInsert.keys[i];
            childrenForOldNode[count] = pageToInsert.pointers[i];
            count++;
        }
        childrenForOldNode[count] = -1;

        ensureCapacity (currentNumberOfNodes + 2, &fh);

        currentNumberOfNodes += 1;

        ((treeMgmtData*)tree->mgmtData)->fmd.numNodes++;

        //Data for right child
        pageData pRightChild;
        pRightChild.leaf = 1;
        pRightChild.pageNumber = currentNumberOfNodes;
        pRightChild.numEntries = (int)floor((maxEntries+1)/2);
        pRightChild.keys = keysForNewNode;
        pRightChild.pointers = childrenForNewNode;
        if(pageToInsert.parentNode == -1)
        {
            pRightChild.parentNode = 3;
        }
        else{
            pRightChild.parentNode = pageToInsert.parentNode;
        }
        

        //Update right child
        char *dataString;
        allocateSpaceForData(&dataString);
        prepareWritablePageData(&pRightChild,dataString);
        writePageData(bm,ph,dataString,pRightChild.pageNumber);
        deallocateSpace(&dataString);

        //Data for left child
        pageData pLeftChild;
        pLeftChild.leaf = 1;
        pLeftChild.pageNumber = pageToInsert.pageNumber;
        pLeftChild.numEntries = (int)ceil((maxEntries+1)/2)+1;
        pLeftChild.keys = keysForOldNode;
        pLeftChild.pointers = childrenForOldNode;
        //pLeftChild.parentNode = pageToInsert.parentNode;
        //pLeftChild.parentNode = 3;
        if(pageToInsert.parentNode == -1)
        {
            pLeftChild.parentNode = 3;
        }
        else{
            pLeftChild.parentNode = pageToInsert.parentNode;
        }

        //Update left child
        allocateSpaceForData(&dataString);
        prepareWritablePageData(&pLeftChild,dataString);
        writePageData(bm,ph,dataString,pLeftChild.pageNumber);
        deallocateSpace(&dataString);

        int pagenumber = pageToInsert.parentNode;
        float left = pLeftChild.pageNumber;
        float right = pRightChild.pageNumber;

        keyData kd;
        kd.key = pRightChild.keys[0];
        kd.left = left;
        kd.right = right;

        //Call the propagate up function now
        insertPropagateUp(tree,pagenumber,kd);

    }
    else{
        char *dataString = malloc(500);
        allocateSpaceForData(&dataString);
        prepareWritablePageData(&pageToInsert,dataString);
        writePageData(bm,ph,dataString,pageToInsert.pageNumber);
        deallocateSpace(&dataString);
    }

    ((treeMgmtData*)tree->mgmtData)->fmd.numEntries++;

    forceFlushPool(bm);

    return RC_OK;
    
}



RC readFileMetaData(BM_BufferPool* bm,BM_PageHandle* ph,fileMetaData* fmd,int pageNumber){
    //Read the index metadata from page 0 
    pinPage(bm,ph,pageNumber);

    char *itr = ph->data;

    itr++; //Skips the first $
    //Read the data between first $ and next $ : root node's page number
    fmd->rootPageNumber = getDataBeforeSeparatorForInt(&itr,'$');

    itr++; //Skips the next $
    fmd->numNodes = getDataBeforeSeparatorForInt(&itr,'$');

    itr++; //Skips the next $
    fmd->numEntries = getDataBeforeSeparatorForInt(&itr,'$');

    itr++; //Skips the next $
    fmd->maxEntriesPerPage = getDataBeforeSeparatorForInt(&itr,'$');

    itr++; //Skips the next $
    fmd->keyType = getDataBeforeSeparatorForInt(&itr,'$');

    unpinPage(bm,ph);

    return RC_OK;
}


RC readPageData(BM_BufferPool* bm,BM_PageHandle* ph,pageData* pd,int pageNumber){
    //Read the index metadata from page 0 
    pinPage(bm,ph,pageNumber);

    char *itr = ph->data;
    itr++;//Skips the first dollar sign
    pd->leaf = getDataBeforeSeparatorForInt(&itr,'$');
    itr++;
    pd->numEntries = getDataBeforeSeparatorForInt(&itr,'$');
    itr++;
    pd->parentNode = getDataBeforeSeparatorForInt(&itr,'$');
    itr++;
    pd->pageNumber = getDataBeforeSeparatorForInt(&itr,'$');
    itr++;
    int i = 0;
    int *keys = malloc(pd->numEntries * sizeof(int));
    //float so that value can be 1 or 1.4
    float *children = malloc((pd->numEntries +1) * sizeof(float));

    //There will be numEntries Keys and numEntries + 1 children
    if(pd->numEntries>0){
        while(i < pd->numEntries){
            children[i] = getDataBeforeSeparatorForFloat(&itr,'$');
            itr++;
            keys[i]  = getDataBeforeSeparatorForInt(&itr,'$');
            itr++;
            i++;
        }
        //Pointer to the next page is -1 for the root
        children[i] = getDataBeforeSeparatorForFloat(&itr,'$');
    }

    pd->keys = keys;
    pd->pointers = children;
    unpinPage(bm,ph);

    return RC_OK;
}

RC writePageData(BM_BufferPool* bm,BM_PageHandle* ph,char* content,int pageNumber){
    pinPage(bm,ph,pageNumber);
    memset(ph->data,'\0',100);
    sprintf(ph->data,"%s",content);
    markDirty(bm,ph);
    unpinPage(bm,ph); 
}  


RC deleteKey (BTreeHandle *tree, Value *key){
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    //Read root page data
    pageData rootPageData;
    readPageData(bm,ph,&rootPageData,rootPageNumber);

    pageData pd  = findPageToInsertNewEntry(bm,ph,rootPageData,key->v.intV);

    //Update the page by deleting the passed key
    int success = deleteKeyAndPointerFromLeaf(&pd,key->v.intV);

    if(success == RC_IM_KEY_NOT_FOUND){
        return RC_IM_KEY_NOT_FOUND;
    }

    char *dataString;
    allocateSpaceForData(&dataString);
    prepareWritablePageData(&pd,dataString);
    writePageData(bm,ph,dataString,pd.pageNumber);
    deallocateSpace(&dataString);

    return RC_OK;
}


RC openTreeScan (BTreeHandle *tree, BT_ScanHandle **handle){
    sHandle = (BT_ScanHandle*)malloc(sizeof(BT_ScanHandle));

    sMgmtData = (scanMgmtData*)malloc(sizeof(scanMgmtData));

    int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    
    //Read root page data
    pageData rootPageData;
    readPageData(bm,ph,&rootPageData,rootPageNumber);

    //Perform Depth first search to find the leaf nodes    
    //100 default but we can assume there will be at most numNodes nodes
    int *leafPageNumbers = (int *)malloc(100*sizeof(int));
    // int *count = malloc(sizeof(int));
    // *count = 0 ;   
    count = 0; //Make static count to 0 again
    findLeafPages(rootPageData,bm,ph,leafPageNumbers);

    sMgmtData->leafPages = leafPageNumbers;
    sMgmtData->currentPage = leafPageNumbers[0];    
    pageData leafPage;
    readPageData(bm,ph,&leafPage,sMgmtData->currentPage);
    sMgmtData->currentPosInPage = 0;
    sMgmtData->isCurrentPageLoaded = 1;
    sMgmtData->nextPagePosInLeafPages = 1;
    sMgmtData->currentPageData = leafPage;
    sMgmtData->numOfLeafPages = count;

    sHandle->mgmtData = sMgmtData;
    sHandle->tree = tree;

    *handle = sHandle;
    return RC_OK;
}



RC findLeafPages(pageData root,BM_BufferPool* bm,BM_PageHandle* ph,int* leafPages){
    //Base case : leaf node
    if(root.leaf){
        leafPages[count] = root.pageNumber;
        count += 1;
        return RC_OK;
    }
    else{
        //For each child of root, call the function findLeafPages
        for (size_t i = 0; i < root.numEntries + 1; i++)
        {
            pageData child;
            readPageData(bm,ph,&child,(int)root.pointers[i]);
            if(findLeafPages(child,bm,ph,leafPages) == RC_OK){
                continue;
            }
        }
    }
    return RC_OK;
}

RC nextEntry (BT_ScanHandle *handle, RID *result){

    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)handle->tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)handle->tree->mgmtData)->ph;
    
    scanMgmtData* smdata = handle->mgmtData;

    //Check if the current position is beyond the number of entries in that page

    if(smdata->currentPosInPage >= smdata->currentPageData.numEntries){
        if(smdata->nextPagePosInLeafPages == -1 ){
            return RC_IM_NO_MORE_ENTRIES;
        }
        //Move to the next page
        smdata->currentPage = smdata->leafPages[smdata->nextPagePosInLeafPages];
        smdata->isCurrentPageLoaded = 0;
        smdata->nextPagePosInLeafPages += 1;

        if(smdata->nextPagePosInLeafPages >= smdata->numOfLeafPages){
            smdata->nextPagePosInLeafPages = -1;
        }
    }

    if(!smdata->isCurrentPageLoaded){
        //Load the current page
        pageData leafPage;
        readPageData(bm,ph,&leafPage,smdata->currentPage);
        smdata->currentPageData = leafPage;
        smdata->currentPosInPage = 0;
        smdata->isCurrentPageLoaded = 1;
        //smdata->nextPagePosInLeafPages += 1;     
    }
    float child = smdata->currentPageData.pointers[smdata->currentPosInPage];
    int childValue = round(child*10);
    result->slot =  childValue % 10;
    result->page =  childValue / 10; 
    smdata->currentPosInPage += 1;
    return RC_OK;
}

RC closeTreeScan (BT_ScanHandle *handle){
    //free(handle);
    free(handle->mgmtData);
    free(handle);
    handle = NULL;
    return RC_OK;
}

// debug and test functions
char *printTree (BTreeHandle *tree){
    int rootPageNumber = ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber;

    int maxEntries = ((treeMgmtData*)tree->mgmtData)->fmd.maxEntriesPerPage;
    
    //Load the root page into buffer manager
    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;
    
    int currentNumberOfNodes = ((treeMgmtData*)tree->mgmtData)->fmd.numNodes;

    //Read root page data
    pageData rootPageData;
    readPageData(bm,ph,&rootPageData,rootPageNumber);

    printNodeContent(bm,ph,rootPageData);

    return "abc";

}

void printNodeContent(BM_BufferPool *bm, BM_PageHandle* ph, pageData root){
    if(root.leaf){
        char *data;
        allocateSpaceForData(&data);
        prepareWritablePageData(&root,data);
        printf("\n%s\n",data);
        deallocateSpace(&data);
        return;
    }
    else{
        for (size_t i = 0; i < root.numEntries + 1 ; i++)
        {
            /* code */
            if(root.pointers[i] != -1){
                pageData pageData;
                pageData.pageNumber = (int)root.pointers[i];
                int pageNumber = pageData.pageNumber;
                readPageData(bm,ph,&pageData,pageNumber);
                printNodeContent(bm,ph,pageData);
            }
            return;
        }   
    }
}

//type = 0 for int and 1 for float
int getDataBeforeSeparatorForInt(char **itr, char sep){
    char *value = (char *)malloc(100);
    char *tempitr = *itr;
    memset(value,'\0',sizeof(value));
    int i = 0;
    while(*tempitr!=sep){
        value[i] = *tempitr;
        tempitr++;
        i++;
    }
    *itr = tempitr;
    int value1 = atoi(value);
    free(value);
    return value1;
}

float getDataBeforeSeparatorForFloat(char **itr, char sep){
    char *value = (char *)malloc(100);
    char *tempitr = *itr;
    memset(value,'\0',sizeof(value));
    int i = 0;
    while(*tempitr!=sep){
        value[i] = *tempitr;
        tempitr++;
        i++;
    }
    *itr = tempitr;
    float value1 = atof(value);
    free(value);
    return value1;

}


RC prepareWritableMetaData(fileMetaData* fmd,char* content){
    //Create metadata for this index tree
    sprintf (content,"$%d$%d$%d$%d$%d$",fmd->rootPageNumber,fmd->numNodes,fmd->numEntries,fmd->maxEntriesPerPage,fmd->keyType);
};

RC prepareWritablePageData(pageData* pd,char* content){
    sprintf (content,"$%d$%d$%d$%d$",pd->leaf,pd->numEntries,pd->parentNode,pd->pageNumber);
    //Code to print keys and pointers to content
    if(pd->numEntries > 0){
        char* keyPointerData;
        allocateSpaceForData(&keyPointerData);
        keyPointerFormattedData(pd,keyPointerData);
        sprintf (content+strlen(content),"%s",keyPointerData);
        deallocateSpace(&keyPointerData);
    }
};

RC keyPointerFormattedData(pageData* pd, char* data){
    char *itr = data;
    int i = 0;
    
    for (i = 0; i < pd->numEntries; i++)
    {   
        int childValue = round(pd->pointers[i]*10);
        int slot =  childValue % 10;
        int pageNum =  childValue / 10;
        sprintf(itr,"%d.%d$",pageNum,slot);
        itr += 4;
        sprintf(itr,"%d$",pd->keys[i]);
        if(pd->keys[i] >=10){
            itr += 3;
        }
        else{
            itr += 2;
        }
    }
    sprintf(itr,"%0.1f$",pd->pointers[i]);
}

RC allocateSpaceForData(char **data){
    *data = (char*)malloc(50 * sizeof(char)) ; 
    memset(*data,'\0',50);
}

RC deallocateSpace(char **data){
    free(*data);
}


pageData findPageToInsertNewEntry(BM_BufferPool* bm,BM_PageHandle* ph,pageData root,int key){
    if(root.leaf){
        return root;
    }
    else{
        //Iterate through root's entries to find the right page in the next level
        if(key < root.keys[0]){
            //Create page data for the new file which is pointed by pointer 0 of the root
            pageData pageToSearchIn;
            int pageToSearchInNumber = round(root.pointers[0]*10) / 10;
            readPageData(bm,ph,&pageToSearchIn,pageToSearchInNumber);
            return findPageToInsertNewEntry(bm,ph,pageToSearchIn,key);

        }
        else{
            int pageFound = 0;
            for (size_t i = 0; i < root.numEntries - 1; i++)
            {
                if(key >= root.keys[i] && key < root.keys[i+1] ){
                    //Create page data for the new file which is pointed by pointer 0 of the root
                    pageFound = 1;
                    pageData pageToSearchIn;
                    int pageToSearchInNumber = round(root.pointers[i+1]*10) / 10;
                    readPageData(bm,ph,&pageToSearchIn,pageToSearchInNumber);
                    return findPageToInsertNewEntry(bm,ph,pageToSearchIn,key);

                }
            }
            if(!pageFound){
                pageData pageToSearchIn;
                int pageToSearchInNumber = round(root.pointers[root.numEntries]*10) / 10;
                readPageData(bm,ph,&pageToSearchIn,pageToSearchInNumber);
                return findPageToInsertNewEntry(bm,ph,pageToSearchIn,key);
            }
        }
    }
}

RC addNewKeyAndPointerToNonLeaf(pageData* page,keyData kd){
    int *newKeys = (int*)malloc(sizeof(int)*10);
    float *newChildren = (float*)malloc(sizeof(int)*10);
    int i = 0;
    while(kd.key > page->keys[i] && i < page->numEntries){
        newKeys[i] = page->keys[i];
        newChildren[i] = page->pointers[i];
        i++;
    }
    if(kd.key == page->keys[i] && i < page->numEntries){
        return RC_IM_KEY_ALREADY_EXISTS;
    }
    else{
        newKeys[i] = kd.key;
        //float childpointer = rid.page+rid.slot*0.1;
        newChildren[i] = kd.left;
        newChildren[i+1] = kd.right;
        i++;
        newKeys[i] = page->keys[i-1];
        i++;
        
    }
    while(i < page->numEntries + 2){
        newKeys[i] = page->keys[i-1];
        newChildren[i] = page->pointers[i-1];
        i++;
    }
    newChildren[i] = -1;
    free(page->keys);
    free(page->pointers);
    page->keys = newKeys;
    page->pointers = newChildren;
    page->numEntries++;
    return RC_OK;
}

RC addNewKeyAndPointerToLeaf(pageData* page,int key,RID rid){
    int *newKeys = (int*)malloc(sizeof(int)*10);
    float *newChildren = (float*)malloc(sizeof(int)*10);
    int i = 0;
    while(key > page->keys[i] && i < page->numEntries){
        newKeys[i] = page->keys[i];
        newChildren[i] = page->pointers[i];
        i++;
    }
    if(key == page->keys[i] && i < page->numEntries){
        return RC_IM_KEY_ALREADY_EXISTS;
    }
    else{
        newKeys[i] = key;
        float childpointer = rid.page+rid.slot*0.1;
        newChildren[i] = childpointer;
        i++;
    }
    while(i < page->numEntries + 1){
        newKeys[i] = page->keys[i-1];
        newChildren[i] = page->pointers[i-1];
        i++;
    }
    newChildren[i] = -1;
    free(page->keys);
    free(page->pointers);
    page->keys = newKeys;
    page->pointers = newChildren;
    page->numEntries++;
    return RC_OK;
}

RC updateParentInDownChildNodes(BTreeHandle* tree,pageData node){

    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;

    int numEntries = node.numEntries;
    //There will be node.numEntries+1 number of children to be updated
    for (size_t i = 0; i < node.numEntries+1; i++)
    {
        pageData child;
        readPageData(bm,ph,&child,node.pointers[i]);

        //Update the parent
        child.parentNode = node.pageNumber;

        char *dataString;
        allocateSpaceForData(&dataString);
        prepareWritablePageData(&child,dataString);
        writePageData(bm,ph,dataString,child.pageNumber);
        deallocateSpace(&dataString);

    }
    
    return RC_OK;

};

RC insertPropagateUp(BTreeHandle *tree,int pageNumber,keyData kd){

    int maxEntries = ((treeMgmtData*)tree->mgmtData)->fmd.maxEntriesPerPage;

    BM_BufferPool *bm = ((treeMgmtData*)tree->mgmtData)->bm;
    BM_PageHandle *ph = ((treeMgmtData*)tree->mgmtData)->ph;
    SM_FileHandle fh = ((treeMgmtData*)tree->mgmtData)->fh;
    
    int currentNumberOfNodes = ((treeMgmtData*)tree->mgmtData)->fmd.numNodes;

    //Check if parent page number is -1. In this case, 
    //just create a new node and make it the root
    if(pageNumber == -1){
        int currentNumberOfNodes = ((treeMgmtData*)tree->mgmtData)->fmd.numNodes;
        ensureCapacity(currentNumberOfNodes+2,&fh);

        //new Node which will be made the root
        pageData newNode;
        newNode.pageNumber = currentNumberOfNodes+1;

        int *keysForNewNode = (int *)malloc(10*sizeof(int));
        keysForNewNode[0] = kd.key;

        float *childrenForNewNode = (float *)malloc(10*sizeof(float));
        childrenForNewNode[0] = kd.left;
        childrenForNewNode[1] = kd.right;

        newNode.keys = keysForNewNode;
        newNode.pointers= childrenForNewNode;
        newNode.numEntries = 1;
        newNode.parentNode = -1;
        newNode.leaf = 0;

        ((treeMgmtData*)tree->mgmtData)->fmd.numNodes++;
        ((treeMgmtData*)tree->mgmtData)->fmd.rootPageNumber = newNode.pageNumber;

        char *dataString;// = malloc(500);
        allocateSpaceForData(&dataString);
        prepareWritablePageData(&newNode,dataString);
        writePageData(bm,ph,dataString,newNode.pageNumber);
        deallocateSpace(&dataString);

        //Update parent of each of the children of this new node
        updateParentInDownChildNodes(tree,newNode);

    }
    else{
        //Either the page is not full or full
        //Either of the cases add the new key and pointer to the existing pagedata
        pageData pageToInsert;
        readPageData(bm,ph,&pageToInsert,pageNumber);
        int success = addNewKeyAndPointerToNonLeaf(&pageToInsert,kd);
        
        //Then see if the leaf node is having max entries + 1, 
        if(pageToInsert.numEntries > maxEntries){
            //If so, split it and paste the content to two new page data
            //Keys For New Node => Keys for right child & Old Node => Left Child
            //Setting Up New Node First

            int *keysForOldNode = (int *)malloc(10*sizeof(int));
            float *childrenForOldNode = (float *)malloc(10*sizeof(float));
            int count = 0;
            for (size_t i = 0; i < (int)ceil((pageToInsert.numEntries)/2); i++)
            {
                keysForOldNode[count] = pageToInsert.keys[i];
                childrenForOldNode[count] = pageToInsert.pointers[i];
                count++;
            }
            childrenForOldNode[count] = pageToInsert.pointers[count];
            count++;

            int *keysForNewNode = (int *)malloc(10*sizeof(int));
            float *childrenForNewNode = (float *)malloc(10*sizeof(float));
            int count2 = 0;
            for (size_t i = count; i < pageToInsert.numEntries + 2; i++)
            {
                keysForNewNode[count2] = pageToInsert.keys[i];
                childrenForNewNode[count2] = pageToInsert.pointers[i];
                count2++;
            }
            childrenForNewNode[count2] = pageToInsert.pointers[count];

            ensureCapacity (currentNumberOfNodes + 2, &fh);

            currentNumberOfNodes += 1;

            ((treeMgmtData*)tree->mgmtData)->fmd.numNodes++;

            //Data for right child
            pageData pRightChild;
            pRightChild.leaf = 0;
            pRightChild.pageNumber = currentNumberOfNodes;
            pRightChild.numEntries = (int)floor((maxEntries+1)/2);
            pRightChild.keys = keysForNewNode;
            pRightChild.pointers = childrenForNewNode;
            pRightChild.parentNode = pageToInsert.parentNode;

            //Update right child
            char *dataString;
            allocateSpaceForData(&dataString);
            prepareWritablePageData(&pRightChild,dataString);
            writePageData(bm,ph,dataString,pRightChild.pageNumber);
            deallocateSpace(&dataString);

            //Data for left child
            pageData pLeftChild;
            pLeftChild.leaf = 0;
            pLeftChild.pageNumber = pageToInsert.pageNumber;
            pLeftChild.numEntries = (int)floor((maxEntries+1)/2);
            pLeftChild.keys = keysForOldNode;
            pLeftChild.pointers = childrenForOldNode;
            pLeftChild.parentNode = pageToInsert.parentNode;

            //Update left child
            allocateSpaceForData(&dataString);
            prepareWritablePageData(&pLeftChild,dataString);
            writePageData(bm,ph,dataString,pLeftChild.pageNumber);
            deallocateSpace(&dataString);

            int pagenumber = pageToInsert.parentNode;
            float left = pLeftChild.pageNumber;
            float right = pRightChild.pageNumber;

            keyData kd;
            kd.key = pageToInsert.keys[(int)ceil((maxEntries+1)/2)];
            kd.left = left;
            kd.right = right;

            //Update parent of each of the children of this new node
            updateParentInDownChildNodes(tree,pRightChild);

            //Call the propagate up function now
            insertPropagateUp(tree,pagenumber,kd);

        }
        else{
            char *dataString;
            allocateSpaceForData(&dataString);
            prepareWritablePageData(&pageToInsert,dataString);
            writePageData(bm,ph,dataString,pageToInsert.pageNumber);
            deallocateSpace(&dataString);
            return RC_OK;   
        }
    }
}