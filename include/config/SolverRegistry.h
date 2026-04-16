#ifndef QEP_SOLVER_REGISTRY_H
#define QEP_SOLVER_REGISTRY_H

#include "qep/QEP.h"
#include "config/GlobalConfig.h"
#include <vector>
#include <string>
#include <functional>

namespace Config {

struct SolverMethod {
    std::string name;
    std::string principle;
    bool use_sigma;
    std::function<QEP::QuadraticEigenvalueResult(
        const QEP::QuadraticEigenvalueProblem&, int, double)> solver;
};

} // namespace Config

#define QEP_SOLVER_ENTRY(name, principle, use_sigma, solver_type, enabled) \
    if (enabled) { \
        methods.push_back(Config::SolverMethod{ \
            name, \
            principle, \
            use_sigma, \
            [](const QEP::QuadraticEigenvalueProblem& p, int nev, double sigma) -> QEP::QuadraticEigenvalueResult { \
                double actual_sigma = (use_sigma ? sigma : 0.0); \
                auto cfg = Config::getAdaptiveConfig(QEP::LinearSolverType::solver_type, p.condition_number_K); \
                return QEP::solveQEP_Unified(p, nev, actual_sigma, QEP::LinearSolverType::solver_type, cfg); \
            } \
        }); \
    }

//**注册求解器方法 SolverRegistry.h */  
namespace Config {

inline std::vector<SolverMethod> getSolverMethods() {
    std::vector<SolverMethod> methods;
    
    QEP_SOLVER_ENTRY("Pardiso直接法", "inv(A-σB)*B + PardisoLU", true, PardisoLU, ENABLE_PARDISO)
    // QEP_SOLVER_ENTRY("LU直接法", "inv(A-σB)*B + SparseLU", true, SparseLU, ENABLE_SPARSELU)
    // QEP_SOLVER_ENTRY("LLT直接法", "A^{-1}B 算子 + SimplicialLLT(K)", true, SimplicialLLT, ENABLE_SIMPLICIALLLT)
    // QEP_SOLVER_ENTRY("BiCGSTAB迭代法", "inv(A-σB)*B + BiCGSTAB", true, BiCGSTAB, ENABLE_BICGSTAB)
    // QEP_SOLVER_ENTRY("GMRES迭代法", "inv(A-σB)*B + GMRES", true, GMRES, ENABLE_GMRES)
    // QEP_SOLVER_ENTRY("CG迭代法", "inv(A-σB)*B + CG", true, ConjugateGradient, ENABLE_CG)
    
    
    return methods;
}

} // namespace Config

#undef QEP_SOLVER_ENTRY

#endif