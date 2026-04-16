// include/qep/QEP.h
#ifndef QEP_H
#define QEP_H

#include <vector>
#include <string>
#include <complex>
#include <Eigen/Core>
#include <Eigen/SparseCore>

namespace QEP {

// ========== 数据结构 ==========
enum class MatrixDefiniteness {
    SymmetricPositiveDefinite,
    SymmetricPositiveSemiDefinite,
    SymmetricIndefinite,
    SymmetricNegativeDefinite,
    NonSymmetric,
    NumericallySingular,
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
    int total_inner_solves = 0;           //
    int total_inner_iterations = 0;       // 
    int max_inner_iterations_per_solve = 0; // 
    double avg_inner_iterations = 0.0;    // 
    int arnoldi_iterations = 0;           // 
    double linear_solve_total_time = 0.0; // 
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
    double inner_tolerance = 1e-10;
    int inner_maxIterations = 1000;
    int outer_maxIterations = 1000;
    double outer_tolerance = 1e-8;
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
    const LinearSolverConfig &config = {});

// ========== IO 函数 ==========
bool convertTextCSRtoBinary(const std::string &text_file, const std::string &bin_file);
void convertBinaryFiles(const std::vector<std::pair<std::string, std::string>> &files);
Eigen::SparseMatrix<double> readBinaryCSR(const std::string &bin_file);
Eigen::SparseMatrix<double> read_sparse_matrix_csr(const std::string &filename);
Eigen::SparseMatrix<double> read_sparse_matrix_coo(const std::string &filename);
Eigen::MatrixXd read_dense_matrix(const std::string &filename);


// ========== 矩阵属性检查 ==========
bool isSymmetric(const Eigen::SparseMatrix<double> &A);
bool isPositiveDefinite(const Eigen::SparseMatrix<double> &A);
double estimateConditionNumber(const Eigen::SparseMatrix<double> &A,bool is_print);
void estimateAndWarnConditionNumber(const Eigen::SparseMatrix<double> &K,bool is_print);
MatrixDefiniteness classifyDefiniteness(const Eigen::SparseMatrix<double> &A, double tol);
void checkMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name);
  void PrintMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name, double cond_est );
MatrixStorageType checkMatrixStorage(const Eigen::SparseMatrix<double> &A);

// ========== 残差验证 ==========
std::pair<double, double> computeResiduals(const Eigen::SparseMatrix<double> &M,
                                           const Eigen::SparseMatrix<double> &C,
                                           const Eigen::SparseMatrix<double> &K,
                                           std::complex<double> lambda,
                                           const Eigen::VectorXcd &x);
double printResidualTable(const Eigen::SparseMatrix<double> &M,
                          const Eigen::SparseMatrix<double> &C,
                          const Eigen::SparseMatrix<double> &K,
                          const QuadraticEigenvalueResult &res);

// ========== 测试框架 ==========
QuadraticEigenvalueProblem createTestProblemFromFiles(
    const std::string &name,
    const std::string &M_file,
    const std::string &C_file,
    const std::string &K_file,
    bool is_sparse);

// ========== 工具函数 ==========
long getCurrentRSS();
bool isVectorHealthy(const Eigen::VectorXd &v, const std::string &name = "vector");
int getDisplayWidth(const std::string &str);
std::string truncateToWidth(const std::string &str, int maxDisplayWidth);
std::string padToWidth(const std::string &str, int targetWidth);
std::string padToWidthRight(const std::string &str, int targetWidth);


} // namespace QEP

#endif // QEP_H

