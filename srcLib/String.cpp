//
// Created by igel on 09.07.24.
//

#include "String.h"
#include <cstring>
#include "Exception.h"

String::String(const char *str) {
    size = strlen(str);
    data = new char[size];
    strcpy(data,str);
}

String::String(const std::string &str) {

    const char *cStr = str.c_str();
    size = strlen(cStr);
    data = (char*) malloc(size);

    strcpy(data,cStr);
}

String::String(const String &str) {
    size = str.size;
    data = new char[size];
    strcpy(data,str.data);
}

String String::operator+(const String &other) const {
    return append(other);
}

String String::operator+(const char *other) const {
    return append(other);
}

String String::append(const String &str) const {
    String string(this->size + str.size);
    strcpy(string.data,this->data);
    strcat(string.data,str.data);
    return string;
}

String String::append(const char *str) const {
    String string(this->size + strlen(str));
    strcpy(string.data,this->data);
    strcat(string.data,str);
    return string;
}

String String::subStr(unsigned int start, unsigned int size) const {
    int newSize = 0;
    if(start + size >= this->size) {
        Exception exc("IndexOutOfBoundsException", String("Tried to access index: ").append(std::to_string(start + size - 1).c_str()).append(" , of string of size: ")
            .append(std::to_string(this->size).c_str()));
        exc.throwInternal();
    }
    String string(newSize);
    strncpy(string.data,this->data + start,size);
    return string;
}

std::string String::cxx_str() const {
    std::string str(data,size);
    return str;
}

String::String(int size) {
    this->size = size;
    data = new char[size];
}

std::ostream & operator<<(std::ostream &lhs, const String &rhs) {
    std::string str(rhs.data,rhs.size);
    return  lhs << str;
}


