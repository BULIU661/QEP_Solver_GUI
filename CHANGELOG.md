# 更新日志

QEP Solver 所有重要变更记录。

---

## [v1.1.0] — 2026-05-26

### 架构重构
- GUI 重构为 CLI 的薄可视化层，不拥有独立核心逻辑
- `SolverWorker::run()` 委托给 `TestHarness::processCaseForGUI()`，实现字节级输出一致性
- 移除并行 Service 层（JsonConfigStore、QepSolverService、IConfigStore 等）
- 移除 ViewModels 层，面板直接与 `ConfigManager` 交互
- 移除预设配置栏，所有参数从 `config.json` 读取
- 清理 5 个空目录和无用文件

### GUI 改进
- 5 标签页 ConfigPanel 覆盖全部 `config.json` 参数（求解设置、线性求解器、性能优化、文件转换、日志诊断）
- 结果表格 6 列：编号、特征值、|λ|、相对残差、绝对残差、精度等级（颜色编码）
- 摘要栏：总求解数、成功/失败、最差残差及颜色等级
- 一键导出 TXT/JSON/CSV 到 `Result/{时间戳}/` 目录
- 双击表格行查看完整详情（概要 + 结果表 + 性能统计 + 求解器日志）
- Ctrl+滚轮缩放文本面板，统一暗色主题
- 移除冗余面板标题和 AVX 编译期显示项
- 求解器面板显示"直接法"/"迭代法"分类标签
- 参数配置各子页面增加介绍说明文字

### CLI 改进
- `print_solver_details` 配置参数控制求解器内部日志输出
- 矩阵属性输出带 `▸ Matrix Properties` 分节标题
- 求解器选择正确遵循 `config.json` 中的 `enabled_solvers` 设置
- 新增 `processCaseForGUI()` 函数支持 GUI 调用

### 修复
- 残差状态统一为 `fmt::residualStatus()`：EXCELLENT (< 1e-10)、OK (< 1e-8)、ACCEPTABLE (< 1e-6)、WARNING (≥ 1e-6)
- 启动时不再用默认值覆盖 `config.json`
- 跨线程 `resultReady` 信号通过 `qRegisterMetaType` + `Q_DECLARE_METATYPE` 正确注册
- 文本输出不再双倍空行（CaptureBuffer 去除行尾 `\n`）
- 导出路径改为项目根目录 `Result/` 而非工作目录
- `check_tolerance` 默认值修正为 `1e-6`

### 文档
- 新增 `ARCHITECTURE.md` 架构设计文档
- 重写 `README.md`（完整目录树、配置参考、构建选项、问题数据格式）
- 新增 `CHANGELOG.md`
- 发布标签 `v1.1.0`

---

## [v1.0.0] — 2025

### 首次发布
- 基于 Spectra 的位移-逆 Arnoldi 求解器
- 6 种内层线性求解器：PardisoLU（MKL）、SparseLU、SimplicialLLT、CG、BiCGSTAB、GMRES
- 稀疏矩阵 I/O：二进制 CSR + 文本 CSR，支持自动转换
- 矩阵属性分析：对称性检查、条件数估计
- 残差验证与格式化表格输出
- CLI 支持 `--case`、`--solver`、`--nev`、`--sigma`、`--verbose` 等参数
- Qt5 GUI（ConfigPanel、SolverPanel、ProblemTreePanel、OutputPanel）
- CLI 与 GUI 共享 `config.json`
- MKL + OpenMP + AVX2 加速支持
- 基于矩阵条件数的自适应参数调整
