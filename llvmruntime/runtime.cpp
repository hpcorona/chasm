
#include <stdio.h>
#include <sstream>
#include <iostream>

#define RUNTIME_API extern "C"

RUNTIME_API char* read() {
    std::string data;
    std::getline(std::cin, data);

    return strdup(data.c_str());
}

RUNTIME_API int integerstr(char*);

RUNTIME_API int readinteger() {
    return integerstr(read());
}

RUNTIME_API double doublestr(char*);

RUNTIME_API double readdouble() {
    return doublestr(read());
}

RUNTIME_API void print(char* data) {
    std::cout << data;

    delete data;
}

RUNTIME_API void println(char* data) {
    std::cout << data << std::endl;

    delete data;
}

RUNTIME_API char* strdupl(char* data) {
    return strdup(data);
}

RUNTIME_API char* strjoin(char* data1, char* data2) {
    size_t len = strlen(data1) + strlen(data2);

    char* data = new char[len + 1];
    data[0] = 0;

    strcat(data, data1);
    strcat(data, data2);

    delete data1;
    delete data2;

    return data;
}

RUNTIME_API void strassign(char* dest, int maxlen, char* data) {
    if (strlen(data) > maxlen) {
        for (int i = 0; i < maxlen - 1; i++) {
            dest[i] = data[i];
        }
        dest[maxlen] = 0;
    } else {
        dest[0] = 0;
        strcat(dest, data);
    }

    delete data;
}

RUNTIME_API char* strdouble(double data) {
    std::ostringstream strs;
    strs << data;
    std::string str = strs.str();

    return strdup(str.c_str());
}

RUNTIME_API char* strinteger(int data) {
    std::ostringstream strs;
    strs << data;
    std::string str = strs.str();

    return strdup(str.c_str());
}

RUNTIME_API double doublestr(char* data) {
    std::istringstream i(data);
    delete data;
    double x;
    if (!(i >> x)) {
        return 0;
    }
    return x;
}

RUNTIME_API int integerstr(char* data) {
    int val = atoi(data);
    delete data;

    return val;
}

RUNTIME_API char strlt(char* d1, char* d2) {
    char ret = strcmp(d1, d2);
    delete d1;
    delete d2;

    return ret < 0;
}

RUNTIME_API char strle(char* d1, char* d2) {
    char ret = strcmp(d1, d2);
    delete d1;
    delete d2;

    return ret <= 0;
}

RUNTIME_API char streq(char* d1, char* d2) {
    char ret = strcmp(d1, d2);
    delete d1;
    delete d2;

    return ret == 0;
}

RUNTIME_API char strgt(char* d1, char* d2) {
    char ret = strcmp(d1, d2);
    delete d1;
    delete d2;

    return ret > 0;
}

RUNTIME_API char strge(char* d1, char* d2) {
    char ret = strcmp(d1, d2);
    delete d1;
    delete d2;

    return ret >= 0;
}

RUNTIME_API void ch_program() {
    // This will be filled by our program
}

int main(int argc, char** vaargs) {
    ch_program();
    return 0;
}
