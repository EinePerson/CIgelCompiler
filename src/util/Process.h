//
// Created by igel on 22.07.24.
//

#ifndef PROCESS_H
#define PROCESS_H
#include <string>

class Process {
public:
    static std::string exec(const char* cmd);
};

#endif //PROCESS_H
