#include "Array.h"

#include <cstring>

extern "C"{

    ArrayWrapper createArray(char type,int idX,int* arr) {
        if(idX == 0) {
            ArrayWrapper wrapper{.len = arr[idX]};
            switch (type) {
                case 0:
                    wrapper.ptr = reinterpret_cast<long *>(new char[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(char));
                case 1:
                    wrapper.ptr = reinterpret_cast<long *>(new short[arr[idX]]);
                std::memset(wrapper.ptr,0,arr[idX] * sizeof(short));
                case 2:
                    wrapper.ptr = reinterpret_cast<long *>(new int[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX * sizeof(int)]);
                case 3:
                    wrapper.ptr = reinterpret_cast<long *>(new long[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(long));
                case 4:
                    wrapper.ptr = reinterpret_cast<long *>(new unsigned char[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned char));
                case 5:
                    wrapper.ptr = reinterpret_cast<long *>(new unsigned short[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned short));
                case 6:
                    wrapper.ptr = reinterpret_cast<long *>(new unsigned int[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned int));
                case 7:
                    wrapper.ptr = reinterpret_cast<long *>(new unsigned long[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(unsigned long));
                case 8:
                    wrapper.ptr = reinterpret_cast<long *>(new float[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(float));
                case 9:
                    wrapper.ptr = reinterpret_cast<long *>(new double[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(double));
                case 10:
                    wrapper.ptr = reinterpret_cast<long *>(new bool[arr[idX]]);
                    std::memset(wrapper.ptr,0,arr[idX] * sizeof(bool));
                default:
                    void* val[arr[idX]];
                    wrapper.ptr = reinterpret_cast<long*>(val);
                std::memset(wrapper.ptr,0,arr[idX] * sizeof(void*));
            }
            return wrapper;
        }
        ArrayWrapper wrapper{.len = arr[idX]};
        wrapper.ptr = reinterpret_cast<long *>(new ArrayWrapper[arr[idX]]);
        for(unsigned int i = 0;i < arr[idX];i++) {
            reinterpret_cast<ArrayWrapper *>(wrapper.ptr)[i] = createArray(type,idX - 1,arr);
        }
        return wrapper;
    }
}
