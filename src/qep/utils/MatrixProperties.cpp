//==============================================================================
//  src/qep/utils/MatrixProperties.cpp  —  对称性/正定性/条件数/存储格式检测
//==============================================================================

#include "MatrixProperties.h"
#include "config/ConfigManager.h"
#include <iostream>
#include <sstream>
#include <iomanip>


#include <Eigen/Sparse>
#include <Eigen/Cholesky>
#include <Eigen/SparseLU>

#ifdef EIGEN_PARDISO_SUPPORT
#include <Eigen/PardisoSupport>
#endif

namespace QEP
{
    //------- 矩阵对称正定性质检查 -------
    // 判定对称矩阵的定性
    MatrixDefiniteness classifyDefiniteness(const Eigen::SparseMatrix<double> &A, double tol)
    {

        if (!isSymmetric(A))
            return MatrixDefiniteness::NonSymmetric;

        // 尝试 LDLT 分解
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> ldlt(A);
        if (ldlt.info() != Eigen::Success)
        {
            // 分解失败，使用 eigs 估计特征值范围（仅对称）
            // std::cout << "SimplicalLDLT: 分解失败" << std::endl;
            return MatrixDefiniteness::SymmetricIndefinite;
        }

        const Eigen::VectorXd &D = ldlt.vectorD();

        int pos = 0, neg = 0, zero = 0;
        for (Eigen::Index i = 0; i < D.size(); ++i)
        {
            double val = D(i);
            if (val > tol)
                ++pos;
            else if (val < -tol)
                ++neg;
            else
                ++zero;
        }

    
        // 判定正定性
        if (neg > 0 && pos > 0)
            return MatrixDefiniteness::SymmetricIndefinite;
        if (neg > 0 && pos == 0)
            return MatrixDefiniteness::SymmetricNegativeDefinite;
        if (pos > 0 && neg == 0)
        {
            if (zero > 0)
                return MatrixDefiniteness::SymmetricPositiveSemiDefinite;
            else
                return MatrixDefiniteness::SymmetricPositiveDefinite;
        }
        if (pos == 0 && neg == 0 && zero > 0)
            return MatrixDefiniteness::SymmetricPositiveSemiDefinite;
        return MatrixDefiniteness::Unknown;
    }

    // 判断矩阵是否为对称矩阵（）
    bool isSymmetric(const Eigen::SparseMatrix<double> &A)
    {
        double tol = 1e-12;
        if (A.rows() != A.cols())
            return false;
        // 遍历所有非零元，检查 (i,j) 与 (j,i) 的一致性
        for (int k = 0; k < A.outerSize(); ++k)
        {
            for (Eigen::SparseMatrix<double>::InnerIterator it(A, k); it; ++it)
            {
                int i = it.row(), j = it.col();
                if (i == j)
                    continue; // 对角元不需检查
                double mirror = A.coeff(j, i);
                if (mirror == 0.0)
                    return false; // 模式不对称
                double diff = std::abs(it.value() - mirror);
                double maxAbs = std::max(std::abs(it.value()), std::abs(mirror));
                if (diff > tol * maxAbs)
                    return false;
            }
        }
        return true;
    }

    //* 判断矩阵是否为正定矩阵（已存在高级判断，该函数未启用）
    bool isPositiveDefinite(const Eigen::SparseMatrix<double> &A)
    {
        if (!isSymmetric(A))
            return false;
        Eigen::SimplicialLDLT<Eigen::SparseMatrix<double>> ldlt(A);
        if (ldlt.info() != Eigen::Success)
            return false;
        const auto &D = ldlt.vectorD();
        for (Eigen::Index i = 0; i < D.size(); ++i)
        {
            if (D(i) <= 1e-9)
                return false;
        }
        return true;
    }

    // 检查矩阵的存储类型
    MatrixStorageType checkMatrixStorage(const Eigen::SparseMatrix<double> &A)
    {
        if (A.rows() != A.cols())
            return MatrixStorageType::NonSquare;
        bool hasUpper = false, hasLower = false;
        for (int k = 0; k < A.outerSize(); ++k)
        {
            for (Eigen::SparseMatrix<double>::InnerIterator it(A, k); it; ++it)
            {
                int i = it.row(), j = it.col();
                if (i < j)
                    hasUpper = true;
                else if (i > j)
                    hasLower = true;
            }
        }
        if (hasUpper && !hasLower)
            return MatrixStorageType::UpperTriangular;
        if (!hasUpper && hasLower)
            return MatrixStorageType::LowerTriangular;
        if (!hasUpper && !hasLower)
            return MatrixStorageType::FullSymmetric; // 只有对角元
        // 上下三角都有，检查数值对称性
        for (int k = 0; k < A.outerSize(); ++k)
        {
            for (Eigen::SparseMatrix<double>::InnerIterator it(A, k); it; ++it)
            {
                int i = it.row(), j = it.col();
                if (i == j)
                    continue;
                double mirror = A.coeff(j, i);
                if (mirror == 0.0)
                    return MatrixStorageType::FullNonSymmetric;
                if (std::abs(it.value() - mirror) > 1e-12 * std::abs(it.value()))
                    return MatrixStorageType::FullNonSymmetric;
            }
        }
        return MatrixStorageType::FullSymmetric;
    }

    // 打印矩阵性质——返回格式化字符串而非直接 std::cout
    std::string printMatrixProperties(const Eigen::SparseMatrix<double> &mat, const std::string &name, double cond_est)
    {
        std::ostringstream oss;
        auto storage = checkMatrixStorage(mat);
        switch (storage)
        {
        case MatrixStorageType::FullSymmetric:
        {
            MatrixDefiniteness def = classifyDefiniteness(mat, 1e-12);
            oss << name << "(";
            switch (def)
            {
            case MatrixDefiniteness::SymmetricPositiveDefinite:
                oss << "SPD";
                break;
            case MatrixDefiniteness::SymmetricPositiveSemiDefinite:
                oss << "SPSD";
                break;
            case MatrixDefiniteness::SymmetricIndefinite:
                oss << "Indefinite";
                break;
            case MatrixDefiniteness::SymmetricNegativeDefinite:
                oss << "Negative Definite";
                break;
            default:
                oss << "Symmetric (unknown definiteness)";
                break;
            }
            if (cond_est > 0 && (def == MatrixDefiniteness::SymmetricPositiveDefinite ||
                                 def == MatrixDefiniteness::SymmetricNegativeDefinite))
            {
                oss << "  Cond: " << std::scientific << std::setprecision(2) << cond_est;
            }
            oss << ") ";
            break;
        }
        case MatrixStorageType::FullNonSymmetric:
            oss << name << "(Non-symmetric, numerically asymmetric) ";
            break;
        case MatrixStorageType::UpperTriangular:
            oss << name << "(Upper triangular) ";
            break;
        case MatrixStorageType::LowerTriangular:
            oss << name << "(Lower triangular) ";
            break;
        case MatrixStorageType::NonSquare:
            oss << name << "(Non-square) ";
            break;
        default:
            oss << name << "(Unknown storage) ";
            break;
        }
        oss << "\n";
        return oss.str();
    }


    //------- 矩阵条件数估计-------
    // 幂法
    double powerMethod(const Eigen::SparseMatrix<double> &A, int max_iter, double tol)
    {
        int n = A.rows();
        Eigen::VectorXd x = Eigen::VectorXd::Random(n);
        x.normalize();
        double lambda_old = 0.0, lambda = 0.0;
        for (int iter = 0; iter < max_iter; ++iter)
        {
            Eigen::VectorXd y = A * x;
            lambda = y.norm();
            if (lambda < 1e-14)
                break;
            y /= lambda;
            if (x.dot(y) < 0)
                lambda = -lambda;
            if (std::abs(lambda - lambda_old) < tol * std::abs(lambda))
                break;
            lambda_old = lambda;
            x = y;
        }
        return std::abs(lambda);
    }

#ifdef EIGEN_PARDISO_SUPPORT
    // 逆幂法（Pardiso 版）
    double inversePowerMethod(const Eigen::PardisoLU<Eigen::SparseMatrix<double>> &solver,
                              int max_iter, double tol)
    {
        int n = solver.rows();
        Eigen::VectorXd x = Eigen::VectorXd::Random(n);
        x.normalize();
        double lambda_old = 0.0, lambda = 0.0;
        for (int iter = 0; iter < max_iter; ++iter)
        {
            Eigen::VectorXd y = solver.solve(x);
            double norm_y = y.norm();
            if (norm_y < 1e-14)
                break;
            lambda = 1.0 / norm_y;
            y *= lambda;
            if (x.dot(y) < 0)
                lambda = -lambda;
            if (std::abs(lambda - lambda_old) < tol * std::abs(lambda))
                break;
            lambda_old = lambda;
            x = y;
        }
        return std::abs(lambda);
    }
#endif

    // 逆幂法（SparseLU 通用版）
    double inversePowerMethodSparseLU(const Eigen::SparseLU<Eigen::SparseMatrix<double>> &solver,
                                      int max_iter, double tol)
    {
        int n = solver.rows();
        Eigen::VectorXd x = Eigen::VectorXd::Random(n);
        x.normalize();
        double lambda_old = 0.0, lambda = 0.0;
        for (int iter = 0; iter < max_iter; ++iter)
        {
            Eigen::VectorXd y = solver.solve(x);
            double norm_y = y.norm();
            if (norm_y < 1e-14)
                break;
            lambda = 1.0 / norm_y;
            y *= lambda;
            if (x.dot(y) < 0)
                lambda = -lambda;
            if (std::abs(lambda - lambda_old) < tol * std::abs(lambda))
                break;
            lambda_old = lambda;
            x = y;
        }
        return std::abs(lambda);
    }

    double estimateConditionNumber(const Eigen::SparseMatrix<double> &A)
    {
        int n = A.rows();
        if (n == 0)
            return -1.0;
        double lambda_max = powerMethod(A, 100, 1e-6);
        if (lambda_max < 1e-14)
            return -1.0;

#ifdef EIGEN_PARDISO_SUPPORT
        // 优先使用 PardisoLU（更快）
        Eigen::PardisoLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(A);
        if (solver.info() != Eigen::Success)
        {
            std::cerr << "[Cond. est.] PardisoLU failed, falling back to SparseLU" << std::endl;
            Eigen::SparseLU<Eigen::SparseMatrix<double>> solver_sl;
            solver_sl.compute(A);
            if (solver_sl.info() != Eigen::Success)
            {
                std::cerr << "[Cond. est.] SparseLU also failed" << std::endl;
                return -1.0;
            }
            double lambda_min = inversePowerMethodSparseLU(solver_sl, 100, 1e-6);
            if (lambda_min < 1e-14)
                return 1e15;
            return lambda_max / lambda_min;
        }
        double lambda_min = inversePowerMethod(solver, 100, 1e-6);
#else
        // 无 MKL：使用 SparseLU
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(A);
        if (solver.info() != Eigen::Success)
        {
            std::cerr << "[Cond. est.] SparseLU failed" << std::endl;
            return -1.0;
        }
        double lambda_min = inversePowerMethodSparseLU(solver, 100, 1e-6);
#endif

        if (lambda_min < 1e-14)
            return 1e15;
        return lambda_max / lambda_min;
    }

    void estimateAndWarnConditionNumber(const Eigen::SparseMatrix<double> &K)
    {
        double cond = estimateConditionNumber(K);
        std::cout << "K condition number estimate: " << std::scientific << std::setprecision(2) << cond << std::defaultfloat << std::endl;
        if (cond > 1e10)
            std::cout << "SEVERE: extremely large condition number, results may be unstable." << std::endl;
        else if (cond > 1e8)
            std::cout << "WARNING: large condition number, iterative methods may converge slowly." << std::endl;
    }
} // namespace QEP
