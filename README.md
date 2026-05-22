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
│   ├── qep/io/                # 矩阵文件读写
│   ├── qep/utils/             # 工具函数
│   └── test/                  # 测试程序
├── examples/                   # 示例程序
└── third_party/                # 第三方库（Eigen / Spectra）
```

## 安装到系统（可选）

`QEP_INSTALL` 选项（默认 ON）用于将 QEP 库安装到系统目录，供其他 CMake 项目通过 `find_package(QEP)` 引用。

### 执行安装

构建完成后运行：

```bash
# 安装到系统默认路径（Linux: /usr/local/, Windows: C:/Program Files/）
cmake --install .

# 或指定安装目录
cmake --install . --prefix D:/my_libs/QEP
```

安装后会生成以下文件结构：

```
<prefix>/
├── include/
│   ├── qep/QEP.h           # 公共头文件
│   └── config/             # 配置文件
├── lib/
│   └── QEP.lib             # 静态库
└── lib/cmake/QEP/
    ├── QEPConfig.cmake      # 包配置文件
    └── QEPTargets.cmake     # 目标导出文件
```

### 在其他项目中使用

安装后，在其他 CMake 项目中只需写：

```cmake
find_package(QEP REQUIRED)
target_link_libraries(my_project PRIVATE QEP::QEP)
```

不需要再手动配置 Eigen、Spectra 的路径，也不需要再设置 MKL 的编译选项，全部由 QEP 的安装包自动管理。

### 注意事项

- Windows 上推荐指定 `--prefix` 安装到自定义目录（避免管理员权限问题）
- 如果安装到了非系统默认目录，需要设置 `CMAKE_PREFIX_PATH` 来帮助 `find_package` 找到 QEP：
  ```bash
  cmake .. -DCMAKE_PREFIX_PATH=D:/my_libs/QEP
  ```
- 如果只是在项目内部使用（不打算导出给其他项目），保持默认 `QEP_INSTALL=ON` 即可，不影响正常构建

## 问题数据说明

程序自带的测试问题存放在 `Problems/` 目录下，分为三类：


### small_demo/ —— 小规模演示问题（100维）

适合快速验证求解器功能。

| 子目录 | 问题类型 | 说明 |
|--------|---------|------|
| `sym_pos_def` | 对称正定 | 数值稳定，适合入门测试 |
| `unsymmetric` | 非对称 | 测试非对称问题求解 |
| `sym_indefinite` | 对称不定 | 测试不定问题求解 |
| `sym_semi_pos_def` | 对称半正定 | 测试半正定问题求解 |

### benchmark/ —— 基准测试问题

用于评估求解器在不同规模、不同特性问题上的表现。

| 子目录 | 维度 | 特点 |
|--------|------|------|
| `spring_200k` | ~200000 | 对称正定，条件数良好，谱分布密集 |
| `damped_beam_10k` | ~10000 | 对称正定，条件数极差 |
| `acoustic_wave_200k` | ~200000 | 非对称 QEP |
| `wiresaw1_10k` | ~10000 | 对称正定，C 为稠密矩阵 |
| `test_prob_1` | - | 额外基准测试 |

### engineering/ —— 工程实际问题

来自实际工程项目的真实问题数据。

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

## 添加自己的问题

如果你想用自己的矩阵数据来求解 QEP，可以按照以下步骤进行。

### 第一步：准备 TXT 格式的矩阵文件

在 `Problems/` 下新建一个目录（例如 `benchmark/my_problem/`），放入三个文本文件：`M.txt`、`C.txt`、`K.txt`，分别对应质量矩阵、阻尼矩阵和刚度矩阵。

#### 文件格式说明

本程序使用稀疏矩阵的 **CSR（Compressed Sparse Row）** 格式存储。一个矩阵文件的内容结构如下：

```
<n>                       ← 第一行：矩阵维度 n
<row_ptr[0]>              ← 接下来 n+1 行：CSR 行指针
<row_ptr[1]>
...
<row_ptr[n]>
<col_idx[0]>              ← 接下来 nnz 行：列索引
<col_idx[1]>
...
<col_idx[nnz-1]>
<value[0]>                ← 接下来 nnz 行：数值
<value[1]>
...
<value[nnz-1]>
```

其中：
- **行指针（row_ptr）**：长度 n+1，`row_ptr[i]` 表示第 i 行第一个非零元素在列索引/数值数组中的起始位置
- **列索引（col_idx）**：长度 nnz，按行记录每个非零元素的列号
- **数值（values）**：长度 nnz，与列索引一一对应，记录非零元素的值
- 支持 **0-based**（行指针从 0 开始）或 **1-based**（行指针从 1 开始）索引，程序会自动检测
- 数值支持科学计数法（例如 `1.234e-3`）
- 可以用 `#` 开头的行作为注释

**一个 3×3 单位矩阵的 CSR 示例（0-based）**：

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

> 除 CSR 格式外，程序也支持 **COO 格式**（每行：`行号 列号 值`），程序会自动检测。同时支持**密集矩阵格式**（首行：`行数 列数`，后接逐行数据）。

### 第二步：将 TXT 转为二进制文件（推荐）

程序运行测试时读取的是二进制 `.bin` 文件，因此需要先将 TXT 转换为二进制。有两种方式：

**方式一（推荐）：程序自动转换**

如果你把问题添加到了测试系统中（见第三步），只需在 `include/config/GlobalConfig.h` 中将 `AUTO_CONVERT_BINARY` 设置为 `true`。程序启动时会自动检查并转换缺失的二进制文件。

**方式二：手动调用转换函数**

在你的代码中调用 `convertTextCSRtoBinary()`：

```cpp
#include <qep/QEP.h>

// 将文本 CSR 文件转换为二进制文件
QEP::convertTextCSRtoBinary("Problems/benchmark/my_problem/M.txt",
                             "Problems/benchmark/my_problem/M.bin");
QEP::convertTextCSRtoBinary("Problems/benchmark/my_problem/C.txt",
                             "Problems/benchmark/my_problem/C.bin");
QEP::convertTextCSRtoBinary("Problems/benchmark/my_problem/K.txt",
                             "Problems/benchmark/my_problem/K.bin");
```

### 第三步：注册到测试系统

编辑 `include/config/TestCases.h`，在 `getTestCaseList()` 函数中添加一个测试用例：

```cpp
{"我的新问题",
 base + "/benchmark/my_problem/M.bin",
 base + "/benchmark/my_problem/C.bin",
 base + "/benchmark/my_problem/K.bin",
 true},
```

参数说明：
- 第一个参数：问题的名称，会显示在测试报告中
- 第二至四个参数：M、C、K 矩阵二进制文件的路径
- 第五个参数：`true` 表示稀疏矩阵（推荐），`false` 表示密集矩阵

### 第四步：注册转换映射

如果你的问题用了 TXT 格式（第二步），还需要编辑 `include/config/BinaryConvertList.h`，在 `getBinaryConvertList()` 中添加转换映射：

```cpp
{base + "/benchmark/my_problem/M.txt", base + "/benchmark/my_problem/M.bin"},
{base + "/benchmark/my_problem/C.txt", base + "/benchmark/my_problem/C.bin"},
{base + "/benchmark/my_problem/K.txt", base + "/benchmark/my_problem/K.bin"},
```

这样，当打开了 `AUTO_CONVERT_BINARY` 开关时，程序会自动为你完成 TXT → BIN 的转换。

### 第五步：构建并运行

```bash
cd build
cmake ..
cmake --build . -j4
./bin/qep_test
```

## 编程接口说明

除了使用测试系统，你也可以在自己的代码中直接调用求解器。

### 从文件读取并求解

```cpp
#include <qep/QEP.h>

int main() {
    // 从二进制文件读取矩阵
    QEP::QuadraticEigenvalueProblem problem;
    problem.M = QEP::readBinaryCSR("path/to/M.bin");
    problem.C = QEP::readBinaryCSR("path/to/C.bin");
    problem.K = QEP::readBinaryCSR("path/to/K.bin");
    problem.dimension = problem.M.rows();
    problem.name = "我的问题";

    // 配置求解器
    QEP::LinearSolverConfig config;
    config.type = QEP::LinearSolverType::SparseLU;

    // 求解：求 10 个特征值，移频点 sigma=0.0
    auto result = QEP::solveQEP_Unified(
        problem, 10, 0.0,
        QEP::LinearSolverType::SparseLU, config);

    // 输出结果
    if (result.success) {
        for (int i = 0; i < result.eigenvalues_real.size(); ++i)
            std::cout << "λ[" << i+1 << "] = "
                      << result.eigenvalues_real(i) << " + "
                      << result.eigenvalues_imag(i) << "i\n";
    }
    return 0;
}
```

完整示例参见：`examples/simple_example.cpp`

### 在代码中直接构建矩阵并求解

也可以不依赖文件，直接在代码中构建稀疏矩阵：

```cpp
#include <qep/QEP.h>

int main() {
    const int n = 3;

    Eigen::SparseMatrix<double> M(n, n), C(n, n), K(n, n);
    // ... 填充矩阵元素 ...

    QEP::QuadraticEigenvalueProblem problem;
    problem.M = M;
    problem.C = C;
    problem.K = K;
    problem.dimension = n;
    problem.name = "自定义问题";

    // 后续求解同上 ...
}
```

## 依赖

| 依赖 | 版本 | 说明 |
|------|------|------|
| CMake | >= 3.20 | 构建系统 |
| C++ 编译器 | C++17 | GCC >= 7, Clang >= 5, MSVC 2019+ |
| Eigen | 3.x | 已包含在 `third_party/eigen` |
| Spectra | 1.x | 已包含在 `third_party/spectra` |
| Intel MKL | 可选 | 加速线性代数运算（强烈推荐） |
