//
// Created by igel on 09.02.24.
//

#ifndef GENERATOR_H
#define GENERATOR_H
#include <exception>

class IllegalGenerationException : std::exception {
public:
    explicit IllegalGenerationException(const char* cause) : cause(cause){}


    const char* what() const noexcept override {
        return cause;
    };

private:
    const char* cause;
};

#endif //GENERATOR_H
