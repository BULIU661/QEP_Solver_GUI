//==============================================================================
//  include/config/SolverParams.h  —  核心求解器参数（纯数值，无 I/O 依赖）
//==============================================================================

#ifndef SOLVER_PARAMS_H
#define SOLVER_PARAMS_H

namespace Config {

struct SolverParams {
    // ---- 外层 Arnoldi ----
    int    arnoldi_max_iterations = 1000;
    double arnoldi_tolerance     = 1e-6;

    // ---- 内层线性求解器 ----
    double inner_tolerance       = 1e-8;
    int    inner_max_iterations  = 1000;
    int    ilut_fill_factor      = 10;
    double ilut_drop_tol         = 1e-4;
    int    gmres_restart         = 30;

    // ---- 自适应参数 ----
    bool   enable_adaptive       = false;

    // ---- 条件数估计 ----
    bool   enable_condition_estimation = false;

    // ---- 并行 ----
    int    omp_threads = 12;
    int    mkl_threads = 12;
};

} // namespace Config

#endif
