// utils/CreatProblem.h
#ifndef CREAT_PROBLEM_H
#define CREAT_PROBLEM_H

#include <string>
#include "qep/QEP.h"

namespace QEP {

QuadraticEigenvalueProblem createTestProblemFromFiles(
    const std::string &name,
    const std::string &M_file,
    const std::string &C_file,
    const std::string &K_file,
    bool is_sparse);

} // namespace QEP

#endif // CREAT_PROBLEM_H
