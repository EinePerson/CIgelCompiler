#include "Array.h"
#include "../bdwgc/include/gc_cpp.h"

#include <cstring>

extern "C"{

    ArrayWrapper createArray(char type,int idX,unsigned int* arr) {
        if(idX == 0) {
            ArrayWrapper wrapper{.len = arr[idX]};
            switch (type) {
                case 0:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) char[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(char));
                    break;
                case 1:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) short[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(short));
                    break;
                case 2:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) int[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX * sizeof(int)]);
                    break;
                case 3:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) long[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(long));
                    break;
                case 4:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) unsigned char[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned char));
                    break;
                case 5:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) unsigned short[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned short));
                    break;
                case 6:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) unsigned int[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned int));
                    break;
                case 7:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) unsigned long[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned long));
                    break;
                case 8:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) float[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(float));
                    break;
                case 9:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) double[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(double));
                    break;
                case 10:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) bool[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(bool));
                    break;
                default:
                    wrapper.ptr = reinterpret_cast<long *>(new (GC) long[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(long));
                    break;
            }
            return wrapper;
        }
        ArrayWrapper wrapper{.len = arr[idX]};
        wrapper.ptr = reinterpret_cast<long *>(new (GC) ArrayWrapper[arr[idX]]);
        for(unsigned int i = 0;i < arr[idX];i++) {
            reinterpret_cast<ArrayWrapper *>(wrapper.ptr)[i] = createArray(type,idX - 1,arr);
        }
        return wrapper;
    }
}
