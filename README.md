# QEP Solver — 二次特征值问题求解器（CLI 版本）

基于 **Eigen** 和 **Spectra** 的稀疏矩阵二次特征值问题求解器。核心算法为**位移求逆 Arnoldi**，内层线性求解器支持 PardisoLU / SparseLU / SimplicialLLT / CG / BiCGSTAB / GMRES 共 6 种。整个求解过程由 `config.json` 驱动，无需重新编译即可切换问题、调整参数、选择求解器。

## 数学问题

```
(λ²M + λC + K) x = 0
```

M, C, K 为稀疏矩阵，λ 为特征值，x 为特征向量。广泛用于结构动力学、声学、转子动力学。

## 目录结构

```
QEP/
├── include/
│   ├── qep/QEP.h                 公共 API（数据结构、求解接口）
│   └── config/
│       ├── ConfigManager.h        运行时配置管理器
│       ├── SolverParams.h         求解器参数结构
│       ├── AppConfig.h            CLI/GUI 应用配置
│       └── TableFormatter.h       表格格式化引擎
├── src/
│   ├── qep/                      核心求解器库（纯 C++，无 Qt 依赖）
│   │   ├── core/UnifiedSolver.cpp 位移求逆 Arnoldi 求解器
│   │   ├── config/                内部配置（GlobalConfig, SolverRegistry）
│   │   ├── io/MatrixIO.cpp        CSR/COO/二进制矩阵读写
│   │   └── utils/                 残差验证、矩阵性质、问题构造
│   └── cli/                      CLI 命令行界面
│       ├── main.cpp              入口、参数解析、用例筛选
│       └── TestHarness.cpp       问题加载、求解调度、格式化输出
├── config.json                   运行时配置文件
├── Problems/                     测试问题数据（.bin / .txt）
└── third_party/                  Eigen + Spectra + nlohmann/json
```

## 快速开始

### 构建

```bash
mkdir build && cd build
cmake .. -DQEP_USE_MKL=OFF    # 无 Intel MKL 环境
cmake ..                       # 自动检测 MKL（推荐）
cmake --build . -j8
```

输出：`bin/qep`

### 运行第一个例子

确保 `config.json` 中 `active_cases` 包含你想跑的问题：

```json
{ "active_cases": ["sym_pos_def"] }
```

```bash
./bin/qep
```

---

## CLI 命令参考

```
Usage: qep [options]

Options:
  --config <path>   指定配置文件（默认自动搜索 config.json）
  --case <pattern>  筛选用例名称（支持 * 通配符和逗号分隔）
  --group <name>    按分组筛选
  --solver <name>   筛选求解器（子串匹配）
  --nev <n>         覆盖特征值个数
  --sigma <value>   覆盖位移点
  --tol <value>     覆盖残差检验容差
  --verbose, -v     详细输出（求解器内部统计 + 文件读取诊断）
  --quiet, -q       静默模式（仅输出汇总报告）
  --list            列出所有可用用例后退出
  --help            打印此帮助
```

### 常用示例

```bash
./bin/qep                                    # 运行 active_cases 中的全部用例
./bin/qep --list                             # 查看有哪些可用用例
./bin/qep --case sym_pos_def                 # 只跑一个用例
./bin/qep --group engineering                # 跑某个分组的所有用例
./bin/qep --case "turbine*" --solver Pardiso # 通配符 + 求解器筛选
./bin/qep --nev 5 --sigma -0.01 --tol 1e-8   # 覆盖求解参数
./bin/qep -v                                 # 详细输出模式
./bin/qep -q                                 # 仅输出汇总报告
```

---

## config.json 参考

### 完整字段

```json
{
    "active_cases": [],
    "problem_base_path": "Problems",

    "solver": {
        "default_nev": 10,
        "default_sigma": 0.0,
        "arnoldi_tolerance": 1e-6,
        "arnoldi_max_iterations": 1000,
        "check_tolerance": 1e-6,
        "enable_adaptive_parameters": false,

        "enabled_solvers": {
            "PardisoLU": true,
            "SparseLU": false,
            "SimplicialLLT": false,
            "ConjugateGradient": false,
            "BiCGSTAB": false,
            "GMRES": false
        },

        "inner_tolerance": 1e-8,
        "inner_max_iterations": 1000,
        "ilut_fill_factor": 10,
        "ilut_drop_tol": 1e-4,
        "gmres_restart": 30
    },

    "logging": {
        "print_eigenvalues": true,
        "print_residuals": true,
        "enable_summary_report": true,
        "enable_condition_estimation": false,
        "enable_matrix_property_check": false
    },

    "parallel": {
        "omp_num_threads": 12,
        "mkl_num_threads": 12
    },

    "binary_conversion": {
        "auto_convert": false,
        "overwrite": false
    },

    "test_cases": [
        {
            "name": "sym_pos_def",
            "M_file": "small_demo/sym_pos_def/M.bin",
            "C_file": "small_demo/sym_pos_def/C.bin",
            "K_file": "small_demo/sym_pos_def/K.bin",
            "group": "small_demo",
            "is_sparse": true,
            "overrides": {
                "nev": 20,
                "sigma": -0.001
            }
        }
    ]
}
```

### 字段说明

**solver — 求解器参数**

| 字段 | 默认值 | 说明 |
|---|---|---|
| `default_nev` | 10 | 需求解的特征值个数 |
| `default_sigma` | 0.0 | 位移求逆的位移点 σ |
| `arnoldi_tolerance` | 1e-6 | 外层 Arnoldi 迭代收敛容差 |
| `arnoldi_max_iterations` | 1000 | Arnoldi 最大迭代次数 |
| `check_tolerance` | 1e-6 | 残差检验 PASS/FAIL 阈值 |
| `enable_adaptive_parameters` | false | 根据 K 矩阵条件数自适应调整内层容差 |

**enabled_solvers — 求解器开关**

6 个求解器均可独立启停。所有求解器始终编译在程序中，`false` 表示本次运行不使用。

| 求解器 | 类型 | 说明 |
|---|---|---|
| `PardisoLU` | 直接法 | Intel MKL 专有，速度最快（需 MKL） |
| `SparseLU` | 直接法 | Eigen 自带，无外部依赖 |
| `SimplicialLLT` | 直接法 | 仅适用于 SPD 矩阵 |
| `ConjugateGradient` | 迭代法 | 适用于 SPD 矩阵 |
| `BiCGSTAB` | 迭代法 | 适用于非对称矩阵 |
| `GMRES` | 迭代法 | 适用于强非对称矩阵 |

**内层求解器参数**（用于迭代法）

| 字段 | 默认值 | 说明 |
|---|---|---|
| `inner_tolerance` | 1e-8 | 内层线性求解器收敛容差 |
| `inner_max_iterations` | 1000 | 内层最大迭代次数 |
| `ilut_fill_factor` | 10 | ILUT 预条件子填充因子 |
| `ilut_drop_tol` | 1e-4 | ILUT 丢弃容差 |
| `gmres_restart` | 30 | GMRES 重启步数 |

**logging — 输出控制**

| 字段 | 默认值 | 说明 |
|---|---|---|
| `print_eigenvalues` | true | 打印特征值表 |
| `print_residuals` | true | 打印残差验证表 |
| `enable_summary_report` | true | 打印汇总报告 |
| `enable_condition_estimation` | false | 估计矩阵条件数（计算开销较大） |
| `enable_matrix_property_check` | false | 打印矩阵对称性/正定性分析 |

**parallel — 并行设置**

| 字段 | 默认值 | 说明 |
|---|---|---|
| `omp_num_threads` | 12 | OpenMP 线程数 |
| `mkl_num_threads` | 12 | Intel MKL 线程数 |

**binary_conversion — 文件格式转换**

| 字段 | 默认值 | 说明 |
|---|---|---|
| `auto_convert` | false | 是否扫描目录自动转换 .txt → .bin |
| `overwrite` | false | 转换时是否覆盖已有 .bin 文件 |

**test_cases — 测试用例**

每个用例的字段：

| 字段 | 说明 |
|---|---|
| `name` | 用例名称（用于筛选和显示） |
| `M_file`, `C_file`, `K_file` | 矩阵文件路径（相对于 `problem_base_path`） |
| `group` | 分组标签（用于 `--group` 筛选） |
| `is_sparse` | 是否为稀疏矩阵 |
| `overrides` | 可选的逐用例参数覆盖 |

### 参数优先级

```
CLI 命令行（--nev, --sigma, --tol） > 用例级 overrides > solver 全局默认值
```

---

## 输出格式解读

运行后依次输出：

1. **环境信息**：MKL 版本、OpenMP 状态、AVX2 状态
2. **配置摘要**：所有生效的参数一览
3. **逐用例输出**：
   - 问题基本信息（维度、稀疏度）
   - 条件数（若开启）
   - 矩阵性质（若开启）
   - 每个求解器的运行结果：耗时、特征值表、残差验证表（4 级状态：EXCELLENT / OK / ACCEPTABLE / WARNING）
4. **汇总报告**：所有用例 × 所有求解器的 PASSED/FAILED 总表

---

## C++ 编程调用

```cpp
#include "qep/QEP.h"
#include "config/ConfigManager.h"
#include "config/SolverParams.h"

Config::ConfigManager::load("config.json");
auto solverParams = Config::ConfigManager::instance().toSolverParams();

auto problem = QEP::createTestProblemFromFiles(
    "my_problem", "M.bin", "C.bin", "K.bin", true, solverParams);

QEP::LinearSolverConfig cfg;
cfg.inner_tolerance = 1e-8;
cfg.outer_tolerance = 1e-6;

auto result = QEP::solveQEP_Unified(
    problem, 10, 0.0,
    QEP::LinearSolverType::PardisoLU, cfg, solverParams);

for (int i = 0; i < result.eigenvalues_real.size(); ++i)
    std::cout << result.eigenvalues_real(i) << "\n";
```

---

## 构建选项

| CMake 选项 | 默认值 | 说明 |
|---|---|---|
| `QEP_USE_MKL` | ON | Intel MKL + PARDISO（自动检测，找不到时降级） |
| `QEP_USE_AVX2` | ON | AVX2 指令集（非 x86-64 自动关闭） |
| `QEP_USE_OPENMP` | ON | OpenMP 并行（自动检测） |
| `QEP_BUILD_CLI` | ON | 构建 CLI 求解器 |
| `QEP_BUILD_GUI` | ON | 构建 Qt GUI（需 Qt5/Qt6） |
| `QEP_BUILD_EXAMPLES` | OFF | 构建示例程序 |
| `QEP_INSTALL` | ON | 生成安装规则 |

跨平台：ARM Mac / 鲲鹏 / 飞腾上 `-DQEP_USE_MKL=OFF` 即可编译运行。

## 矩阵文件格式

| 格式 | 扩展名 | 说明 |
|---|---|---|
| 二进制 CSR | `.bin` | 推荐，读写最快 |
| 文本 Matrix Market | `.txt` | 通用交换格式 |
| 自动转换 | — | `.txt` 文件首次加载时自动转为 `.bin` |

## 依赖

- **Eigen 3.4+** — 已内置在 `third_party/`
- **Spectra 1.1+** — 已内置在 `third_party/`
- **nlohmann/json** — 已内置在 `third_party/`
- **Intel MKL**（可选）— PARDISO 直接法 + BLAS 加速
