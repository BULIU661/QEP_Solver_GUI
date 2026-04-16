// TestCases.h
#ifndef TEST_CASE_LIST_H
#define TEST_CASE_LIST_H

#include "config/GlobalConfig.h"
#include <vector>
#include <string>

namespace Config
{
    struct TestCase
    {
        std::string name;
        std::string M_file;
        std::string C_file;
        std::string K_file;
        bool is_sparse;
    };

    inline std::vector<TestCase> getTestCaseList()
    {
        std::string base = Config::PROBLEM_BASE_PATH;
        return {
            // 用例顺序决定了输出报告的顺序，且输入矩阵顺序要按照结构体中设置的名称顺序,即M,C,K
            {"100维测试问题，测试移频方法有效性",// 
            // {"问题1(C++测试)",// 
             base + "problemTest/M.bin", base + "problemTest/C.bin", base + "problemTest/K.bin", true},
            // {"100维对称正定问题",
            //  base + "problemA/M.bin", base + "problemA/C.bin", base + "problemA/K.bin", true},
            // {"100维对称半正定问题",
            //  base + "problemB/M.bin", base + "problemB/C.bin", base + "problemB/K.bin", true},
            // {"100维不对称问题",
            //  base + "problemC/M.bin", base + "problemC/C.bin", base + "problemC/K.bin", true},
            // {"100维对称不定问题",
            //  base + "problemD/M.bin", base + "problemD/C.bin", base + "problemD/K.bin", true},
            // {"1千维发电机转子问题",
            //  base + "pro1/M.bin", base + "pro1/C.bin", base + "pro1/K.bin", true},
            // {"1万维板子问题",
            //  base + "pro2/M.bin", base + "pro2/C.bin", base + "pro2/K.bin", true},
            // {"56万维水轮机问题",
            //  base + "pro3/M.bin", base + "pro3/C.bin", base + "pro3/K.bin", true},
            // {"78万维振动筛问题",
            //  base + "pro4/M.bin", base + "pro4/C.bin", base + "pro4/K.bin", true},
            //   {"3维qep1问题",
            //   "../TestProblems/M1.bin", "../TestProblems/C1.bin", "../TestProblems/K1.bin",true},
            // {"20万维 spring 问题",
            //  "../TestProblems/M1.bin", "../TestProblems/C1.bin", "../TestProblems/K1.bin", true},  // 20万维对称正定问题，条件数良好，但是谱分布密集
            // {"1万维 damped_beam 问题",
            //  "../TestProblems/M2.bin", "../TestProblems/C2.bin", "../TestProblems/K2.bin", true},  // 1万维对称正定问题,条件数极差，C半正定
            // {"20万维 acoustic_wave_1d 问题",
            //  "../TestProblems/M3.bin", "../TestProblems/C3.bin", "../TestProblems/K3.bin", true},  // 20万维非对称QEP问题，M对称不定，C非对称
            // {"1万维 wiresaw1 问题",
            //  "../TestProblems/M4.bin", "../TestProblems/C4.bin", "../TestProblems/K4.bin", true},  // 1万维对称正定问题，C非对称
            // {"5000维 gen_hyper2 问题(满矩阵)",
            //  "../TestProblems/M5.bin", "../TestProblems/C5.bin", "../TestProblems/K5.bin", false},
        };
    }
}
#endif // TEST_CASE_LIST_H
