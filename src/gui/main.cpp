//==============================================================================
//  src/gui/main.cpp  —  GUI 应用程序入口
//==============================================================================

#include <QApplication>
#include <QStyleFactory>
#include "MainWindow.h"
#include "config/ConfigManager.h"
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

static void initConsoleEncoding()
{
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE) {
        DWORD mode;
        if (GetConsoleMode(hConsole, &mode))
            SetConsoleMode(hConsole, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

int main(int argc, char *argv[])
{
    initConsoleEncoding();

    QApplication app(argc, argv);
    app.setApplicationName("QEP Solver");
    app.setOrganizationName("QEP");
    app.setStyle(QStyleFactory::create("Fusion"));

    QFont defaultFont = app.font();
    defaultFont.setFamily("Microsoft YaHei");
    defaultFont.setPointSize(10);
    app.setFont(defaultFont);

    // Load config
    std::string configPath;
    if (argc > 1)
        configPath = argv[1];
    if (!Config::ConfigManager::load(configPath)) {
        std::cerr << "Warning: Could not load config.json. Starting with empty config.\n";
    }

    // Apply OMP thread settings
    auto &cfg = Config::ConfigManager::instance();
#ifdef _OPENMP
    omp_set_num_threads(cfg.ompNumThreads());
#endif
#ifdef EIGEN_USE_MKL_ALL
    mkl_set_num_threads(cfg.mklNumThreads());
#endif

    app.setStyleSheet(R"(
        QMainWindow { background-color: #f5f5f5; }
        QGroupBox {
            font-weight: bold;
            border: 1px solid #c0c0c0;
            border-radius: 4px;
            margin-top: 12px;
            padding-top: 16px;
            background-color: #ffffff;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 4px 8px;
            color: #2c3e50;
        }
        QPushButton {
            background-color: #3498db;
            color: white;
            border: none;
            border-radius: 3px;
            padding: 6px 16px;
            font-weight: bold;
        }
        QPushButton:hover { background-color: #2980b9; }
        QPushButton:pressed { background-color: #2471a3; }
        QPushButton:disabled { background-color: #bdc3c7; }
        QTableWidget {
            gridline-color: #e0e0e0;
            selection-background-color: #d5e8f7;
            border: 1px solid #c0c0c0;
        }
        QTableWidget::item { padding: 4px; }
        QHeaderView::section {
            background-color: #ecf0f1;
            padding: 4px;
            border: 1px solid #c0c0c0;
            font-weight: bold;
        }
        QMenuBar {
            background-color: #2c3e50;
            color: white;
            padding: 2px;
        }
        QMenuBar::item:selected { background-color: #34495e; }
        QMenu {
            background-color: #ffffff;
            border: 1px solid #c0c0c0;
        }
        QMenu::item:selected { background-color: #3498db; color: white; }
        QStatusBar {
            background-color: #ecf0f1;
            border-top: 1px solid #c0c0c0;
        }
        QProgressBar {
            border: 1px solid #c0c0c0;
            border-radius: 3px;
            text-align: center;
            background-color: #ecf0f1;
        }
        QProgressBar::chunk {
            background-color: #3498db;
            border-radius: 2px;
        }
        QSpinBox, QDoubleSpinBox {
            border: 1px solid #c0c0c0;
            border-radius: 3px;
            padding: 2px 4px;
            background-color: #ffffff;
        }
        QSpinBox:focus, QDoubleSpinBox:focus {
            border-color: #3498db;
        }
        QCheckBox { spacing: 6px; }
        QSplitter::handle {
            background-color: #bdc3c7;
            width: 5px;
        }
        QSplitter::handle:hover { background-color: #3498db; }
        QTreeView {
            border: none;
            background-color: #ffffff;
            font-size: 10pt;
        }
        QTreeView::item {
            padding: 5px 4px;
            border-bottom: 1px solid #f0f0f0;
        }
        QTreeView::item:hover {
            background-color: #ebf5fb;
        }
        QTreeView::item:selected {
            background-color: #d5e8f7;
            color: #2c3e50;
        }
        QTreeView::branch:has-siblings:!adjoins-item {
            border-image: none;
        }
        QTabWidget::pane {
            border: 1px solid #c0c0c0;
            border-radius: 4px;
            background-color: #ffffff;
        }
        QTabBar::tab {
            background-color: #ecf0f1;
            border: 1px solid #c0c0c0;
            border-bottom: none;
            border-top-left-radius: 4px;
            border-top-right-radius: 4px;
            padding: 8px 20px;
            margin-right: 2px;
            color: #555555;
        }
        QTabBar::tab:selected {
            background-color: #ffffff;
            color: #2c3e50;
            font-weight: bold;
        }
        QTabBar::tab:hover:!selected {
            background-color: #d5dbdb;
        }
    )");

    MainWindow w;
    w.resize(1280, 800);
    w.show();

    return app.exec();
}
