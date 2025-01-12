//
// Created by igel on 29.11.24.
//

#include "String.h"

std::vector<std::string> String::split(const std::string& str, std::string delimiter) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = str.find(delimiter);

    while (end != std::string::npos) {
        result.push_back(str.substr(start, end - start));
        start = end + 1;
        end = str.find(delimiter, start);
    }
    result.push_back(str.substr(start));

    return result;
}