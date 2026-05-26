//==============================================================================
//  src/gui/workers/SolverWorker.cpp  —  异步求解（调用 CLI 的 TestHarness 函数）
//==============================================================================

#include "SolverWorker.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"
#include "config/SolverParams.h"
#include "cli/TestHarness.h"
#include <QThread>
#include <iostream>
#include <streambuf>
#include <mutex>
#include <functional>
#include <atomic>

// ==================== 流捕获（捕获 stdout → logMessage 信号） ====================
namespace {

class CaptureBuffer : public std::streambuf {
public:
    static std::atomic<bool> s_active;
    using LineCallback = std::function<void(const std::string &)>;
    void setCallback(LineCallback cb) { callback_ = std::move(cb); }
protected:
    int overflow(int c) override {
        if (c == EOF) return c;
        std::lock_guard<std::mutex> lock(mtx_);
        buffer_ += static_cast<char>(c);
        if (c == '\n') { if (callback_) callback_(buffer_); buffer_.clear(); }
        return c;
    }
    int sync() override {
        std::lock_guard<std::mutex> lock(mtx_);
        if (!buffer_.empty()) { if (callback_) callback_(buffer_); buffer_.clear(); }
        return 0;
    }
private:
    std::mutex mtx_;
    std::string buffer_;
    LineCallback callback_;
};

std::atomic<bool> CaptureBuffer::s_active{false};

struct StreamRedirectGuard {
    std::streambuf *oldCout = nullptr, *oldCerr = nullptr;
    StreamRedirectGuard() {
        if (CaptureBuffer::s_active.exchange(true)) return;
        oldCout = std::cout.rdbuf();
        oldCerr = std::cerr.rdbuf();
    }
    ~StreamRedirectGuard() noexcept {
        if (oldCout) std::cout.rdbuf(oldCout);
        if (oldCerr) std::cerr.rdbuf(oldCerr);
        if (oldCout || oldCerr) CaptureBuffer::s_active = false;
    }
};

} // namespace

// ==================== SolverWorker ====================

void SolverWorker::setRequest(const SolveRequest &req) { request_ = req; }

void SolverWorker::run()
{
    auto &cfg = Config::ConfigManager::instance();
    auto appCfg = cfg.toAppConfig();
    auto solParams = cfg.toSolverParams();

    int total = static_cast<int>(request_.cases.size());
    emit progressChanged(0, total);

    // Stream capture — redirects stdout to logMessage (same output path as CLI)
    StreamRedirectGuard guard;
    CaptureBuffer capture;
    capture.setCallback([this](const std::string &line) {
        std::string t = line;
        while (!t.empty() && (t.back() == '\n' || t.back() == '\r')) t.pop_back();
        emit logMessage(QString::fromUtf8(t.c_str(), t.size()));
    });
    if (guard.oldCout) std::cout.rdbuf(&capture);
    if (guard.oldCerr) std::cerr.rdbuf(&capture);

    // Build solver name list
    std::vector<std::string> solverNameList;
    for (const auto &s : request_.solverNames)
        solverNameList.push_back(s.toStdString());

    std::vector<std::vector<QEP::CaseResult>> allCaseResults;
    std::vector<Config::TestCase> savedCases;

    for (int i = 0; i < total; ++i) {
        if (QThread::currentThread()->isInterruptionRequested()) break;

        const auto &tc = request_.cases[i];
        emit progressChanged(i, total);
        emit caseStarted(QString::fromStdString(tc.name));
        emit solverProgress(QString::fromStdString(tc.name), "");

        // ====== 调用 CLI 的核心函数 — 与 CLI 完全相同的执行路径 ======
        try {
            std::vector<std::pair<std::string, QEP::QuadraticEigenvalueResult>> rawResults;
            auto results = processCaseForGUI(tc, appCfg, solParams,
                              solverNameList, request_.nev, request_.sigma,
                              &rawResults);
            for (auto &[name, r] : rawResults)
                emit resultReady(QString::fromStdString(tc.name),
                                 QString::fromStdString(name), r);
            allCaseResults.push_back(results);
            savedCases.push_back(tc);
        } catch (const std::exception &e) {
            emit logMessage(QString("[FATAL] %1\n").arg(e.what()));
        }

        emit progressChanged(i + 1, total);
    }

    // ====== CLI 的汇总报告 ======
    if (appCfg.enable_summary && !allCaseResults.empty())
        printSummaryReport(savedCases, allCaseResults, request_.sigma);

    emit progressChanged(total, total);
    emit finished();
}
