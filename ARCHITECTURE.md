# QEP Solver 架构设计文档

## 概述

QEP (Quadratic Eigenvalue Problem) Solver 是一个二次特征值问题求解器，提供 CLI 命令行界面和 GUI 图形界面两种交互方式。两种界面共享完全相同的底层核心库和配置系统。

## 分层架构

```
┌──────────────────────────────────────────────┐
│  GUI 层 (src/gui/)                            │
│  main.cpp → MainWindow                        │
│  ├─ ConfigPanel    (参数配置，直接读写 JSON)    │
│  ├─ SolverPanel    (求解器选择)                │
│  ├─ ProblemTreePanel (问题浏览)                │
│  ├─ OutputPanel    (日志+表格+导出)            │
│  └─ SolverWorker   (QThread 异步求解)          │
├──────────────────────────────────────────────┤
│  CLI 层 (src/cli/)                            │
│  main.cpp → TestHarness                       │
│  ├─ processCase()       案例处理              │
│  ├─ runSolver()          求解器调度            │
│  ├─ runAndReport()       结果格式化            │
│  └─ printSummaryReport() 汇总报告             │
├──────────────────────────────────────────────┤
│  核心库 (src/qep/ → libQEP.a)                 │
│  ├─ UnifiedSolver.cpp     Arnoldi 求解        │
│  ├─ MatrixIO.cpp           矩阵读写           │
│  ├─ ConfigManager.cpp      配置管理(单例)      │
│  ├─ CreateProblem.cpp      问题构建           │
│  ├─ MatrixProperties.cpp   矩阵属性分析       │
│  └─ ResidualValidation.cpp 残差验证           │
├──────────────────────────────────────────────┤
│  工具层 (include/)                            │
│  ├─ TableFormatter.h     表格格式化           │
│  ├─ SolverRegistry.h      求解器注册(X宏)      │
│  ├─ SolverParams.h        求解器参数           │
│  └─ AppConfig.h           应用配置结构体       │
└──────────────────────────────────────────────┘
```

## 关键设计决策

### 1. GUI 作为 CLI 的可视化层

GUI 不拥有任何核心逻辑。所有求解、格式化、问题发现均调用 CLI 已有的函数：

- 参数读写：`Config::ConfigManager::instance().data()` — 与 CLI 共享同一个单例
- 问题发现：`Config::ConfigManager::testCases()` — 与 CLI 完全相同
- 求解执行：`SolverWorker::run()` → `processCaseForGUI()` — TestHarness.cpp 中共享
- 输出格式化：`fmt::TableBuilder`, `fmt::subHdr`, `QEP::formatResidualTable` — 与 CLI 完全相同

### 2. 配置系统

`ConfigManager` 是全局单例，持有 `config.json` 的完整 JSON 树。

- GUI 的 ConfigPanel 直接读写 `ConfigManager::instance().data()`，即时保存到磁盘
- CLI 和 GUI 共享同一份 `config.json`，配置修改即时同步
- 无中间抽象层，避免数据不一致

### 3. 求解流程

```
用户点击"求解"
  → MainWindow::onSolveClicked()
    → 校验问题/求解器选择
    → 构建 SolveRequest (TestCase + solverNames + nev + sigma)
    → 创建 SolverWorker，moveToThread
    → connect 信号 (resultReady, logMessage, progressChanged)
    → QThread::start()

SolverWorker::run() (后台线程)
  → CaptureBuffer 捕获 stdout
  → processCaseForGUI()  ← 与 CLI 相同的函数
    → QEP::createTestProblemFromFiles()  加载矩阵
    → Config::getSolverMethods()         获取已启用求解器
    → method.solver() → QEP::solveQEP_Unified()
    → runAndReport()                    表格格式化
    → computeResiduals()                残差预计算
  → printSummaryReport()               汇总表格
  → emit resultReady()                  信号回主线程

MainWindow (主线程)
  → outputPanel_->addResult()           表格行插入
  → outputPanel_->appendLog()           文本日志
  → progressBar 更新
```

### 4. 线程模型

| 线程 | 职责 | 数据访问 |
|------|------|----------|
| 主线程 | UI 渲染、用户交互、ConfigPanel 编辑 | ConfigManager (写) |
| 工作线程 | 矩阵加载、求解器执行、结果格式化 | ConfigManager (读，求解开始前快照) |
| Qt 信号 | resultReady → 主线程更新表格 (QueuedConnection) | 自动序列化 |

### 5. 输出一致性保证

GUI 的文本日志输出与 CLI 的控制台输出**字节级一致**，因为：
- 同一函数：`processCaseForGUI` 和 `runAndReport` 写入 `std::cout`
- 同一捕获：`CaptureBuffer` 重定向 `std::cout` → `logMessage` 信号
- 同一格式：`fmt::TableBuilder`, `fmt::subHdr`, `formatResidualTable`

## 文件组织

```
QEP_Solver_GUI/
├── include/
│   ├── config/          # 公共配置头文件
│   │   ├── ConfigManager.h
│   │   ├── SolverParams.h
│   │   ├── AppConfig.h
│   │   └── TableFormatter.h
│   └── qep/
│       └── QEP.h        # 核心数据结构 + 求解接口
├── src/
│   ├── qep/             # 核心库
│   │   ├── config/
│   │   │   ├── GlobalConfig.h
│   │   │   └── SolverRegistry.h
│   │   ├── core/
│   │   │   └── UnifiedSolver.cpp
│   │   ├── io/
│   │   │   └── MatrixIO.cpp/h
│   │   └── utils/
│   │       ├── ConfigManager.cpp
│   │       ├── CreateProblem.cpp/h
│   │       ├── MatrixProperties.cpp/h
│   │       └── ResidualValidation.cpp/h
│   ├── cli/             # CLI 可执行文件
│   │   ├── main.cpp
│   │   └── TestHarness.cpp/h
│   └── gui/             # GUI 可执行文件
│       ├── main.cpp
│       ├── MainWindow.cpp/h
│       ├── panels/
│       │   ├── ConfigPanel.cpp/h
│       │   ├── SolverPanel.cpp/h
│       │   ├── ProblemTreePanel.cpp/h
│       │   └── OutputPanel.cpp/h
│       ├── workers/
│       │   └── SolverWorker.cpp/h
│       ├── styles/
│       │   └── StyleConstants.h
│       └── utils/
│           ├── EigenvalueFormatter.h
│           ├── QepMetaType.h
├── config.json           # 运行时配置 (CLI/GUI 共享)
├── config.schema.json    # 配置 JSON Schema
├── Problems/             # 测试问题数据
├── Result/               # 导出结果目录
├── ARCHITECTURE.md       # 本文档
└── README.md             # 项目说明
```
