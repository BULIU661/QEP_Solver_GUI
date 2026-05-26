//==============================================================================
//  src/gui/main.cpp  —  GUI 入口（直接基于 ConfigManager，与 CLI 共享底层）
//==============================================================================
#include <QApplication>
#include <QStyleFactory>
#include <QFont>
#include "MainWindow.h"
#include "config/ConfigManager.h"
#include "utils/QepMetaType.h"
#include "styles/StyleConstants.h"
#include <iostream>
#ifdef _WIN32
#include <windows.h>
#endif
#ifdef _OPENMP
#include <omp.h>
#endif
#ifdef EIGEN_USE_MKL_ALL
#include <mkl.h>
#endif

static void initConsoleEncoding() {
#ifdef _WIN32
    SetConsoleOutputCP(65001); SetConsoleCP(65001);
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    if (h != INVALID_HANDLE_VALUE) {
        DWORD mode; if (GetConsoleMode(h, &mode))
            SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

int main(int argc, char *argv[])
{
    initConsoleEncoding();
    QApplication app(argc, argv);
    qRegisterMetaType<QEP::QuadraticEigenvalueResult>();
    app.setApplicationName("QEP Solver");
    app.setOrganizationName("QEP");
    app.setStyle(QStyleFactory::create("Fusion"));
    QFont f = app.font(); f.setFamily("Microsoft YaHei"); f.setPointSize(10);
    app.setFont(f);
    app.setStyleSheet(style::kGlobalStyleSheet);

    // Load config — same as CLI (qep.exe)
    std::string cfgPath = (argc > 1) ? argv[1] : "";
    if (!Config::ConfigManager::load(cfgPath))
        std::cerr << "Warning: Could not load config.json.\n";

    auto &cfg = Config::ConfigManager::instance();
#ifdef _OPENMP
    omp_set_num_threads(cfg.ompNumThreads());
#endif
#ifdef EIGEN_USE_MKL_ALL
    mkl_set_num_threads(cfg.mklNumThreads());
#endif

    MainWindow w;
    w.resize(1280, 800);
    w.show();
    return app.exec();
}
