// utils/CreatProblem.cpp
#include "CreatProblem.h"
#include "io/MatrixIO.h"
#include <iostream>
#include <Eigen/Sparse>
#include <Eigen/Dense>

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
    bool is_sparse) {
    QuadraticEigenvalueProblem problem;
    problem.name = name;
    // 尝试读取矩阵文件
    try {
        if (is_sparse) {
            problem.M = readBinaryCSR(M_file);
            problem.C = readBinaryCSR(C_file);
            problem.K = readBinaryCSR(K_file);
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
            std::cerr << "错误: 矩阵维度不一致!" << std::endl;
            return problem;
        }
        std::cout << "成功创建问题: " << name << ", 维度: " << problem.dimension << std::endl;
    } catch (const std::exception &e) {
        std::cerr << "创建测试问题时出错: " << e.what() << std::endl;
    }

    // 估计 K 矩阵条件数，这个矩阵的选取与线性化方法有关
    // problem.condition_number_K = estimateConditionNumber(problem.K, false);


    // // 判断矩阵是否对称或正定（判断正定对性能影响较大）
    // // 对称性
    // problem.is_symmetric = QEP::isSymmetric(problem.K);
    // // 正定性
    // problem.is_positive_definite = QEP::isPositiveDefinite(problem.K);
    // //0: 定向性未知, 1: 正定, 2: 半正定, 3: 负定, 4: 半负定, 5: 不定
    // problem.matrix_definiteness = QEP::classifyDefiniteness(problem.K, 1e-6);

    return problem;
}

} // namespace QEP
