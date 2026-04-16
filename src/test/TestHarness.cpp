// utils/TestHarness.cpp
#include "TestHarness.h"
#include "config/SolverRegistry.h"
#include <iostream>
#include <chrono>
#include <iomanip>
using namespace QEP;

// ---------- 结果汇总函数 ----------
CaseResult runAndReport(const std::string &method_name,
                        const QuadraticEigenvalueResult &result,
                        const QuadraticEigenvalueProblem &problem)
{
    CaseResult cr;
    cr.method_name = method_name;
    cr.success = result.success;
    cr.n_eig = static_cast<int>(result.eigenvalues_real.size());
    std::cout << "求得特征向量数目: " << cr.n_eig << std::endl;
    cr.time = result.solution_time;
    cr.max_rel_residual = -1.0;
    cr.validation = "N/A";

    if (result.success && cr.n_eig > 0)
    {
        if (Config::PRINT_EIGENVALUES)
        {
            int show = std::min(cr.n_eig, 10);
            std::cout << "  求得特征值 (前" << show << "个,按|lambda|排序):\n";
            for (int i = 0; i < show; ++i)
            {
                std::complex<double> lambda(result.eigenvalues_real(i), result.eigenvalues_imag(i));
                std::cout << "    [" << std::setw(2) << (i + 1) << "]  ";
                std::cout << std::fixed << std::setprecision(6);
                if (std::abs(lambda.imag()) < 1e-8)
                    std::cout << std::setw(16) << lambda.real();
                else
                    std::cout << std::setw(12) << lambda.real()
                              << (lambda.imag() >= 0 ? "+" : "") << lambda.imag() << "i";
                std::cout << std::endl;
            }
        }

        cr.max_rel_residual = printResidualTable(problem.M, problem.C, problem.K, result);
        cr.validation = (cr.max_rel_residual >= 0) ? ((cr.max_rel_residual < Config::CHECK_TOLERANCE) ? "PASSED" : "FAILED") : "无向量";
    }
    else
    {
        std::cout << "  求解失败\n";
        cr.validation = "失败";
    }
    std::cout << "  求解时间: " << std::fixed << std::setprecision(3) << result.solution_time << " s\n";
    return cr;
}

CaseResult runSolver(const Config::SolverMethod &method,
                     const QuadraticEigenvalueProblem &problem,
                     int nev, double sigma)
{
    std::string principle = method.principle;
    if (method.use_sigma)
        principle += ", sigma=" + std::to_string(sigma);
    std::cout << "\n---------- [" << method.name << "] ----------\n"
              << principle << "\n";

    auto start = std::chrono::high_resolution_clock::now();
    QuadraticEigenvalueResult result;
    try
    {
        result = method.solver(problem, nev, sigma);
    }
    catch (const std::exception &e)
    {
        std::cerr << "错误: " << method.name << "求解失败 - " << e.what() << std::endl;
        return {method.name, false, 0, 0.0, -1.0, std::string("异常: ") + e.what()};
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    std::cout << "耗时: " << elapsed << " 秒\n";

    // 调用 runAndReport 进行报告
    return runAndReport(method.name, result, problem);
}
//
std::vector<CaseResult> processCase(const Config::TestCase &tc, int nev, double sigma,
                                    bool needCondition, bool needCheckMatrixProperty)
{
    std::cout << "\n"
              << std::string(60, '=') << "\n  " << tc.name << "\n"
              << std::string(60, '=') << "\n";

    auto problem = createTestProblemFromFiles(tc.name, tc.M_file, tc.C_file, tc.K_file, tc.is_sparse);
    if (problem.dimension == 0)
    {
        std::cerr << "矩阵加载失败,跳过此案例\n";
        std::vector<CaseResult> failed;
        auto methods = Config::getSolverMethods();
        for (const auto &m : methods)
            failed.push_back({m.name, false, 0, 0.0, -1.0, "加载失败"});
        return failed;
    }

    std::cout << "矩阵维度n=" << problem.dimension
              << "  M_nnz=" << problem.M.nonZeros()
              << "  C_nnz=" << problem.C.nonZeros()
              << "  K_nnz=" << problem.K.nonZeros() << "\n";

    // 估计条件数和打印矩阵性质
    double condM = -1;
    double condC = -1;
    double condK = -1;
    if (needCondition)
    {
        condC = estimateConditionNumber(problem.C, true);
        condM = estimateConditionNumber(problem.M, true);
        condK = estimateConditionNumber(problem.K, true);
    }

    if (needCheckMatrixProperty)
    {
        PrintMatrixProperties(problem.M, "M", condM);
        PrintMatrixProperties(problem.C, "C", condC);
        PrintMatrixProperties(problem.K, "K", condK);
        std::cout << std::endl;
    }

    std::vector<CaseResult> results;
    auto methods = Config::getSolverMethods();
    for (const auto &method : methods)
    {
        results.push_back(runSolver(method, problem, nev, sigma));
    }
    return results;
}

void printSummaryReport(const std::vector<Config::TestCase> &cases,
                        const std::vector<std::vector<CaseResult>> &allResults,
                        double sigma)
{
    const int W_CASE = 18, W_METHOD = 14, W_STATUS = 8, W_NEIG = 8, W_TIME = 10, W_VALID = 12;
    int tableWidth = 1 + W_CASE + 3 + W_METHOD + 3 + W_STATUS + 3 + W_NEIG + 3 + W_TIME + 3 + W_VALID + 2;
    std::string separator = "+" + std::string(W_CASE + 2, '-') + "+" + std::string(W_METHOD + 2, '-') +
                            "+" + std::string(W_STATUS + 2, '-') + "+" + std::string(W_NEIG + 2, '-') +
                            "+" + std::string(W_TIME + 2, '-') + "+" + std::string(W_VALID + 2, '-') + "+";

    std::cout << "\n\n"
              << std::string(tableWidth, '=') << "\n  汇总报告\n"
              << std::string(tableWidth, '=') << "\n";
    std::cout << separator << "\n"
              << "| " << padToWidth("案例", W_CASE)
              << " | " << padToWidth("方法", W_METHOD)
              << " | " << padToWidth("状态", W_STATUS)
              << " | " << padToWidthRight("特征值数", W_NEIG)
              << " | " << std::fixed << std::setprecision(3) << padToWidthRight("时间(s)", W_TIME) << std::defaultfloat
              << " | " << padToWidth("验证状态", W_VALID)
              << " |\n"
              << separator << "\n";

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        for (size_t mi = 0; mi < allResults[ci].size(); ++mi)
        {
            const CaseResult &cr = allResults[ci][mi];
            std::string caseName = (mi == 0) ? cases[ci].name : "";
            std::string status = cr.success ? "成功" : "失败";
            std::ostringstream timeStr, neigStr;
            timeStr << std::fixed << std::setprecision(3) << cr.time;
            neigStr << cr.n_eig;

            std::cout << "| " << padToWidth(caseName, W_CASE)
                      << " | " << padToWidth(cr.method_name, W_METHOD)
                      << " | " << padToWidth(status, W_STATUS)
                      << " | " << padToWidthRight(neigStr.str(), W_NEIG)
                      << " | " << padToWidthRight(timeStr.str(), W_TIME)
                      << " | " << padToWidth(cr.validation, W_VALID)
                      << " |\n";
        }
        std::cout << separator << "\n";
    }

    std::cout << "\n方法说明:\n";
    auto methods = Config::getSolverMethods();
    for (const auto &method : methods)
    {
        std::cout << "  " << method.name;
        if (method.use_sigma)
            std::cout << ", sigma=" << sigma;
        std::cout << "\n";
    }
    std::cout << "\n验证标准:\n"
              << " 相对残差 < 1e-6 为 PASSED\n"
              << " 相对残差 > 1e6  为 FAILED\n"
              << " 内部求解器未收敛 为 失败\n\n"
              << std::string(tableWidth, '=') << "\n  程序执行完毕\n"
              << std::string(tableWidth, '=') << "\n";
}
