# Changelog

All notable changes to QEP Solver.

---

## [v1.1.0] — 2026-05-26

### Architecture
- GUI refactored as a thin visualization layer over CLI core
- `SolverWorker::run()` delegates to `TestHarness::processCaseForGUI()` for byte-level output consistency
- Removed parallel service layer (JsonConfigStore, QepSolverService, IConfigStore, etc.)
- Removed ViewModels; panels interact with `ConfigManager` directly
- Removed preset configuration bar
- Cleaned up 5 empty/unused directories and files

### GUI
- 5-tab ConfigPanel covering all `config.json` parameters
- Result table: 6 columns (#, eigenvalue, |λ|, relative residual, absolute residual, status)
- Summary bar: total solves, success/fail, worst residual with color grade
- One-click TXT/JSON/CSV export to `Result/{timestamp}/`
- Double-click row for full detail view (summary + result table + performance stats + solver logs)
- Ctrl+wheel zoom on text panels, consistent dark theme
- Removed redundant group titles and AVX compile-time display
- Solver panel shows "直接法"/"迭代法" category labels

### CLI
- `print_solver_details` config parameter controls solver internal log output
- Matrix Properties section with `▸` header for consistent formatting
- Solver selection respects `enabled_solvers` in `config.json`
- `processCaseForGUI()` function for GUI-to-TestHarness integration

### Fixes
- Residual status unified to `fmt::residualStatus()`: EXCELLENT (<1e-10), OK (<1e-8), ACCEPTABLE (<1e-6), WARNING (≥1e-6)
- Config initialization no longer overwrites `config.json` with defaults
- Cross-thread `resultReady` signal registered with `qRegisterMetaType` + `Q_DECLARE_METATYPE`
- Text output no longer double-spaced (CaptureBuffer strips trailing `\n`)
- Export path uses project root `Result/` instead of working directory
- `check_tolerance` default corrected to `1e-6`

### Documentation
- Comprehensive `ARCHITECTURE.md` added
- `README.md` rewritten with directory tree, config reference, build options, problem format
- `CHANGELOG.md` created
- Release tagged `v1.1.0`

---

## [v1.0.0] — 2025

### Initial Release
- Shift-invert Arnoldi solver via Spectra
- 6 linear solvers: PardisoLU (MKL), SparseLU, SimplicialLLT, CG, BiCGSTAB, GMRES
- Sparse matrix I/O: binary CSR + text CSR with auto-conversion
- Matrix property analysis: symmetry check, condition number estimation
- Residual validation with formatted tables
- CLI with `--case`, `--solver`, `--nev`, `--sigma`, `--verbose` flags
- Qt5 GUI with ConfigPanel, SolverPanel, ProblemTreePanel, OutputPanel
- `config.json` shared between CLI and GUI
- MKL + OpenMP + AVX2 acceleration support
- Adaptive solver parameters based on matrix condition number
