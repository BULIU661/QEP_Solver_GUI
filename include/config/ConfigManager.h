//==============================================================================
//  include/config/ConfigManager.h  —  运行时配置管理器单例接口
//==============================================================================

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>
#include "SolverParams.h"
#include "AppConfig.h"

namespace Config {

// TestCase 定义在 AppConfig.h 中

// 配置管理器（单例）
class ConfigManager
{
public:
    // 加载配置文件，path 为 config.json 路径
    // 如果 path 为空，尝试默认路径
    static bool load(const std::string &path = "");

    // 获取单例实例
    static ConfigManager &instance();

    // ---- 日志与调试 ----
    bool enableMatrixPropertyCheck() const;
    bool enableConditionEstimation() const;
    bool printEigenvalues() const;
    bool printResiduals() const;
    bool enableSummaryReport() const;

    // ---- 求解器全局参数 ----
    int defaultNEV() const;
    double defaultSigma() const;
    int arnoldiMaxIterations() const;
    double arnoldiTolerance() const;
    double checkTolerance() const;

    // ---- 线性求解器基线参数 ----
    double innerTolerance() const;
    int innerMaxIterations() const;
    int ilutFillFactor() const;
    double ilutDropTol() const;
    int gmresRestart() const;

    // ---- 求解器开关 ----
    bool isSolverEnabled(const std::string &name) const;
    bool enableAdaptiveParameters() const;

    // ---- 二进制转换 ----
    bool autoConvertBinary() const;
    bool overwriteBinary() const;

    // ---- 并行环境 ----
    int ompNumThreads() const;
    int mklNumThreads() const;

    // ---- 测试用例列表 ----
    std::vector<TestCase> testCases() const;

    // ---- 激活的用例（JSON 临时选择，不改 test_cases 数组）----
    std::vector<std::string> activeCases() const;
    bool hasActiveCases() const;

    // ---- 问题基础路径 ----
    std::string problemBasePath() const;

    // ---- 配置文件路径 ----
    std::string configFilePath() const { return config_path_; }

    // ---- 读写 JSON 数据（GUI 双向绑定用） ----
    nlohmann::json& data() { return *data_; }
    const nlohmann::json& data() const { return *data_; }

    // ---- 保存配置到文件 ----
    bool save() const;

    // ---- 工厂方法：生成类型化配置结构 ----
    SolverParams toSolverParams() const;
    AppConfig    toAppConfig() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;

    std::unique_ptr<nlohmann::json> data_;
    std::string config_path_;
    std::string base_path_;
};

} // namespace Config

#endif // CONFIG_MANAGER_H
