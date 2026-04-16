// utils/PrintUtils.cpp
#include "PrintUtils.h"
#include <iostream>
#include <iomanip>
#include <sstream>


namespace QEP {

    // ==================== 信息打印函数 ====================
    // 打印稀疏矩阵的基本信息:维度、非零元素数量、稀疏度
    void printMatrixInfo(const Eigen::SparseMatrix<double> &matrix, const std::string &name)
    {
        std::cout << "矩阵 " << name << ": "
                  << matrix.rows() << "x" << matrix.cols()
                  << ", 非零元素: " << matrix.nonZeros()
                  << ", 稀疏度: " << std::fixed << std::setprecision(2)
                  << (100.0 * matrix.nonZeros() / (matrix.rows() * matrix.cols())) << "%"
                  << std::endl;
    }

    // 打印问题的基本信息:名称、维度、M/C/K矩阵的非零元素数量
    void printProblemInfo(const QuadraticEigenvalueProblem &problem)
    {
        std::cout << "问题: " << problem.name
                  << ", 维度: " << problem.dimension
                  << ", M非零: " << problem.M.nonZeros()
                  << ", C非零: " << problem.C.nonZeros()
                  << ", K非零: " << problem.K.nonZeros() << std::endl;
    }



} // namespace QEP
