/**
 * @file UnifiedSolver.cpp
 * @brief 统一二次特征值求解器实现
 * 算法的核心部分，构造op算子，实现线性化求解器，并封装为统一的二次特征值求解接口。
 */

#include "qep/QEP.h"
#include "config/GlobalConfig.h"
#include <iostream>
#include <chrono>
#include <stdexcept>
#include <variant>
#include <iomanip>
#include <algorithm>
#include <Eigen/SparseLU>
#include <Eigen/PardisoSupport>
#include <Eigen/IterativeLinearSolvers>
#include <Eigen/SparseCholesky>
#include <unsupported/Eigen/IterativeSolvers>
#include <Spectra/GenEigsSolver.h>

namespace QEP
{

    // ----------------------------------------------------------------------------
    // 使用 variant 容纳不同类型的求解器对象
    // ----------------------------------------------------------------------------
    using SolverVariant = std::variant<
        std::unique_ptr<Eigen::SparseLU<Eigen::SparseMatrix<double>>>,
        std::unique_ptr<Eigen::PardisoLU<Eigen::SparseMatrix<double>>>,
        std::unique_ptr<Eigen::SimplicialLLT<Eigen::SparseMatrix<double>>>,
        std::unique_ptr<Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                                                 Eigen::Lower | Eigen::Upper,
                                                 Eigen::IncompleteCholesky<double>>>,
        std::unique_ptr<Eigen::BiCGSTAB<Eigen::SparseMatrix<double>,
                                        Eigen::IncompleteLUT<double>>>,
        std::unique_ptr<Eigen::GMRES<Eigen::SparseMatrix<double>,
                                     Eigen::IncompleteLUT<double>>>>;

    // ----------------------------------------------------------------------------
    // 泛型移频算子
    // ----------------------------------------------------------------------------
    class UnifiedShiftInvertOp
    {
    public:
        using Scalar = double;
        LinearSolverConfig config_; // 线性求解器配置
        double preprocessing_time;  // 预处理时间
        double build_time;          // 构建目标矩阵时间，包含条件数估计时间
        double condition_number_S;  // 目标矩阵条件数
        double cond_time;           // 条件数估计时间

        UnifiedShiftInvertOp(const Eigen::SparseMatrix<double> &M,
                             const Eigen::SparseMatrix<double> &C,
                             const Eigen::SparseMatrix<double> &K,
                             double sigma,
                             LinearSolverType solverType,
                             const LinearSolverConfig &config)
            : M_(M), C_(C), K_(K), n_(M.rows()), sigma_(sigma),
              solverType_(solverType), config_(config)
        {
            work1_.resize(n_);
            work2_.resize(n_);

            buildTargetMatrix();

            auto t_pre0 = std::chrono::high_resolution_clock::now();
            setupSolver();
            auto t_pre1 = std::chrono::high_resolution_clock::now();
            preprocessing_time = std::chrono::duration<double>(t_pre1 - t_pre0).count();
            if (Config::LOG_LEVEL >= 1)
            {
                std::cout << std::fixed << std::setprecision(3) << "S预处理时间: "
                          << preprocessing_time << std::defaultfloat << " s\n";
            }
        }

        int rows() const { return 2 * n_; }
        int cols() const { return 2 * n_; }

        // 核心数学原理与核心操作
        void perform_op(const double *x_in, double *y_out) const
        {
            Eigen::Map<const Eigen::VectorXd> x(x_in, 2 * n_);
            Eigen::Map<Eigen::VectorXd> y(y_out, 2 * n_);

            const auto x1 = x.head(n_);
            const auto x2 = x.tail(n_);

            // rhs = -(M*x1 + (C+σM)*x2)
            work1_ = -(M_ * x1 + CM_ * x2);
            work2_ = solveWithTracking(work1_);

            // 输出 w = [x2 + σ*w2; w2]
            y.head(n_) = x2 + sigma_ * work2_;
            y.tail(n_) = work2_;
        }

        // 内层迭代统计信息 (public以便外部访问)
        mutable int total_inner_iterations_ = 0;
        mutable int num_solves_ = 0;
        mutable bool last_solve_converged_ = true;
        mutable int last_solve_iterations_ = 0;
        mutable int max_inner_iterations_per_solve_ = 0;
        mutable double total_solve_time_ = 0.0;

    private:
        const Eigen::SparseMatrix<double> &M_;
        const Eigen::SparseMatrix<double> &C_;
        const Eigen::SparseMatrix<double> &K_;
        int n_;
        double sigma_;
        LinearSolverType solverType_;

        Eigen::SparseMatrix<double> S_;  // 移频矩阵 S = K + σC + σ²M
        Eigen::SparseMatrix<double> CM_; // C + σM

        SolverVariant solver_;
        mutable Eigen::VectorXd work1_, work2_;

        // 构建目标矩阵 S = K + σC + σ²M (内层关键求解矩阵 inv(S))    CM = C + σM (减少重复计算过程，提高性能)
        void buildTargetMatrix()
        {
            //------------- 构建S矩阵 ---------------
            auto t_build0 = std::chrono::high_resolution_clock::now();
            std::vector<Eigen::Triplet<double>> trips;
            trips.reserve(K_.nonZeros() + C_.nonZeros() + M_.nonZeros());

            // K
            for (int k = 0; k < K_.outerSize(); ++k)
            {
                for (Eigen::SparseMatrix<double>::InnerIterator it(K_, k); it; ++it)
                {
                    trips.emplace_back(it.row(), it.col(), it.value());
                }
            }

            if (std::abs(sigma_) > 1e-14)
            {
                double sigma2 = sigma_ * sigma_;
                // σC
                for (int k = 0; k < C_.outerSize(); ++k)
                {
                    for (Eigen::SparseMatrix<double>::InnerIterator it(C_, k); it; ++it)
                    {
                        trips.emplace_back(it.row(), it.col(), sigma_ * it.value());
                    }
                }
                // σ²M
                for (int k = 0; k < M_.outerSize(); ++k)
                {
                    for (Eigen::SparseMatrix<double>::InnerIterator it(M_, k); it; ++it)
                    {
                        trips.emplace_back(it.row(), it.col(), sigma2 * it.value());
                    }
                }
            }

            S_.resize(n_, n_);
            S_.setFromTriplets(trips.begin(), trips.end());
            S_.makeCompressed();
            auto t_build1 = std::chrono::high_resolution_clock::now();
            double build_time = std::chrono::duration<double>(t_build1 - t_build0).count();
            if (Config::LOG_LEVEL >= 1)
            {

                std::cout << std::fixed << std::setprecision(3) << "S矩阵构建时间: "
                          << build_time << std::defaultfloat << " s\n";
            }
            
            //------------- 估计S矩阵条件数---------------
            if (Config::LOG_LEVEL >= 1)
            {
                auto t_cond0 = std::chrono::high_resolution_clock::now();
                condition_number_S = estimateConditionNumber(S_, false);
                std::cout << std::scientific << std::setprecision(2) << "S矩阵条件数: "
                          << condition_number_S << std::defaultfloat << std::endl;
                auto t_cond1 = std::chrono::high_resolution_clock::now();
                double cond_time = std::chrono::duration<double>(t_cond1 - t_cond0).count();

                std::cout << std::fixed << std::setprecision(3) << "S矩阵条件数估计时间: "
                          << cond_time << std::defaultfloat << " s\n";
            }
            //------------- 预组合矩阵 CM = C + σM--------------
            if (std::abs(sigma_) > 1e-14)
            {
                std::vector<Eigen::Triplet<double>> cm_trips;
                cm_trips.reserve(C_.nonZeros() + M_.nonZeros());
                for (int k = 0; k < C_.outerSize(); ++k)
                {
                    for (Eigen::SparseMatrix<double>::InnerIterator it(C_, k); it; ++it)
                    {
                        cm_trips.emplace_back(it.row(), it.col(), it.value());
                    }
                }
                for (int k = 0; k < M_.outerSize(); ++k)
                {
                    for (Eigen::SparseMatrix<double>::InnerIterator it(M_, k); it; ++it)
                    {
                        cm_trips.emplace_back(it.row(), it.col(), sigma_ * it.value());
                    }
                }
                CM_.resize(n_, n_);
                CM_.setFromTriplets(cm_trips.begin(), cm_trips.end());
                CM_.makeCompressed();
            }
            else
            {
                CM_ = C_;
            }
        }

        // 设置求解器并使用预条件子进行预处理
        void
        setupSolver()
        {
            switch (solverType_)
            {
            case LinearSolverType::SparseLU:
            {
                auto lu = std::make_unique<Eigen::SparseLU<Eigen::SparseMatrix<double>>>();
                lu->compute(S_); // LU分解预处理，直接法直接分解，一次分解，永久调用
                if (lu->info() != Eigen::Success)
                    throw std::runtime_error("SparseLU decomposition failed");
                solver_ = std::move(lu);
                break;
            }
            case LinearSolverType::PardisoLU:
            {
                auto pardiso = std::make_unique<Eigen::PardisoLU<Eigen::SparseMatrix<double>>>();
                pardiso->compute(S_);
                if (pardiso->info() != Eigen::Success)
                    throw std::runtime_error("PardisoLU decomposition failed");
                solver_ = std::move(pardiso);
                break;
            }
            case LinearSolverType::SimplicialLLT:
            {
                auto llt = std::make_unique<Eigen::SimplicialLLT<Eigen::SparseMatrix<double>>>();
                llt->compute(S_);
                if (llt->info() != Eigen::Success)
                    throw std::runtime_error("SimplicialLLT decomposition failed");
                solver_ = std::move(llt);
                break;
            }
            case LinearSolverType::ConjugateGradient:
            {
                using CG = Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                                                    Eigen::Lower | Eigen::Upper,
                                                    Eigen::IncompleteCholesky<double>>;
                auto cg = std::make_unique<CG>();
                cg->setTolerance(config_.inner_tolerance);
                cg->setMaxIterations(config_.inner_maxIterations);
                cg->compute(S_); // 利用预条件子预处理矩阵，仅改良矩阵性质，并不分解
                if (cg->info() != Eigen::Success)
                    throw std::runtime_error("CG setup failed");
                solver_ = std::move(cg);
                break;
            }
            case LinearSolverType::BiCGSTAB:
            {
                using BICG = Eigen::BiCGSTAB<Eigen::SparseMatrix<double>,
                                             Eigen::IncompleteLUT<double>>;
                auto bicg = std::make_unique<BICG>();
                bicg->preconditioner().setFillfactor(config_.fillFactor);
                bicg->preconditioner().setDroptol(config_.dropTol);
                bicg->setTolerance(config_.inner_tolerance);
                bicg->setMaxIterations(config_.inner_maxIterations);
                bicg->compute(S_);
                if (bicg->info() != Eigen::Success)
                    throw std::runtime_error("BiCGSTAB setup failed");
                solver_ = std::move(bicg);
                break;
            }
            case LinearSolverType::GMRES:
            {
                using GMRES = Eigen::GMRES<Eigen::SparseMatrix<double>,
                                           Eigen::IncompleteLUT<double>>;
                auto gmres = std::make_unique<GMRES>();
                gmres->preconditioner().setFillfactor(config_.fillFactor);
                gmres->preconditioner().setDroptol(config_.dropTol);
                gmres->set_restart(config_.restart);
                gmres->setTolerance(config_.inner_tolerance);
                gmres->setMaxIterations(config_.inner_maxIterations);
                gmres->compute(S_);
                if (gmres->info() != Eigen::Success)
                    throw std::runtime_error("GMRES setup failed");
                solver_ = std::move(gmres);
                break;
            }
            }
        }

        // 可以在次进行矩阵条件数估计，以判断迭代法预处理子效果
        // estimateAndWarnConditionNumber(S_, true);

        // 解线性方程组solver(rhs)
        Eigen::VectorXd solve(const Eigen::VectorXd &rhs) const
        {
            return std::visit([&rhs](auto &&solver) -> Eigen::VectorXd
                              { return solver->solve(rhs); },
                              solver_);
        }

        // 统计求解次数，记录求解时间，记录迭代次数,仅针对迭代法
        Eigen::VectorXd solveWithTracking(const Eigen::VectorXd &rhs) const
        {
            num_solves_++;
            auto t_start = std::chrono::high_resolution_clock::now();

            Eigen::VectorXd result;

            // Check if using iterative solver
            // 判断是否为迭代求解器 ??? 语法原理？
            bool is_iterative = std::holds_alternative<
                                    std::unique_ptr<Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                                                                             Eigen::Lower | Eigen::Upper,
                                                                             Eigen::IncompleteCholesky<double>>>>(solver_) ||
                                std::holds_alternative<
                                    std::unique_ptr<Eigen::BiCGSTAB<Eigen::SparseMatrix<double>,
                                                                    Eigen::IncompleteLUT<double>>>>(solver_) ||
                                std::holds_alternative<
                                    std::unique_ptr<Eigen::GMRES<Eigen::SparseMatrix<double>,
                                                                 Eigen::IncompleteLUT<double>>>>(solver_);

            if (is_iterative)
            { // 存在重复代码，可以改进
                // 处理迭代求解器：捕获统计信息
                if (std::holds_alternative<
                        std::unique_ptr<Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                                                                 Eigen::Lower | Eigen::Upper,
                                                                 Eigen::IncompleteCholesky<double>>>>(solver_))
                {
                    auto &solver = std::get<std::unique_ptr<Eigen::ConjugateGradient<Eigen::SparseMatrix<double>,
                                                                                     Eigen::Lower | Eigen::Upper,
                                                                                     Eigen::IncompleteCholesky<double>>>>(solver_);
                    result = solver->solve(rhs);
                    last_solve_iterations_ = solver->iterations();
                    last_solve_converged_ = (solver->info() == Eigen::Success);
                    total_inner_iterations_ += last_solve_iterations_;
                    max_inner_iterations_per_solve_ = std::max(max_inner_iterations_per_solve_, last_solve_iterations_);
                }
                else if (std::holds_alternative<
                             std::unique_ptr<Eigen::BiCGSTAB<Eigen::SparseMatrix<double>,
                                                             Eigen::IncompleteLUT<double>>>>(solver_))
                {
                    auto &solver = std::get<std::unique_ptr<Eigen::BiCGSTAB<Eigen::SparseMatrix<double>,
                                                                            Eigen::IncompleteLUT<double>>>>(solver_);
                    result = solver->solve(rhs);
                    last_solve_iterations_ = solver->iterations();
                    last_solve_converged_ = (solver->info() == Eigen::Success);
                    total_inner_iterations_ += last_solve_iterations_;
                    max_inner_iterations_per_solve_ = std::max(max_inner_iterations_per_solve_, last_solve_iterations_);
                }
                else if (std::holds_alternative<
                             std::unique_ptr<Eigen::GMRES<Eigen::SparseMatrix<double>,
                                                          Eigen::IncompleteLUT<double>>>>(solver_))
                {
                    auto &solver = std::get<std::unique_ptr<Eigen::GMRES<Eigen::SparseMatrix<double>,
                                                                         Eigen::IncompleteLUT<double>>>>(solver_);
                    result = solver->solve(rhs);
                    last_solve_iterations_ = solver->iterations();
                    last_solve_converged_ = (solver->info() == Eigen::Success);
                    total_inner_iterations_ += last_solve_iterations_;
                    max_inner_iterations_per_solve_ = std::max(max_inner_iterations_per_solve_, last_solve_iterations_);
                }
            }
            else
            {
                // 处理直接求解器：直接求解
                result = std::visit([&rhs](auto &&solver) -> Eigen::VectorXd
                                    { return solver->solve(rhs); },
                                    solver_);
            }

            auto t_end = std::chrono::high_resolution_clock::now();
            total_solve_time_ += std::chrono::duration<double>(t_end - t_start).count();

            return result;
        }
    };

    // ============================================================================
    // 统一求解函数实现（唯一入口）
    // ============================================================================
    QuadraticEigenvalueResult solveQEP_Unified(
        const QuadraticEigenvalueProblem &problem,
        int nev,
        double sigma,
        LinearSolverType solverType,
        const LinearSolverConfig &config)
    {

        QuadraticEigenvalueResult result;
        result.success = false;
        result.problem_dimension = problem.dimension;
        result.linearized_dimension = 2 * problem.dimension;

        // 生成方法名称
        switch (solverType)
        {
        case LinearSolverType::SparseLU:
            result.method_used = "SparseLU";
            break;
        case LinearSolverType::PardisoLU:
            result.method_used = "PardisoLU";
            break;
        case LinearSolverType::SimplicialLLT:
            result.method_used = "SimplicialLLT";
            break;
        case LinearSolverType::ConjugateGradient:
            result.method_used = "ConjugateGradient";
            break;
        case LinearSolverType::BiCGSTAB:
            result.method_used = "BiCGSTAB";
            break;
        case LinearSolverType::GMRES:
            result.method_used = "GMRES";
            break;
        }

        auto t0 = std::chrono::high_resolution_clock::now();

        try
        {
            int n = problem.dimension;
            if (n <= 0)
                throw std::runtime_error("Dimension is zero");

            int ncv;
            if (n < 10000)
                ncv = std::min(3 * nev + 6, 2 * n);
            else if (n < 100000)
                ncv = std::min(2 * nev + 10, 2 * n);
            else
                ncv = std::min(2 * nev + 10, std::min(100, 2 * n));

            if (Config::LOG_LEVEL >= 1)
            {
                std::cout << "[" << result.method_used << "] n=" << n
                          << ", sigma=" << sigma
                          << ", nev=" << nev << ", ncv=" << ncv << std::endl;
            }

            UnifiedShiftInvertOp op(problem.M, problem.C, problem.K,
                                    sigma, solverType, config);

            Spectra::GenEigsSolver<UnifiedShiftInvertOp> eigs(op, nev, ncv); // 初始化(包括预处理)不单独计时，但是计入总时间
            eigs.init();

            auto t_iter0 = std::chrono::high_resolution_clock::now();
            int outer_maxIterations = op.config_.outer_maxIterations;
            double outer_tolerance = op.config_.outer_tolerance;
            eigs.compute(Spectra::SortRule::LargestMagn, outer_maxIterations, outer_tolerance); // Anorldi迭代过程
            auto t_iter1 = std::chrono::high_resolution_clock::now();

            result.arnoldi_time = std::chrono::duration<double>(t_iter1 - t_iter0).count();
            result.preprocessing_time = op.preprocessing_time;

            if (Config::LOG_LEVEL >= 1)
            {
                std::cout << std::fixed << std::setprecision(3) << "Arnoldi迭代时间: "
                          << result.arnoldi_time << std::defaultfloat << " s\n";
                std::cout << "Arnoldi迭代次数: "
                          << eigs.num_iterations() << std::endl;
            }

            // 填充内层迭代统计信息，全统计到结果中
            result.total_inner_solves = op.num_solves_;
            result.total_inner_iterations = op.total_inner_iterations_;
            result.max_inner_iterations_per_solve = op.max_inner_iterations_per_solve_;
            result.avg_inner_iterations = op.num_solves_ > 0 ? static_cast<double>(op.total_inner_iterations_) / op.num_solves_ : 0.0;
            result.arnoldi_iterations = eigs.num_iterations();
            result.linear_solve_total_time = op.total_solve_time_;

            if (Config::LOG_LEVEL >= 2)
            {
                std::cout << "内层求解统计:\n";
                std::cout << "总求解次数: " << result.total_inner_solves << "\n";
                std::cout << "总迭代次数: " << result.total_inner_iterations << "\n";
                std::cout << "最大单次迭代次数: " << result.max_inner_iterations_per_solve << "\n";
                std::cout << "平均迭代次数: " << result.avg_inner_iterations << "\n";
                std::cout << "内层求解总时间: " << std::fixed << std::setprecision(3) << result.linear_solve_total_time << std::defaultfloat << " s\n";
                std::cout << "平均单次求解时间: "
                          << std::fixed << std::setprecision(3) << (op.num_solves_ > 0 ? result.linear_solve_total_time / op.num_solves_ : 0.0) << std::defaultfloat << " s\n";
            }

            bool converged = (eigs.info() == Spectra::CompInfo::Successful);
            result.eigensolver_fully_converged = converged;

            if (!converged)
            {
                std::cerr << "Warning: not fully converged (code="
                          << static_cast<int>(eigs.info()) << ")\n";
            }

            if (converged || eigs.info() == Spectra::CompInfo::NotConverging)
            {
                Eigen::VectorXcd thetas = eigs.eigenvalues();
                Eigen::MatrixXcd evec_2n = eigs.eigenvectors();
                int m = static_cast<int>(thetas.size()); // ?static_cast<int>

                // 特征值恢复：λ = σ + 1/θ
                Eigen::VectorXcd lambdas(m);
                for (int i = 0; i < m; ++i)
                {
                    if (std::abs(thetas(i)) > 1e-14)
                        lambdas(i) = sigma + 1.0 / thetas(i);
                    else
                        lambdas(i) = std::complex<double>(1e15, 0.0);
                }

                // 按 |λ - σ| 升序排列
                std::vector<std::pair<double, int>> sorter;
                for (int i = 0; i < m; ++i)
                {
                    if (std::isfinite(std::abs(lambdas(i))))
                        sorter.push_back({std::abs(lambdas(i) - sigma), i});
                }
                std::sort(sorter.begin(), sorter.end(),
                          [](const auto &a, const auto &b)
                          { return a.first < b.first; });

                int sz = static_cast<int>(sorter.size());
                result.eigenvalues_real.resize(sz);
                result.eigenvalues_imag.resize(sz);
                result.eigenvectors.resize(n, sz);

                for (int k = 0; k < sz; ++k)
                {
                    int j = sorter[k].second;
                    result.eigenvalues_real(k) = lambdas(j).real();
                    result.eigenvalues_imag(k) = lambdas(j).imag();
                    if (j < evec_2n.cols())
                        result.eigenvectors.col(k) = evec_2n.col(j).tail(n);
                }
                result.success = true;
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << std::endl;
            result.success = false;
            result.warning_message = e.what();
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        result.solution_time = std::chrono::duration<double>(t1 - t0).count();
        if (Config::LOG_LEVEL >= 1)
        {
            std::cout << "总求解时间: "
                      << std::fixed << std::setprecision(3) << result.solution_time << std::defaultfloat << " s" << std::endl;
        }

        return result;
    }

} // namespace QEP