//==============================================================================
//  include/qep/QEP.h  —  QEP 求解器主公共 API（数据结构、求解接口、矩阵 IO）
//==============================================================================

#ifndef QEP_H
#define QEP_H

#include <vector>
#include <string>
#include <complex>
#include <Eigen/Core>
#include <Eigen/SparseCore>
#include "config/SolverParams.h"

namespace QEP {

// ========== 数据结构 ==========
enum class MatrixDefiniteness {
    SymmetricPositiveDefinite,
    SymmetricPositiveSemiDefinite,
    SymmetricIndefinite,
    SymmetricNegativeDefinite,
    NonSymmetric,
    Unknown
};

struct QuadraticEigenvalueProblem {
    Eigen::SparseMatrix<double> M;
    Eigen::SparseMatrix<double> C;
    Eigen::SparseMatrix<double> K;
    bool use_sparse = true;
    bool is_symmetric = true;
    int dimension = 0;
    std::string name;
    double condition_number_K = 0.0; // K矩阵的条件数，用于自适应参数调整
    double condition_number_S = 0.0; // 构建矩阵的条件数
};

struct QuadraticEigenvalueResult {
    Eigen::VectorXd eigenvalues_real;
    Eigen::VectorXd eigenvalues_imag;
    Eigen::MatrixXcd eigenvectors;
    double solution_time = 0.0;
    double preprocessing_time = 0.0;
    double arnoldi_time = 0.0;
    bool success = false;
    int problem_dimension = 0;
    int linearized_dimension = 0;
    std::string method_used;
    bool linear_solver_converged = true;
    bool eigensolver_fully_converged = true;
    std::string warning_message;
    double condition_number_K = 0.0;

    // 内层迭代求解器统计信息
    int total_inner_solves = 0;
    int total_inner_iterations = 0;
    int max_inner_iterations_per_solve = 0;
    double avg_inner_iterations = 0.0;
    int arnoldi_iterations = 0;
    double linear_solve_total_time = 0.0;

    // 构建/预处理时间细分（从 UnifiedShiftInvertOp 获取）
    double build_time = 0.0;
    double condition_number_S = 0.0;
    double cond_estimation_time = 0.0;

    // 日志消息缓冲区（替代直接 cout，由调用方决定如何展示）
    std::vector<std::string> log_messages;
    std::vector<std::string> warning_messages;

    // 逐特征值的残差（由 SolverWorker 预计算，GUI 直接使用）
    std::vector<double> residuals_abs;
    std::vector<double> residuals_rel;
    std::vector<std::string> residual_status; // "OK" / "FAIL" / "N/A"

    // 便捷方法：添加日志消息
    void addLog(const std::string &msg) {
        log_messages.push_back(msg);
    }
    void addWarning(const std::string &msg) {
        warning_messages.push_back(msg);
    }
};

// ========== 内层线性求解器类型 ==========
enum class LinearSolverType {
    SparseLU,
    PardisoLU,
    SimplicialLLT,
    ConjugateGradient,
    BiCGSTAB,
    GMRES
};

struct LinearSolverConfig {
    LinearSolverType type = LinearSolverType::PardisoLU;
    double inner_tolerance = 1e-8;
    int inner_maxIterations = 1000;
    int outer_maxIterations = 1000;
    double outer_tolerance = 1e-6;
    int restart = 100;
    int fillFactor = 10;
    double dropTol = 1e-4;
    bool usePreconditioner = true;

    LinearSolverConfig() = default;
    explicit LinearSolverConfig(LinearSolverType t) : type(t) {}
};

// 汇总报告相关结构
struct CaseResult {
    std::string method_name;
    bool success;
    int n_eig;
    double time;
    double max_rel_residual;
    std::string validation;
    std::vector<std::string> logs;
};

   enum class MatrixStorageType
    {
        FullSymmetric,    // 完整存储且数值对称
        FullNonSymmetric, // 上下三角都存在但数值不对称
        UpperTriangular,  // 只存上三角
        LowerTriangular,  // 只存下三角
        NonSquare
    };


// ========== 统一求解接口 ==========
QuadraticEigenvalueResult solveQEP_Unified(
    const QuadraticEigenvalueProblem &problem,
    int nev,
    double sigma,
    LinearSolverType solverType,
    const LinearSolverConfig &config = {},
    const Config::SolverParams &params = {});

// ========== IO 函数 ==========
bool convertTextCSRtoBinary(const std::string &text_file, const std::string &bin_file);
Eigen::SparseMatrix<double> readBinaryCSR(const std::string &bin_file);
Eigen::MatrixXd read_dense_matrix(const std::string &filename);


// ========== 矩阵属性检查 ==========
bool isSymmetric(const Eigen::SparseMatrix<double> &A);
double estimateConditionNumber(const Eigen::SparseMatrix<double> &A);
MatrixDefiniteness classifyDefiniteness(const Eigen::SparseMatrix<double> &A, double tol);
std::string printMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name, double cond_est );

MatrixStorageType checkMatrixStorage(const Eigen::SparseMatrix<double> &A);

// ========== 残差验证 ==========
std::pair<double, double> computeResiduals(const Eigen::SparseMatrix<double> &M,
                                           const Eigen::SparseMatrix<double> &C,
                                           const Eigen::SparseMatrix<double> &K,
                                           std::complex<double> lambda,
                                           const Eigen::VectorXcd &x);
// 计算并格式化残差表（纯计算，不输出到 cout），返回 <最大相对残差, 格式化表格字符串>
std::pair<double, std::string> computeAndFormatResiduals(
    const Eigen::SparseMatrix<double> &M,
    const Eigen::SparseMatrix<double> &C,
    const Eigen::SparseMatrix<double> &K,
    const QuadraticEigenvalueResult &res);

// 格式化残差表为字符串（GUI 使用），返回 <最大相对残差, 格式化表格字符串>
std::pair<double, std::string> formatResidualTable(
    const Eigen::SparseMatrix<double> &M,
    const Eigen::SparseMatrix<double> &C,
    const Eigen::SparseMatrix<double> &K,
    const QuadraticEigenvalueResult &res);


// ========== 测试框架 ==========
QuadraticEigenvalueProblem createTestProblemFromFiles(
    const std::string &name,
    const std::string &M_file,
    const std::string &C_file,
    const std::string &K_file,
    bool is_sparse,
    const Config::SolverParams &params = {},
    bool overwrite = false);

} // namespace QEP

#endif // QEP_H
