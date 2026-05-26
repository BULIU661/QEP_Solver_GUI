//==============================================================================
//  src/gui/MainWindow.cpp  —  主窗口（直接基于 ConfigManager + SolverWorker）
//==============================================================================
#include "MainWindow.h"
#include "panels/ConfigPanel.h"
#include "panels/ProblemTreePanel.h"
#include "panels/SolverPanel.h"
#include "panels/OutputPanel.h"
#include "workers/SolverWorker.h"
#include "config/ConfigManager.h"
#include "config/AppConfig.h"
#include "styles/StyleConstants.h"
#include <QSplitter>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStatusBar>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QCloseEvent>
#include <QMessageBox>
#include <QThread>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    createCentralWidget();
    createStatusBar();
    setWindowTitle("QEP Solver");
}

MainWindow::~MainWindow() = default;

void MainWindow::closeEvent(QCloseEvent *event) {
    if (solverThread_ && solverThread_->isRunning()) {
        QMessageBox::warning(this, tr("求解中"), tr("请等待求解完成后再关闭窗口"));
        event->ignore(); return;
    }
    Config::ConfigManager::instance().save();
    event->accept();
}

// ==================== 求解（直接使用 CLI 的 SolverWorker + ConfigManager） ====================
void MainWindow::onSolveClicked()
{
    if (solverThread_ && solverThread_->isRunning()) {
        solverThread_->requestInterruption();
        return;
    }

    auto &cfg = Config::ConfigManager::instance();
    auto checkedNames = problemTreePanel_->checkedProblems();
    auto allCases = cfg.testCases();
    std::vector<Config::TestCase> checked;
    for (const auto &tc : allCases)
        if (checkedNames.contains(QString::fromStdString(tc.name)))
            checked.push_back(tc);
    if (checked.empty()) {
        QMessageBox::information(this, tr("提示"), tr("请先勾选至少一个测试问题")); return;
    }

    auto solverNames = solverPanel_->checkedSolvers();
    if (solverNames.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先勾选至少一个求解器")); return;
    }

    cfg.save();

    auto *worker = new SolverWorker();
    SolverWorker::SolveRequest req;
    req.cases = checked;
    req.solverNames = solverNames;
    req.nev = cfg.defaultNEV();
    req.sigma = cfg.defaultSigma();
    worker->setRequest(req);

    solverThread_ = new QThread(this);
    worker->moveToThread(solverThread_);

    connect(solverThread_, &QThread::started, worker, &SolverWorker::run);
    connect(worker, &SolverWorker::finished, this, &MainWindow::onSolverFinished);
    connect(worker, &SolverWorker::finished, solverThread_, &QThread::quit);
    connect(worker, &SolverWorker::finished, worker, &QObject::deleteLater);
    connect(solverThread_, &QThread::finished, solverThread_, &QObject::deleteLater);

    double sigma = cfg.defaultSigma();
    connect(worker, &SolverWorker::resultReady, this,
            [this, sigma](const QString &cn, const QString &sn,
                          const QEP::QuadraticEigenvalueResult &r) {
                outputPanel_->addResult(cn, sn,
                    std::make_shared<const QEP::QuadraticEigenvalueResult>(r), sigma);
            });
    connect(worker, &SolverWorker::logMessage, this, &MainWindow::appendLog);
    connect(worker, &SolverWorker::progressChanged, this,
            [this](int c, int t) { progressBar_->setMaximum(t); progressBar_->setValue(c); });
    connect(worker, &SolverWorker::solverProgress, this,
            [this](const QString &cn, const QString &sn) {
                statusLabel_->setText(tr("求解中: %1 — %2").arg(cn, sn));
            });

    solveBtn_->setText(tr("取消"));
    solveBtn_->setStyleSheet(style::kSolveBtnCancel);
    progressBar_->setVisible(true);
    statusLabel_->setText(tr("求解中..."));
    outputPanel_->clearLog();
    tabWidget_->setCurrentWidget(outputPanel_);
    solverThread_->start();
}

void MainWindow::onSolverFinished()
{
    solveBtn_->setText(tr("求解选中"));
    solveBtn_->setStyleSheet(style::kSolveBtnNormal);
    progressBar_->setVisible(false);
    statusLabel_->setText(tr("求解完成"));
    solverThread_ = nullptr;
}

void MainWindow::appendLog(const QString &msg) { outputPanel_->appendLog(msg); }

// ==================== UI ====================
void MainWindow::createStatusBar() {
    progressBar_ = new QProgressBar(); progressBar_->setMaximumWidth(200); progressBar_->setVisible(false);
    statusLabel_ = new QLabel(tr("就绪")); statusLabel_->setMinimumWidth(300);
    statusBar()->addWidget(statusLabel_, 1); statusBar()->addPermanentWidget(progressBar_);
}

void MainWindow::createCentralWidget() {
    auto *c = new QWidget(this); auto *hl = new QHBoxLayout(c);
    auto *left = new QSplitter(Qt::Vertical);
    problemTreePanel_ = new ProblemTreePanel();
    left->addWidget(problemTreePanel_);
    auto *sw = new QWidget(); auto *sl = new QVBoxLayout(sw); sl->setContentsMargins(0,0,0,0);
    solverPanel_ = new SolverPanel(); sl->addWidget(solverPanel_);
    solveBtn_ = new QPushButton(tr("求解选中")); solveBtn_->setMinimumHeight(38);
    solveBtn_->setStyleSheet(style::kSolveBtnNormal);
    connect(solveBtn_, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    sl->addWidget(solveBtn_); left->addWidget(sw);
    tabWidget_ = new QTabWidget();
    configPanel_ = new ConfigPanel();
    outputPanel_ = new OutputPanel();
    tabWidget_->addTab(configPanel_, tr("参数配置"));
    tabWidget_->addTab(outputPanel_, tr("输出"));
    hl->addWidget(left, 1); hl->addWidget(tabWidget_, 2);
    setCentralWidget(c);

    problemTreePanel_->scanProblems(); // initial scan
}
