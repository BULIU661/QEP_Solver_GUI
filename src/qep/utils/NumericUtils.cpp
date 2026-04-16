// utils/NumericUtils.cpp
#include "NumericUtils.h"
#include <cmath>
#include <iostream>
#include <Eigen/Core>

namespace QEP {

bool isVectorHealthy(const Eigen::VectorXd &v, const std::string &name) {
#ifndef NDEBUG
    for (int i = 0; i < v.size(); ++i) {
        if (std::isnan(v(i)) || std::isinf(v(i))) {
            std::cerr << "[健康检查] " << name << " 包含 NaN/Inf at index " << i
                      << " value=" << v(i) << std::endl;
            return false;
        }
    }
    if (v.norm() < 1e-14) {
        std::cerr << "[健康检查] " << name << " 接近零向量" << std::endl;
    }
#endif
    return true;
}

} // namespace QEP
