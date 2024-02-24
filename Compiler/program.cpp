#include <iostream>
#include <string>
#include "compiler.h"

int main()
{
    char* data = new char[63];

    memcpy(data, "{ a = 1; b = 1; while(a<10) { b = b * a; a = a + 1; print(b)}}",63);

    compiler comp = compiler(data, 63);

    std::string result;

    comp.compile(result);

    std::cout << result;
}