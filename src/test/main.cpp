// main.cpp
#include "TestHarness.h"
#include "config/BinaryConvertList.h"
#include "config/TestCases.h"
#include <iostream>
#include <fstream>
#include <mkl.h>
#include <omp.h>

#ifdef _WIN32
#include <windows.h>
#endif

static void initEnvironment()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE)
    {
        DWORD mode;
        if (GetConsoleMode(hConsole, &mode))
            SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
    omp_set_nested(0);
    omp_set_dynamic(0);
    omp_set_num_threads(12);
    mkl_set_num_threads(12);
    mkl_set_dynamic(0);
    std::cout << "MKL acceleration: "
#ifdef EIGEN_USE_MKL_ALL
              << "ENABLED\n";
#else
              << "NOT enabled\n";
#endif
    char version[1024];
    MKL_Get_Version_String(version, 1024);
    std::cout << "MKL version: " << version << "\n"
              << "MKL threads: " << mkl_get_max_threads() << "\n"
              << "OpenMP threads: " << omp_get_max_threads() << "\n";
}

// 自动转换二进制文件(自动检查目录下是否有缺失的二进制文件，有则转换)
static void autoConvertBinaryFiles()
{
    if (!Config::AUTO_CONVERT_BINARY)
        return;
    auto files = Config::getBinaryConvertList();
    for (const auto &pair : files)
    {
        std::ifstream test(pair.second);
        if (!test.good() || Config::OVERWRITE_BINARY)
        { // 文件不存在或者Config::OVERWRITE_BINARY 为 true，则执行转换
            std::cout << "转换 " << pair.first << " -> " << pair.second << " ...\n";
            QEP::convertTextCSRtoBinary(pair.first, pair.second);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc > 1)
        Config::DEFAULT_SIGMA = std::stod(argv[1]);
    if (argc > 2)
        Config::DEFAULT_NEV = std::stoi(argv[2]);

    // 初始化环境
    initEnvironment();

    // 自动转换二进制文件
    autoConvertBinaryFiles();

    // 获取测试用例列表、设置求解参数
    const auto testCases = Config::getTestCaseList();
    const int NEV = Config::DEFAULT_NEV;
    const double SIGMA = Config::DEFAULT_SIGMA;

    // 运行测试、收集结果
    std::vector<std::vector<QEP::CaseResult>> allResults;
    for (const auto &tc : testCases)
    {
        allResults.push_back(
            processCase(tc, NEV, SIGMA,
                        Config::ENABLE_CONDITION_ESTIMATION,
                        Config::ENABLE_MATRIX_PROPERTY_CHECK));
    }

    // 输出报告
    if (Config::ENABLE_SUMMARY_REPORT)
        printSummaryReport(testCases, allResults, SIGMA);
    return 0;
}
