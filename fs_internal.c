#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buf.h"
#include "fs.h"

void SetInodeBitmap(int inodeno)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) Blcok 1을 BufRead 함수를 통해 메모리로 읽는다. 
    BufRead(INODE_BITMAP_BLOCK_NUM, memory);

    // (3), (4) byte inodeno를 1로 설정함.  
    /** 만약 원래 TRUE일 경우 예외처리 -> 굳이 BufWrite를 사용할 필요가 없다 */
    int byteIndex = inodeno / 8;
    int bitIndex = inodeno % 8;

    memory[byteIndex] |= (1 << bitIndex);

    // (6) Block 1에 BufWrite 함수를 통해 저장함.
    BufWrite(INODE_BITMAP_BLOCK_NUM, memory);

    free(memory);
}


void ResetInodeBitmap(int inodeno)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) Blcok 1을 BufRead 함수를 통해 메모리로 읽는다. 
    BufRead(INODE_BITMAP_BLOCK_NUM, memory);

    // (3), (4) byte inodeno를 1로 설정함.
    int byteIndex = inodeno / 8;
    int bitIndex = inodeno % 8;

    memory[byteIndex] &= ~(1 << bitIndex);
    // (6) Block 1에 BufWrite 함수를 통해 저장함.
    BufWrite(INODE_BITMAP_BLOCK_NUM, memory);

    free(memory);
}


void SetBlockBitmap(int blkno)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) Block 2를 BufRead 함수를 통해 메모리로 읽는다. 
    BufRead(BLOCK_BITMAP_BLOCK_NUM, memory);

    // (3), (4) byte blkno를 1로 설정함. 
    /** 만약 원래 TRUE일 경우 예외처리 -> 굳이 BufWrite를 사용할 필요가 없다 */
    int byteIndex = blkno / 8;
    int bitIndex = blkno % 8;

    memory[byteIndex] |= (1 << bitIndex);

    // (6) Block 1에 BufWrite 함수를 통해 저장함.
    BufWrite(BLOCK_BITMAP_BLOCK_NUM, memory);

    free(memory);
}


void ResetBlockBitmap(int blkno)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) Blcok 1을 BufRead 함수를 통해 메모리로 읽는다. 
    BufRead(BLOCK_BITMAP_BLOCK_NUM, memory);

    // (3), (4) byte blkno를 1로 설정함. 
    /** 만약 원래 TRUE일 경우 예외처리 -> 굳이 BufWrite를 사용할 필요가 없다 */
    int byteIndex = blkno / 8;
    int bitIndex = blkno % 8;

    memory[byteIndex] &= ~(1 << bitIndex);

    // (6) Block 1에 BufWrite 함수를 통해 저장함.
    BufWrite(BLOCK_BITMAP_BLOCK_NUM, memory);

    free(memory);
}


int findInodeBlockno(int inodeno) {

    if (inodeno >= 0 && inodeno < 16) return INODELIST_BLOCK_FIRST + 0;
    else if (inodeno >= 16 && inodeno < 32) return INODELIST_BLOCK_FIRST + 1;
    else if (inodeno >= 32 && inodeno < 48) return INODELIST_BLOCK_FIRST + 2;
    else if (inodeno >= 48 && inodeno < 64) return INODELIST_BLOCK_FIRST + 3;
    else return -1; // 잘못된 inode number
}


void PutInode(int inodeno, Inode* pInode)
{

    // (1) Block 크기의 메모리 할당
    Inode* memory = (Inode*)malloc(BLOCK_SIZE);

    // (2) Inodeno이 저장된 Block 번호를 구한다. 
    int inodeBlockno = findInodeBlockno(inodeno);

    // (3) inodeBlockno를 BufRead 함수를 통해 메모리로 읽는다.
    BufRead(inodeBlockno, (char*)memory);

    // (4), (5) pInode의 내용을 memory의 Inodeno에 복사한다.
    memcpy(&memory[inodeno % 16], pInode, sizeof(Inode));

    // (6) inodenno에 BufWrite 함수를 통해 저장.
    BufWrite(inodeBlockno, (char*)memory);

    free(memory);
 }


void GetInode(int inodeno, Inode* pInode)
{

    // (1) Block 크기의 메모리 할당
    Inode* memory = (Inode*)malloc(BLOCK_SIZE);

    // (2) Inodeno이 저장된 Block 번호를 구한다. 
    int inodeBlockno = findInodeBlockno(inodeno);

    // (3) inodeBlockno를 BufRead 함수를 통해 메모리로 읽는다.
    BufRead(inodeBlockno, (char*)memory);

    // (4), (5) inodeno의 내용을 pInode로 복사한다.
    memcpy(pInode, &memory[inodeno % 16], sizeof(Inode));

    free(memory);
}


int GetFreeInodeNum(void)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) BufRead 함수를 통해 메모리로 읽는다.
    BufRead(INODE_BITMAP_BLOCK_NUM, memory);

    // (3), (4) Byte 0부터 1씩 증가하면서 0을 가지는 Byte를 찾는다. (First Fit searching)
    /*
    for (int i=0; i<BLOCK_SIZE; i++) {

        if (memory[i] == FALSE) return i;
    }*/
    for (int i=0; i<8; i++) {
        for(int j=0; j<8; j++) {
            if ((memory[i] & (1 << j)) == 0) {
                return i * 8 + j;
            }
        }
    }

    free(memory);

    return -1;
 }


int GetFreeBlockNum(void)
{

    // (1) Block 크기의 메모리 할당
    char* memory = (char*)malloc(BLOCK_SIZE);

    // (2) BufRead 함수를 통해 메모리로 읽는다.
    BufRead(BLOCK_BITMAP_BLOCK_NUM, memory);

    // (3), (4) Byte 0부터 1씩 증가하면서 0을 가지는 Byte를 찾는다. (First Fit searching)
    for (int i=0; i<64; i++) {
        for(int j=0; j<8; j++) {
            if ((memory[i] & (1 << j)) == 0) {
                return i * 8 + j;
            }
        }
    }

    free(memory);

    return -1;
}

void FileSysInit(void)
{

    /** TODO: Buf와 Disk 생성 및 초기화 (구현되었으면 삭제) */
    DevCreateDisk();
    BufInit();

    /** File System Info를 블록 크기의 메모리로 할당 받은 후 0으로 초기화하고 디스크로 저장 */
    /** TODO: 전부 다 0으로 설정할지 고민하자 */
    pFileSysInfo = (FileSysInfo*)malloc(BLOCK_SIZE);

    pFileSysInfo->blocks = 512;
    pFileSysInfo->rootInodeNum = 0;
    pFileSysInfo->diskCapacity = FS_DISK_CAPACITY;
    pFileSysInfo->numAllocBlocks = 7;
    pFileSysInfo->numFreeBlocks = 505; 
    pFileSysInfo->numAllocInodes = 0;
    pFileSysInfo->blockBitmapBlock = BLOCK_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeBitmapBlock = INODE_BITMAP_BLOCK_NUM;
    pFileSysInfo->inodeListBlock = INODELIST_BLOCK_FIRST;

    /** InodeByteMap 할당 및 초기화 */
    char* inodeByteMap = (char*)malloc(BLOCK_SIZE);
    memset(inodeByteMap, 0, BLOCK_SIZE);

    /** BlockByeMap 할당 및 초기화 */
    char* blockByteMap = (char*)malloc(BLOCK_SIZE);
    memset(blockByteMap, 0, BLOCK_SIZE);

    
    for(int i=0; i<7; i++) {
        blockByteMap[0] |= (1 << i);
    }

    /** InodeList 할당 및 초기화 */
    Inode* inodeList0 = (Inode*)malloc(BLOCK_SIZE);
    Inode* inodeList1 = (Inode*)malloc(BLOCK_SIZE);
    Inode* inodeList2 = (Inode*)malloc(BLOCK_SIZE);
    Inode* inodeList3 = (Inode*)malloc(BLOCK_SIZE);

    int inodeCntPerBlock = BLOCK_SIZE / sizeof(Inode); // Block 당 16개
    
    for(int i=0; i<inodeCntPerBlock; i++) {
        
        inodeList0[i].allocBlocks = 0;
        inodeList0[i].size = 0;

        inodeList1[i].allocBlocks = 0;
        inodeList1[i].size = 0;

        inodeList2[i].allocBlocks = 0;
        inodeList2[i].size = 0;

        inodeList3[i].allocBlocks = 0;
        inodeList3[i].size = 0;

        for(int j=0; j<NUM_OF_DIRECT_BLOCK_PTR; j++) {
            
            inodeList0[i].dirBlockPtr[j] = 0;
            inodeList1[i].dirBlockPtr[j] = 0;
            inodeList2[i].dirBlockPtr[j] = 0;
            inodeList3[i].dirBlockPtr[j] = 0;
        }
    }

    pFileSysInfo->dataRegionBlock = INODELIST_BLOCK_FIRST + INODELIST_BLOCKS; // 7

    // Buf Write
    BufWrite(FILESYS_INFO_BLOCK, (char*)pFileSysInfo);
    BufWrite(INODE_BITMAP_BLOCK_NUM, inodeByteMap);
    BufWrite(BLOCK_BITMAP_BLOCK_NUM, blockByteMap);
    BufWrite(INODELIST_BLOCK_FIRST + 0, (char*)inodeList0);
    BufWrite(INODELIST_BLOCK_FIRST + 1, (char*)inodeList1);
    BufWrite(INODELIST_BLOCK_FIRST + 2, (char*)inodeList2);
    BufWrite(INODELIST_BLOCK_FIRST + 3, (char*)inodeList3);
    
    free(inodeByteMap);
    free(blockByteMap);
    free(inodeList0);
    free(inodeList1);
    free(inodeList2);
    free(inodeList3);

    // Data Region 할당 만
    for(int i=7; i<512; i++) {
        char* block = (char*)malloc(BLOCK_SIZE);
        BufWrite(i, block);
        free(block);
    }
}