//==============================================================================
//  src/qep/config/SolverRegistry.h  —  线性求解器注册与调度
//==============================================================================

#ifndef QEP_SOLVER_REGISTRY_H
#define QEP_SOLVER_REGISTRY_H

#include "qep/QEP.h"
#include "config/GlobalConfig.h"
#include "config/SolverParams.h"
#include "config/ConfigManager.h"
#include <vector>
#include <string>
#include <functional>
#include <set>

namespace Config {

struct SolverMethod {
    std::string name;
    std::string principle;
    bool use_sigma;
    std::function<QEP::QuadraticEigenvalueResult(
        const QEP::QuadraticEigenvalueProblem&, int, double)> solver;
};

// 求解器描述符 —— SolverPanel 与 QEP_SOLVER_ENTRY 宏的唯一数据源
// 约束：jsonKey 与 registeredName 必须与下方 QEP_SOLVER_ENTRY 调用保持一致
struct SolverDescriptor {
    const char* jsonKey;
    const char* displayName;       // GUI 中文显示名
    const char* registeredName;    // SolverMethod::name（与 QEP_SOLVER_ENTRY 宏一致）
    bool        isDirect;
};

constexpr SolverDescriptor kSolverDescriptors[] = {
    {"PardisoLU",          "PardisoLU 直接法",         "Pardiso直接法",     true},
    {"SparseLU",           "SparseLU 直接法",           "LU直接法",          true},
    {"SimplicialLLT",      "SimplicialLLT 直接法",      "LLT直接法",         true},
    {"ConjugateGradient",  "ConjugateGradient 迭代法",  "CG迭代法",         false},
    {"BiCGSTAB",           "BiCGSTAB 迭代法",           "BiCGSTAB迭代法",    false},
    {"GMRES",              "GMRES 迭代法",              "GMRES迭代法",       false},
};

} // namespace Config

#define QEP_SOLVER_ENTRY(name, principle, use_sigma, solver_type, json_key) \
    if (enabledSolvers.count(json_key)) { \
        methods.push_back(Config::SolverMethod{ \
            name, \
            principle, \
            use_sigma, \
            [params](const QEP::QuadraticEigenvalueProblem& p, int nev, double sigma) -> QEP::QuadraticEigenvalueResult { \
                double actual_sigma = (use_sigma ? sigma : 0.0); \
                auto cfg = Config::getAdaptiveConfig(QEP::LinearSolverType::solver_type, p.condition_number_K, params); \
                return QEP::solveQEP_Unified(p, nev, actual_sigma, QEP::LinearSolverType::solver_type, cfg, params); \
            } \
        }); \
    }

namespace Config {

inline std::vector<SolverMethod> getSolverMethods(const SolverParams& params,
                                                    const std::set<std::string>& enabledSolvers) {
    std::vector<SolverMethod> methods;

    QEP_SOLVER_ENTRY("Pardiso直接法", "inv(A-σB)*B + PardisoLU", true, PardisoLU, "PardisoLU")
    QEP_SOLVER_ENTRY("LU直接法", "inv(A-σB)*B + SparseLU", true, SparseLU, "SparseLU")
    QEP_SOLVER_ENTRY("LLT直接法", "A^{-1}B 算子 + SimplicialLLT(K)", true, SimplicialLLT, "SimplicialLLT")
    QEP_SOLVER_ENTRY("BiCGSTAB迭代法", "inv(A-σB)*B + BiCGSTAB", true, BiCGSTAB, "BiCGSTAB")
    QEP_SOLVER_ENTRY("GMRES迭代法", "inv(A-σB)*B + GMRES", true, GMRES, "GMRES")
    QEP_SOLVER_ENTRY("CG迭代法", "inv(A-σB)*B + CG", true, ConjugateGradient, "ConjugateGradient")

    return methods;
}

// Backward-compatible overload for CLI (reads enabled_solvers from ConfigManager)
inline std::vector<SolverMethod> getSolverMethods(const SolverParams& params) {
    std::set<std::string> enabled;
    for (const auto &d : kSolverDescriptors)
        if (ConfigManager::instance().isSolverEnabled(d.jsonKey))
            enabled.insert(d.jsonKey);
    return getSolverMethods(params, enabled);
}

} // namespace Config

#undef QEP_SOLVER_ENTRY

#endif
