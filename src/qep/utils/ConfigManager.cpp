//==============================================================================
//  src/qep/utils/ConfigManager.cpp  —  JSON 配置文件解析与运行时配置管理
//==============================================================================

#include "config/ConfigManager.h"
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <filesystem>
#include <functional>
#include <set>
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef EIGEN_USE_MKL_ALL
#include <mkl.h>
#endif
#include <algorithm>

namespace Config {

// ============ 辅助函数 ============

// 尝试获取配置文件的搜索路径列表
static std::vector<std::string> getConfigSearchPaths()
{
    std::vector<std::string> paths;

    // 1. 当前工作目录
    paths.push_back("config.json");

    // 2. 从当前目录往上搜索（最多 6 级），适配深层构建目录
    try {
        auto dir = std::filesystem::current_path();
        for (int i = 0; i < 6; ++i) {
            paths.push_back((dir / "config.json").string());
            auto parent = dir.parent_path();
            if (parent == dir) break; // 到达根目录
            dir = parent;
        }
    } catch (...) {}

    return paths;
}

// ============ ConfigManager 实现 ============

bool ConfigManager::load(const std::string &path)
{
    auto &inst = instance();

    // Always initialize with empty JSON so data() is never null
    if (!inst.data_)
        inst.data_ = std::make_unique<nlohmann::json>();

    std::string config_path = path;
    if (config_path.empty())
    {
        // 尝试搜索默认路径
        auto search_paths = getConfigSearchPaths();
        for (const auto &p : search_paths)
        {
            std::ifstream test(p);
            if (test.good())
            {
                config_path = p;
                break;
            }
        }
    }

    if (config_path.empty())
    {
        std::cerr << "[ConfigManager] Error: config.json not found.\n";
        std::cerr << "[ConfigManager] Place config.json in the project root or working directory.\n";
        return false;
    }

    std::ifstream file(config_path);
    if (!file.is_open())
    {
        std::cerr << "[ConfigManager] Error: cannot open config file: " << config_path << "\n";
        return false;
    }

    try
    {
        auto tmp = std::make_unique<nlohmann::json>();
        file >> *tmp;

        if (tmp->empty())
        {
            std::cerr << "[ConfigManager] Error: config file is empty.\n";
            return false;
        }

        // 解析成功，替换 data_
        inst.data_ = std::move(tmp);
        inst.config_path_ = config_path;

        // 获取问题基础路径
        inst.base_path_ = inst.data_->value("problem_base_path", "../Problems");

        // 计算 config.json 所在目录，用于相对路径解析
        auto config_dir = std::filesystem::path(config_path).parent_path();
        if (!config_dir.empty())
        {
            auto base = std::filesystem::path(inst.base_path_);
            if (base.is_relative())
            {
                inst.base_path_ = (config_dir / base).lexically_normal().string();
            }
        }

        // 自动应用并行设置
#ifdef _OPENMP
        omp_set_num_threads(inst.ompNumThreads());
#endif
#ifdef EIGEN_USE_MKL_ALL
        mkl_set_num_threads(inst.mklNumThreads());
#endif
    }
    catch (const std::exception &e)
    {
        std::cerr << "[ConfigManager] Failed to parse config: " << e.what() << "\n";
        return false;
    }

    return true;
}

ConfigManager &ConfigManager::instance()
{
    static ConfigManager inst;
    return inst;
}

// ---- 日志与调试 ----
bool ConfigManager::enableMatrixPropertyCheck() const
{
    return data_->value("logging", nlohmann::json::object()).value("enable_matrix_property_check", false);
}

bool ConfigManager::enableConditionEstimation() const
{
    return data_->value("logging", nlohmann::json::object()).value("enable_condition_estimation", false);
}

bool ConfigManager::printEigenvalues() const
{
    return data_->value("logging", nlohmann::json::object()).value("print_eigenvalues", true);
}

bool ConfigManager::printResiduals() const
{
    return data_->value("logging", nlohmann::json::object()).value("print_residuals", true);
}

bool ConfigManager::enableSummaryReport() const
{
    return data_->value("logging", nlohmann::json::object()).value("enable_summary_report", false);
}

// ---- 求解器全局参数 ----
int ConfigManager::defaultNEV() const
{
    return data_->value("solver", nlohmann::json::object()).value("default_nev", 10);
}

double ConfigManager::defaultSigma() const
{
    return data_->value("solver", nlohmann::json::object()).value("default_sigma", 0.0);
}

int ConfigManager::arnoldiMaxIterations() const
{
    return data_->value("solver", nlohmann::json::object()).value("arnoldi_max_iterations", 1000);
}

double ConfigManager::arnoldiTolerance() const
{
    return data_->value("solver", nlohmann::json::object()).value("arnoldi_tolerance", 1e-6);
}

double ConfigManager::checkTolerance() const
{
    return data_->value("solver", nlohmann::json::object()).value("check_tolerance", 1e-6);
}

// ---- 线性求解器基线参数 ----
double ConfigManager::innerTolerance() const
{
    return data_->value("solver", nlohmann::json::object()).value("inner_tolerance", 1e-8);
}

int ConfigManager::innerMaxIterations() const
{
    return data_->value("solver", nlohmann::json::object()).value("inner_max_iterations", 1000);
}

int ConfigManager::ilutFillFactor() const
{
    return data_->value("solver", nlohmann::json::object()).value("ilut_fill_factor", 10);
}

double ConfigManager::ilutDropTol() const
{
    return data_->value("solver", nlohmann::json::object()).value("ilut_drop_tol", 1e-4);
}

int ConfigManager::gmresRestart() const
{
    return data_->value("solver", nlohmann::json::object()).value("gmres_restart", 30);
}

// ---- 求解器开关 ----
bool ConfigManager::isSolverEnabled(const std::string &name) const
{
    auto solvers = data_->value("solver", nlohmann::json::object()).value("enabled_solvers", nlohmann::json::object());
    return solvers.value(name, false);
}

bool ConfigManager::enableAdaptiveParameters() const
{
    return data_->value("solver", nlohmann::json::object()).value("enable_adaptive_parameters", false);
}

// ---- 二进制转换 ----
bool ConfigManager::autoConvertBinary() const
{
    return data_->value("binary_conversion", nlohmann::json::object()).value("auto_convert", false);
}

bool ConfigManager::overwriteBinary() const
{
    return data_->value("binary_conversion", nlohmann::json::object()).value("overwrite", false);
}

// ---- 并行环境 ----
int ConfigManager::ompNumThreads() const
{
    return data_->value("parallel", nlohmann::json::object()).value("omp_num_threads", 12);
}

int ConfigManager::mklNumThreads() const
{
    return data_->value("parallel", nlohmann::json::object()).value("mkl_num_threads", 12);
}

// ---- 工厂方法 ----
SolverParams ConfigManager::toSolverParams() const {
    SolverParams p;
    p.arnoldi_max_iterations = arnoldiMaxIterations();
    p.arnoldi_tolerance      = arnoldiTolerance();
    p.inner_tolerance        = innerTolerance();
    p.inner_max_iterations   = innerMaxIterations();
    p.ilut_fill_factor       = ilutFillFactor();
    p.ilut_drop_tol          = ilutDropTol();
    p.gmres_restart          = gmresRestart();
    p.enable_adaptive        = enableAdaptiveParameters();
    p.enable_condition_estimation = enableConditionEstimation();
    p.omp_threads = ompNumThreads();
    p.mkl_threads = mklNumThreads();
    return p;
}

AppConfig ConfigManager::toAppConfig() const {
    return AppConfig::fromConfig(*this);
}

// ---- 测试用例列表 ----
std::vector<TestCase> ConfigManager::testCases() const
{
    std::vector<TestCase> cases;
    auto &base = base_path_;

    std::set<std::string> configuredNames;

    auto arr = data_->value("test_cases", nlohmann::json::array());
    for (const auto &item : arr)
    {
        TestCase tc;
        tc.name = item.value("name", "Unnamed");
        tc.M_file = base + "/" + item.value("M_file", "");
        tc.C_file = base + "/" + item.value("C_file", "");
        tc.K_file = base + "/" + item.value("K_file", "");
        tc.is_sparse = item.value("is_sparse", true);
        tc.group = item.value("group", "");
        tc.description = item.value("description", "");
        tc.overrides = item.value("overrides", nlohmann::json::object());
        configuredNames.insert(tc.name);
        cases.push_back(tc);
    }

    // 递归扫描 Problems/ 目录
    try {
        std::filesystem::path basePath(base);
        if (!std::filesystem::exists(basePath)) {
            if (cases.empty())
                std::cerr << "[ConfigManager] Problem path not found: " << base << "\n";
                // 仅仅警告，让调用方处理空列表
        } else {
            // 递归收集所有包含 M/C/K 文件的目录
            std::function<void(const std::filesystem::path&)> scanDir =
                [&](const std::filesystem::path &dir) {
                for (const auto &entry : std::filesystem::directory_iterator(dir)) {
                    if (!entry.is_directory()) continue;
                    auto &sub = entry.path();
                    // 跳过隐藏文件和空占位目录
                    auto dirname = sub.filename().string();
                    if (dirname.empty() || dirname[0] == '.') continue;

                    // 检查此目录是否直接包含 M/C/K
                    auto hasFile = [&](const std::string &suffix) {
                        return std::filesystem::exists(sub / ("M" + suffix)) &&
                               std::filesystem::exists(sub / ("C" + suffix)) &&
                               std::filesystem::exists(sub / ("K" + suffix));
                    };
                    std::string ext;
                    if (hasFile(".bin")) ext = ".bin";
                    else if (hasFile(".txt")) ext = ".txt";

                    if (!ext.empty()) {
                        // 找到问题：目录名 = name，父目录名 = group
                        std::string nm = sub.filename().string();
                        std::string grp = (sub.parent_path() == basePath)
                            ? "" : sub.parent_path().filename().string();

                        if (configuredNames.count(nm)) {
                            std::cerr << "[ConfigManager] Duplicate problem '" << nm
                                      << "' at " << sub << " — skipped\n";
                            continue;
                        }

                        TestCase tc;
                        tc.name = nm;
                        tc.M_file = (sub / ("M" + ext)).string();
                        tc.C_file = (sub / ("C" + ext)).string();
                        tc.K_file = (sub / ("K" + ext)).string();
                        tc.is_sparse = true;
                        tc.group = grp;

                        // 读取 problem.json（可选）
                        auto descPath = sub / "problem.json";
                        if (std::filesystem::exists(descPath)) {
                            try {
                                std::ifstream df(descPath);
                                nlohmann::json desc;
                                df >> desc;
                                if (desc.contains("name")) tc.name = desc["name"].get<std::string>();
                                if (desc.contains("group")) tc.group = desc["group"].get<std::string>();
                                if (desc.contains("description")) tc.description = desc["description"].get<std::string>();
                                if (desc.contains("is_sparse")) tc.is_sparse = desc["is_sparse"].get<bool>();
                                if (desc.contains("overrides")) tc.overrides = desc["overrides"];
                            } catch (...) {}
                        }
                        configuredNames.insert(nm);
                        cases.push_back(tc);
                    } else {
                        // 不包含 M/C/K，递归深入
                        scanDir(sub);
                    }
                }
            };
            scanDir(basePath);
        }
    } catch (...) {}

    if (cases.empty())
        std::cerr << "[ConfigManager] No problems found. Place M.bin/C.bin/K.bin files under "
                  << std::filesystem::absolute(base).string() << "/<problem_name>/\n";

    return cases;
}

// ---- 激活的用例（JSON 临时选择）----
std::vector<std::string> ConfigManager::activeCases() const
{
    std::vector<std::string> names;
    auto arr = data_->value("active_cases", nlohmann::json::array());
    for (const auto &item : arr)
    {
        if (item.is_string())
            names.push_back(item.get<std::string>());
    }
    return names;
}

bool ConfigManager::hasActiveCases() const
{
    auto it = data_->find("active_cases");
    return it != data_->end() && it->is_array() && !it->empty();
}

// ---- 问题基础路径 ----
std::string ConfigManager::problemBasePath() const
{
    return base_path_;
}

// ---- 保存配置到文件 ----
bool ConfigManager::save() const
{
    if (config_path_.empty()) return false;
    std::ofstream f(config_path_);
    if (!f.is_open()) return false;
    f << std::setw(4) << *data_ << std::endl;
    return true;
}

// ---- AppConfig::fromConfig 实现 ----
AppConfig AppConfig::fromConfig(const ConfigManager &cfg) {
    AppConfig c;
    const auto &j = cfg.data();
    auto &log = j.value("logging", nlohmann::json::object());
    c.print_eigenvalues  = log.value("print_eigenvalues", true);
    c.print_residuals    = log.value("print_residuals", true);
    c.enable_summary     = log.value("enable_summary_report", false);
    c.enable_matrix_property_check = log.value("enable_matrix_property_check", false);
    c.enable_condition_estimation  = log.value("enable_condition_estimation", false);
    c.problem_base_path = j.value("problem_base_path", "Problems");
    c.check_tolerance = j.value("solver", nlohmann::json::object()).value("check_tolerance", 1e-6);
    auto &sol = j.value("solver", nlohmann::json::object());
    c.default_nev   = sol.value("default_nev", 10);
    c.default_sigma = sol.value("default_sigma", 0.0);
    auto &bc = j.value("binary_conversion", nlohmann::json::object());
    c.auto_convert = bc.value("auto_convert", false);
    c.overwrite    = bc.value("overwrite", false);
    c.active_cases = cfg.activeCases();
    c.test_cases   = cfg.testCases();
    return c;
}

} // namespace Config
