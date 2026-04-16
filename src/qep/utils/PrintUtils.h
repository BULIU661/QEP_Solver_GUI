// utils/PrintUtils.h
#ifndef PRINT_UTILS_H
#define PRINT_UTILS_H
#include <string>
#include <Eigen/Sparse>
#include <Eigen/Dense>
#include "qep/QEP.h"

namespace QEP {

// 打印工具函数声明
void printMatrixInfo(const Eigen::SparseMatrix<double> &matrix, const std::string &name);
void printProblemInfo(const QuadraticEigenvalueProblem &problem);


} // namespace QEP

#endif // PRINT_UTILS_H
