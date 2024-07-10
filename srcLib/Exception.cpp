//
// Created by igel on 08.07.24.
//
 #include "Exception.h"

#include <iostream>

Exception::Exception(String name,String message) : name(name), message(message) {
    std::stacktrace trace = std::stacktrace::current();
    stacktrace = std::to_string(trace);
}

void Exception::print(std::ostream &stream) {
    std::cout << name << ": " << message<< " at:\n" << stacktrace << std::endl;
}

void Exception::throwInternal() {
    print(std::cout);
    throw this;
}
