//==============================================================================
//  src/cli/TestHarness.cpp  —  问题加载、求解器调度、结果汇总与格式化输出
//==============================================================================

#include "TestHarness.h"
#include <iostream>
#include <sstream>
#include <cstdio>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <filesystem>
using namespace QEP;

// ---- 参数解析辅助 ----
int resolveParamI(const nlohmann::json &overrides, const std::string &key, int defaultVal)
{
    if (overrides.contains(key) && overrides[key].is_number())
        return overrides[key].get<int>();
    return defaultVal;
}

double resolveParamD(const nlohmann::json &overrides, const std::string &key, double defaultVal)
{
    if (overrides.contains(key) && overrides[key].is_number())
        return overrides[key].get<double>();
    return defaultVal;
}

// 构建合并后的参数覆盖（优先级：CLI > case > global）
static nlohmann::json buildOverrides(const Config::TestCase &tc,
                                      const nlohmann::json &cliOverrides,
                                      const Config::AppConfig &appCfg)
{
    nlohmann::json merged;
    merged["nev"]             = appCfg.default_nev;
    merged["sigma"]           = appCfg.default_sigma;
    merged["check_tolerance"] = appCfg.check_tolerance;

    if (!tc.overrides.is_null() && tc.overrides.is_object()) {
        for (auto it = tc.overrides.begin(); it != tc.overrides.end(); ++it)
            merged[it.key()] = it.value();
    }
    if (!cliOverrides.is_null() && cliOverrides.is_object()) {
        for (auto it = cliOverrides.begin(); it != cliOverrides.end(); ++it)
            merged[it.key()] = it.value();
    }
    return merged;
}

// ---------- 结果汇总函数 ----------
CaseResult runAndReport(const std::string &method_name,
                        const QuadraticEigenvalueResult &result,
                        const QuadraticEigenvalueProblem &problem,
                        const Config::AppConfig &appCfg)
{
    CaseResult cr;
    cr.method_name = method_name;
    cr.success = result.success;
    cr.n_eig = static_cast<int>(result.eigenvalues_real.size());
    cr.time = result.solution_time;
    cr.max_rel_residual = -1.0;
    cr.validation = "N/A";

    if (result.success && cr.n_eig > 0)
    {
        // ---- 特征值表 (TableBuilder) ----
        if (appCfg.print_eigenvalues)
        {
            int show = std::min(cr.n_eig, 10);
            fmt::TableBuilder eigTbl;
            eigTbl.addCol("#", 3, fmt::Align::Right);
            eigTbl.addCol("Eigenvalue", 28, fmt::Align::Center);
            eigTbl.addCol("|λ|", 14, fmt::Align::Right);

            for (int i = 0; i < show; ++i)
            {
                double re = result.eigenvalues_real(i);
                double im = result.eigenvalues_imag(i);
                double absVal = std::sqrt(re * re + im * im);

                char eigBuf[64], absBuf[32];
                if (std::abs(im) < 1e-8)
                    snprintf(eigBuf, sizeof(eigBuf), "%.6g", re);
                else if (std::abs(re) < 1e-8)
                    snprintf(eigBuf, sizeof(eigBuf), "%.6gi", im);
                else
                    snprintf(eigBuf, sizeof(eigBuf), "%.6g%+.6gi", re, im);
                snprintf(absBuf, sizeof(absBuf), "%.6g", absVal);

                eigTbl.addRow({std::to_string(i + 1), eigBuf, absBuf});
            }
            if (cr.n_eig > show)
                eigTbl.addRow({"", "(" + std::to_string(cr.n_eig - show) + " more)", ""});

            std::cout << fmt::subHdr("Eigenvalues (top " + std::to_string(show) + ")") << "\n";
            std::cout << eigTbl.render(2);
        }

        // ---- 残差验证表 ----
        {
            auto [maxRel, tableStr] = computeAndFormatResiduals(problem.M, problem.C, problem.K, result);
            cr.max_rel_residual = maxRel;
            if (appCfg.print_residuals && !tableStr.empty())
            {
                int n_eig = static_cast<int>(result.eigenvalues_real.size());
                std::cout << fmt::subHdr("Residual Validation (" + std::to_string(n_eig) + " eigenvalues)") << "\n";
                std::cout << tableStr;
            }
        }
        cr.validation = (cr.max_rel_residual >= 0) ? ((cr.max_rel_residual < appCfg.check_tolerance) ? "PASSED" : "FAILED") : "No vectors";
    }
    else
    {
        std::cout << "  Solve FAILED\n";
        cr.validation = "FAILED";
    }

    cr.logs = result.log_messages;
    for (const auto &w : result.warning_messages)
        cr.logs.push_back("[WARN] " + w);

    return cr;
}

CaseResult runSolver(const Config::SolverMethod &method,
                     const QuadraticEigenvalueProblem &problem,
                     int nev, double sigma,
                     const Config::AppConfig &appCfg)
{
    std::string principle = method.principle;
    if (method.use_sigma)
        principle += ", sigma=" + std::to_string(sigma);
    std::cout << fmt::subHdr("Solver: " + method.name) << "\n";
    std::cout << "    " << principle << "\n";

    auto start = std::chrono::high_resolution_clock::now();
    QuadraticEigenvalueResult result;
    try
    {
        result = method.solver(problem, nev, sigma);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << method.name << " solve failed - " << e.what() << std::endl;
        return {method.name, false, 0, 0.0, -1.0, std::string("Exception: ") + e.what()};
    }
    auto end = std::chrono::high_resolution_clock::now();
    double elapsed = std::chrono::duration<double>(end - start).count();
    {
        std::ostringstream ts;
        ts << std::fixed << std::setprecision(4) << elapsed << " s";
        std::cout << "    Time: " << ts.str() << "\n";
    }

    return runAndReport(method.name, result, problem, appCfg);
}

std::vector<CaseResult> processCase(const Config::TestCase &tc,
                                         const Config::AppConfig &appCfg,
                                         const Config::SolverParams &solParams,
                                         const nlohmann::json &cliOverrides,
                                         const std::string &solverFilter)
{
    auto overrides = buildOverrides(tc, cliOverrides, appCfg);
    int    nev   = resolveParamI(overrides, "nev", 10);
    double sigma = resolveParamD(overrides, "sigma", 0.0);

    std::cout << fmt::secHdr("Case: " + tc.name) << "\n";

    auto problem = createTestProblemFromFiles(tc.name, tc.M_file, tc.C_file, tc.K_file, tc.is_sparse, solParams, appCfg.overwrite);
    if (problem.dimension == 0)
    {
        std::cerr << "  Matrix load failed, skipping case.\n";
        std::vector<CaseResult> failed;
        auto methods = Config::getSolverMethods(solParams);
        for (const auto &m : methods)
            failed.push_back({m.name, false, 0, 0.0, -1.0, "Load failed"});
        return failed;
    }

    if (nev > problem.dimension) {
        std::cerr << "Warning: nev(" << nev << ") > dimension(" << problem.dimension
                  << "), clamping to dimension\n";
        nev = problem.dimension;
    }

    {
        std::ostringstream dimInfo;
        dimInfo << "n=" << problem.dimension
                << "  M_nnz=" << problem.M.nonZeros()
                << "  C_nnz=" << problem.C.nonZeros()
                << "  K_nnz=" << problem.K.nonZeros()
                << "  nev=" << nev << "  sigma=" << sigma;
        std::cout << fmt::secLine("Problem", dimInfo.str()) << "\n";
    }

    std::cout << fmt::secLine("Source", std::filesystem::path(tc.M_file).parent_path().string()) << "\n";

    bool needCond  = appCfg.enable_condition_estimation;
    bool needCheck = appCfg.enable_matrix_property_check;
    double condM = -1, condC = -1;
    double condK = problem.condition_number_K;
    if (needCond)
    {
        condC = estimateConditionNumber(problem.C);
        condM = estimateConditionNumber(problem.M);
        if (condK <= 0.0)
            condK = estimateConditionNumber(problem.K);
        if (condM > 0)
            std::cout << "  cond(M)=" << condM << "  cond(C)=" << condC << "  cond(K)=" << condK << "\n";
    }
    if (needCheck)
    {
        std::cout << "  " << printMatrixProperties(problem.M, "M", condM);
        std::cout << "  " << printMatrixProperties(problem.C, "C", condC);
        std::cout << "  " << printMatrixProperties(problem.K, "K", condK);
        std::cout << std::endl;
    }

    auto allMethods = Config::getSolverMethods(solParams);
    std::vector<Config::SolverMethod> methods;
    for (const auto &m : allMethods)
    {
        if (solverFilter.empty() ||
            m.name.find(solverFilter) != std::string::npos)
            methods.push_back(m);
    }
    if (methods.empty())
    {
        std::cout << "  No solver matches filter \"" << solverFilter << "\", using all enabled.\n";
        methods = allMethods;
    }

    std::vector<CaseResult> results;
    for (const auto &method : methods)
        results.push_back(runSolver(method, problem, nev, sigma, appCfg));
    return results;
}

void printSummaryReport(const std::vector<Config::TestCase> &cases,
                        const std::vector<std::vector<CaseResult>> &allResults,
                        double sigma)
{
    fmt::TableBuilder tbl;
    tbl.addCol("Case", 18, fmt::Align::Left);
    tbl.addCol("Method", 14, fmt::Align::Left);
    tbl.addCol("Status", 7, fmt::Align::Center);
    tbl.addCol("#Eig", 5, fmt::Align::Right);
    tbl.addCol("Time(s)", 8, fmt::Align::Right);
    tbl.addCol("Validation", 12, fmt::Align::Left);

    for (size_t ci = 0; ci < cases.size(); ++ci)
    {
        bool first = true;
        for (size_t mi = 0; mi < allResults[ci].size(); ++mi)
        {
            const CaseResult &cr = allResults[ci][mi];
            std::ostringstream timeStr;
            timeStr << std::fixed << std::setprecision(3) << cr.time;
            tbl.addRow({
                first ? cases[ci].name : "",
                cr.method_name,
                cr.success ? "OK" : "FAIL",
                std::to_string(cr.n_eig),
                timeStr.str(),
                cr.validation
            });
            first = false;
        }
        if (ci + 1 < cases.size())
            tbl.sep();
    }

    std::cout << "\n\n" << fmt::secHdr("Summary Report") << "\n";
    std::cout << tbl.render(2);
    std::cout << "\n  Validation: rel. residual < tolerance => PASSED, else FAILED\n\n";
}
