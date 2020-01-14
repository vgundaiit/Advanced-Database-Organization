#include <stdio.h>
#include <stdlib.h>
#include <time.h> 
#include <sys/time.h> 
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include "dberror.h"
#include "buffer_mgr.h"
#include "storage_mgr.h"
#include "buffer_mgr_stat.h"
#include "storage_mgr.h"

//This holds information about page frame loaded into buffer pool
typedef struct pageTableEntry{
    int pinCounter;
    long timeStamp; 
    BM_PageHandle pageHandleInMemory;
    bool dirty;
    SM_PageHandle data; 
}pageTableEntry;

//This stores information about the entire page table and others like number of read/write operations 
//and number of active pages in buffer pool
typedef struct pageTableData{
    pageTableEntry* pageTable;
    int numberOfPagesInBufferPool;
    int numberOfWriteOperations;
    int numberOfReadOperations;
    SM_FileHandle fh;
}pageTableData;

//This function gets current time in microseconds
long getTime(){
    struct timeval tv;
    gettimeofday(&tv,NULL);
    long time_in_micros = 1000000 * tv.tv_sec + tv.tv_usec;
    return time_in_micros;                
}

RC initBufferPool(BM_BufferPool *const bm, const char *const pageFileName,const int numPages, ReplacementStrategy strategy,void *stratData){
    
    //Initiate Storage Manager
    initStorageManager();

    bm->pageFile = NULL;

    //Initialize page table data 
    pageTableData* pTableData = malloc(sizeof(pageTableData));

    pTableData->numberOfPagesInBufferPool = 0;
    pTableData->numberOfReadOperations = 0;
    pTableData->numberOfWriteOperations = 0;

    //Open the given page file
    int returnVal = openPageFile (pageFileName, &(pTableData->fh));

    //Return if the page file is invalid
    if(returnVal != RC_OK){
        return RC_NOT_OK;
    }
    
    //Initialize buffer manager
    bm->pageFile = pageFileName;
    bm->numPages = numPages;
    bm->strategy = strategy;

    //Create array of page table entries of size buffer pool size
    pTableData->pageTable = malloc(numPages * sizeof(pageTableEntry));
    
    for (size_t i = 0; i < numPages; i++)
    {
        /* Initialize page table entries with default values */
        pTableData->pageTable[i].dirty = false;
        pTableData->pageTable[i].pinCounter = 0;
        pTableData->pageTable[i].timeStamp = -1;
        pTableData->pageTable[i].pageHandleInMemory.pageNum = NO_PAGE;
        pTableData->pageTable[i].pageHandleInMemory.data = NULL;
    }
    
    bm->mgmtData = pTableData;
    
    printf("\n=====================================\nBuffer Pool is Initialized\n========================================\n");
    return RC_OK;
}

//Shut down the buffer pool
RC shutdownBufferPool(BM_BufferPool *const bm){

    //Check if invalid pagefile is passed     
    if(bm->pageFile == NULL){
        return RC_NOT_OK;
    }
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;
    
    pageTableEntry* pageTable = pTableData->pageTable;
    
    //Write the contents in the buffer pool back to hard disk
    forceFlushPool(bm);

    //Free each page table entry's page memory
    for(PageNumber i=0;i< bm->numPages;i++){
        char *d = pageTable[i].pageHandleInMemory.data;
        pageTable[i].pageHandleInMemory.data = NULL;
        free(d);
    }
    
    //free page table
    free(pageTable);
    
    //free page table data
    free(pTableData);

    return RC_OK;
}

RC forceFlushPool(BM_BufferPool *const bm){
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;
    
    pageTableEntry* pageTable = pTableData->pageTable;

    SM_FileHandle fh;

    if(bm->pageFile == NULL){
        return RC_NOT_OK;
    }

    for (PageNumber i = 0; i < bm->numPages; i++)
    {   
        //Check if dirty bit is set and pin count = 0
        if(pageTable[i].dirty && pageTable[i].pinCounter == 0){
            //Make sure the pagefile has enough capacity
            ensureCapacity(pageTable[i].pageHandleInMemory.pageNum+1,&(pTableData->fh));
            //Write to page file on disk
            int returnVal = writeBlock(pageTable[i].pageHandleInMemory.pageNum,&(pTableData->fh),pageTable[i].pageHandleInMemory.data);
            pTableData->numberOfWriteOperations++; //Increment the number of write operations
            if(returnVal != RC_OK){
                return returnVal;
            }
            pageTable[i].dirty = false; //reset the dirty bit
        }
    }

    return RC_OK;
}

// pin the page to buffer pool based on the strategy
RC pinPage (BM_BufferPool *const bm, BM_PageHandle *const page, const PageNumber pageNum){
    
    //check for the cases where invalid pagefie or negative pagenumber of invalid pagehandle is passed
    if(bm->pageFile == NULL){
        return RC_NOT_OK;
    }

    if(pageNum < 0){
        return RC_NOT_OK;
    }

    if(page == NULL){
        return RC_NOT_OK;
    }

    pageTableData* pTableData = (pageTableData*)bm->mgmtData;
    
    pageTableEntry* pageTable = pTableData->pageTable;

    //Check if the page requested is already in the pagetable.
    for (size_t i = 0; i < bm->numPages; i++)
    {
        
        if(pageTable[i].pageHandleInMemory.pageNum == pageNum){
            page->pageNum = pageNum;
            page->data = pageTable[i].pageHandleInMemory.data;
            pageTable[i].pinCounter++;
            //If the strategy is LRU, update the timestamp of the page frame
            if(bm->strategy == RS_LRU){
               pageTable[i].timeStamp = getTime(); 
            }
            return RC_OK;
        }
    }

    //Else Load the pageNum page in the page file into one of the page frames
    //Before strating loading process, check if all the pages have fix count zero. Return error in that case
    int numOfPagesInUse = 0;
    //Check if the page requested is already in the pagetable.
    for (size_t i = 0; i < bm->numPages; i++)
    {
        if(pageTable[i].pinCounter != 0){
            numOfPagesInUse++;
        }
    }
    //Return error if all pages have pin count>0
    if(numOfPagesInUse == bm->numPages){
        return RC_NOT_OK;
    }
    
    
    SM_FileHandle fh;

    SM_PageHandle ph = malloc(PAGE_SIZE*sizeof(char));

    //Read the given page (based on the page number) in the page file
    int returnVal = readBlock(pageNum, &(pTableData->fh),ph);    
    pTableData->numberOfReadOperations++;
	
    
    //TO DO: Load this into the existing buffer pool using some strategy 
    //Create a new entry in the page table
    //Check if the new entry fits within the size of the page table
    //Also update the time stamp in the page table with the time at which this is opened
    //Update mgmtdata(page table) with this newly pinned page
    //Close the file after using

    //Check the replacement strategy for the given buffer pool
    switch (bm->strategy)
    {
        case RS_FIFO:
            //Check if the page table is full. 
            if(pTableData->numberOfPagesInBufferPool >= bm->numPages){
                //Delete the page frame that came in first but does not have any active clients i.e., pinCounter == 0
     	        
                PageNumber indexOfEarliestPage = -1;
                long timeStampOfEarliestPage = LONG_MAX;

                for (size_t i = 0; i < bm->numPages; i++)
                {
                    if(pageTable[i].pinCounter == 0 && pageTable[i].timeStamp < timeStampOfEarliestPage){
                        //set the index of the earliest page 
                        indexOfEarliestPage = i;
                        //Update time stamp of earliest page with this time stamp
                        timeStampOfEarliestPage = pageTable[i].timeStamp;
                    }
                }

                //Delete the content of the earlist page from page table entry
                //Before that Check the dirty bit of the page to be removed
                //Write content back to the disk if dirty bit is set
                if(pageTable[indexOfEarliestPage].dirty == true){
                    //write the content back to the disk
                    ensureCapacity(pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum+1,&(pTableData->fh));
                    writeBlock(pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum,&(pTableData->fh),pageTable[indexOfEarliestPage].pageHandleInMemory.data);
                    pTableData->numberOfWriteOperations++;
                }
                //Now set the properties of this removed page's page table entry to default values
                pageTable[indexOfEarliestPage].dirty = false;
                pageTable[indexOfEarliestPage].pinCounter = 0;
                pageTable[indexOfEarliestPage].timeStamp = -1;
                pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum = NO_PAGE;
                free(pageTable[indexOfEarliestPage].pageHandleInMemory.data);
                pageTable[indexOfEarliestPage].pageHandleInMemory.data = NULL; 
                pTableData->numberOfPagesInBufferPool--;
            }

            //Add the page frame in the first available slot (which can also be the freshly deleted entry in the above if condition)
            for (PageNumber i = 0; i < bm->numPages; i++)
            {
                /* First available slot is determined by iterating
                through the page table and finding the first entry
                that has pageHandleInmemory.pageNum = -1 which implies 
                that frame is not having any page*/
                if(pageTable[i].pageHandleInMemory.pageNum == NO_PAGE){
                    //Add details of loaded page into the page table
                    pageTable[i].pageHandleInMemory.pageNum = pageNum;
                    
                    pageTable[i].pageHandleInMemory.data = ph;
                    
                    pageTable[i].dirty = FALSE;
                    pageTable[i].timeStamp = getTime();
                    pageTable[i].pinCounter++;
                    
                    //Increment the number of active pages in page table
                    pTableData->numberOfPagesInBufferPool++;

                    //Update the given page with the data read from the page file
                    page->data = pageTable[i].pageHandleInMemory.data;
                    page->pageNum = pageNum;

                    //Now exit this for loop
                    break;
                }
            }
            break;
        case RS_LRU:
                //Check if the buffer pool is full. If so, delete a page frame using LRU strategy
	            if(pTableData->numberOfPagesInBufferPool >= bm->numPages){
                    PageNumber indexOfEarliestPage = -1;
					long timeStampOfEarliestPage = LONG_MAX;

					for (size_t i = 0; i < bm->numPages; i++)
					{
						if(pageTable[i].pinCounter == 0 && pageTable[i].timeStamp < timeStampOfEarliestPage){
                            //set the index of the earliest page with pin count 0
							indexOfEarliestPage = i;
                            //Update time stamp with this new time stamp
							timeStampOfEarliestPage = pageTable[i].timeStamp;
						}
					}

                    //Write content of the page to be removed to the disk if its dirty bit is set           
					if(pageTable[indexOfEarliestPage].dirty == true){
						ensureCapacity(pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum+1,&(pTableData->fh));
						writeBlock(pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum,&(pTableData->fh),pageTable[indexOfEarliestPage].pageHandleInMemory.data);
						pTableData->numberOfWriteOperations++;
					}
               
					pageTable[indexOfEarliestPage].dirty = false;
					pageTable[indexOfEarliestPage].pinCounter = 0;
					pageTable[indexOfEarliestPage].timeStamp = -1;
					pageTable[indexOfEarliestPage].pageHandleInMemory.pageNum = NO_PAGE;
					free(pageTable[indexOfEarliestPage].pageHandleInMemory.data);
					pageTable[indexOfEarliestPage].pageHandleInMemory.data = NULL; 
                    pTableData->numberOfPagesInBufferPool--;
				}
                for (PageNumber i = 0; i < bm->numPages; i++)
                {
                    /* First available slot is determined by iterating
                    through the page table and finding the first entry
                    that has pageHandleInmemory.pageNum = -1 which implies
                    that frame is not having any page*/
                    if(pageTable[i].pageHandleInMemory.pageNum == NO_PAGE){
                        //Add details of loaded page into the page table
                        pageTable[i].pageHandleInMemory.pageNum = pageNum;

                        pageTable[i].pageHandleInMemory.data = ph;

                        pageTable[i].dirty = FALSE;
                        pageTable[i].timeStamp = getTime();
                        pageTable[i].pinCounter++;

                        //Increment the number of active pages in page table
                        pTableData->numberOfPagesInBufferPool++;

                        //Update the given page with the data read from the page file
                        page->data = pageTable[i].pageHandleInMemory.data;
                        page->pageNum = pageNum;

                        //Now exit this loop
                        break;
                    }
				}


            break;
        default:
            break;
    }

    if(returnVal == RC_NOT_OK){
        return RC_NOT_OK;
    }

    return RC_OK;
}

RC unpinPage (BM_BufferPool *const bm, BM_PageHandle *const page){
    //Get the page number of the page in pageFile
    PageNumber pageNum = page->pageNum; 
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    //Find the page in the page table by iterating through page table array
    for (size_t i = 0; i < bm->numPages; i++)
    {
        if(pageTable[i].pageHandleInMemory.pageNum == pageNum){
            pageTable[i].pinCounter--;
            return RC_OK;
        }
    
    }

    return RC_NOT_OK;
}



RC forcePage (BM_BufferPool *const bm, BM_PageHandle *const page){

    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    PageNumber pageNum = page->pageNum;

    SM_FileHandle fh;

    //Find the page in the page table by iterating through page table array
    for (size_t i = 0; i < bm->numPages; i++)
    {
        if(pageTable[i].pageHandleInMemory.pageNum == pageNum){
            writeBlock(pageNum,&(pTableData->fh),page->data); 
            pTableData->numberOfWriteOperations++;   
            return RC_OK;
        }
    
    }

    return RC_NOT_OK;
}

RC markDirty (BM_BufferPool *const bm, BM_PageHandle *const page){
    
    //Get the page number of the page in pageFile
    PageNumber pageNum = page->pageNum; 
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    //Find the page in the page table by iterating through page table array
    for (size_t i = 0; i < bm->numPages; i++)
    {
        if(pageTable[i].pageHandleInMemory.pageNum == pageNum){
            pageTable[i].dirty = true;
            return RC_OK;
        }
    }

    return RC_NOT_OK;
    
}

// Statistics Interface
// Get Page Frame contents by iterating through page table
PageNumber *getFrameContents (BM_BufferPool *const bm){
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    PageNumber* pnum = malloc(bm->numPages * sizeof(int));

    for (size_t i = 0; i < bm->numPages; i++)
    {
        pnum[i] = pageTable[i].pageHandleInMemory.pageNum;
    }

    return pnum;
}

// Get Dirty flags by iterating through page table 
bool *getDirtyFlags (BM_BufferPool *const bm){
    
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    bool* bools = malloc(bm->numPages * sizeof(int));

    for (size_t i = 0; i < bm->numPages; i++)
    {
        bools[i] = pageTable[i].dirty;
    }

    return bools;
}

// Get fix counts by iterating through page table
int *getFixCounts (BM_BufferPool *const bm){

    pageTableData* pTableData = (pageTableData*)bm->mgmtData;

    pageTableEntry* pageTable = pTableData->pageTable;

    int* fixes = malloc(bm->numPages * sizeof(int));

    for (size_t i = 0; i < bm->numPages; i++)
    {
        fixes[i] = pageTable[i].pinCounter;
    }

    return fixes;

}

int getNumReadIO (BM_BufferPool *const bm){
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;
    return pTableData->numberOfReadOperations;
}

int getNumWriteIO (BM_BufferPool *const bm){
    pageTableData* pTableData = (pageTableData*)bm->mgmtData;
    return pTableData->numberOfWriteOperations;
}