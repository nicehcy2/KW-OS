#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <assert.h>
#include "disk.h"
#include "fs.h"
#include "buf.h"

// Test하는 Main 함수
int main(){

    printf("Hello OS World\n");

    DevCreateDisk();
    BufInit();

    CreateFileSystem();

    SetInodeBitmap(0);
    SetInodeBitmap(1);

    for(int j=0; j<8; j++){
        SetInodeBitmap(j);
    }
    printf("freeInodeBLock: %d\n", GetFreeInodeNum());


    char* memory = (char*)malloc(BLOCK_SIZE);
    BufRead(1, memory);

    for(int i=0; i<8; i++){
        printf("%d\n", memory[i]);
        
    }

    

    return 0;
}
