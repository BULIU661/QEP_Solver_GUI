//==============================================================================
//  src/qep/utils/CreateProblem.cpp  —  从矩阵文件构造二次特征值问题
//==============================================================================

#include "CreateProblem.h"
#include "io/MatrixIO.h"
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include <fstream>

// QuadraticEigenvalueProblem 结构体包含以下字段：
    // Eigen::SparseMatrix<double> M;
    // Eigen::SparseMatrix<double> C;
    // Eigen::SparseMatrix<double> K;
    // bool use_sparse = true;
    // bool is_symmetric = true;
    // int dimension = 0;
    // std::string name;
    // double condition_number_K = 0.0; // K矩阵的条件数，用于自适应参数调整
    // 可扩展至自动化求解问题，例如：通过创建问题时判断矩阵性质和条件数自动选取方法等

namespace QEP {

QuadraticEigenvalueProblem createTestProblemFromFiles(
    const std::string &name,
    const std::string &M_file,
    const std::string &C_file,
    const std::string &K_file,
    bool is_sparse,
    const Config::SolverParams &params,
    bool overwrite) {
    QuadraticEigenvalueProblem problem;
    problem.name = name;

    // 自动将 .txt 转为 .bin（加载 .txt 必须先转换，无开关）
    auto ensureBinary = [&](const std::string &filePath) -> std::string {
        if (filePath.size() < 4) return filePath;
        std::string ext = filePath.substr(filePath.size() - 4);
        std::string base = filePath.substr(0, filePath.size() - 4);
        std::string binPath = base + ".bin";
        std::string txtPath = base + ".txt";

        // 已经是 .bin → 检查是否存在，不存在则尝试从 .txt 转换
        if (ext == ".bin" || ext == ".BIN") {
            std::ifstream test(binPath);
            if (test.good()) return binPath;
            // .bin 不存在，尝试从 .txt 创建
            std::ifstream txtTest(txtPath);
            if (txtTest.good()) {
                convertTextCSRtoBinary(txtPath, binPath);
                return binPath;
            }
            return filePath; // 都不存在，让 readBinaryCSR 报错
        }

        // .txt → 转为 .bin
        if (ext == ".txt" || ext == ".TXT") {
            std::ifstream test(binPath);
            if (!test.good() || overwrite)
                convertTextCSRtoBinary(filePath, binPath);
            return binPath;
        }

        return filePath;
    };
    std::string M_bin = ensureBinary(M_file);
    std::string C_bin = ensureBinary(C_file);
    std::string K_bin = ensureBinary(K_file);

    // 尝试读取矩阵文件
    try {
        if (is_sparse) {
            problem.M = readBinaryCSR(M_bin);
            problem.C = readBinaryCSR(C_bin);
            problem.K = readBinaryCSR(K_bin);
        } else {
            Eigen::MatrixXd M_dense = read_dense_matrix(M_file);
            Eigen::MatrixXd C_dense = read_dense_matrix(C_file);
            Eigen::MatrixXd K_dense = read_dense_matrix(K_file);
            problem.M = M_dense.sparseView();
            problem.C = C_dense.sparseView();
            problem.K = K_dense.sparseView();
        }
        problem.dimension = problem.M.rows();
        if (problem.C.rows() != problem.dimension || problem.K.rows() != problem.dimension) {
            problem.dimension = 0; // 调用方通过 dimension==0 检测失败
            return problem;
        }
    } catch (const std::exception &e) {
        problem.dimension = 0;
        return problem;
    }

    // 条件按需计算：仅当自适应参数或条件估计开启时才计算 K 矩阵条件数
    if (params.enable_condition_estimation || params.enable_adaptive)
    {
        problem.condition_number_K = estimateConditionNumber(problem.K);
    }

    return problem;
}

} // namespace QEP
