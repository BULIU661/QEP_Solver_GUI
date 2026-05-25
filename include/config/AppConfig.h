//==============================================================================
//  include/config/AppConfig.h  —  CLI/GUI 应用配置（展示、路径、测试用例）
//==============================================================================

#ifndef APP_CONFIG_H
#define APP_CONFIG_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace Config {

struct TestCase {
    std::string name;
    std::string M_file;
    std::string C_file;
    std::string K_file;
    bool is_sparse = true;
    std::string group;
    std::string description;
    nlohmann::json overrides;
};

class ConfigManager;
struct AppConfig {
    bool print_eigenvalues  = true;
    bool print_residuals    = true;
    bool enable_summary     = false;

    bool enable_matrix_property_check = false;
    bool enable_condition_estimation  = false;

    std::vector<std::string> active_cases;
    std::vector<TestCase>    test_cases;
    std::string              problem_base_path = "Problems";

    bool auto_convert = false;
    bool overwrite    = false;

    double check_tolerance = 1e-6;
    int    default_nev   = 10;
    double default_sigma = 0.0;

    static AppConfig fromConfig(const ConfigManager &cfg);
};

} // namespace Config

#endif
