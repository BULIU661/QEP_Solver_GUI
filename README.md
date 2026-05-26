# QEP Solver — Quadratic Eigenvalue Problem Solver

[![C++17](https://img.shields.io/badge/C++-17-blue)](https://en.cppreference.com/w/cpp/17)
[![Qt](https://img.shields.io/badge/Qt-5%2F6-green)](https://www.qt.io/)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

An industrial-grade solver for the quadratic eigenvalue problem **(λ²M + λC + K)x = 0** based on the shift-invert Arnoldi method (Spectra). Supports sparse/dense matrices, 6 linear solvers, MKL + OpenMP + AVX2 acceleration.

## Three Ways to Use

QEP Solver offers three interaction modes, all sharing the same **core library**, **configuration file**, and **output format**:

| Mode | Interface | Best For |
|------|-----------|----------|
| **GUI** | Qt5/Qt6 graphical interface | Interactive parameter tuning, visual result browsing |
| **CLI** | Command-line `qep` | Batch processing, scripting, HPC clusters |
| **JSON** | Directly edit `config.json` | CI/CD pipelines, programmatic configuration |

All three write to and read from the same `config.json`. A parameter changed in the GUI is immediately visible to the CLI, and vice versa.

### GUI

```bash
./qep_gui [config_path]
```

1. Select problems in the tree panel
2. Choose solvers in the solver panel
3. Configure parameters across 5 tabs (solver, linear solver, performance, file I/O, logging)
4. Click "求解选中" to run
5. View results in formatted table and structured text log
6. Export TXT/JSON/CSV with one click

### CLI

```bash
./qep --list                    # List all problems
./qep --case test               # Solve with default config
./qep --case test --solver PardisoLU --nev 20 --sigma 1.0 --verbose
```

Full parameter support: `--case`, `--group`, `--solver`, `--nev`, `--sigma`, `--tol`, `--verbose`, `--quiet`.

### JSON Configuration

Edit `config.json` directly:

```json
{
  "solver": {
    "default_nev": 10,
    "default_sigma": 0.0,
    "enabled_solvers": { "PardisoLU": true, "SparseLU": false }
  },
  "logging": {
    "print_eigenvalues": true,
    "print_residuals": true,
    "enable_summary_report": true,
    "print_solver_details": false
  }
}
```

All parameters are documented in `config.schema.json`.

## Build

```bash
cmake -B build -DQEP_BUILD_CLI=ON -DQEP_BUILD_GUI=ON
cmake --build build --config Release
```

| Option | Default | Description |
|--------|---------|-------------|
| `QEP_USE_MKL` | ON | Intel MKL (PardisoLU) |
| `QEP_USE_OPENMP` | ON | OpenMP parallelization |
| `QEP_BUILD_CLI` | ON | CLI executable `qep` |
| `QEP_BUILD_GUI` | ON | GUI executable `qep_gui` |

**Dependencies**: C++17, Eigen3, Spectra, nlohmann/json (all included in `third_party/`), Qt5/Qt6 (GUI only), Intel MKL (optional), OpenMP (optional).

## Output Format

Both CLI and GUI produce **byte-level identical** text output:

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
    ...
  ▸ Residual Validation (10 eigenvalues)
    #           Eigenvalue            Abs. Residual    Rel. Residual      Status
  ---------------------------------------------------------------------------------
    1            0.654587                  1.31e-15         1.28e-15    EXCELLENT
  ── Summary Report ──
    Case           Method           Status     #Eig    Time(s)   Validation
```

## Architecture

The GUI is a thin visualization layer over the CLI's core. See [ARCHITECTURE.md](ARCHITECTURE.md).

## License

MIT
