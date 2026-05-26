# QEP Solver — Quadratic Eigenvalue Problem Solver

[![C++17](https://img.shields.io/badge/C++-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-5%2F6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

An industrial-grade solver for the quadratic eigenvalue problem **(λ²M + λC + K)x = 0** based on the shift-invert Arnoldi method.

## Features

- **6 linear solvers**: PardisoLU (MKL), SparseLU, SimplicialLLT, ConjugateGradient, BiCGSTAB, GMRES
- **MKL + OpenMP + AVX2** acceleration for large-scale problems
- **CLI** for batch processing and scripting
- **Qt5/Qt6 GUI** for interactive configuration, solving, and result visualization
- **JSON-driven** configuration shared between CLI and GUI
- **Industrial-grade output** with TableBuilder formatting, residual validation, and multi-format export (TXT/JSON/CSV)

## Quick Start

### Build

```bash
cmake -B build -DQEP_BUILD_CLI=ON -DQEP_BUILD_GUI=ON
cmake --build build --config Release
```

Options: `QEP_USE_MKL` (default ON), `QEP_USE_OPENMP` (ON), `QEP_BUILD_CLI` (ON), `QEP_BUILD_GUI` (ON).

### CLI

```bash
cd bin
./qep --list                          # List all problems
./qep --case test                     # Solve with default settings
./qep --case test --solver PardisoLU  # Use specific solver
./qep --nev 20 --sigma 1.0            # Custom parameters
./qep --verbose                       # Detailed output
```

### GUI

```bash
cd bin
./qep_gui
```

1. Select problems in the tree panel (left)
2. Choose solvers in the solver panel (bottom-left)
3. Configure parameters in the "参数配置" tab (right)
4. Click "求解选中" to run
5. View results in the "结果表格" and "文本输出" tabs
6. Export results as TXT/JSON/CSV with a single click

All GUI settings are written directly to `config.json`, shared with the CLI.

## Architecture

The GUI is a **thin visualization layer** over the CLI's core functions. Both share:

- `ConfigManager` — single JSON config source
- `TestHarness` — solve dispatch, formatting, summary
- `QEP` core library — Arnoldi solver, matrix I/O, residual validation

See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed design.

## Directory Structure

```
├── include/          # Public headers (QEP.h, ConfigManager.h, TableFormatter.h)
├── src/
│   ├── qep/          # Core library (solver, I/O, config, validation)
│   ├── cli/          # CLI executable (main + TestHarness)
│   └── gui/          # GUI executable (Qt panels + SolverWorker)
├── Problems/         # Test problem data
├── config.json       # Runtime configuration (CLI/GUI shared)
├── config.schema.json
└── third_party/      # Eigen3, Spectra, nlohmann/json (header-only)
```

## License

MIT
