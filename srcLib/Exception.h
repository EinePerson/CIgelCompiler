//
// Created by igel on 05.07.24.
//

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <stacktrace>

#include "String.h"


/**
 * \brief this is the super class for the exception framework employed by igel
 *        in it there is a message which indicates a better reason to why it was thrown,
 *        the stacktrace at which
 */

class Exception {
    String name;
public:
    String stacktrace;
    String message;

    explicit Exception(String name,String message);

    virtual ~Exception() = default;

    virtual void print(std::ostream& stream);

    virtual void throwInternal();
};

#endif //EXCEPTION_H
