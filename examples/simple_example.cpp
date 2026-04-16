/**
 * @file simple_example.cpp
 * @brief QEP库简单使用示例
 *
 * 本示例展示如何使用QEP库求解二次特征值问题：
 *   (λ²M + λC + K)x = 0
 */

#include <qep/QEP.h>
#include <iostream>
#include <Eigen/Sparse>

int main() {
    std::cout << "QEP Library Simple Example\n";
    std::cout << "==========================\n\n";

    // 示例：创建简单的3x3测试矩阵
    // 在实际应用中，这些矩阵通常从文件读取
    const int n = 3;

    // 创建质量矩阵 M (对角矩阵)
    Eigen::SparseMatrix<double> M(n, n);
    M.insert(0, 0) = 1.0;
    M.insert(1, 1) = 1.0;
    M.insert(2, 2) = 1.0;
    M.makeCompressed();

    // 创建阻尼矩阵 C (对角矩阵)
    Eigen::SparseMatrix<double> C(n, n);
    C.insert(0, 0) = 0.1;
    C.insert(1, 1) = 0.1;
    C.insert(2, 2) = 0.1;
    C.makeCompressed();

    // 创建刚度矩阵 K (对称正定)
    Eigen::SparseMatrix<double> K(n, n);
    K.insert(0, 0) = 2.0; K.insert(0, 1) = -1.0;
    K.insert(1, 0) = -1.0; K.insert(1, 1) = 2.0; K.insert(1, 2) = -1.0;
    K.insert(2, 1) = -1.0; K.insert(2, 2) = 2.0;
    K.makeCompressed();

    // 构建二次特征值问题
    EigenSolvers::QuadraticEigenvalueProblem problem;
    problem.M = M;
    problem.C = C;
    problem.K = K;
    problem.dimension = n;
    problem.name = "Simple 3x3 Example";

    std::cout << "Problem: " << problem.name << "\n";
    std::cout << "Dimension: " << problem.dimension << "\n\n";

    // 求解参数
    int nev = 2;          // 求2个特征值
    double sigma = 0.0;   // 移频点（0表示求模最小特征值）

    // 配置求解器
    EigenSolvers::LinearSolverConfig config;
    config.type = EigenSolvers::LinearSolverType::SparseLU;
    config.inner_tolerance = 1e-10;
    config.outer_tolerance = 1e-8;

    std::cout << "Solving for " << nev << " eigenvalues near sigma=" << sigma << "...\n\n";

    // 求解
    auto result = EigenSolvers::solveQEP_Unified(
        problem, nev, sigma,
        EigenSolvers::LinearSolverType::SparseLU, config);

    // 输出结果
    if (result.success) {
        std::cout << "Solution successful!\n";
        std::cout << "Method used: " << result.method_used << "\n";
        std::cout << "Solution time: " << result.solution_time << " seconds\n";
        std::cout << "\nEigenvalues:\n";

        for (int i = 0; i < result.eigenvalues_real.size(); ++i) {
            std::complex<double> lambda(
                result.eigenvalues_real(i),
                result.eigenvalues_imag(i));

            std::cout << "  λ[" << i + 1 << "] = ";
            if (std::abs(lambda.imag()) < 1e-10) {
                std::cout << lambda.real();
            } else {
                std::cout << lambda.real() << " + " << lambda.imag() << "i";
            }
            std::cout << "\n";
        }

        // 计算并显示残差
        if (result.eigenvectors.cols() > 0) {
            double max_residual = EigenSolvers::printResidualTable(
                problem.M, problem.C, problem.K, result);
            std::cout << "\nMaximum relative residual: " << max_residual << "\n";
        }
    } else {
        std::cerr << "Solution failed!\n";
        if (!result.warning_message.empty()) {
            std::cerr << "Error: " << result.warning_message << "\n";
        }
        return 1;
    }

    std::cout << "\nExample completed successfully.\n";
    return 0;
}
