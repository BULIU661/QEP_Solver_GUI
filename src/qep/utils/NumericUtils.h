// utils/NumericUtils.h
#ifndef NUMERIC_UTILS_H
#define NUMERIC_UTILS_H

#include <string>
#include <Eigen/Core>

namespace QEP {

bool isVectorHealthy(const Eigen::VectorXd &v, const std::string &name = "vector");

} // namespace QEP

#endif // NUMERIC_UTILS_H
