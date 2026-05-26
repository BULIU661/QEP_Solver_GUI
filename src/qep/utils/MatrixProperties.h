//==============================================================================
//  src/qep/utils/MatrixProperties.h  —  矩阵性质分析接口声明
//==============================================================================

#ifndef MATRIX_PROPERTIES_H
#define MATRIX_PROPERTIES_H

#include <Eigen/SparseCore>
#include "qep/QEP.h"  // for MatrixDefiniteness

#ifdef EIGEN_PARDISO_SUPPORT
#include <Eigen/PardisoSupport>   // 提供 Eigen::PardisoLU
#endif

#include <Eigen/SparseLU>         // 提供 Eigen::SparseLU（备用方案）

namespace QEP {
// 条件数估计辅助函数（内部使用）
double powerMethod(const Eigen::SparseMatrix<double> &A, int max_iter, double tol);
// 逆幂法（Pardiso版，仅用于MKL启用时）
#ifdef EIGEN_PARDISO_SUPPORT
double inversePowerMethod(const Eigen::PardisoLU<Eigen::SparseMatrix<double>> &solver,int max_iter, double tol);
#endif
// 逆幂法（SparseLU版，通用备用方案）
double inversePowerMethodSparseLU(const Eigen::SparseLU<Eigen::SparseMatrix<double>> &solver, int max_iter, double tol);
// 对称性检查
bool isSymmetric(const Eigen::SparseMatrix<double> &A);
bool isPositiveDefinite(const Eigen::SparseMatrix<double> &A);
MatrixDefiniteness classifyDefiniteness(const Eigen::SparseMatrix<double> &A, double tol);
  std::string printMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name, double cond_est );

// 条件数估计
double estimateConditionNumber(const Eigen::SparseMatrix<double> &A);
MatrixStorageType checkMatrixStorage(const Eigen::SparseMatrix<double> &A);
void estimateAndWarnConditionNumber(const Eigen::SparseMatrix<double> &K);

} // namespace QEP

#endif // MATRIX_PROPERTIES_H
