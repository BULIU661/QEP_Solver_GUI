//==============================================================================
//  src/gui/panels/OutputPanel.h  —  求解输出面板接口
//==============================================================================
#ifndef QEP_GUI_OUTPUT_PANEL_H
#define QEP_GUI_OUTPUT_PANEL_H

#include <QGroupBox>
#include <QVector>
#include "qep/QEP.h"

class QTextEdit;
class QPushButton;
class QTableWidget;
class QTabWidget;
class QLineEdit;
class QLabel;
class QSplitter;
class QFrame;
class QVBoxLayout;

class OutputPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit OutputPanel(QWidget *parent = nullptr);

public slots:
    void appendLog(const QString &msg);
    void clearLog();
    void addResult(const QString &caseName,
                   const QString &solverName,
                   const QEP::QuadraticEigenvalueResult &result,
                   double sigma = 0.0);

private slots:
    void onSaveOutput();
    void onClearLog();
    void onFilterTextChanged(const QString &text);
    void onExportJson();
    void onExportCsv();
    void onResultRowClicked(int row, int col);

private:
    void setupUI();
    void setupLogPage(QVBoxLayout *parentLayout);
    void setupTablePage(QVBoxLayout *parentLayout);
    void fillDetailForRow(int row);
    void updateSummaryBar(int metaIndex);
    void updateCountLabel();

    // 日志页面
    QTextEdit   *textEdit_ = nullptr;
    QPushButton *clearBtn_ = nullptr;
    QPushButton *saveBtn_  = nullptr;
    QLineEdit   *filterEdit_ = nullptr;

    // 结果表格页面 — 工业级 6 列合并格式
    QTabWidget   *mainTab_ = nullptr;
    QWidget      *logPage_ = nullptr;
    QWidget      *tablePage_ = nullptr;
    QTableWidget *resultTable_ = nullptr;
    QPushButton  *exportJsonBtn_ = nullptr;
    QPushButton  *exportCsvBtn_ = nullptr;

    // 详情面板
    QTextEdit *detailText_ = nullptr;
    QSplitter *tableSplitter_ = nullptr;

    // 摘要栏
    QFrame *summaryBar_ = nullptr;
    QLabel *summaryLabel_ = nullptr;

    // 状态
    QLabel *statusLabel_ = nullptr;
    QLabel *countLabel_ = nullptr;

    // 结果元数据
    struct ResultMeta {
        QString caseName;
        QString solverName;
        QEP::QuadraticEigenvalueResult result;
        double sigma = 0.0;
    };
    QList<ResultMeta> resultMetas_;
    QVector<int> rowToResultIndex_; // 表格行 → resultMetas_ 索引
};

#endif // QEP_GUI_OUTPUT_PANEL_H
