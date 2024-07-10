//
// Created by igel on 09.07.24.
//

#ifndef STRING_H
#define STRING_H
#include <stacktrace>
#include <string>

class String {
public:
    String(const char* str);
    String(const std::string &str);
    String(const String& str);

    ~String() {
        //delete[] buf.data;
    }

    std::ostream& operator<<(std::ostream& stream) const {
        stream << std::string(data,size);
        return stream;
    }

    String operator+(const String &other) const;
    String operator+(const char* other) const;

    [[nodiscard]] String append(const String &str) const;
    [[nodiscard]] String append(const char* str) const;

    [[nodiscard]] String subStr(unsigned int start,unsigned int size) const;

    [[nodiscard]] std::string cxx_str() const;

//private:
    explicit String(int size);

    String() = default;

    char* data = nullptr;
    unsigned int size = 0;
};

std::ostream& operator<<(std::ostream & lhs, const String & rhs);



#endif //STRING_H
