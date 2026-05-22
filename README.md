# QEP Solver

基于 **Eigen** 和 **Spectra** 库的稀疏矩阵二次特征值问题（QEP）求解器。

## 问题描述

求解二次特征值问题：

```
(λ²M + λC + K) x = 0
```

其中 M, C, K 为稀疏矩阵，λ 为特征值，x 为特征向量。这类问题广泛存在于结构动力学、声学、转子动力学等工程领域。

## 快速构建

项目已包含 Eigen 和 Spectra 依赖，下载后可直接构建：

```bash
mkdir build && cd build
cmake .. -DQEP_USE_MKL=OFF    # 无 MKL 环境使用此选项
cmake --build . -j4
```

如需 Intel MKL 加速（推荐），配置时确保 MKL 环境可用：

```bash
cmake ..                      # 自动检测 MKL
```

## 运行测试

```bash
./bin/qep_test                # 默认参数运行测试
./bin/qep_test 0.1 15         # 指定移频点 sigma=0.1，求 15 个特征值
```

## 运行示例

```bash
cmake .. -DQEP_BUILD_EXAMPLES=ON
cmake --build . -j4
./bin/simple_example
```

## CMake 配置选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `QEP_USE_MKL` | ON | 启用 Intel MKL 加速 |
| `QEP_USE_AVX2` | ON | 启用 AVX2 指令集 |
| `QEP_USE_OPENMP` | ON | 启用 OpenMP 并行 |
| `QEP_BUILD_TEST` | ON | 构建测试程序 |
| `QEP_BUILD_EXAMPLES` | OFF | 构建示例程序 |
| `QEP_INSTALL` | ON | 生成安装规则 |
| `QEP_PROBLEM_BASE_PATH` | `../Problems` | 问题数据文件路径 |

## 项目结构

```
QEP/
├── CMakeLists.txt              # 主构建文件
├── Problems/                   # 测试问题数据（按分类存放）
│   ├── small_demo/             # 小规模演示问题（100维）
│   ├── engineering/            # 工程实际问题（1千~78万维）
│   └── benchmark/              # 基准测试问题
├── include/
│   ├── qep/QEP.h              # 公共接口（数据结构与函数声明）
│   └── config/                 # 配置头文件
├── src/
│   ├── qep/core/              # 核心求解器实现
│   ├── qep/io/                # 矩阵文件读写（CSR/COO/二进制）
│   ├── qep/utils/             # 工具函数
│   └── test/                  # 测试程序
├── examples/                   # 示例程序
└── third_party/                # 第三方库（Eigen / Spectra）
```

## 问题数据分类

问题数据统一存放在 `Problems/` 目录下，按类别分为三个子目录：

### small_demo/ — 小规模演示问题（100维）

| 子目录 | 问题类型 | 特点 |
|--------|---------|------|
| `sym_pos_def` | 对称正定 | 数值稳定，适合入门测试 |
| `unsymmetric` | 非对称 | 测试非对称问题求解 |
| `sym_indefinite` | 对称不定 | 测试不定问题求解 |
| `sym_semi_pos_def` | 对称半正定 | 测试半正定问题求解 |

### benchmark/ — 基准测试问题

| 子目录 | 维度 | 特点 |
|--------|------|------|
| `spring_200k` | ~200000 | 对称正定，条件数良好，谱分布密集 |
| `damped_beam_10k` | ~10000 | 对称正定，条件数极差 |
| `acoustic_wave_200k` | ~200000 | 非对称 QEP |
| `wiresaw1_10k` | ~10000 | 对称正定，C 为稠密矩阵 |
| `test_prob_1` | - | 额外基准测试 |

### engineering/ — 工程实际问题

| 子目录 | 维度 | 说明 |
|--------|------|------|
| `generator_rotor_1k` | ~1000 | 发电机转子问题 |
| `plate_10k` | ~10000 | 板子振动问题 |
| `turbine_560k` | ~560000 | 水轮机问题 |
| `vibrating_screen_780k` | ~780000 | 振动筛问题 |

## 求解器列表

| 求解器 | 类型 | 依赖 | 适用场景 |
|--------|------|------|---------|
| PardisoLU | 直接法 | MKL | 通用，性能最佳 |
| SparseLU | 直接法 | 无 | 小规模问题 |
| SimplicialLLT | 直接法 | 无 | 对称正定问题 |
| ConjugateGradient | 迭代法 | 无 | 对称正定，内存有限 |
| BiCGSTAB | 迭代法 | 无 | 非对称问题 |
| GMRES | 迭代法 | 无 | 病态问题 |

## 如何添加自己的问题

### 1. 准备矩阵数据文件

#### 方案 A：准备文本文件（.txt）后转为二进制（推荐）

本程序使用稀疏矩阵的 **CSR 格式**，支持 **0-based** 和 **1-based** 两种索引。TXT 文件格式如下：

```
<n>                       # 第一行：矩阵维度 n
<row_ptr[0]>              # 接下来 n+1 行：CSR 行指针
<row_ptr[1]>
...
<row_ptr[n]>
<col_idx[0]>              # 接下来 nnz 行：列索引
<col_idx[1]>
...
<col_idx[nnz-1]>
<value[0]>                # 接下来 nnz 行：数值
<value[1]>
...
<value[nnz-1]>
```

说明：
- 支持 **0-based**（行指针从 0 开始）或 **1-based**（行指针从 1 开始），程序会自动检测
- 列索引同样支持 0-based 或 1-based
- 可以通过 `#` 开头的注释行
- 数值为浮点数，支持科学计数法（如 `1.234e-3`）
- 程序会自动过滤掉显式的零元

**示例**（3×3 单位矩阵，0-based CSR）：

```
3
0
1
2
3
0
1
2
1.0
1.0
1.0
```

#### 方案 B：直接准备二进制文件（.bin）

如果跳过 TXT 格式，也可以直接准备二进制文件。二进制文件格式为：

```
int n                    // 矩阵维度
int nnz                  // 非零元素个数
int row_ptr[n+1]         // 行指针（0-based）
int col_idx[nnz]         // 列索引（0-based）
double values[nnz]       // 数值
```

> 需要为 M、C、K 三个矩阵各准备一个文件，分别命名为 M、C、K（扩展名为 .bin 或 .txt）。

### 2. 将 TXT 文件转换为二进制文件（如果在方案 A 中使用了 TXT 格式）

**方法一：程序自动转换（推荐）**

修改 `GlobalConfig.h` 中的 `AUTO_CONVERT_BINARY` 为 `true`，程序在运行测试时会自动检查并转换缺失的二进制文件。

**方法二：手动转换**

调用 `convertTextCSRtoBinary()` 函数即可将 TXT 转为 BIN：

```cpp
#include <qep/QEP.h>

// 将文本 CSR 文件转换为二进制文件
QEP::convertTextCSRtoBinary("my_problem/M.txt", "my_problem/M.bin");
QEP::convertTextCSRtoBinary("my_problem/C.txt", "my_problem/C.bin");
QEP::convertTextCSRtoBinary("my_problem/K.txt", "my_problem/K.bin");
```

### 3. 注册问题到测试系统

编辑 `include/config/TestCases.h`，在 `getTestCaseList()` 中添加新用例：

```cpp
{"我的新问题",
 base + "/benchmark/my_problem/M.bin",
 base + "/benchmark/my_problem/C.bin",
 base + "/benchmark/my_problem/K.bin", true},
```

> 第四个参数 `true` 表示使用稀疏矩阵读取（`readBinaryCSR`），如果矩阵是**密集格式**，设为 `false`（使用 `read_dense_matrix`）。

### 4. 注册 TXT→BIN 转换映射（如果使用 TXT 格式）

编辑 `include/config/BinaryConvertList.h`，在 `getBinaryConvertList()` 中添加转换对：

```cpp
{base + "/benchmark/my_problem/M.txt", base + "/benchmark/my_problem/M.bin"},
{base + "/benchmark/my_problem/C.txt", base + "/benchmark/my_problem/C.bin"},
{base + "/benchmark/my_problem/K.txt", base + "/benchmark/my_problem/K.bin"},
```

> 只有打开了 `AUTO_CONVERT_BINARY` 开关时才会自动执行此转换列表。

### 5. 提交到 Git

数据文件（.txt、.png、.vtk）默认被 `.gitignore` 忽略，不会被提交到远程仓库。
如需保留目录结构，在各子目录下放 `.gitkeep` 文件即可。

### 6. 编程调用示例

除了使用测试系统，也可以在代码中直接构建问题并求解：

```cpp
#include <qep/QEP.h>

int main() {
    // 方式一：从二进制文件读取
    QEP::QuadraticEigenvalueProblem problem;
    problem.M = QEP::readBinaryCSR("path/to/M.bin");
    problem.C = QEP::readBinaryCSR("path/to/C.bin");
    problem.K = QEP::readBinaryCSR("path/to/K.bin");
    problem.dimension = problem.M.rows();
    problem.name = "My Problem";

    // 配置求解器并求解
    QEP::LinearSolverConfig config;
    config.type = QEP::LinearSolverType::SparseLU;
    auto result = QEP::solveQEP_Unified(problem, 10, 0.0,
        QEP::LinearSolverType::SparseLU, config);

    // 输出结果
    if (result.success) {
        for (int i = 0; i < result.eigenvalues_real.size(); ++i)
            std::cout << result.eigenvalues_real(i) << " + "
                      << result.eigenvalues_imag(i) << "i\n";
    }
    return 0;
}
```

完整示例参见：`examples/simple_example.cpp`

## 依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| CMake | >= 3.20 | 构建系统 |
| C++ 编译器 | C++17 | GCC >= 7, Clang >= 5, MSVC 2019+ |
| Eigen | 3.x | 已包含在 `third_party/eigen` |
| Spectra | 1.x | 已包含在 `third_party/spectra` |
| Intel MKL | 可选 | 加速线性代数运算（强烈推荐） |
