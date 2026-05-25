//==============================================================================
//  src/qep/utils/ResidualValidation.h  —  QEP 残差计算与验证接口
//==============================================================================

#ifndef RESIDUAL_VALIDATION_H
#define RESIDUAL_VALIDATION_H

#include <Eigen/SparseCore>
#include <complex>
#include "qep/QEP.h"

namespace QEP {

std::pair<double, double> computeResiduals(const Eigen::SparseMatrix<double> &M,
                                           const Eigen::SparseMatrix<double> &C,
                                           const Eigen::SparseMatrix<double> &K,
                                           std::complex<double> lambda,
                                           const Eigen::VectorXcd &x);

// 格式化残差表为字符串，返回 <最大相对残差, 格式化表格字符串>
std::pair<double, std::string> formatResidualTable(
    const Eigen::SparseMatrix<double> &M,
    const Eigen::SparseMatrix<double> &C,
    const Eigen::SparseMatrix<double> &K,
    const QuadraticEigenvalueResult &res);

} // namespace QEP


#endif // RESIDUAL_VALIDATION_H
