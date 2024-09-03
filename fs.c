#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "fs.h"

#define NUM_OF_DIRENT_PER_BLOCK_INFO 25

FileDesc pFileDesc[MAX_FD_ENTRY_MAX];
FileSysInfo* pFileSysInfo;

void Sync() {
    BufSync();
}

void    InitDirEntry(DirEntryInfo* dirEntry) {

    for(int i=0; i<NUM_OF_DIRENT_PER_BLOCK_INFO; i++) {
        strcpy(dirEntry[i].name, "NULL");
    }
}

int     parsePath(const char* szDirName, char pathNames[100][MAX_NAME_LEN])  
{

    char* str = strdup(szDirName);
    if (strcmp(str, "/") == 0) return 0;
    char* token = strtok(str, "/");
    int cnt = 0;

    while (token != NULL) {

        strncpy(pathNames[cnt], token, MAX_NAME_LEN - 1);
        pathNames[cnt++][MAX_NAME_LEN - 1] = '\0';
        token = strtok(NULL, "/");
    }

    free(str);

    return cnt;
}

int     CreateFile(const char* szFileName)
{   

    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szFileName, pathNames);

    int curInodeNum = 0,  dirFound = 0, freeInodeNum, freeBlockNum = 0;
    int parentInodeNum = 0;

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    char fileName[1200];
    fileName[0] = '\0';
    for(int i=0; i<pathCnt - 1; i++){
        strcat(fileName, "/");
        strcat(fileName, pathNames[i]);
    }
    MakeDir(fileName);

    for(int i=0; i<pathCnt - 1; i++) {

        dirFound = 0;
        GetInode(curInodeNum, pInode);
        for(int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            if (dirFound == 1){
                break;
            }
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);
            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
                
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    curInodeNum = dirEntry[k].inodeNum;
                    dirFound = 1;
                    break;
                }
            }
        }
        /*
        if (dirFound != 1) { 

            if (dirFound != 1) { 
                char fileName[1200] ='\0';
                for(int m=0; m<= i; m++){
                    strcat(fileName, "/");
                    strcat(fileName, pathNames[m]);
                }
                MakeDir(fileName);
            }
        }*/
    }

    dirFound = 0;
    GetInode(curInodeNum, pInode);
    
    for(int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
        
        if (pInode->dirBlockPtr[j] == 0 || dirFound == 1) break;
        BufRead(pInode->dirBlockPtr[j], (char*)dirEntry); 

        for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            if (strcmp(dirEntry[k].name, "NULL") == 0) {
                
                freeInodeNum = GetFreeInodeNum();
                dirEntry[k].inodeNum = freeInodeNum;
                SetInodeBitmap(dirEntry[k].inodeNum);
                strcpy(dirEntry[k].name, pathNames[pathCnt - 1]);
                dirEntry[k].type = FILE_TYPE_FILE;
                BufWrite(pInode->dirBlockPtr[j], (char*)dirEntry);
                GetInode(dirEntry[k].inodeNum, pInode);

                pInode->allocBlocks = 0;
               
                pInode->type= FILE_TYPE_FILE;
                pInode->size = 0;
                //int freeBlockNum = GetFreeBlockNum();
                //pInode->dirBlockPtr[0] = freeBlockNum;
                //SetBlockBitmap(freeBlockNum);
                
                for(int i=1; i<NUM_OF_DIRECT_BLOCK_PTR; i++) {
                    pInode->dirBlockPtr[i] = 0;
                }
                
                PutInode(dirEntry[k].inodeNum, pInode);

                BufRead(0, (char*)pFileSysInfo);
                pFileSysInfo->numAllocInodes++;
                BufWrite(0, (char*)pFileSysInfo);

                // (6)
                int desIndex = 0;
                for(int i=0; i<MAX_FD_ENTRY_MAX; i++){

                    if (pFileDesc[i].bUsed == 0) {
                        desIndex = i;
                        pFileDesc[i].pOpenFile = (File*)malloc(sizeof(File));
                        pFileDesc[i].pOpenFile->fileOffset = 0;
                        pFileDesc[i].pOpenFile->inodeNum = freeInodeNum;

                        pFileDesc[i].bUsed = TRUE;

                        free(pInode);
                        free(dirEntry);
                        
                        return desIndex;
                    } 
                }

                dirFound = 1;
                break;
            }
        }
    }
    dirFound = 1;

    free(pInode);
    free(dirEntry);
    
    return -1;
}

int     OpenFile(const char* szFileName)
{

    int fd = -1, inode = 0;

    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szFileName, pathNames);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            int flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);
            int savedInode = pInode->dirBlockPtr[j];

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
                
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    inode = dirEntry[k].inodeNum;

                    flag = 1;
                    break;
                }
            }

            if (flag == 1) break;
        }
    }

    for(int i=0; i<MAX_FD_ENTRY_MAX; i++){

        if (pFileDesc[i].bUsed == 0) {
            fd = i;
            pFileDesc[i].pOpenFile = (File*)malloc(sizeof(File));
            pFileDesc[i].pOpenFile->fileOffset = 0;
            pFileDesc[i].pOpenFile->inodeNum = inode;

            pFileDesc[i].bUsed = TRUE;
            break;
        } 
    }

    free(pInode);
    free(dirEntry);

    return fd;
}


int     WriteFile(int fileDesc, char* pBuffer, int length)
{
    int freeBlockNum = 0, remainingLength = length, bytesWritten = 0, pBufferSize = 0;
    int fileOffset = pFileDesc[fileDesc].pOpenFile->fileOffset;
    int fileInodeNum = pFileDesc[fileDesc].pOpenFile->inodeNum;
    int offsetInBlock=0;

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    char* buffer = (char*)malloc(BLOCK_SIZE);
    int i = 0;
    
    while (remainingLength > 0) {
        offsetInBlock = fileOffset % BLOCK_SIZE;
        GetInode(fileInodeNum, pInode);

        if (i > 4) {
            free(pInode);
            free(buffer);

            return -1;
        }

        if (pInode->dirBlockPtr[i] == 0) {
            freeBlockNum = GetFreeBlockNum();
            pInode->allocBlocks++;
            pInode->dirBlockPtr[i] = freeBlockNum;
            pInode->size += BLOCK_SIZE;
            SetBlockBitmap(freeBlockNum);
            PutInode(fileInodeNum, pInode);
            GetInode(fileInodeNum, pInode);
        }

        int currentBlock = pInode->dirBlockPtr[i];
        int pBufferSize= (remainingLength < BLOCK_SIZE - offsetInBlock) ? remainingLength : (BLOCK_SIZE - offsetInBlock);

        BufRead(currentBlock, buffer);
        memcpy(buffer + offsetInBlock, pBuffer + bytesWritten, pBufferSize);
        BufWrite(currentBlock, buffer);

        remainingLength -= pBufferSize;
        bytesWritten += pBufferSize;
        offsetInBlock = (offsetInBlock + pBufferSize) % BLOCK_SIZE;
        i++;
    }
    
    /** TODO: 수정 */
    pFileDesc[fileDesc].pOpenFile->fileOffset += length;
    if (offsetInBlock == 0 && i < 5) {
        GetInode(fileInodeNum, pInode);
        freeBlockNum = GetFreeBlockNum();
        pInode->allocBlocks++;
        pInode->dirBlockPtr[i] = freeBlockNum;
        pInode->size += BLOCK_SIZE;
        SetBlockBitmap(freeBlockNum);
        PutInode(fileInodeNum, pInode);
    }

    BufRead(0, (char*)pFileSysInfo);
    pFileSysInfo->numAllocBlocks += length / BLOCK_SIZE;
    pFileSysInfo->numFreeBlocks -= length / BLOCK_SIZE;
    BufWrite(0, (char*)pFileSysInfo);

    free(pInode);
    free(buffer);

    return length;
}

int     ReadFile(int fileDesc, char* pBuffer, int length)
{
    int freeBlockNum = 0, logicalBlockNum;
    int fileOffset = pFileDesc[fileDesc].pOpenFile->fileOffset;
    int fileInodeNum = pFileDesc[fileDesc].pOpenFile->inodeNum;

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    char* buffer = (char*)malloc(BLOCK_SIZE);
    
    GetInode(fileInodeNum, pInode);
    logicalBlockNum = pFileDesc[fileDesc].pOpenFile->fileOffset % BLOCK_SIZE;
    BufRead(pInode->dirBlockPtr[logicalBlockNum], buffer); 
    memcpy(pBuffer, buffer, BLOCK_SIZE);

    free(buffer);

    return length;
}


int     CloseFile(int fileDesc)
{
    free(pFileDesc[fileDesc].pOpenFile);
    pFileDesc[fileDesc].bUsed = FALSE;

    return 0;
}

int     RemoveFile(const char* szFileName)
{

    // 경로 찾기
    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szFileName, pathNames);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            int flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);
            int savedInode = pInode->dirBlockPtr[j];

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
                
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    if (i == pathCnt - 1){
                        strcpy(dirEntry[k].name, "NULL");
                        PutInode(dirEntry[k].inodeNum, pInode);
                        ResetInodeBitmap(dirEntry[k].inodeNum);

                        BufWrite(savedInode, (char*)dirEntry);

                        // 어차피 아무것도 없으니까 0 번째로 접근
                        ResetBlockBitmap(pInode->dirBlockPtr[0]);
                        pInode->dirBlockPtr[0] = 0;
                        pInode->allocBlocks--;
                        pInode->size = 0;
                        PutInode(dirEntry[k].inodeNum, pInode);
                    }

                    flag = 1;
                    break;
                }
            }

            if (flag == 1) break;
        }
    }

    free(pInode);
    free(dirEntry);

    return 0;
}


int     MakeDir(const char* szDirName)
{

    // 경로 찾기
    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szDirName, pathNames);

    BufRead(FILESYS_INFO_BLOCK, (char*)pFileSysInfo);

    //빈 block, inode 찾기
    int freeBlockNum, freeInodeNum, preInodeNum;

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    // 해당 경로를 찾는 로직
    for(int i=0; i<pathCnt; i++) {

        freeBlockNum = GetFreeBlockNum(); freeInodeNum = GetFreeInodeNum();

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
            preInodeNum = 0;
        }

        int dirFlag = 0, realPreNum = 0;
        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {

            if (dirFlag == 1) break;
            
            if (pInode->dirBlockPtr[j] == 0) {
                
                pInode->dirBlockPtr[j] = freeBlockNum;
                pInode->size += BLOCK_SIZE;
                pInode->allocBlocks++;
                    
                BufRead(freeBlockNum, (char*)dirEntry); 
                InitDirEntry(dirEntry);

                dirEntry[0].inodeNum = preInodeNum;
                strcpy(dirEntry[0].name, ".");
                dirEntry[0].type = FILE_TYPE_DIR;
                dirEntry[1].inodeNum = realPreNum;
                strcpy(dirEntry[1].name, "..");
                dirEntry[1].type = FILE_TYPE_DIR;

                SetBlockBitmap(freeBlockNum);
                PutInode(preInodeNum, pInode);
                GetInode(preInodeNum, pInode);

                BufRead(0, (char*)pFileSysInfo);
                pFileSysInfo->numAllocBlocks++;
                pFileSysInfo->numFreeBlocks--;
                BufWrite(0, (char*)pFileSysInfo);

                freeBlockNum = GetFreeBlockNum();         
            }
            else BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            
                // 기존에 이미 있을 경우
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    realPreNum = preInodeNum;
                    GetInode(dirEntry[k].inodeNum, pInode);
                    preInodeNum = dirEntry[k].inodeNum;
                    dirFlag = 1;
                    break;
                }

                // 없어서 빈 곳을 찾은 경우
                if (strcmp(dirEntry[k].name, "NULL") == 0) {
                    dirEntry[k].inodeNum = freeInodeNum;
                    strcpy(dirEntry[k].name, pathNames[i]);
                    dirEntry[k].type = FILE_TYPE_DIR; 

                    // (2) - 4 
                    BufWrite(pInode->dirBlockPtr[j], (char*)dirEntry);

                    // (3) - 1, 2
                    BufRead(freeBlockNum, (char*)dirEntry);
                    InitDirEntry(dirEntry); 

                    // (3) - 3
                    dirEntry[0].inodeNum = freeInodeNum;
                    strcpy(dirEntry[0].name, ".");
                    dirEntry[0].type = FILE_TYPE_DIR;
                    dirEntry[1].inodeNum = preInodeNum;
                    strcpy(dirEntry[1].name, "..");
                    dirEntry[1].type = FILE_TYPE_DIR;

                    // (3) - 4
                    BufWrite(freeBlockNum, (char*)dirEntry);

                    GetInode(freeInodeNum, pInode);
                    pInode->dirBlockPtr[0] = freeBlockNum;
                    pInode->allocBlocks = 1;
                    pInode->size = BLOCK_SIZE;
                    pInode->type = FILE_TYPE_DIR;

                    PutInode(freeInodeNum, pInode);

                    SetBlockBitmap(freeBlockNum);
                    SetInodeBitmap(freeInodeNum);

                    // (5)
                    pFileSysInfo->numAllocBlocks++;
                    pFileSysInfo->numAllocInodes++;
                    pFileSysInfo->numFreeBlocks--;

                    BufWrite(FILESYS_INFO_BLOCK, (char*)pFileSysInfo);
                    
                    dirFlag = 1;
                    break;
                }
            }

            if (dirFlag == 1) break;
            
        }
    }

    free(dirEntry);
    free(pInode);
}



int     RemoveDir(const char* szDirName)
{
    
    int count = 0;

    // 경로 찾기
    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szDirName, pathNames);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    // 해당 경로를 찾는 로직
    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            int flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    flag = 1;
                    break;
                }
            }

            if (flag == 1) break;
        }
    }

    for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
        int flag = 0;
        if (pInode->dirBlockPtr[j] == 0) break;
        BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);

        for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            
            if (strcmp(dirEntry[k].name, "NULL") != 0) {
                count++;
            }
            else {
                flag = 1;
                break;
            }
        }

        if (flag == 1) break;
    }

    if (count > 2) {

        free(pInode);
        free(dirEntry);
        return -1;
    }

    int dirCount = 0, savedInode = -1;
    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            int flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);
            savedInode = pInode->dirBlockPtr[j];

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {

                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    if (i == pathCnt - 1){
                        strcpy(dirEntry[k].name, "NULL");
                        PutInode(dirEntry[k].inodeNum, pInode);
                        ResetInodeBitmap(dirEntry[k].inodeNum);

                        BufWrite(savedInode, (char*)dirEntry);

                        // 어차피 아무것도 없으니까 0 번째로 접근
                        for (int i=0; i<5; i++) {
                            ResetBlockBitmap(pInode->dirBlockPtr[i]);
                            pInode->dirBlockPtr[i] = 0;
                        }
                        PutInode(dirEntry[k].inodeNum, pInode);
                    }

                    flag = 1;
                }

                if (i == pathCnt - 1) {
                    if (strcmp(dirEntry[k].name, "NULL") != 0) {
                        dirCount++;
                    }
                }
            }

            if (flag == 1) break;
        }
    }
    /*
    if (dirCount == 2 && savedInode != -1) {

        BufWrite(savedInode, (char*)dirEntry);

        for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {

                if ((strcmp(dirEntry[k].name, ".") == 0) || (strcmp(dirEntry[k].name, "..") == 0 )) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    strcpy(dirEntry[k].name, "NULL");
                    PutInode(dirEntry[k].inodeNum, pInode);
                    ResetInodeBitmap(dirEntry[k].inodeNum);

                    BufWrite(savedInode, (char*)dirEntry);

                    // 어차피 아무것도 없으니까 0 번째로 접근
                    ResetBlockBitmap(pInode->dirBlockPtr[0]);
                    pInode->dirBlockPtr[0] = 0;
                    PutInode(dirEntry[k].inodeNum, pInode);

                }
            }
    }*/

    free(pInode);
    free(dirEntry);

    return 0;
}


int   EnumerateDirStatus(const char* szDirName, DirEntryInfo* pDirEntry, int dirEntrys)
{

    int count = 0;

    // 경로 찾기
    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szDirName, pathNames);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    // 해당 경로를 찾는 로직
    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            int flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    flag = 1;
                    break;
                }
            }

            if (flag == 1) break;
        }
    }

    if (pathCnt == 0) {
        GetInode(pFileSysInfo->rootInodeNum, pInode);
    }

    for (int i=0; i<NUM_OF_DIRECT_BLOCK_PTR; i++) {

        int flag = 0;
        if (pInode->dirBlockPtr[i] == 0) break;
        BufRead(pInode->dirBlockPtr[i], (char*)dirEntry);
        for (int j=0; j<NUM_OF_DIRENT_PER_BLOCK_INFO; j++) {
            if (strcmp(dirEntry[j].name, "NULL") != 0) {
                pDirEntry[count++] = dirEntry[j];
            }
        }
    }

    free(pInode);
    free(dirEntry);

    return count;
}

int      GetFileStatus(const char* szPathName, FileStatus* pStatus)
{
    
    int count = 0, flag = 0;

    // 경로 찾기
    char pathNames[100][MAX_NAME_LEN];
    int pathCnt = parsePath(szPathName, pathNames);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(BLOCK_SIZE);

    // 해당 경로를 찾는 로직
    for (int i=0; i<pathCnt; i++) {

        if (i == 0) {
            // Root directory 가져오기
            GetInode(pFileSysInfo->rootInodeNum, pInode);
        }

        for (int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            flag = 0;
            if (pInode->dirBlockPtr[j] == 0) break;
            BufRead(pInode->dirBlockPtr[j], (char*)dirEntry);

            for (int k=0; k<NUM_OF_DIRENT_PER_BLOCK_INFO; k++) {
            
                if (strcmp(dirEntry[k].name, pathNames[i]) == 0) {
                    
                    GetInode(dirEntry[k].inodeNum, pInode);

                    flag = 1;
                    break;
                }
            }

            if (flag == 1) break;
        }
    }

    if (flag != 1) {
        free(pInode);
        free(dirEntry);

        return -1;
    }

    pStatus->type = pInode->type;
    pStatus->size = pInode->size;
    pStatus->allocBlocks = pInode->allocBlocks;
    
    for(int i=0; i< NUM_OF_DIRECT_BLOCK_PTR; i++) {
        pStatus->dirBlockPtr[i] = pInode->dirBlockPtr[i];
    }

    free(pInode);
    free(dirEntry);

    return 0; 
}


void    CreateFileSystem()
{
    
    // (0) Disk 초기화
    FileSysInit();
    for(int i=0; i<MAX_FD_ENTRY_MAX; i++){
        pFileDesc[i].bUsed = FALSE;
    }

    // (1) free Block 검색.
    int freeBlockNum = GetFreeBlockNum();

    // (2) free inode 검색.
    int freeInodeNum = GetFreeInodeNum();

    // (3), (4) Block 크기의 메모리 할당, DirEntry 구조체의 배열로 반환
    DirEntryInfo* dirEntry = (DirEntryInfo*)malloc(sizeof(BLOCK_SIZE));
    dirEntry->type = FILE_TYPE_DIR;
    InitDirEntry(dirEntry);

    // (5) DirEntry의 변수들을 설정
    strcpy(dirEntry[0].name, ".");
    dirEntry[0].inodeNum = 0;

    // (6) 변경된 Block을 저장
    BufWrite(7, (char*)dirEntry);

    // (7)(8) Block 크기의 메모리 할당 후 FileSysInfo 변경
    pFileSysInfo->numAllocBlocks++;
    pFileSysInfo->numFreeBlocks--; 
    pFileSysInfo->numAllocInodes++;

    // (9) 저장
    BufWrite(0, (char*)pFileSysInfo);
    SetBlockBitmap(7);
    SetInodeBitmap(0);

    Inode* pInode = (Inode*)malloc(sizeof(Inode));
    GetInode(0, pInode);

    pInode->dirBlockPtr[0] = 7;
    pInode->size = BLOCK_SIZE;
    pInode->type = FILE_TYPE_DIR;
    pInode->allocBlocks = 1;

    PutInode(0, pInode);

    free(dirEntry);
    free(pInode);
}


void    OpenFileSystem()
{

    DevOpenDisk();
    BufInit();
    pFileSysInfo = (FileSysInfo*)malloc(BLOCK_SIZE);
}

void    CloseFileSystem()
{

    BufSync();
    DevCloseDisk();
}