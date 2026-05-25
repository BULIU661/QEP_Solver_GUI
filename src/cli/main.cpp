//==============================================================================
//  src/cli/main.cpp  —  CLI 主入口，命令行参数解析，用例筛选，测试调度
//==============================================================================

#include "TestHarness.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"
#include "config/SolverParams.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <vector>
#include <string>
#include <algorithm>
#include <nlohmann/json.hpp>

#ifdef EIGEN_USE_MKL_ALL
#include <mkl.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

// ---- 通配符匹配 ----
static bool wildcardMatch(const std::string &text, const std::string &pattern)
{
    if (pattern == "*") return true;
    size_t ti = 0, pi = 0;
    size_t starIdx = std::string::npos, matchIdx = 0;
    while (ti < text.size())
    {
        if (pi < pattern.size() && (pattern[pi] == '?' || pattern[pi] == text[ti]))
        {
            ++ti; ++pi;
        }
        else if (pi < pattern.size() && pattern[pi] == '*')
        {
            starIdx = pi;
            matchIdx = ti;
            ++pi;
        }
        else if (starIdx != std::string::npos)
        {
            pi = starIdx + 1;
            matchIdx++;
            ti = matchIdx;
        }
        else return false;
    }
    while (pi < pattern.size() && pattern[pi] == '*') ++pi;
    return pi == pattern.size();
}

static bool matchesFilter(const std::string &name, const std::string &filter)
{
    if (filter.empty()) return true;
    // 支持逗号分隔的多值
    std::string f = filter;
    size_t pos = 0;
    while ((pos = f.find(',')) != std::string::npos)
    {
        std::string part = f.substr(0, pos);
        // 去除前后空格
        part.erase(0, part.find_first_not_of(" \t"));
        part.erase(part.find_last_not_of(" \t") + 1);
        if (wildcardMatch(name, part)) return true;
        f.erase(0, pos + 1);
    }
    f.erase(0, f.find_first_not_of(" \t"));
    f.erase(f.find_last_not_of(" \t") + 1);
    return wildcardMatch(name, f);
}

// 打印用法
static void printHelp(const char *prog)
{
    std::cout << "Usage: " << prog << " [options]\n\n"
              << "Options:\n"
              << "  --config <path>   Path to config.json (auto-searched if omitted)\n"
              << "  --case <pattern>  Run cases matching name (comma-separated, * wildcard)\n"
              << "  --group <name>    Run cases in specified group\n"
              << "  --solver <name>   Run only solver(s) matching name (substring)\n"
              << "  --nev <n>         Override number of eigenvalues\n"
              << "  --sigma <value>   Override shift point\n"
              << "  --tol <value>     Override check tolerance\n"
              << "  --verbose, -v     Verbose output (solver stats, file read diagnostics)\n"
              << "  --quiet, -q       Quiet mode (summary report only)\n"
              << "  --list            List all available test cases and exit\n"
              << "  --help            Show this help and exit\n"
              << "\nExamples:\n"
              << "  " << prog << " --list\n"
              << "  " << prog << " --case sym_pos_def\n"
              << "  " << prog << " --case \"turbine*,plate*\" --solver Pardiso\n"
              << "  " << prog << " --group engineering --nev 5\n"
              << "  " << prog << " --case test1 --sigma -0.01 --nev 10 --tol 1e-8\n";
}

// 列出所有可用用例
static void listCases(const std::vector<Config::TestCase> &cases)
{
    std::cout << "\nAvailable test cases (" << cases.size() << " total):\n\n";
    // 按 group 分组显示
    std::string currentGroup;
    for (const auto &tc : cases)
    {
        if (tc.group != currentGroup)
        {
            currentGroup = tc.group;
            std::cout << "  [" << (currentGroup.empty() ? "ungrouped" : currentGroup) << "]\n";
        }
        std::cout << "    " << tc.name
                  << "  [" << tc.group << "]";
        if (!tc.description.empty())
            std::cout << "  — " << tc.description;
        std::cout << "\n";
    }
    std::cout << std::endl;
}

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

#ifdef _OPENMP
    omp_set_nested(0);
    omp_set_dynamic(0);
#endif

#ifdef EIGEN_USE_MKL_ALL
    mkl_set_dynamic(0);
#endif

    // ---- 环境信息 ----
    std::cout << "\n  QEP Solver -- Quadratic Eigenvalue Problem Solver\n\n";
    {
#ifdef EIGEN_USE_MKL_ALL
        char ver[128]; MKL_Get_Version_String(ver, 128);
        std::string v(ver);
        // 提取 "Intel ... oneAPI ... MKL ... Version X.Y" 作为简短版本
        auto pos = v.find("Version");
        std::string shortVer = (pos != std::string::npos) ? v.substr(0, pos + 13) : v.substr(0, 70);
        if (shortVer.size() < v.size()) shortVer += " ...";
        std::cout << "  MKL: " << shortVer << "\n";
#else
        std::cout << "  MKL: disabled\n";
#endif
    }
    {
#ifdef _OPENMP
        std::cout << "  OpenMP: enabled\n";
#else
        std::cout << "  OpenMP: disabled\n";
#endif
    }
    {
#ifdef EIGEN_VECTORIZE_AVX
        std::cout << "  AVX2 : enabled\n";
#else
        std::cout << "  AVX2 : disabled\n";
#endif
    }
}

int main(int argc, char *argv[])
{
    // ---- 第1步：解析命令行参数 ----
    std::string configPath;
    std::string caseFilter;
    std::string groupFilter;
    std::string solverFilter;
    nlohmann::json cliOverrides = nlohmann::json::object();
    bool doList = false;
    bool verbose = false;
    bool quiet = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--help")
        {
            printHelp(argv[0]);
            return 0;
        }
        else if (arg == "--list")
        {
            doList = true;
        }
        else if (arg == "--config" && i + 1 < argc)
        {
            configPath = argv[++i];
        }
        else if (arg == "--case" && i + 1 < argc)
        {
            caseFilter = argv[++i];
        }
        else if (arg == "--group" && i + 1 < argc)
        {
            groupFilter = argv[++i];
        }
        else if (arg == "--solver" && i + 1 < argc)
        {
            solverFilter = argv[++i];
        }
        else if (arg == "--nev" && i + 1 < argc)
        {
            try { cliOverrides["nev"] = std::stoi(argv[++i]); }
            catch (...) { std::cerr << "Error: invalid value for --nev: " << argv[i] << "\n"; return 1; }
        }
        else if (arg == "--sigma" && i + 1 < argc)
        {
            try { cliOverrides["sigma"] = std::stod(argv[++i]); }
            catch (...) { std::cerr << "Error: invalid value for --sigma: " << argv[i] << "\n"; return 1; }
        }
        else if (arg == "--tol" && i + 1 < argc)
        {
            try { cliOverrides["check_tolerance"] = std::stod(argv[++i]); }
            catch (...) { std::cerr << "Error: invalid value for --tol: " << argv[i] << "\n"; return 1; }
        }
        else if (arg == "--verbose" || arg == "-v")
        {
            verbose = true;
        }
        else if (arg == "--quiet" || arg == "-q")
        {
            quiet = true;
        }
        // 兼容旧式位置参数
        else if (arg[0] != '-' && !cliOverrides.contains("sigma"))
        {
            try { cliOverrides["sigma"] = std::stod(arg); }
            catch (...) { std::cerr << "Error: invalid positional argument: " << arg << "\n"; return 1; }
        }
        else if (arg[0] != '-' && cliOverrides.contains("sigma") && !cliOverrides.contains("nev"))
        {
            try { cliOverrides["nev"] = std::stoi(arg); }
            catch (...) { std::cerr << "Error: invalid positional argument: " << arg << "\n"; return 1; }
        }
    }

    // ---- 第2步：加载配置文件 ----
    if (!Config::ConfigManager::load(configPath))
    {
        std::cerr << "Fatal: cannot load config file, exiting.\n";
        return 1;
    }

    auto &cfg = Config::ConfigManager::instance();

    auto appCfg  = cfg.toAppConfig();
    auto solParams = cfg.toSolverParams();

    // 将 CLI nev/sigma 也写入 config data（供旧代码路径使用）
    if (cliOverrides.contains("nev"))
        cfg.data()["solver"]["default_nev"] = cliOverrides["nev"].get<int>();
    if (cliOverrides.contains("sigma"))
        cfg.data()["solver"]["default_sigma"] = cliOverrides["sigma"].get<double>();

    // ---- 第3步：初始化环境 ----
    initEnvironment();

    // ---- 第4步：获取并筛选测试用例 ----
    auto allCases = appCfg.test_cases;

    // --list 模式
    if (doList)
    {
        listCases(allCases);
        return 0;
    }

    std::vector<Config::TestCase> selectedCases;

    // 第1层：JSON active_cases（如果配了，只跑这些）
    if (!appCfg.active_cases.empty())
    {
        const auto &active = appCfg.active_cases;
        for (const auto &tc : allCases)
        {
            bool inActive = false;
            for (const auto &name : active)
            {
                if (wildcardMatch(tc.name, name))
                { inActive = true; break; }
            }
            if (!inActive) continue;
            // 第2层：CLI --case/--group 进一步筛选
            if (!caseFilter.empty() && !matchesFilter(tc.name, caseFilter))
                continue;
            if (!groupFilter.empty() && tc.group != groupFilter)
                continue;
            selectedCases.push_back(tc);
        }
    }
    else
    {
        for (const auto &tc : allCases)
        {
            if (!caseFilter.empty() && !matchesFilter(tc.name, caseFilter))
                continue;
            if (!groupFilter.empty() && tc.group != groupFilter)
                continue;
            selectedCases.push_back(tc);
        }
    }

    if (selectedCases.empty())
    {
        std::cerr << "No problems found in " << std::filesystem::absolute(appCfg.problem_base_path).string()
                  << "\nPlace M.bin/C.bin/K.bin files in a subdirectory (e.g. Problems/my_problem/).\n"
                  << "Or add a problem.json for metadata. Use --list to see available cases.\n";
        if (!appCfg.active_cases.empty())
            std::cout << "  active_cases is set in config.json\n";
        if (!caseFilter.empty())
            std::cout << "  case filter: \"" << caseFilter << "\"\n";
        if (!groupFilter.empty())
            std::cout << "  group filter: \"" << groupFilter << "\"\n";
        return 1;
    }

    int nevDisp = resolveParamI(cliOverrides, "nev", appCfg.default_nev);
    double sigmaDisp = resolveParamD(cliOverrides, "sigma", appCfg.default_sigma);

    std::cout << fmt::secHdr("Configuration") << "\n";
    std::cout << fmt::secLine("Config file", fmt::truncate(cfg.configFilePath(), 60)) << "\n";
    std::cout << fmt::secLine("Problem path", cfg.problemBasePath()) << "\n";
    std::cout << fmt::secLine("NEV", std::to_string(nevDisp)) << "\n";
    {
        std::ostringstream s; s << std::scientific << std::setprecision(2) << sigmaDisp;
        std::cout << fmt::secLine("Sigma", s.str()) << "\n";
    }
    if (!appCfg.active_cases.empty()) {
        std::ostringstream ac;
        for (size_t i = 0; i < appCfg.active_cases.size(); ++i)
            ac << (i ? ", " : "") << appCfg.active_cases[i];
        std::cout << fmt::secLine("Active cases", ac.str()) << "\n";
    }
    std::cout << fmt::secLine("Test cases", std::to_string(selectedCases.size()) + " / " + std::to_string(allCases.size()) + " selected") << "\n";
    {
        std::ostringstream s;
        s << "OpenMP: " << solParams.omp_threads << " threads, MKL: " << solParams.mkl_threads << " threads";
        std::cout << fmt::secLine("Parallel", s.str()) << "\n";
    }
    if (appCfg.auto_convert) {
        std::cout << fmt::secLine("Binary conv", "auto_convert: on, overwrite: " + std::string(appCfg.overwrite ? "on" : "off")) << "\n";
    }
    {
        std::ostringstream s;
        s << "summary: " << (appCfg.enable_summary ? "on" : "off")
          << ", cond-est: " << (appCfg.enable_condition_estimation ? "on" : "off")
          << ", matrix-check: " << (appCfg.enable_matrix_property_check ? "on" : "off");
        std::cout << fmt::secLine("Diagnostics", s.str()) << "\n";
    }

    // ---- 第5步：参数校验 ----
    int nevVal = resolveParamI(cliOverrides, "nev", appCfg.default_nev);
    double sigmaVal = resolveParamD(cliOverrides, "sigma", appCfg.default_sigma);
    double tolVal = resolveParamD(cliOverrides, "check_tolerance", appCfg.check_tolerance);

    if (nevVal <= 0) {
        std::cerr << "Error: nev must be positive, got " << nevVal << "\n";
        return 1;
    }
    if (!std::isfinite(sigmaVal)) {
        std::cerr << "Error: sigma is not a finite number\n";
        return 1;
    }
    if (tolVal < 0 || !std::isfinite(tolVal)) {
        std::cerr << "Error: tolerance must be >= 0 and finite, got " << tolVal << "\n";
        return 1;
    }

    // ---- 第6步：运行测试、收集结果 ----
    if (quiet)
    {
        cfg.data()["logging"]["print_eigenvalues"] = false;
        cfg.data()["logging"]["print_residuals"] = false;
    }

    std::vector<std::vector<QEP::CaseResult>> allResults;
    for (const auto &tc : selectedCases)
    {
        allResults.push_back(processCase(tc, appCfg, solParams, cliOverrides, solverFilter));
    }

    if (verbose)
    {
        for (const auto &caseResults : allResults)
            for (const auto &cr : caseResults)
                for (const auto &l : cr.logs)
                    std::cout << "  " << l << "\n";
    }

    // ---- 第7步：输出报告 ----
    if (appCfg.enable_summary)
        printSummaryReport(selectedCases, allResults, resolveParamD(cliOverrides, "sigma", appCfg.default_sigma));
    return 0;
}
