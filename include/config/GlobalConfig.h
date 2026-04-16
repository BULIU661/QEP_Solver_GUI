// config/GlobalConfig.h
#ifndef GLOBAL_CONFIG_H
#define GLOBAL_CONFIG_H

#include <string>
#include <algorithm> // for std::min, std::max
#include "qep/QEP.h"

namespace Config
{
    // ========== 日志与调试 ==========
    inline int LOG_LEVEL = 2;                        // 0=仅输出结果, 1=简洁关键信息, 2=显示求解过程，3=显示文件读取过程
    inline bool ENABLE_MATRIX_PROPERTY_CHECK = false; // 启用矩阵属性检查（定性判断不准确，还未完善）
    inline bool ENABLE_CONDITION_ESTIMATION = true;  // 启用条件数估计 需启用 Pardiso
    inline bool PRINT_EIGENVALUES = false;           // 启用特征值打印，显示求解得到的特征值
    inline bool PRINT_RESIDUALS = true;             // 启用残差打印，显示每个特征对的残差，便于分析收敛情况
    inline bool ENABLE_SUMMARY_REPORT = false;       // 启用总结报告，输出每个问题的关键统计信息，便于分析和比较
    

    // ========== 求解器全局参数 ==========
    inline int DEFAULT_NEV = 10;              // 默认求特征值个数
    inline double DEFAULT_SIGMA = 0.0;        // 移频点
    inline int ARNOLDI_MAX_ITERATIONS = 1000; // 外层最大迭代次数（良态时)
    inline double ARNOLDI_TOLERANCE = 1e-6;   // 外层容差（良态时）
    inline double CHECK_TOLERANCE = 1e-6;     // 检查容差   用于打印表格时判断收敛(未设置 默认为1e-6)

    // 是否启用自适应参数
    inline bool ENABLE_ADAPTIVE_PARAMETERS = false; // 启用自适应参数(未完善 暂时不可以)

    // ncv 基础参数（内部未使用）
    // inline int NCV_FACTOR_SMALL = 3;  // n < 10000
    // inline int NCV_FACTOR_MEDIUM = 2; // 10000 ≤ n < 100000
    // inline int NCV_FACTOR_LARGE = 2;  // n ≥ 100000
    // inline int NCV_OFFSET = 6;       // ncv = nev * factor + offset
    // inline int NCV_MAX = 100;        // 提高上限，适应大问题

    // ========== 线性求解器基线参数 ==========
    // 迭代法基线（良态时使用）
    inline double INNER_TOLERANCE = 1e-5;   // 内层容差
    inline int INNER_MAX_ITERATIONS = 2000; // 内层最大迭代次数
    inline int ILUT_FILL_FACTOR = 10;         // ILUT 预条件填充因子，控制非零元素数量，较大值增强预条件但增加内存和计算成本
    inline double ILUT_DROP_TOL = 1e-4;  // ILUT 丢弃容差，控制预条件中小元素的丢弃，较小值增强预条件但增加内存和计算成本
    inline int GMRES_RESTART = 100;  // GMRES 重启参数，控制 Krylov 子空间的维度，较大值增强收敛性但增加内存和计算成本

    // ========== 求解器开关 ==========
    inline bool ENABLE_PARDISO = true;       // 直接法，性能优异，适用范围广，但依赖 MKL
    inline bool ENABLE_SPARSELU = true;      // 直接法，纯 Eigen 实现，无外部依赖，但性能较差，适合小规模问题
    inline bool ENABLE_SIMPLICIALLLT = true; // 直接法，适用于对称正定问题，性能较好，但适用范围受限
    inline bool ENABLE_CG = true;            // 适用于对称正定问题，内存占用低，但对预条件要求高，可能不收敛
    inline bool ENABLE_BICGSTAB = true;      // 适用于非对称或不定问题，内存占用适中，但可能震荡不收敛
    inline bool ENABLE_GMRES = true;         // 适用于非对称或不定问题，内存占用较高，但收敛性较好，适合病态问题

    // ========== 二进制转换 ==========
    inline bool AUTO_CONVERT_BINARY = false; // 自动转换文本矩阵为二进制（会检查文件是否存在，如果存在，默认不转换，除非打开覆盖开关）
    inline bool OVERWRITE_BINARY = false;    // 覆盖二进制文件

    // ========== 路径配置 ==========
    inline std::string PROBLEM_BASE_PATH = "../Problems/";

    // ========== 并行环境 ==========
    inline int OMP_NUM_THREADS = 4;
    inline int MKL_NUM_THREADS = 4;

    // 直接法无需内层迭代参数，但可调整外层容差

    // ========== 自适应参数策略 ==========
    // 参数配置（不包含自适应），目前仅被自适应参数函数调用
    inline QEP::LinearSolverConfig getSolverConfig(QEP::LinearSolverType type)
    {
        QEP::LinearSolverConfig cfg(type);
        cfg.inner_tolerance = INNER_TOLERANCE;
        cfg.inner_maxIterations = INNER_MAX_ITERATIONS;
        cfg.outer_maxIterations = ARNOLDI_MAX_ITERATIONS;
        cfg.outer_tolerance = ARNOLDI_TOLERANCE;
        cfg.fillFactor = ILUT_FILL_FACTOR;
        cfg.dropTol = ILUT_DROP_TOL;
        cfg.restart = GMRES_RESTART;
        cfg.usePreconditioner = true;
        return cfg;
    }

    // 自适应参数配置(未完善，暂不启用)
    inline QEP::LinearSolverConfig getAdaptiveConfig(
        QEP::LinearSolverType type,
        double condition_number_K)
    {
        auto config = getSolverConfig(type);

        if (!ENABLE_ADAPTIVE_PARAMETERS || condition_number_K <= 0)
        {
            return config;
        }

        // 直接法：仅调整外层容差（因为内层直接求解精度固定）
        bool is_direct = (type == QEP::LinearSolverType::SparseLU ||
                          type == QEP::LinearSolverType::PardisoLU ||
                          type == QEP::LinearSolverType::SimplicialLLT);
        if (is_direct)
        {
            if (condition_number_K > 1e10)
            {
                config.outer_tolerance = 1e-4;
            }
            else if (condition_number_K > 1e8)
            {
                config.outer_tolerance = 1e-6;
            }
            else
            {
                config.outer_tolerance = ARNOLDI_TOLERANCE; // 1e-8
            }
            return config;
        }

        // 迭代法：协调内外层容差，增强预条件子
        // 条件数阈值划分
        if (condition_number_K < 1e6)
        {
            // 良态：高精度内外层
            config.inner_tolerance = 1e-10;
            config.outer_tolerance = 1e-8;
            config.inner_maxIterations = 500;
            config.fillFactor = 10;
            config.dropTol = 1e-4;
            if (type == QEP::LinearSolverType::GMRES)
                config.restart = 100;
        }
        else if (condition_number_K < 1e8)
        {
            // 中等病态：适度放宽
            config.inner_tolerance = 1e-8;
            config.outer_tolerance = 1e-6;
            config.inner_maxIterations = 800;
            config.fillFactor = std::min(20, ILUT_FILL_FACTOR * 2);
            config.dropTol = std::max(1e-5, ILUT_DROP_TOL / 2.0);
            if (type == QEP::LinearSolverType::GMRES)
                config.restart = 120;
        }
        else if (condition_number_K < 1e10)
        {
            // 病态：显著放宽容差，增强预条件
            config.inner_tolerance = 1e-6;
            config.outer_tolerance = 1e-5;
            config.inner_maxIterations = 1500;
            config.fillFactor = std::min(40, ILUT_FILL_FACTOR * 4);
            config.dropTol = std::max(1e-6, ILUT_DROP_TOL / 5.0);
            if (type == QEP::LinearSolverType::GMRES)
                config.restart = 150;
        }
        else
        {
            // 严重病态：极度宽松，强力预条件
            config.inner_tolerance = 1e-4;
            config.outer_tolerance = 1e-4;
            config.inner_maxIterations = 3000;
            config.fillFactor = std::min(100, ILUT_FILL_FACTOR * 10);
            config.dropTol = std::max(1e-7, ILUT_DROP_TOL / 10.0);
            if (type == QEP::LinearSolverType::GMRES)
                config.restart = 200;
        }

        return config;
    }

} // namespace Config

#endif // GLOBAL_CONFIG_H