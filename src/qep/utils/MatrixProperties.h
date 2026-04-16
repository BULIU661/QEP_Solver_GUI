// utils/MatrixProperties.h
#ifndef MATRIX_PROPERTIES_H
#define MATRIX_PROPERTIES_H

#include <Eigen/SparseCore>
#include "qep/QEP.h"  // for MatrixDefiniteness
#include <Eigen/PardisoSupport>   // 提供 Eigen::PardisoLU


namespace QEP {
// 条件数估计辅助函数（内部使用）
double powerMethod(const Eigen::SparseMatrix<double> &A, int max_iter, double tol);
// 逆幂法
double inversePowerMethod(const Eigen::PardisoLU<Eigen::SparseMatrix<double>> &solver,int max_iter, double tol);
// 对称性检查
bool isSymmetric(const Eigen::SparseMatrix<double> &A);
bool isPositiveDefinite(const Eigen::SparseMatrix<double> &A);
MatrixDefiniteness classifyDefiniteness(const Eigen::SparseMatrix<double> &A, double tol);
  void PrintMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name, double cond_est );
// 条件数估计
double estimateConditionNumber(const Eigen::SparseMatrix<double> &A, bool is_print);
MatrixStorageType checkMatrixStorage(const Eigen::SparseMatrix<double> &A);
// 辅助打印
void checkMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name);
void estimateAndWarnConditionNumber(const Eigen::SparseMatrix<double> &K,bool is_print);

} // namespace QEP

#endif // MATRIX_PROPERTIES_H
