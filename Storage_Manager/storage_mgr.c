#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "storage_mgr.h"

//This metadata is to store the number of pages in the file :  Extra Feature
#define METADATAOFFSET sizeof(int)

/* Helper Function */
int checkFileExistance(const char *);

typedef struct mgmtInformation
{
    FILE* fp;
} mgmtInformation;

void initStorageManager (void)
{

    printf("Storage manager is initiated\n");

}


//Returns RC_OK on successful creation of page file else returns RC_NOT_OK. 
//RC_NOT_OK is -1 defined in dberror.h
RC createPageFile (char *fileName)
{
    FILE* filePointer = fopen(fileName,"w");

    if(filePointer != NULL)
    {

        int * val;
        int numPages = 1;
        val = &numPages;
        fwrite(val,sizeof(int),1,filePointer);
        fseek(filePointer, PAGE_SIZE + METADATAOFFSET - 1, SEEK_SET);
        fputc('\0', filePointer);
        fclose(filePointer);
        return RC_OK;
    }
    else
    {
        printf("Creation Failed");
        return RC_NOT_OK;
    }

}



RC openPageFile (char *fileName, SM_FileHandle *fHandle)
{

    /*
        Creating a pointer type variable for management information
    */

    mgmtInformation* mgmtInfo = malloc(sizeof(struct mgmtInformation));

    /*
        Check if file already exists
     */

    int fileExists = checkFileExistance(fileName);

    if(fileExists)
    {

        mgmtInfo->fp = fopen(fileName,"r+");
        fHandle->fileName = fileName;
        fHandle->mgmtInfo = mgmtInfo;

        int *buf = malloc(sizeof(int));
        fread(buf,sizeof(int),1,mgmtInfo->fp);

        fHandle->totalNumPages = *buf;
        fHandle->curPagePos = 0;

        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }
}

RC closePageFile (SM_FileHandle *fHandle)
{

    FILE *fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    int numPages = fHandle->totalNumPages;
    int *numPagesPointer = &numPages;

    fwrite(numPagesPointer,sizeof(int),1,fp);

    if(fclose(fp))
    {
        return RC_NOT_OK;
    }

    return RC_OK;
}

RC destroyPageFile (char *fileName)
{
    if(remove(fileName) == 0)
    {
        return RC_OK;
    }
    else
    {
        return RC_FILE_NOT_FOUND;
    }
}


//pageNum starts from zero
RC readBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    if(pageNum < 0 || (pageNum > fHandle->totalNumPages - 1))
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    FILE *fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    /*
        Move file pointer to the location of pageNumth starting location using fseek
        Here pageNum starts from 0
     */

    fseek(fp, PAGE_SIZE * pageNum + METADATAOFFSET, SEEK_SET);

    fHandle->curPagePos = pageNum;

    if(fread(memPage,PAGE_SIZE,1,fp))
    {
        return RC_OK;
    }

    return RC_NOT_OK;

}


RC readFirstBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    fseek(fp, METADATAOFFSET, SEEK_SET);

    fHandle->curPagePos = 0;


    if(fread(memPage,sizeof(char),PAGE_SIZE,fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;
};

RC readNextBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    int pageNum = fHandle->curPagePos;

    if((pageNum+1) >= fHandle->totalNumPages)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    fseek(fp, (pageNum + 1) * PAGE_SIZE + METADATAOFFSET, SEEK_SET);

    fHandle->curPagePos++;

    if(fread(memPage,PAGE_SIZE,1,fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;
}



RC readLastBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    fseek(fp, -PAGE_SIZE, SEEK_END);

    fHandle->curPagePos = fHandle->totalNumPages - 1;
    if(fread(memPage,sizeof(char),PAGE_SIZE,fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;

}

RC readCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    int pageNum = fHandle->curPagePos;

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    /*
        Set file pointer to the beginning of the block
     */

    fseek(fp, PAGE_SIZE * pageNum + METADATAOFFSET, SEEK_SET);

    if(fread(memPage,sizeof(char),PAGE_SIZE,fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;

}

RC readPreviousBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

    int pageNum = fHandle->curPagePos;

    if((pageNum-1) < 0 )
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    /*
        Go to the previous page's first byte
     */
    fseek(fp, (pageNum - 1) * PAGE_SIZE + METADATAOFFSET, SEEK_SET);

    fHandle->curPagePos--;

    if(fread(memPage,sizeof(char),PAGE_SIZE,fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;
}

RC writeBlock (int pageNum, SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    if(pageNum < 0 || pageNum > fHandle->totalNumPages - 1)
    {
        return RC_READ_NON_EXISTING_PAGE;
    }

    /*
        Set file pointer to the beginning of the block
     */

    fseek((((mgmtInformation*)fHandle->mgmtInfo)->fp), PAGE_SIZE * pageNum + METADATAOFFSET, SEEK_SET);

    fwrite(memPage,1,PAGE_SIZE,((mgmtInformation*)fHandle->mgmtInfo)->fp);

    /*
        Setting current page position
     */
    fHandle->curPagePos = pageNum;

    return RC_OK;
}

int getBlockPos (SM_FileHandle *fHandle)
{

    int pos = fHandle->curPagePos;

    return pos;

}



RC writeCurrentBlock (SM_FileHandle *fHandle, SM_PageHandle memPage)
{

    int pageNum = fHandle->curPagePos;

    /*
        Set file pointer to the beginning of the block
     */
    fseek((((mgmtInformation*)fHandle->mgmtInfo)->fp), PAGE_SIZE * pageNum + METADATAOFFSET, SEEK_SET);

    /*
        Setting current page position
     */

    fHandle->curPagePos = pageNum;

    if(fwrite(memPage,1,PAGE_SIZE,((mgmtInformation*)fHandle->mgmtInfo)->fp))
    {
        return RC_OK;
    };

    return RC_NOT_OK;

}

RC appendEmptyBlock (SM_FileHandle *fHandle)
{

    FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;


    fseek(fp, PAGE_SIZE * (fHandle->totalNumPages + 1) + METADATAOFFSET, SEEK_SET);

    fputc('\0', fp);

    fHandle->curPagePos = fHandle->totalNumPages;

    fHandle->totalNumPages++;

    return RC_OK;
}



RC ensureCapacity (int numberOfPages, SM_FileHandle *fHandle)
{
    int existingNumOfPages = ceil(fHandle->totalNumPages);

    if(existingNumOfPages < numberOfPages)
    {

        FILE* fp = ((mgmtInformation*)fHandle->mgmtInfo)->fp;

        fseek(fp, PAGE_SIZE * numberOfPages  + METADATAOFFSET, SEEK_SET);

        fputc('\0', fp);

        fHandle->totalNumPages = numberOfPages;
    }

    return RC_OK;
}

/*
    Helper functions definitions
 */

int checkFileExistance(const char * fileName)
{
    FILE *fP;
    if(fP = fopen(fileName,"r"))
    {
        fclose(fP);
        return 1;
    }
    return 0;
}