# QEP Solver — 二次特征值问题求解器

[![C++17](https://img.shields.io/badge/C++-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-5%2F6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

工业级二次特征值问题求解器，求解 **(λ²M + λC + K)x = 0**。基于位移-逆 Arnoldi 方法（Spectra 库），支持 6 种内层线性求解器，可选 MKL + OpenMP + AVX2 加速。

---

## 三种使用方式

QEP Solver 提供 **GUI / CLI / JSON** 三种交互方式，共享同一核心库和配置文件：

| 方式 | 入口 | 适用场景 |
|------|------|----------|
| **GUI** | `qep_gui.exe` | 交互式参数调节、可视化结果浏览 |
| **CLI** | `qep.exe` | 批量处理、脚本调用、HPC 集群 |
| **JSON** | `config.json` | CI/CD 流程、程序化配置 |

三种方式读写同一份 `config.json`。GUI 中修改的参数 CLI 立即可见，反之亦然。

---

## 快速开始

### 构建

```bash
cmake -B build -DQEP_BUILD_CLI=ON -DQEP_BUILD_GUI=ON
cmake --build build --config Release
```

输出：`bin/qep.exe`（CLI）、`bin/qep_gui.exe`（GUI）、`lib/QEP.lib`（核心库）。

### 构建选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `QEP_USE_MKL` | ON | Intel MKL 加速（PardisoLU 直接法） |
| `QEP_USE_OPENMP` | ON | OpenMP 多线程并行 |
| `QEP_USE_AVX2` | ON | AVX2 指令集优化 |
| `QEP_BUILD_CLI` | ON | 构建 CLI 可执行文件 |
| `QEP_BUILD_GUI` | ON | 构建 GUI 可执行文件（需 Qt5/Qt6） |
| `QEP_BUILD_EXAMPLES` | OFF | 构建示例程序 |
| `QEP_INSTALL` | ON | 生成安装规则 |

### 依赖

| 库 | 版本要求 | 说明 |
|----|----------|------|
| C++17 | — | 编译器需支持 C++17 |
| Eigen3 | ≥3.3 | 稀疏/稠密线性代数（`third_party/eigen/`） |
| Spectra | ≥1.0 | Arnoldi 特征值求解（`third_party/spectra/`） |
| nlohmann/json | ≥3.0 | JSON 解析（`third_party/nlohmann/`） |
| Qt5/Qt6 | ≥5.12 | GUI 框架（仅 GUI 需要） |
| Intel MKL | ≥2020 | PardisoLU 直接法（可选） |
| OpenMP | — | 多线程并行（可选） |

前三项已内置在 `third_party/` 目录中，无需额外安装。

---

## 使用指南

### GUI

```bash
./qep_gui [config_path]
```

界面布局：
- **左侧**：问题树（勾选要求解的问题） + 求解器选择（勾选要使用的求解器）
- **右侧**：参数配置（5 个标签页） + 输出面板（结果表格 + 文本日志）

参数配置标签页：
1. **求解设置** — NEV、位移点 σ、Arnoldi 容差/迭代、残差检验容差
2. **线性求解器** — 内层容差/迭代、ILUT 参数、GMRES 重启步数
3. **性能优化** — OpenMP/MKL 线程数、自适应参数开关
4. **文件转换** — 自动将 .txt 矩阵转换为 .bin 格式
5. **日志诊断** — 控制输出内容：特征值表、残差表、汇总报告、求解器详细日志

操作流程：勾选问题 → 勾选求解器 → 点击"求解选中" → 查看结果表格/文本日志 → 一键导出 TXT/JSON/CSV。

### CLI

```bash
# 列出所有可求解的问题
./qep --list

# 使用默认配置求解
./qep --case test

# 指定求解器和参数
./qep --case test --solver PardisoLU --nev 20 --sigma 1.0 --verbose

# 批量求解某个分组
./qep --group benchmark --verbose

# 安静模式（仅输出汇总报告）
./qep --case test --quiet
```

支持的 CLI 参数：`--case`、`--group`、`--solver`、`--nev`、`--sigma`、`--tol`、`--verbose`/`-v`、`--quiet`/`-q`、`--list`。

### JSON 配置

直接编辑 `config.json`，完整结构参见 `config.schema.json`：

```json
{
  "problem_base_path": "Problems",
  "active_cases": ["test"],
  "solver": {
    "default_nev": 10,
    "default_sigma": 0.0,
    "arnoldi_tolerance": 1e-6,
    "arnoldi_max_iterations": 1000,
    "check_tolerance": 1e-6,
    "inner_tolerance": 1e-8,
    "inner_max_iterations": 1000,
    "ilut_fill_factor": 10,
    "ilut_drop_tol": 1e-4,
    "gmres_restart": 30,
    "enable_adaptive_parameters": false,
    "enabled_solvers": {
      "PardisoLU": true, "SparseLU": true,
      "SimplicialLLT": false, "ConjugateGradient": false,
      "BiCGSTAB": false, "GMRES": false
    }
  },
  "parallel": { "omp_num_threads": 12, "mkl_num_threads": 12 },
  "logging": {
    "print_eigenvalues": true,
    "print_residuals": true,
    "enable_summary_report": true,
    "enable_matrix_property_check": false,
    "enable_condition_estimation": false,
    "print_solver_details": false
  },
  "binary_conversion": { "auto_convert": false, "overwrite": false }
}
```

---

## 项目结构

```
QEP_Solver_GUI/
├── include/                # 公共头文件
│   ├── config/             #   ConfigManager, SolverParams, AppConfig, TableFormatter
│   └── qep/                #   QEP.h — 核心数据结构和求解接口
├── src/
│   ├── qep/                # 核心静态库 (libQEP.a)
│   │   ├── config/         #   GlobalConfig, SolverRegistry (X 宏注册)
│   │   ├── core/           #   UnifiedSolver — Arnoldi 求解器
│   │   ├── io/             #   MatrixIO — 二进制/文本矩阵读写
│   │   └── utils/          #   ConfigManager, CreateProblem, MatrixProperties, ResidualValidation
│   ├── cli/                # CLI 可执行文件 (qep)
│   │   ├── main.cpp        #   入口，参数解析，测试调度
│   │   └── TestHarness     #   问题加载、求解器调度、格式化输出、汇总报告
│   └── gui/                # GUI 可执行文件 (qep_gui)
│       ├── main.cpp        #   入口，ConfigManager 加载
│       ├── MainWindow      #   主窗口，线程管理，信号路由
│       ├── panels/         #   ConfigPanel, SolverPanel, ProblemTreePanel, OutputPanel
│       ├── workers/        #   SolverWorker — QThread 异步求解
│       ├── styles/         #   StyleConstants — 全局 Qt 样式表
│       └── utils/          #   EigenvalueFormatter, QepMetaType
├── Problems/               # 测试问题数据
│   ├── benchmark/          #   基准测试问题
│   ├── engineering/        #   工程案例
│   ├── small_demo/         #   小型演示问题
│   └── user/               #   用户自定义问题
├── Result/                 # 求解结果导出目录（GUI 自动创建时间戳子目录）
├── third_party/            # 第三方依赖（header-only）
│   ├── eigen/              #   Eigen3 线性代数库
│   ├── spectra/            #   Spectra 特征值求解库
│   └── nlohmann/           #   JSON 解析库
├── examples/               # C++ API 示例程序
├── cmake/                  # CMake 打包配置
├── config.json             # 运行时配置（GUI/CLI 共享）
├── config.schema.json      # 配置 JSON Schema
├── ARCHITECTURE.md         # 架构设计文档
├── LICENSE                 # MIT 许可证
└── README.md               # 本文档
```

`build/`、`lib/`、`bin/`、`release/` 为构建产物目录，不纳入版本管理。

---

## 问题数据格式

每个问题是一个目录，包含 M、C、K 三个矩阵文件：

```
Problems/user/test/
├── M.bin          # 质量矩阵（二进制 CSR 格式）
├── C.bin          # 阻尼矩阵
├── K.bin          # 刚度矩阵
└── problem.json   # 元数据（名称、分组、描述）
```

支持 `.bin`（二进制 CSR）和 `.txt`（文本 CSR）两种格式。`binary_conversion.auto_convert` 设为 `true` 可自动转换。

---

## 输出格式

CLI 和 GUI 的文本输出**字节级一致**（共享 `TestHarness` 中的格式化函数）：

```
── Case: test ──
  Problem : n=100  M_nnz=100  C_nnz=298  K_nnz=100
  Source : Problems/user/test
  ▸ Matrix Properties
    cond(M)=1.09  cond(C)=311.14  cond(K)=98.99
  ▸ Solver: Pardiso直接法
    inv(A-σB)*B + PardisoLU, sigma=0.000000
    Time: 0.0124 s
  ▸ Eigenvalues (top 10)
    #            Eigenvalue                      |λ|
  ---------------------------------------------------
    1             0.654587                   0.654587
    2       -0.0184807-0.709491i             0.709731
    ...
  ▸ Residual Validation (10 eigenvalues)
    #           Eigenvalue            Abs. Residual    Rel. Residual      Status
  ---------------------------------------------------------------------------------
    1            0.654587                  1.31e-15         1.28e-15    EXCELLENT
    ...
  ── Summary Report ──
    Case           Method           Status     #Eig    Time(s)   Validation
```

残差精度等级：**EXCELLENT** (< 1e-10) → **OK** (< 1e-8) → **ACCEPTABLE** (< 1e-6) → **WARNING** (≥ 1e-6)。

---

## 架构

GUI 是 CLI 的可视化薄层。详见 [ARCHITECTURE.md](ARCHITECTURE.md)。

## 许可证

MIT
