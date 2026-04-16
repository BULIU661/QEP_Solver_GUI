// BinaryConvertList.h
#ifndef BINARY_CONVERT_LIST_H
#define BINARY_CONVERT_LIST_H

#include "config/GlobalConfig.h"
#include <vector>
#include <string>
#include <utility>
namespace Config
{
    inline std::vector<std::pair<std::string, std::string>> getBinaryConvertList()
    {
        std::string base = Config::PROBLEM_BASE_PATH;
        return {
            // 格式：{"文本文件路径", "二进制文件路径"}
            {base + "problemTest/M.txt", base + "problemTest/M.bin"},
            {base + "problemTest/K.txt", base + "problemTest/K.bin"},
            {base + "problemTest/C.txt", base + "problemTest/C.bin"},
            {base + "problemA/M.txt", base + "problemA/M.bin"},
            {base + "problemA/K.txt", base + "problemA/K.bin"},
            {base + "problemA/C.txt", base + "problemA/C.bin"},
            {base + "problemB/M.txt", base + "problemB/M.bin"},
            {base + "problemB/K.txt", base + "problemB/K.bin"},
            {base + "problemB/C.txt", base + "problemB/C.bin"},
            {base + "problemC/M.txt", base + "problemC/M.bin"},
            {base + "problemC/K.txt", base + "problemC/K.bin"},
            {base + "problemC/C.txt", base + "problemC/C.bin"},
            {base + "problemD/M.txt", base + "problemD/M.bin"},
            {base + "problemD/K.txt", base + "problemD/K.bin"},
            {base + "problemD/C.txt", base + "problemD/C.bin"},
            {base + "pro1/C.txt", base + "pro1/C.bin"},
            {base + "pro1/K.txt", base + "pro1/K.bin"},
            {base + "pro2/M.txt", base + "pro2/M.bin"},
            {base + "pro2/C.txt", base + "pro2/C.bin"},
            {base + "pro2/K.txt", base + "pro2/K.bin"},
            {base + "pro3/M1.txt", base + "pro3/M.bin"},
            {base + "pro3/C1.txt", base + "pro3/C.bin"},
            {base + "pro3/K1.txt", base + "pro3/K.bin"},
            {base + "pro4/M.txt", base + "pro4/M.bin"},
            {base + "pro4/C.txt", base + "pro4/C.bin"},
            {base + "pro4/K.txt", base + "pro4/K.bin"},

        };
    }
}

#endif // BINARY_CONVERT_LIST_H
