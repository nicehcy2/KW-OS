#include <stdio.h>
#include <stdlib.h>
#include "buf.h"
#include "queue.h"
#include "disk.h"
#include <string.h>


struct bufList      bufList[MAX_BUFLIST_NUM];
struct stateList    stateList[MAX_BUF_STATE_NUM];
struct freeList     freeListHead;
struct lruList      lruListHead;

void BufInit(void)
{
    
    for (int i=0; i < MAX_BUFLIST_NUM; i++) CIRCLEQ_INIT(&bufList[i]);
    for (int i=0; i < MAX_BUF_STATE_NUM; i++) CIRCLEQ_INIT(&stateList[i]);
    
    CIRCLEQ_INIT(&freeListHead);
    CIRCLEQ_INIT(&lruListHead);

    // 빈 버퍼 생성
    for (int i=0; i< MAX_BUF_NUM; i++) {
        
        Buf* fBuf = malloc(sizeof(Buf));

        // 빈 kernel memory(block) 생성
        fBuf->pMem = malloc(BLOCK_SIZE);

        CIRCLEQ_INSERT_TAIL(&freeListHead, fBuf, flist);
    }
}

Buf* BufGetNewBuffer(void) {

    // free List가 비었으면 null 반환
    if (CIRCLEQ_EMPTY(&freeListHead)) return NULL;
    else {

        Buf* target = CIRCLEQ_FIRST(&freeListHead);
        CIRCLEQ_REMOVE(&freeListHead, target, flist);

        return target;
    }
}

// 같은 것이 있다면 해당 블럭을 삭제, 없다면 head 삭제
// free List가 비워지면 사용 가능 -> LRU List도 가득찼다는 뜻.
Buf* HandleFullLlist(int blkno, BufState state) {

    Buf* lruHeadBuffer = CIRCLEQ_FIRST(&lruListHead);
    int headBlkno = lruHeadBuffer -> blkno;

    if (lruHeadBuffer->state == BUF_STATE_CLEAN) {
        
        // LRU의 header 노드를 모든 list에서 삭제
        CIRCLEQ_REMOVE(&lruListHead, lruHeadBuffer, llist);
        CIRCLEQ_REMOVE(&bufList[headBlkno % MAX_BUFLIST_NUM], lruHeadBuffer, blist);
        CIRCLEQ_REMOVE(&stateList[BUF_CLEAN_LIST], lruHeadBuffer, slist);
    }
    else if (lruHeadBuffer->state == BUF_STATE_DIRTY) {
            
        // DIRTY 상태면 DISK 저장하고 삭제
            
        DevWriteBlock(headBlkno, lruHeadBuffer->pMem);

        CIRCLEQ_REMOVE(&lruListHead, lruHeadBuffer, llist);
        CIRCLEQ_REMOVE(&bufList[headBlkno % MAX_BUFLIST_NUM], lruHeadBuffer, blist);
        CIRCLEQ_REMOVE(&stateList[BUF_DIRTY_LIST], lruHeadBuffer, slist);
    }

    // list에서 삭제한 노드 값 변경
    lruHeadBuffer->blkno = blkno;
    lruHeadBuffer->state = state;

    // TAIL에 값을 변경한 노드 추가
    CIRCLEQ_INSERT_HEAD(&bufList[blkno % MAX_BUFLIST_NUM], lruHeadBuffer, blist);
    CIRCLEQ_INSERT_TAIL(&stateList[state], lruHeadBuffer, slist);
    CIRCLEQ_INSERT_TAIL(&lruListHead, lruHeadBuffer, llist);

    return lruHeadBuffer;
}

Buf* BufFind(int blkno)
{

    Buf* targetBuf;

    // 해당 bufferList(hashTable)을 순회하면서 특정 buffer를 찾는다.
    CIRCLEQ_FOREACH(targetBuf, &bufList[blkno % MAX_BUFLIST_NUM], blist) {

        if (targetBuf->blkno == blkno) {
            
            return targetBuf;
        }
    }
    
    return NULL;
}


void BufRead(int blkno, char* pData)
{

    // 1. bufferList의 buffer를 검색.
    Buf* searchBuf = BufFind(blkno);

    // 만약 없다면?
    if (searchBuf == NULL) {
        
        // 만약 null이라면 freeList가 empty인 상태고 LRU LIST를 사용해야함.
        // 동시에 Buffer Cache Block의 수가 BUF_NUM_MAX라는 뜻.
        Buf* newBuf = BufGetNewBuffer();
        if (newBuf == NULL) {

            // LRU List 관리 
            newBuf = HandleFullLlist(blkno, BUF_STATE_CLEAN);
        } else {
            
            newBuf->state = BUF_STATE_CLEAN;
            newBuf->blkno = blkno;
            CIRCLEQ_INSERT_HEAD(&bufList[blkno % MAX_BUFLIST_NUM], newBuf , blist);
            CIRCLEQ_INSERT_TAIL(&stateList[BUF_CLEAN_LIST], newBuf , slist);
            CIRCLEQ_INSERT_TAIL(&lruListHead, newBuf , llist);
        }

        // 4. DevReadBlock을 사용하여 블록을 읽어서 blk 메모리에 저장
        DevReadBlock(blkno, newBuf->pMem);

        // 7. pMem에 저장된 block 데이터를 BufRead로 전달된 pData에 복사.
        memcpy(pData, newBuf->pMem, BLOCK_SIZE);
    }
    else {

        CIRCLEQ_REMOVE(&lruListHead, searchBuf, llist);
        CIRCLEQ_INSERT_TAIL(&lruListHead, searchBuf, llist);
        memcpy(pData, searchBuf->pMem, BLOCK_SIZE);
        // 추가 작업이 필요할수도?
    }
}


void BufWrite(int blkno, char* pData)
{

    // 1. bufferList의 buffer를 검색.
    Buf* searchBuf = BufFind(blkno);

    // 만약 없다면?
    if (searchBuf == NULL) {

        Buf* newBuf = BufGetNewBuffer();
        if (newBuf == NULL) {
            
            // LRU List 관리 
            newBuf = HandleFullLlist(blkno, BUF_STATE_DIRTY);
        } else {
            
            newBuf->state = BUF_STATE_DIRTY;
            newBuf->blkno = blkno;
            CIRCLEQ_INSERT_HEAD(&bufList[blkno % MAX_BUFLIST_NUM], newBuf , blist);
            CIRCLEQ_INSERT_TAIL(&stateList[BUF_DIRTY_LIST], newBuf , slist);
            CIRCLEQ_INSERT_TAIL(&lruListHead, newBuf , llist);
        }
    
        // 4. pData의 데이터를 pMem 메모리 공간에 복사함.
        memcpy(newBuf->pMem, pData, BLOCK_SIZE);
    }
    else {

        // block이 dirty이면?
        if (searchBuf->state == BUF_STATE_DIRTY) {
            
            CIRCLEQ_REMOVE(&lruListHead, searchBuf, llist);
            CIRCLEQ_INSERT_TAIL(&lruListHead, searchBuf, llist);

            memcpy(searchBuf->pMem, pData, BLOCK_SIZE);
        }
        // block이 clean이라면?
        else if (searchBuf->state == BUF_STATE_CLEAN) {

            CIRCLEQ_REMOVE(&stateList[BUF_CLEAN_LIST], searchBuf, slist);
            CIRCLEQ_INSERT_TAIL(&stateList[BUF_DIRTY_LIST], searchBuf, slist);

            CIRCLEQ_REMOVE(&lruListHead, searchBuf, llist);
            CIRCLEQ_INSERT_TAIL(&lruListHead, searchBuf, llist);
            
            searchBuf->state = BUF_STATE_DIRTY;
            memcpy(searchBuf->pMem, pData, BLOCK_SIZE);
        }
    }
}

void BufSync(void)
{   

    // DIRTY LIST가 빌 때까지 지운다.
    while (CIRCLEQ_EMPTY(&stateList[BUF_DIRTY_LIST]) == 0) {

        // DIRTY list의 head를 확인
        Buf* head = CIRCLEQ_FIRST(&stateList[BUF_DIRTY_LIST]);

        // 디스크로 저장
        head->state = BUF_STATE_CLEAN;
        
        // DevOpenDisk();
        DevWriteBlock(head->blkno, (char*)head->pMem);

        // DIRTY list에서 CLEAN list로 이동
        CIRCLEQ_REMOVE(&stateList[BUF_DIRTY_LIST], head, slist);
        CIRCLEQ_INSERT_TAIL(&stateList[BUF_CLEAN_LIST], head, slist);
    }
}

void BufSyncBlock(int blkno)
{

    Buf* var;

    CIRCLEQ_FOREACH(var, &stateList[BUF_DIRTY_LIST], slist) {

        if (var->blkno == blkno) {

            // 디스크로 저장
            var->state = BUF_STATE_CLEAN;

            // DevOpenDisk();
            DevWriteBlock(var->blkno, (char*)var->pMem);

            // DIRTY list에서 CLEAN list로 이동
            CIRCLEQ_REMOVE(&stateList[BUF_DIRTY_LIST], var, slist);
            CIRCLEQ_INSERT_TAIL(&stateList[BUF_CLEAN_LIST], var, slist);

            return;
        }
    }
}


int GetBufInfoInStateList(BufStateList listnum, Buf* ppBufInfo[], int numBuf)
{

    Buf* var;
    int cnt = 0;

    CIRCLEQ_FOREACH(var, &stateList[listnum], slist) {

        if (cnt < numBuf) ppBufInfo[cnt++] = var;
    }

    return cnt;
}

int GetBufInfoInLruList(Buf* ppBufInfo[], int numBuf)
{

    Buf* var;
    int cnt = 0;

    CIRCLEQ_FOREACH(var, &lruListHead, llist) {

        if (cnt < numBuf) ppBufInfo[cnt++] = var;
    }

    return cnt;
}


int GetBufInfoInBufferList(int index, Buf* ppBufInfo[], int numBuf)
{

    Buf* var;
    int cnt = 0;

    CIRCLEQ_FOREACH(var, &bufList[index], blist) {

        if (cnt < numBuf) ppBufInfo[cnt++] = var;
    }

    return cnt;
}

