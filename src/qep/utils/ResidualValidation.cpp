//==============================================================================
//  src/qep/utils/ResidualValidation.cpp  —  残差计算、4 级状态判定、格式化残差表
//==============================================================================

#include "ResidualValidation.h"
#include "config/TableFormatter.h"
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

    // 格式化残差表为字符串（核心函数，被 computeAndFormatResiduals 和 GUI 通用）
    std::pair<double, std::string> formatResidualTable(
        const Eigen::SparseMatrix<double> &M,
        const Eigen::SparseMatrix<double> &C,
        const Eigen::SparseMatrix<double> &K,
        const QuadraticEigenvalueResult &res)
    {
        int n_eig = static_cast<int>(res.eigenvalues_real.size());
        if (n_eig == 0) return {-1.0, ""};
        bool has_vecs = (res.eigenvectors.cols() > 0);

        fmt::TableBuilder tbl;
        tbl.addCol("#", 3, fmt::Align::Right);
        tbl.addCol("Eigenvalue", 26, fmt::Align::Center);
        tbl.addCol("Abs. Residual", 14, fmt::Align::Right);
        tbl.addCol("Rel. Residual", 14, fmt::Align::Right);
        tbl.addCol("Status", 12, fmt::Align::Center);

        double max_rel = 0.0;
        for (int i = 0; i < n_eig; ++i)
        {
            std::complex<double> lambda(res.eigenvalues_real(i), res.eigenvalues_imag(i));
            double abs_res = -1.0, rel_res = -1.0;
            const char *status = "No vec";

            if (has_vecs && i < res.eigenvectors.cols())
            {
                Eigen::VectorXcd xvec = res.eigenvectors.col(i);
                double xnorm = xvec.norm();
                if (xnorm > 1e-14) xvec /= xnorm;
                auto residuals = computeResiduals(M, C, K, lambda, xvec);
                abs_res = residuals.first;
                rel_res = residuals.second;
                status = fmt::residualStatus(rel_res);
                max_rel = std::max(max_rel, rel_res);
            }

            // 特征值字符串
            char eigBuf[64], absBuf[32], relBuf[32];
            if (std::abs(lambda.imag()) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6g", lambda.real());
            else if (std::abs(lambda.real()) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6gi", lambda.imag());
            else
                snprintf(eigBuf, sizeof(eigBuf), "%.6g%+.6gi", lambda.real(), lambda.imag());

            if (abs_res >= 0) {
                snprintf(absBuf, sizeof(absBuf), "%.2e", abs_res);
                snprintf(relBuf, sizeof(relBuf), "%.2e", rel_res);
            }

            tbl.addRow({
                std::to_string(i + 1),
                eigBuf,
                abs_res >= 0 ? absBuf : "N/A",
                rel_res >= 0 ? relBuf : "N/A",
                status
            });
        }

        return {has_vecs ? max_rel : -1.0, tbl.render(2)};
    }

    std::pair<double, std::string> computeAndFormatResiduals(
        const Eigen::SparseMatrix<double> &M,
        const Eigen::SparseMatrix<double> &C,
        const Eigen::SparseMatrix<double> &K,
        const QuadraticEigenvalueResult &res)
    {
        auto result = formatResidualTable(M, C, K, res);
        return result;
    }


} // namespace QEP
