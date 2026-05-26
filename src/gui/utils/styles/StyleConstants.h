//==============================================================================
//  src/gui/styles/StyleConstants.h  —  全局 Qt 样式表常量（header-only）
//==============================================================================
#ifndef QEP_GUI_STYLE_CONSTANTS_H
#define QEP_GUI_STYLE_CONSTANTS_H

namespace style {

// ==================== 全局应用样式表 (main.cpp) ====================

constexpr const char* kGlobalStyleSheet = R"(
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
)";

// ==================== 主窗口求解按钮样式 (MainWindow.cpp) ====================

constexpr const char* kSolveBtnNormal =
    "QPushButton { background-color: #27ae60; color: white; border: none;"
    " border-radius: 4px; font-size: 13px; font-weight: bold; }"
    "QPushButton:hover { background-color: #219a52; }"
    "QPushButton:pressed { background-color: #1e8449; }"
    "QPushButton:disabled { background-color: #bdc3c7; }";

constexpr const char* kSolveBtnCancel =
    "QPushButton { background-color: #e74c3c; color: white; border: none;"
    " border-radius: 4px; font-size: 13px; font-weight: bold; }"
    "QPushButton:hover { background-color: #c0392b; }";

// ==================== 输出面板暗色主题 (OutputPanel.cpp) ====================

constexpr const char* kDarkTextEdit = R"(
    QTextEdit {
        background-color: #1e1e1e;
        color: #d4d4d4;
        border: 1px solid #3c3c3c;
        border-radius: 4px;
        font-family: "Consolas", "Courier New", monospace;
        font-size: 9pt;
    }
)";

constexpr const char* kDarkTable = R"(
    QTableWidget {
        background-color: #1e1e1e;
        color: #d4d4d4;
        gridline-color: #3c3c3c;
        border: 1px solid #3c3c3c;
        border-radius: 4px;
    }
    QTableWidget::item { padding: 5px 6px; background-color: #1e1e1e; color: #d4d4d4; }
    QTableWidget::item:selected { background-color: #094771; }
    QHeaderView::section {
        background-color: #2d2d30;
        color: #cccccc;
        padding: 7px 8px;
        border: none;
        border-right: 1px solid #3c3c3c;
        border-bottom: 1px solid #3c3c3c;
        font-weight: bold;
    }
)";

constexpr const char* kDarkInput = R"(
    QLineEdit {
        background: #2d2d30;
        color: #cccccc;
        border: 1px solid #3c3c3c;
        border-radius: 3px;
        padding: 4px 8px;
    }
    QLineEdit:focus { border: 1px solid #007acc; }
)";

constexpr const char* kDarkBtn = R"(
    QPushButton {
        background: #3c3c3c;
        color: #cccccc;
        border: none;
        border-radius: 3px;
        padding: 6px 12px;
    }
    QPushButton:hover { background: #505050; }
)";

constexpr const char* kDarkExportBtn = R"(
    QPushButton {
        background: #0e639c;
        color: #ffffff;
        border: none;
        border-radius: 3px;
        padding: 6px 12px;
    }
    QPushButton:hover { background: #1177bb; }
)";

constexpr const char* kDarkTab = R"(
    QTabWidget::pane {
        border: 1px solid #3c3c3c;
        border-radius: 4px;
        background: #1e1e1e;
    }
    QTabBar::tab {
        background: #2d2d30;
        color: #cccccc;
        padding: 8px 16px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
    }
    QTabBar::tab:selected { background: #007acc; color: #ffffff; }
    QTabBar::tab:hover { background: #3e3e42; }
)";

// ==================== 配置面板样式 (ConfigPanel.cpp) ====================

constexpr const char* kCardStyle = R"(
    QFrame#configCard {
        background: #ffffff;
        border: 1px solid #e8e8e8;
        border-radius: 6px;
    }
    QFrame#configCard:hover {
        border-color: #3498db;
    }
)";

constexpr const char* kConfigTabStyle = R"(
    QTabWidget::pane {
        border: 1px solid #e8e8e8;
        border-radius: 4px;
        background: #fafafa;
    }
    QTabBar::tab {
        background: #f0f0f0;
        padding: 8px 16px;
        margin-right: 2px;
        border-top-left-radius: 4px;
        border-top-right-radius: 4px;
    }
    QTabBar::tab:selected {
        background: #ffffff;
        border-bottom: 2px solid #3498db;
    }
    QTabBar::tab:hover:!selected {
        background: #e8e8e8;
    }
)";

constexpr const char* kPresetStyle = R"(
    QComboBox {
        padding: 6px 12px;
        border: 1px solid #d0d0d0;
        border-radius: 4px;
        background: #fafafa;
    }
    QComboBox:hover {
        border-color: #3498db;
    }
)";

constexpr const char* kGroupTitleStyle = R"(
    QLabel {
        color: #2c3e50;
        font-weight: bold;
        font-size: 11pt;
        padding: 8px 12px;
        background: #f8f9fa;
        border-bottom: 1px solid #e8e8e8;
    }
)";

constexpr const char* kTooltipStyle = R"(
    QLabel#hintLabel {
        color: #7f8c8d;
        font-size: 9pt;
        padding-left: 4px;
    }
)";

} // namespace style

#endif // QEP_GUI_STYLE_CONSTANTS_H
