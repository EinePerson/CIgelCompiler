#include "Array.h"

#include <iostream>
#include <cstring>

#include "../bdwgc/include/gc.h"

extern "C"{

    void* newArray(unsigned int size,int idX,unsigned long* arr) {
        if(idX == 0) {
            void* ptr = GC_MALLOC(arr[0] * size + sizeof(unsigned long));
            ///NULL init
            memset(ptr,0,arr[0] * size + sizeof(unsigned long));
            ((unsigned long*)ptr)[0] = arr[0];
            return ptr;
        }
        void* ptr = GC_MALLOC(arr[idX] * sizeof(void*) + sizeof(unsigned long));
        ((long*)ptr)[0] = arr[idX];
        for(unsigned long i = 0;i < arr[idX];i++) {
            void* _n = newArray(size,idX - 1,arr);
            memcpy(((long*)ptr) + 1 + i,&_n,8);
        }
        return ptr;
    }

}