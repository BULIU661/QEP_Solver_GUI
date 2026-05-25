//==============================================================================
//  examples/simple_example.cpp  —  QEP 求解器简单编程调用示例
//==============================================================================

#include <qep/QEP.h>
#include <iostream>
#include <Eigen/Sparse>

#ifdef _WIN32
#include <windows.h>
#endif

static void initConsole()
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
}

int main() {
    initConsole();

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
    QEP::QuadraticEigenvalueProblem problem;
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
    QEP::LinearSolverConfig config;
    config.type = QEP::LinearSolverType::SparseLU;
    config.inner_tolerance = 1e-10;
    config.outer_tolerance = 1e-8;

    std::cout << "Solving for " << nev << " eigenvalues near sigma=" << sigma << "...\n\n";

    // 求解
    auto result = QEP::solveQEP_Unified(
        problem, nev, sigma,
        QEP::LinearSolverType::SparseLU, config);

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

            std::cout << "  \xce\xbb[" << i + 1 << "] = ";
            if (std::abs(lambda.imag()) < 1e-10) {
                std::cout << lambda.real();
            } else {
                std::cout << lambda.real() << " + " << lambda.imag() << "i";
            }
            std::cout << "\n";
        }

        // 计算并显示残差
        if (result.eigenvectors.cols() > 0) {
            auto [maxRes, tableStr] = QEP::computeAndFormatResiduals(
                problem.M, problem.C, problem.K, result);
            if (!tableStr.empty()) std::cout << tableStr;
            std::cout << "\nMaximum relative residual: " << maxRes << "\n";
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
