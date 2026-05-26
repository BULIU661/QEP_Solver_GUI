//==============================================================================
//  src/qep/config/GlobalConfig.h  —  求解器自适应参数配置（header-only）
//==============================================================================

#ifndef QEP_GLOBAL_CONFIG_H
#define QEP_GLOBAL_CONFIG_H

#include <algorithm>
#include "qep/QEP.h"
#include "config/SolverParams.h"

namespace Config {

inline QEP::LinearSolverConfig getSolverConfig(QEP::LinearSolverType type, const SolverParams& p) {
    QEP::LinearSolverConfig c;
    c.type = type;
    c.outer_tolerance     = p.arnoldi_tolerance;
    c.outer_maxIterations = p.arnoldi_max_iterations;
    c.inner_tolerance     = p.inner_tolerance;
    c.inner_maxIterations = p.inner_max_iterations;
    c.fillFactor  = p.ilut_fill_factor;
    c.dropTol     = p.ilut_drop_tol;
    c.restart     = p.gmres_restart;
    c.usePreconditioner = true;
    return c;
}

inline QEP::LinearSolverConfig getAdaptiveConfig(QEP::LinearSolverType type, double cond_K, const SolverParams& p) {
    if (!p.enable_adaptive || cond_K <= 0.0)
        return getSolverConfig(type, p);

    QEP::LinearSolverConfig c;
    c.type = type;
    c.usePreconditioner = true;
    c.outer_maxIterations = p.arnoldi_max_iterations;

    bool isDirect = (type == QEP::LinearSolverType::SparseLU ||
                     type == QEP::LinearSolverType::PardisoLU ||
                     type == QEP::LinearSolverType::SimplicialLLT);

    // Tiered adjustment based on condition number
    if (cond_K < 1e6) {
        c.outer_tolerance   = p.arnoldi_tolerance;
        c.inner_tolerance   = 1e-10;
        c.inner_maxIterations = 500;
        c.fillFactor  = 10;
        c.dropTol     = 1e-4;
        c.restart     = 100;
    } else if (cond_K < 1e8) {
        c.outer_tolerance   = isDirect ? p.arnoldi_tolerance : 1e-4;
        c.inner_tolerance   = 1e-8;
        c.inner_maxIterations = 800;
        c.fillFactor  = std::min(20, p.ilut_fill_factor * 2);
        c.dropTol     = std::max(1e-6, p.ilut_drop_tol / 10.0);
        c.restart     = 120;
    } else if (cond_K < 1e10) {
        c.outer_tolerance   = isDirect ? p.arnoldi_tolerance : 1e-4;
        c.inner_tolerance   = 1e-6;
        c.inner_maxIterations = 1500;
        c.fillFactor  = std::min(30, p.ilut_fill_factor * 3);
        c.dropTol     = std::max(1e-8, p.ilut_drop_tol / 100.0);
        c.restart     = 150;
    } else {
        c.outer_tolerance   = isDirect ? p.arnoldi_tolerance : 1e-4;
        c.inner_tolerance   = 1e-4;
        c.inner_maxIterations = 3000;
        c.fillFactor  = std::min(50, p.ilut_fill_factor * 5);
        c.dropTol     = std::max(1e-10, p.ilut_drop_tol / 1000.0);
        c.restart     = 200;
    }
    return c;
}

} // namespace Config
#endif
