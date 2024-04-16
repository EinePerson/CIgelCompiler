#pragma once

extern "C"{
    /**
    * \brief Recursive function which creates n-dimensional arrays with size information
    * \param size size of the base type in bytes
    * \param idX the current dimension
    * \param arr the array sizes for every dimension
    * \return the pointer to the array
    */
    void* newArray(unsigned int size,int idX,unsigned long* arr);
}