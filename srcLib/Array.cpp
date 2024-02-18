#include "Array.h"
#include "../bdwgc/include/gc_cpp.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C"{
    long* newArray(char size,int idX,unsigned long* arr) {
        if(idX == 0)return static_cast<long*>(GC_MALLOC(arr[0] * size + sizeof(unsigned long)));
        long* ptr = static_cast<long*>(GC_MALLOC(arr[idX] * sizeof(void*) + sizeof(unsigned long)));
        long l = arr[idX];
        ptr[0] = l;
        for(unsigned long i = 0;i < arr[idX];i++) {
            (ptr + 8)[i] = reinterpret_cast<long>(newArray(size, idX - 1, arr));
        }
        return ptr;
    }
}
