# QEP Solver — 二次特征值问题求解器

[![C++17](https://img.shields.io/badge/C++-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-5%2F6-green)](https://www.qt.io/)

工业级二次特征值问题求解器，求解 **(λ²M + λC + K)x = 0**。基于位移-逆 Arnoldi 方法（Spectra 库），支持 6 种内层线性求解器，根目录Cmakelists编译可选 MKL + OpenMP + AVX2 加速。

---

## 三种使用方式

QEP Solver 提供 **GUI / CLI / JSON** 三种交互方式，共享同一核心库和配置文件：

| 方式  | 入口              |          适用场景          |
|------|------|----------|
| **GUI** | `qep_gui.exe`  | 交互式参数调节、可视化结果浏览 |
| **CLI** | `qep.exe`      | 批量处理、脚本调用、HPC 集群 |
| **JSON** | `config.json` | CI/CD 流程、程序化配置     |

三种方式读写同一份 `config.json`。GUI 中修改的参数 CLI 立即可见，反之亦然。

### JSON 动态实时配置

**核心特性：所有参数通过 `config.json` 文件控制，无需重新编译。**

- 程序启动时自动读取 `config.json`，运行时修改该文件后重新运行即可生效
- GUI 中任何参数改动会**即时写回** `config.json`，无需手动保存
- CLI 执行批量任务时，可在脚本中动态生成不同的 `config.json`，无需修改代码
- 所有参数均有默认值，缺失的字段自动回退，配置文件可以只包含需要修改的字段
- `config.schema.json` 提供完整字段校验和 IDE 自动补全

这意味着参数调节、求解器切换、问题选择等操作完全由数据驱动，二进制可执行文件始终保持不变。

---

## 添加自定义问题

### 目录结构

每个问题是一个独立目录，放在 `Problems/` 下任意层级：

```
Problems/{任意分类}/{问题名称}/
├── M.bin          # 质量矩阵
├── C.bin          # 阻尼矩阵
├── K.bin          # 刚度矩阵
└── problem.json   # 问题元数据（可选，可由求解器自动生成）
```

程序会递归扫描 `Problems/` 目录，自动发现所有包含 M/C/K 文件的子目录。

### 矩阵存储格式

支持两种 CSR（Compressed Sparse Row）格式：

**文本 CSR（.txt）** 

```
每行一个数按下面四种按顺序排序
n                         # 矩阵维度 n（方阵，单个整数）
r0 r1 r2 ... rn           # 行指针 row_ptr（n+1 个整数，空格分隔）
c0 c1 c2 ... c(nnz-1)     # 列索引 col_idx（nnz 个整数，0-based）
v0 v1 v2 ... v(nnz-1)     # 非零元数值 values（nnz 个 double）
```
通过自动转换功能可以将上述格式.txt文本转换为下述可直接用于读取求解的.bin格式
**二进制 CSR（.bin）**
```
[int n: 4字节]                 # 矩阵维度 n
[int nnz: 4字节]               # 非零元总数
[int row_ptr[n+1]: 每项4字节]    # 行偏移数组
[int col_idx[nnz]: 每项4字节]    # 列索引数组（0-based）
[double values[nnz]: 每项8字节]  # 非零元数值数组
```


二进制格式读取速度极快（直接 mmap 到内存，无需文本解析），大文件推荐使用。

### 文本→二进制自动转换

在 `config.json` 中设置：
```json
"binary_conversion": { "auto_convert": true, "overwrite": false }
```

- `auto_convert: true` — 求解前自动将 .txt 转为 .bin，转换后保留原始 .txt 文件
- `overwrite: true` — 覆盖已存在的 .bin 文件；设为 `false` 可跳过已转换的问题

**为什么需要转换：**
- 二进制格式读取速度比文本解析快 10-100 倍（不需要逐行解析和字符串转换）
- 大规模矩阵（百万级非零元）下文本文件可能高达数 GB，解析耗时巨大
- 二进制文件可直接内存映射（memory-mapped I/O），操作系统级零拷贝读取
- 一次转换后可重复求解，无需反复解析
- 本求解器默认只能求解按照上述.txt格式通过自动转换功能得到的.bin矩阵存储文件

### problem.json 字段说明

`problem.json` 中的字段分为两类：

**功能性字段（被求解器读取并影响行为）：**
求解器不能自动生成的数据，需要手动设置具有高优先级覆盖config.json的功能
| 字段 | 类型 | 说明 |
|------|------|------|
| `name` | string | 问题名称（在 GUI 问题树和 CLI 中显示，作为唯一标识） |
| `group` | string | 问题分组（用于 GUI 分类显示和 CLI `--group` 筛选） |
| `is_sparse` | bool | 矩阵是否为稀疏格式（`true`=CSR, `false`=稠密列优先） |
| `overrides` | object | 为该问题单独覆盖求解参数（如 `"nev": 20, "sigma": 1.0`），优先级高于 `config.json` 全局设置 |

**元数据字段（求解器自动写入，仅供参考）：**
求解器求解后自动更新生成的数据
| 字段 | 说明 |
|------|------|
| `description` | 问题文字描述（在 GUI 和 CLI 中展示） | （也需要手动设置）
| `dimension` | 矩阵维度 n（求解器运行后自动写入） |
| `M_nnz`, `C_nnz`, `K_nnz` | 各矩阵非零元数量 |
| `M_properties`, `C_properties`, `K_properties` | 矩阵属性（对称性、定性、存储格式） |
| `condition_numbers` | 条件数估计（M、C、K 分别估算） |

`problem.json` 是可选的 —— 如果不提供，程序会自动从目录名推断名称和分组。但建议提供，以获得更好的显示效果和分组组织。

---

## 结果导出

GUI 提供一键导出功能，点击结果表格上方的"导出结果"按钮，自动在 `Result/` 目录下创建时间戳子文件夹，同时生成三种格式文件：

| 文件 | 格式 | 内容 |
|------|------|------|
| `results.txt` | 整齐文本表格 | 编号、特征值、\|λ\|、绝对残差、相对残差、状态等级 |
| `results.json` | JSON | 结构化数据，方便程序化处理 |
| `results.csv` | CSV | 逗号分隔，可直接导入 Excel、MATLAB、Python pandas |

```bash
# CLI 使用重定向同样可以导出
./qep --case test > output.txt
```

## 快速开始

### 构建

CLI 和 GUI 默认均为打开状态，直接配置即可：

```bash
cmake -B build
cmake --build build --config Release
```

仅在需要禁用某项时显式关闭：

```bash
cmake -B build -DQEP_BUILD_GUI=OFF    # 仅构建 CLI
cmake -B build -DQEP_BUILD_CLI=OFF    # 仅构建 GUI
cmake -B build -DQEP_USE_MKL=OFF      # 不使用 MKL
```

Windows 下若未检测到 Qt，需指定 Qt 路径：

```bash
cmake -B build -DCMAKE_PREFIX_PATH=C:/Qt/5.15.2/msvc2019_64
```

构建产物：
- `bin/qep` / `bin/qep.exe` — CLI 可执行文件
- `bin/qep_gui` / `bin/qep_gui.exe` — GUI 可执行文件
- `lib/libQEP.a` / `lib/QEP.lib` — 核心静态库

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

启动 `bin/qep_gui.exe`，自动扫描并加载所有问题。

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

在命令行进入包含./qep.exe的bin文件下，可以执行以下命令行操作
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
