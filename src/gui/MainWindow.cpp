//==============================================================================
//  src/gui/MainWindow.cpp  —  主窗口布局与菜单管理
//==============================================================================

#include "MainWindow.h"
#include "panels/ConfigPanel.h"
#include "panels/ProblemTreePanel.h"
#include "panels/SolverPanel.h"
#include "panels/OutputPanel.h"
#include "workers/SolverWorker.h"
#include "config/ConfigManager.h"
#include "config/SolverRegistry.h"
#include "config/GlobalConfig.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
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
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("QEP Solver");
    createMenuBar();
    createCentralWidget();
    createStatusBar();
    loadConfigToUI();
}

MainWindow::~MainWindow()
{
    if (solverThread_ && solverThread_->isRunning()) {
        solverThread_->requestInterruption();
        solverThread_->quit();
        solverThread_->wait(5000);
    }
}

void MainWindow::createMenuBar()
{
    auto *fileMenu = menuBar()->addMenu(tr("文件(&F)"));
    auto *loadAction = fileMenu->addAction(tr("加载配置(&L)..."), this, &MainWindow::onLoadConfig);
    loadAction->setShortcut(QKeySequence("Ctrl+L"));
    auto *saveAction = fileMenu->addAction(tr("保存配置(&S)"), this, &MainWindow::onSaveConfig);
    saveAction->setShortcut(QKeySequence("Ctrl+S"));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("退出(&X)"), this, &QWidget::close, QKeySequence("Ctrl+Q"));
}

void MainWindow::createCentralWidget()
{
    auto *central = new QWidget(this);
    setCentralWidget(central);
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(6, 6, 6, 6);
    mainLayout->setSpacing(6);

    // ===== Left sidebar =====
    auto *sidebar = new QWidget();
    sidebar->setMinimumWidth(200);
    sidebar->setStyleSheet("background-color: #ffffff; border-radius: 4px;");
    auto *sidebarLayout = new QVBoxLayout(sidebar);
    sidebarLayout->setContentsMargins(4, 4, 4, 4);
    sidebarLayout->setSpacing(4);

    problemTreePanel_ = new ProblemTreePanel();
    solverPanel_      = new SolverPanel();

    // Solver + button wrapper widget (for splitter)
    auto *solverWrap = new QWidget();
    auto *wrapLayout = new QVBoxLayout(solverWrap);
    wrapLayout->setContentsMargins(0, 0, 0, 0);
    wrapLayout->setSpacing(4);
    auto *sep1 = new QFrame();
    sep1->setFrameShape(QFrame::HLine);
    sep1->setStyleSheet("color: #e0e0e0;");
    wrapLayout->addWidget(sep1);
    wrapLayout->addWidget(solverPanel_, 1);
    auto *sep2 = new QFrame();
    sep2->setFrameShape(QFrame::HLine);
    sep2->setStyleSheet("color: #e0e0e0;");
    wrapLayout->addWidget(sep2);

    solveBtn_ = new QPushButton(tr("求解选中"));
    solveBtn_->setMinimumHeight(38);
    solveBtn_->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none;"
        " border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #219a52; }"
        "QPushButton:pressed { background-color: #1e8449; }"
        "QPushButton:disabled { background-color: #bdc3c7; }");
    wrapLayout->addWidget(solveBtn_, 0);

    // Vertical splitter: problem tree | solver
    auto *leftSplitter = new QSplitter(Qt::Vertical);
    leftSplitter->addWidget(problemTreePanel_);
    leftSplitter->addWidget(solverWrap);
    leftSplitter->setStretchFactor(0, 3);
    leftSplitter->setStretchFactor(1, 1);
    leftSplitter->setHandleWidth(4);
    leftSplitter->setChildrenCollapsible(false);
    leftSplitter->setSizes({400, 200});
    sidebarLayout->addWidget(leftSplitter, 1);

    // ===== Right area: QTabWidget =====
    tabWidget_ = new QTabWidget();
    tabWidget_->setDocumentMode(true);

    outputPanel_ = new OutputPanel();
    configPanel_ = new ConfigPanel();

    tabWidget_->addTab(configPanel_, tr("参数配置"));
    tabWidget_->addTab(outputPanel_, tr("求解输出"));

    // ===== Splitter =====
    auto *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(sidebar);
    splitter->addWidget(tabWidget_);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 3);
    splitter->setHandleWidth(5);
    splitter->setChildrenCollapsible(false);
    splitter->setSizes({380, 900});
    mainLayout->addWidget(splitter);

    // Signals
    connect(solveBtn_, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    connect(problemTreePanel_, &ProblemTreePanel::statusMessage, this, [this](const QString &msg) {
        statusLabel_->setText(msg);
    });
    connect(configPanel_, &ConfigPanel::configChanged, this, [this]() {
        Config::ConfigManager::instance().save();
    });
}

void MainWindow::createStatusBar()
{
    statusLabel_ = new QLabel(tr("就绪"));
    progressBar_ = new QProgressBar();
    progressBar_->setMaximumWidth(200);
    progressBar_->setVisible(false);
    statusBar()->addWidget(statusLabel_, 1);
    statusBar()->addPermanentWidget(progressBar_);
}

void MainWindow::loadConfigToUI()
{
    configPanel_->setFromConfig();
    problemTreePanel_->scanProblems();
    solverPanel_->setFromConfig();

    auto path = QString::fromStdString(Config::ConfigManager::instance().configFilePath());
    setWindowTitle(QString("QEP Solver — %1").arg(
        path.isEmpty() ? QString("config.json") : path));
}

void MainWindow::onLoadConfig()
{
    QString path = QFileDialog::getOpenFileName(
        this, tr("选择配置文件"), "config.json",
        tr("JSON 文件 (*.json);;所有文件 (*)"));
    if (path.isEmpty()) return;
    Config::ConfigManager::load(path.toStdString());
    loadConfigToUI();
    statusLabel_->setText(tr("配置已加载"));
}

void MainWindow::onSaveConfig()
{
    problemTreePanel_->syncToConfig();
    Config::ConfigManager::instance().save();
    statusLabel_->setText(tr("配置已保存"));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (solverThread_ && solverThread_->isRunning()) {
        QMessageBox::warning(this, tr("求解中"), tr("请等待求解完成后再关闭窗口"));
        event->ignore();
        return;
    }
    event->accept();
}

void MainWindow::onSolveClicked()
{
    problemTreePanel_->syncToConfig();
    Config::ConfigManager::instance().save();

    auto checkedNames = problemTreePanel_->checkedProblems();
    if (checkedNames.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先勾选至少一个测试问题"));
        return;
    }

    QStringList solverNames = solverPanel_->checkedSolvers();
    if (solverNames.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("请先勾选至少一个求解器"));
        return;
    }

    // Build Config::TestCase list from config.json
    auto &cfg = Config::ConfigManager::instance();
    auto allCases = cfg.testCases();
    std::vector<Config::TestCase> selectedCases;
    for (const auto &tc : allCases) {
        if (checkedNames.contains(QString::fromStdString(tc.name)))
            selectedCases.push_back(tc);
    }

    auto *worker = new SolverWorker();
    SolverWorker::SolveRequest req;
    req.cases = selectedCases;
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
    connect(worker, &SolverWorker::resultReady, outputPanel_,
            [this](const QString &caseName, const QString &solverName,
                   const QEP::QuadraticEigenvalueResult &result) {
                outputPanel_->addResult(caseName, solverName, result,
                    Config::ConfigManager::instance().defaultSigma());
            });
    connect(worker, &SolverWorker::logMessage, this, &MainWindow::appendLog);

    connect(worker, &SolverWorker::progressChanged, this,
            [this](int cur, int total) {
                progressBar_->setMaximum(total);
                progressBar_->setValue(cur);
            });
    connect(worker, &SolverWorker::solverProgress, this,
            [this](const QString &caseName, const QString &solverName) {
                statusLabel_->setText(tr("求解中: %1 — %2").arg(caseName, solverName));
            });

    solveBtn_->setText(tr("取消"));
    solveBtn_->setStyleSheet(
        "QPushButton { background-color: #e74c3c; color: white; border: none;"
        " border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #c0392b; }");
    disconnect(solveBtn_, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    connect(solveBtn_, &QPushButton::clicked, this, [this]() {
        if (solverThread_ && solverThread_->isRunning()) {
            solverThread_->requestInterruption();
            statusLabel_->setText(tr("正在取消..."));
        }
    });
    progressBar_->setVisible(true);
    statusLabel_->setText(tr("求解中..."));
    outputPanel_->clearLog();
    tabWidget_->setCurrentWidget(outputPanel_);

    solverThread_->start();
}

void MainWindow::onSolverFinished()
{
    solveBtn_->setText(tr("求解选中"));
    solveBtn_->setStyleSheet(
        "QPushButton { background-color: #27ae60; color: white; border: none;"
        " border-radius: 4px; font-size: 13px; font-weight: bold; }"
        "QPushButton:hover { background-color: #219a52; }"
        "QPushButton:disabled { background-color: #bdc3c7; }");
    disconnect(solveBtn_, &QPushButton::clicked, nullptr, nullptr);
    connect(solveBtn_, &QPushButton::clicked, this, &MainWindow::onSolveClicked);
    solveBtn_->setEnabled(true);
    progressBar_->setVisible(false);
    statusLabel_->setText(tr("求解完成"));
    solverThread_ = nullptr;
}

void MainWindow::appendLog(const QString &msg)
{
    outputPanel_->appendLog(msg);
}
