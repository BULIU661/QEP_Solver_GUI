// utils/ResidualValidation.cpp
#include "ResidualValidation.h"
#include "ConsoleUtils.h" // for padToWidth etc.
#include "config/GlobalConfig.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace QEP
{

    std::pair<double, double> computeResiduals(const Eigen::SparseMatrix<double> &M,
                                               const Eigen::SparseMatrix<double> &C,
                                               const Eigen::SparseMatrix<double> &K,
                                               std::complex<double> lambda,
                                               const Eigen::VectorXcd &x)
    {
        std::complex<double> lambda2 = lambda * lambda;
        Eigen::VectorXcd Mx = M * x;
        Eigen::VectorXcd Cx = C * x;
        Eigen::VectorXcd Kx = K * x;
        Eigen::VectorXcd r = lambda2 * Mx + lambda * Cx + Kx;
        double abs_res = r.norm();
        double denom = std::abs(lambda2) * Mx.norm() + std::abs(lambda) * Cx.norm() + Kx.norm();
        double rel_res = (denom < 1e-14) ? abs_res : abs_res / denom;
        return {abs_res, rel_res};
    }

#include <iomanip>
#include <sstream>
#include <string>

    // 辅助函数：将字符串居中于指定宽度
    std::string center(const std::string &s, int width)
    {
        int pad = width - static_cast<int>(s.size());
        if (pad <= 0)
            return s.substr(0, width);
        int left = pad / 2;
        int right = pad - left;
        return std::string(left, ' ') + s + std::string(right, ' ');
    }

    double printResidualTable(const Eigen::SparseMatrix<double> &M,
                              const Eigen::SparseMatrix<double> &C,
                              const Eigen::SparseMatrix<double> &K,
                              const QuadraticEigenvalueResult &res)
    {
        if (!Config::PRINT_RESIDUALS)
            return -1.0;
        int n_eig = static_cast<int>(res.eigenvalues_real.size());
        if (n_eig == 0)
            return -1.0;
        bool has_vecs = (res.eigenvectors.cols() > 0);

        const int w_id = 4;   // 编号宽度
        const int w_eig = 27; // 特征值宽度
        const int w_abs = 12; // 绝对残差宽度
        const int w_rel = 12; // 相对残差宽度
        const int w_stat = 6; // 状态宽度

        // 打印表头（居中或左对齐，这里统一左对齐，但特征值列通过后续居中）
        std::cout << "\n  残差验证 (共" << n_eig << "个特征值):\n";
        std::cout << std::left
                  << std::setw(10 + w_id) << "   #"
                  << " " << std::setw(w_eig - 3) << "特征值"
                  << " " << std::setw(w_abs + 4) << "绝对残差"
                  << " " << std::setw(w_rel) << "相对残差"
                  << " " << std::setw(w_stat) << "   状态"
                  << "\n";

        double max_rel = 0.0;
        for (int i = 0; i < n_eig; ++i)
        {
            std::complex<double> lambda(res.eigenvalues_real(i), res.eigenvalues_imag(i));
            double abs_res = -1.0, rel_res = -1.0;
            std::string status = "无向量";
            if (has_vecs && i < res.eigenvectors.cols())
            {
                Eigen::VectorXcd xvec = res.eigenvectors.col(i);
                double xnorm = xvec.norm();
                if (xnorm > 1e-14)
                    xvec /= xnorm;
                auto residuals = computeResiduals(M, C, K, lambda, xvec);
                abs_res = residuals.first;
                rel_res = residuals.second;
                status = (rel_res < Config::CHECK_TOLERANCE) ? "OK" : "FAIL";
                max_rel = std::max(max_rel, rel_res);
            }

            // 构造特征值原始字符串（不带宽度填充）
            std::ostringstream tmp;
            tmp << std::fixed << std::setprecision(4);
            if (std::abs(lambda.imag()) < 1e-8)
            {
                tmp << lambda.real();
            }
            else if (std::abs(lambda.real()) < 1e-8)
            {
                tmp << lambda.imag() << "i";
            }
            else
            {
                tmp << lambda.real()
                    << (lambda.imag() >= 0 ? "+" : "")
                    << lambda.imag() << "i";
            }
            std::string raw_eig = tmp.str();
            // 居中特征值字符串
            std::string eig_str = center(raw_eig, w_eig);

            // 输出一行：编号（右对齐）、特征值（居中）、残差（右对齐）、状态（右对齐）
            std::cout << std::right << std::setw(w_id) << i + 1
                      << " " << eig_str
                      << " ";
            if (abs_res >= 0)
            {
                std::cout << std::scientific << std::setprecision(2)
                          << std::right << std::setw(w_abs) << abs_res
                          << " " << std::right << std::setw(w_rel) << rel_res
                          << " " << std::right << std::setw(w_stat) << status;
            }
            else
            {
                std::cout << std::setw(w_abs) << "N/A"
                          << " " << std::setw(w_rel) << "N/A"
                          << " " << std::right << std::setw(w_stat) << status;
            }
            std::cout << "\n";
        }

        if (has_vecs)
        {
            std::cout << std::fixed << std::setprecision(2)
                      << "  最大相对残差: " << std::scientific << max_rel
                      << (max_rel < 1e-6 ? "  => PASSED\n" : "  => FAILED\n");
        }
        return has_vecs ? max_rel : -1.0;
    }

} // namespace QEP
