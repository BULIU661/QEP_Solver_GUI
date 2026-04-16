// TestHarness.h
#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include "qep/QEP.h"
#include "config/TestCases.h"
#include "config/SolverRegistry.h"
#include <vector>
#include <string>

QEP::CaseResult runAndReport(const std::string &method_name,
                             const QEP::QuadraticEigenvalueResult &result,
                             const QEP::QuadraticEigenvalueProblem &problem);
QEP::CaseResult runSolver(const Config::SolverMethod &method,
                          const QEP::QuadraticEigenvalueProblem &problem,
                          int nev, double sigma);

std::vector<QEP::CaseResult> processCase(const Config::TestCase &tc, int nev, double sigma,
                                         bool needCondition, bool needCheckMatrixProperty);

void printSummaryReport(const std::vector<Config::TestCase> &cases,
                        const std::vector<std::vector<QEP::CaseResult>> &allResults,
                        double sigma);

#endif // TEST_HARNESS_H
