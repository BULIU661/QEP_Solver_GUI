//==============================================================================
//  src/cli/TestHarness.h  —  CLI 测试框架接口声明
//==============================================================================

#ifndef TEST_HARNESS_H
#define TEST_HARNESS_H

#include "qep/QEP.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"
#include "config/SolverParams.h"
#include "config/SolverRegistry.h"
#include "config/TableFormatter.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>

QEP::CaseResult runAndReport(const std::string &method_name,
                             const QEP::QuadraticEigenvalueResult &result,
                             const QEP::QuadraticEigenvalueProblem &problem,
                             const Config::AppConfig &appCfg);
QEP::CaseResult runSolver(const Config::SolverMethod &method,
                          const QEP::QuadraticEigenvalueProblem &problem,
                          int nev, double sigma,
                          const Config::AppConfig &appCfg);

std::vector<QEP::CaseResult> processCase(const Config::TestCase &tc,
                                         const Config::AppConfig &appCfg,
                                         const Config::SolverParams &solParams,
                                         const nlohmann::json &cliOverrides = nlohmann::json::object(),
                                         const std::string &solverFilter = "");

void printSummaryReport(const std::vector<Config::TestCase> &cases,
                        const std::vector<std::vector<QEP::CaseResult>> &allResults,
                        double sigma);

int    resolveParamI(const nlohmann::json &overrides, const std::string &key, int defaultVal);
double resolveParamD(const nlohmann::json &overrides, const std::string &key, double defaultVal);

#endif // TEST_HARNESS_H
