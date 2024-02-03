#pragma once

/**
 * \brief A struct which contains the size of an array and its respective pointer
 */
struct ArrayWrapper {
    unsigned int len;
    long* ptr;
};

extern "C"{
    /**
    * \brief Recursive function which creates n-dimensional arrays with size information
    * \param type The base type of the array
    * \param idX the current dimension
    * \param arr the array sizes
    * \return An information struct for the array
    */
    ArrayWrapper createArray(char type,int idX,unsigned int* arr);
}