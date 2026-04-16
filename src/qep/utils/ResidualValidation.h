// utils/ResidualValidation.h
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

double printResidualTable(const Eigen::SparseMatrix<double> &M,
                          const Eigen::SparseMatrix<double> &C,
                          const Eigen::SparseMatrix<double> &K,
                          const QuadraticEigenvalueResult &res);

} // namespace QEP

#endif // RESIDUAL_VALIDATION_H
