//==============================================================================
//  src/gui/workers/SolverWorker.cpp  —  异步求解执行、残差预计算、日志输出
//==============================================================================

#include "SolverWorker.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"
#include "config/SolverParams.h"
#include "config/SolverRegistry.h"
#include "qep/QEP.h"
#include "io/MatrixIO.h"
#include <QThread>
#include <QDir>
#include <QFileInfo>
#include <iostream>
#include <streambuf>
#include <mutex>
#include <functional>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <cmath>

// 简化的流捕获缓冲区 — 仅用于捕获第三方库的意外输出
class CaptureBuffer : public std::streambuf {
public:
    using LineCallback = std::function<void(const std::string &)>;
    void setCallback(LineCallback cb) { callback_ = std::move(cb); }

protected:
    int overflow(int c) override {
        if (c == EOF) return c;
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_ += static_cast<char>(c);
        if (c == '\n') {
            if (callback_) callback_(buffer_);
            buffer_.clear();
        }
        return c;
    }

    int sync() override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!buffer_.empty()) {
            if (callback_) callback_(buffer_);
            buffer_.clear();
        }
        return 0;
    }

private:
    std::mutex mtx_;
    std::string buffer_;
    LineCallback callback_;
};

struct StreamRedirectGuard {
    std::streambuf *oldCout;
    std::streambuf *oldCerr;
    ~StreamRedirectGuard() {
        std::cout.rdbuf(oldCout);
        std::cerr.rdbuf(oldCerr);
    }
};

void SolverWorker::setRequest(const SolveRequest &req)
{
    request_ = req;
}

void SolverWorker::run()
{
    auto& cfg = Config::ConfigManager::instance();
    auto appCfg = cfg.toAppConfig();
    auto solParams = cfg.toSolverParams();

    double checkTol = appCfg.check_tolerance;
    bool printEigenvals = appCfg.print_eigenvalues;
    int ompThreads = solParams.omp_threads;
    int mklThreads = solParams.mkl_threads;

    int total = static_cast<int>(request_.cases.size());
    emit progressChanged(0, total);

    auto allMethods = Config::getSolverMethods(solParams);
    std::vector<Config::SolverMethod> selectedMethods;
    for (auto &m : allMethods) {
        if (request_.solverNames.contains(QString::fromStdString(m.name)))
            selectedMethods.push_back(m);
    }

    if (selectedMethods.empty()) {
        emit logMessage(QString("[ERROR] No solvers selected.\n"));
        emit finished();
        return;
    }

    int nSolverMethods = static_cast<int>(selectedMethods.size());

    CaptureBuffer capture;
    capture.setCallback([this](const std::string &line) {
        emit logMessage(QString::fromUtf8(line.c_str(), line.size()));
    });
    auto *oldCout = std::cout.rdbuf(&capture);
    auto *oldCerr = std::cerr.rdbuf(&capture);
    StreamRedirectGuard guard{oldCout, oldCerr};

    // ========================================================================
    //  Environment header
    // ========================================================================
    const char *mklStatus = "disabled";
#ifdef EIGEN_USE_MKL_ALL
    mklStatus = "enabled";
#endif

    const char *avxStatus = "off";
#ifdef EIGEN_VECTORIZE_AVX
    avxStatus = "on";
#endif

    const QChar DH(0x2550);  // ═
    const QChar TL(0x2554);  // ╔
    const QChar TR(0x2557);  // ╗
    const QChar BL(0x255A);  // ╚
    const QChar BR(0x255D);  // ╝
    const QChar VH(0x2551);  // ║

    emit logMessage("\n");
    emit logMessage(QString("%1%2%3\n")
        .arg(TL).arg(QString(60, DH)).arg(TR));
    emit logMessage(QString("%1  QEP Solver  |  OMP=%2  MKL=%3  MKL: %4  AVX2: %5    %1\n")
        .arg(VH).arg(ompThreads).arg(mklThreads).arg(mklStatus).arg(avxStatus));
    emit logMessage(QString("%1%2%3\n")
        .arg(BL).arg(QString(60, DH)).arg(BR));

    // ========================================================================
    //  Per-case processing
    // ========================================================================

    for (int i = 0; i < total; ++i) {
        if (QThread::currentThread()->isInterruptionRequested())
            break;

        const auto &tc = request_.cases[i];
        emit progressChanged(i, total);
        emit caseStarted(QString::fromStdString(tc.name));

        // Load problem (auto-conversion handled inside createTestProblemFromFiles)
        QEP::QuadraticEigenvalueProblem problem;
        try {
            problem = QEP::createTestProblemFromFiles(
                tc.name, tc.M_file, tc.C_file, tc.K_file, tc.is_sparse, solParams,
                appCfg.overwrite);
        } catch (const std::exception &e) {
            emit logMessage(QString("  [ERROR] Load failed: %1\n").arg(e.what()));
            continue;
        }

        if (problem.dimension == 0) {
            emit logMessage(QString("  [ERROR] Matrix load failed, skipping.\n"));
            continue;
        }

        // Matrix property checks
        bool doCond = appCfg.enable_condition_estimation;
        bool doCheck = appCfg.enable_matrix_property_check;
        double condM = -1, condC = -1;
        double condK = problem.condition_number_K;

        if (doCond) {
            condC = QEP::estimateConditionNumber(problem.C);
            condM = QEP::estimateConditionNumber(problem.M);
            if (condK <= 0.0)
                condK = QEP::estimateConditionNumber(problem.K);
        }
        if (doCheck) {
            emit logMessage(QString::fromStdString(
                QEP::printMatrixProperties(problem.M, "M", condM)));
            emit logMessage(QString::fromStdString(
                QEP::printMatrixProperties(problem.C, "C", condC)));
            emit logMessage(QString::fromStdString(
                QEP::printMatrixProperties(problem.K, "K", condK)));
            emit logMessage("\n");
        }

        // Case info header
        emit logMessage("\n");
        emit logMessage(QString("  %1  |  dim=%2  |  M=%3  C=%4  K=%5  |  cond(K)=%6\n")
            .arg(QString::fromStdString(tc.name), -26)
            .arg(problem.dimension, 6)
            .arg(problem.M.nonZeros(), 6)
            .arg(problem.C.nonZeros(), 6)
            .arg(problem.K.nonZeros(), 7)
            .arg(condK > 0 ? QString::number(condK, 'e', 2) : QString("-")));

        int caseConvergedCount = 0;

        // Run each selected solver
        for (const auto &method : selectedMethods) {
            if (QThread::currentThread()->isInterruptionRequested())
                break;

            emit solverProgress(
                QString::fromStdString(tc.name),
                QString::fromStdString(method.name));

            emit logMessage("\n");
            emit logMessage(QString("  --- %1 ---\n")
                .arg(QString::fromStdString(method.name)));

            auto start = std::chrono::high_resolution_clock::now();
            QEP::QuadraticEigenvalueResult result;
            bool solverOk = true;

            try {
                result = method.solver(problem, request_.nev, request_.sigma);
            } catch (const std::exception &e) {
                emit logMessage(QString("  [SOLVER ERROR] %1\n").arg(e.what()));
                solverOk = false;
            }

            auto end = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(end - start).count();
            int nEig = static_cast<int>(result.eigenvalues_real.size());

            if (!solverOk || !result.success || nEig <= 0) {
                emit resultReady(QString::fromStdString(tc.name),
                                 QString::fromStdString(method.name),
                                 result);
                emit logMessage(QString("  %1  |  FAILED  |  %2 s\n")
                    .arg(QString::fromStdString(method.name), -26)
                    .arg(elapsed, 0, 'f', 3));
                continue;
            }

            const char *status = result.eigensolver_fully_converged
                ? "converged" : "partial";

            emit logMessage(QString("  %1  |  %2 eig  |  %3 s  |  %4\n")
                .arg(QString::fromStdString(method.name), -26)
                .arg(nEig, 4)
                .arg(elapsed, 0, 'f', 3)
                .arg(status));

            // Pre-compute residuals
            {
                int nVecs = static_cast<int>(result.eigenvectors.cols());
                result.residuals_abs.clear();
                result.residuals_rel.clear();
                result.residual_status.clear();
                result.residuals_abs.reserve(nEig);
                result.residuals_rel.reserve(nEig);
                result.residual_status.reserve(nEig);

                for (int k = 0; k < nEig; ++k) {
                    if (k < nVecs) {
                        std::complex<double> lambda(
                            result.eigenvalues_real(k),
                            result.eigenvalues_imag(k));
                        Eigen::VectorXcd xvec = result.eigenvectors.col(k);
                        double xnorm = xvec.norm();
                        if (xnorm > 1e-14) xvec /= xnorm;
                        auto resPair = QEP::computeResiduals(
                            problem.M, problem.C, problem.K, lambda, xvec);
                        result.residuals_abs.push_back(resPair.first);
                        result.residuals_rel.push_back(resPair.second);
                        result.residual_status.push_back(
                            resPair.second < checkTol ? "OK" : "FAIL");
                    } else {
                        result.residuals_abs.push_back(-1.0);
                        result.residuals_rel.push_back(-1.0);
                        result.residual_status.push_back("N/A");
                    }
                }
            }

            emit resultReady(QString::fromStdString(tc.name),
                             QString::fromStdString(method.name),
                             result);

            if (printEigenvals) {
                int show = std::min(nEig, 10);
                emit logMessage(QString("  Eigenvalues (top %1):\n").arg(show));
                for (int k = 0; k < show; ++k) {
                    double re = result.eigenvalues_real(k);
                    double im = result.eigenvalues_imag(k);
                    QString eigStr;
                    if (std::abs(im) < 1e-8)
                        eigStr = QString::number(re, 'f', 6);
                    else
                        eigStr = QString("%1 %2%3i")
                            .arg(re, 0, 'f', 6)
                            .arg(im >= 0 ? '+' : '-')
                            .arg(std::abs(im), 0, 'f', 6);
                    emit logMessage(QString("    [%1]  %2\n")
                        .arg(k + 1, 2).arg(eigStr));
                }
            }

            // Performance stats
            emit logMessage(QString("  Performance:\n")
                + QString("    Arnoldi iterations: %1\n").arg(result.arnoldi_iterations)
                + QString("    Build time: %1 s\n").arg(result.build_time, 0, 'f', 4)
                + QString("    Preprocessing time: %1 s\n").arg(result.preprocessing_time, 0, 'f', 4)
                + QString("    Arnoldi time: %1 s\n").arg(result.arnoldi_time, 0, 'f', 4)
                + QString("    Inner solves: %1\n").arg(result.total_inner_solves)
                + QString("    Inner iterations: %1 (avg %2/solve)\n")
                    .arg(result.total_inner_iterations)
                    .arg(result.avg_inner_iterations, 0, 'f', 1)
                + QString("    Linear solve total time: %1 s\n").arg(result.linear_solve_total_time, 0, 'f', 4));

            // Residual summary
            {
                emit logMessage(QString("  Residuals: max_rel="));
                double maxRel = -1.0;
                for (auto r : result.residuals_rel) {
                    if (r > maxRel) maxRel = r;
                }
                if (maxRel >= 0)
                    emit logMessage(QString::number(maxRel, 'e', 2));
                else
                    emit logMessage("N/A");
                emit logMessage("\n");
            }

            caseConvergedCount++;

            // Log messages
            for (const auto &log : result.log_messages) {
                emit logMessage(QString::fromUtf8(log.c_str(), log.size()));
            }
            for (const auto &warn : result.warning_messages) {
                emit logMessage(QString("  [WARN] %1\n")
                    .arg(QString::fromUtf8(warn.c_str(), warn.size())));
            }
        }

        // Per-case convergence summary
        emit logMessage("\n");
        emit logMessage(QString("  >> %1: %2/%3 solvers converged.\n")
            .arg(QString::fromStdString(tc.name))
            .arg(caseConvergedCount)
            .arg(nSolverMethods));
    }

    emit progressChanged(total, total);
    emit finished();
}
