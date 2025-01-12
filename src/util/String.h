//
// Created by igel on 29.11.24.
//

#ifndef IGEL_COMPILER_STRING_H
#define IGEL_COMPILER_STRING_H

#include <vector>
#include <string>

class String {
public:
    static std::vector<std::string> split(const std::string& str, std::string delimiter);
};


#endif //IGEL_COMPILER_STRING_H
