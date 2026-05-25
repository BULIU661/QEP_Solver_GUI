//==============================================================================
//  src/gui/panels/OutputPanel.cpp  —  结果表格、文本日志、详情面板、JSON/CSV 导出
//==============================================================================
#include "OutputPanel.h"
#include "utils/ResidualValidation.h"
#include "config/TableFormatter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QStringList>
#include <QTextEdit>
#include <QPushButton>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTableWidget>
#include <QHeaderView>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QColor>
#include <QFont>
#include <cmath>

// ==================== 暗色主题样式 ====================
static const char *kDarkTextEditStyle = R"(
    QTextEdit {
        background-color: #1e1e1e;
        color: #d4d4d4;
        border: 1px solid #3c3c3c;
        border-radius: 4px;
        font-family: "Consolas", "Courier New", monospace;
        font-size: 9pt;
    }
)";

static const char *kDarkTableStyle = R"(
    QTableWidget {
        background-color: #1e1e1e;
        color: #d4d4d4;
        grid-color: #3c3c3c;
        border: 1px solid #3c3c3c;
        border-radius: 4px;
    }
    QTableWidget::item { padding: 5px 6px; }
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

static const char *kDarkInputStyle = R"(
    QLineEdit {
        background: #2d2d30;
        color: #cccccc;
        border: 1px solid #3c3c3c;
        border-radius: 3px;
        padding: 4px 8px;
    }
    QLineEdit:focus { border: 1px solid #007acc; }
)";

static const char *kBtnStyle = R"(
    QPushButton {
        background: #3c3c3c;
        color: #cccccc;
        border: none;
        border-radius: 3px;
        padding: 6px 12px;
    }
    QPushButton:hover { background: #505050; }
)";

static const char *kExportBtnStyle = R"(
    QPushButton {
        background: #0e639c;
        color: #ffffff;
        border: none;
        border-radius: 3px;
        padding: 6px 12px;
    }
    QPushButton:hover { background: #1177bb; }
)";

static const char *kTabStyle = R"(
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

// ==================== 构造函数 ====================
OutputPanel::OutputPanel(QWidget *parent)
    : QGroupBox(tr("求解输出"), parent)
{
    setupUI();
}

// ==================== 界面布局 ====================
void OutputPanel::setupUI()
{
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 12, 8, 8);

    // 主 TabWidget
    mainTab_ = new QTabWidget();
    mainTab_->setTabPosition(QTabWidget::North);
    mainTab_->setDocumentMode(true);
    mainTab_->setStyleSheet(kTabStyle);

    // ---- 第1页：结果表格（默认显示）----
    tablePage_ = new QWidget();
    auto *tableLayout = new QVBoxLayout(tablePage_);
    tableLayout->setContentsMargins(6, 6, 6, 6);
    tableLayout->setSpacing(4);
    setupTablePage(tableLayout);

    // ---- 第2页：文本输出（日志）----
    logPage_ = new QWidget();
    auto *logLayout = new QVBoxLayout(logPage_);
    logLayout->setContentsMargins(6, 6, 6, 6);
    logLayout->setSpacing(4);
    setupLogPage(logLayout);

    mainTab_->addTab(tablePage_, tr("结果表格"));
    mainTab_->addTab(logPage_, tr("文本输出"));

    mainLayout->addWidget(mainTab_, 1);

    // ---- 状态栏 ----
    auto *statusBar = new QHBoxLayout();
    statusBar->setContentsMargins(0, 4, 0, 0);
    statusLabel_ = new QLabel(tr("就绪"));
    statusLabel_->setStyleSheet(
        "QLabel { color: #6a9955; padding: 2px 8px;"
        " background: #1e1e1e; border-radius: 3px; }");
    countLabel_ = new QLabel(tr("共 0 条记录"));
    countLabel_->setStyleSheet("color: #cccccc; padding: 4px;");
    statusBar->addWidget(statusLabel_);
    statusBar->addStretch();
    statusBar->addWidget(countLabel_);
    mainLayout->addLayout(statusBar);
}

// ---- 日志页 ----
void OutputPanel::setupLogPage(QVBoxLayout *logLayout)
{
    auto *filterBar = new QHBoxLayout();
    filterBar->addWidget(new QLabel(tr("过滤:")));
    filterEdit_ = new QLineEdit();
    filterEdit_->setPlaceholderText(tr("输入关键字过滤日志..."));
    filterEdit_->setStyleSheet(kDarkInputStyle);
    filterBar->addWidget(filterEdit_, 1);
    logLayout->addLayout(filterBar);

    textEdit_ = new QTextEdit();
    textEdit_->setReadOnly(true);
    textEdit_->setFont(QFont("Consolas", 9));
    textEdit_->setStyleSheet(kDarkTextEditStyle);
    textEdit_->setLineWrapMode(QTextEdit::NoWrap);
    logLayout->addWidget(textEdit_, 1);

    auto *btnLayout = new QHBoxLayout();
    clearBtn_ = new QPushButton(tr("清空"));
    clearBtn_->setStyleSheet(kBtnStyle);
    saveBtn_ = new QPushButton(tr("保存输出"));
    saveBtn_->setStyleSheet(kExportBtnStyle);
    btnLayout->addWidget(clearBtn_);
    btnLayout->addWidget(saveBtn_);
    btnLayout->addStretch();
    logLayout->addLayout(btnLayout);

    connect(clearBtn_, &QPushButton::clicked, this, &OutputPanel::onClearLog);
    connect(saveBtn_, &QPushButton::clicked, this, &OutputPanel::onSaveOutput);
    connect(filterEdit_, &QLineEdit::textChanged, this, &OutputPanel::onFilterTextChanged);
}

// ---- 结果表格页 —— 工业级 6 列合并格式 ----
void OutputPanel::setupTablePage(QVBoxLayout *tableLayout)
{
    // ---- 工业级摘要栏 ----
    summaryBar_ = new QFrame();
    summaryBar_->setObjectName("summaryBar");
    summaryBar_->setStyleSheet(
        "QFrame#summaryBar {"
        "  background: #252526;"
        "  border: 1px solid #3c3c3c;"
        "  border-radius: 4px;"
        "}");
    auto *summaryLay = new QHBoxLayout(summaryBar_);
    summaryLay->setContentsMargins(12, 6, 12, 6);

    summaryLabel_ = new QLabel(tr("就绪 — 尚未求解"));
    summaryLabel_->setStyleSheet(
        "color: #cccccc; font-family: 'Consolas', 'Courier New'; font-size: 10pt;");
    summaryLabel_->setTextFormat(Qt::RichText);
    summaryLay->addWidget(summaryLabel_);
    summaryLay->addStretch();
    tableLayout->addWidget(summaryBar_);

    // 导出按钮栏
    auto *toolBar = new QHBoxLayout();
    toolBar->addStretch();
    exportJsonBtn_ = new QPushButton(tr("导出 JSON"));
    exportJsonBtn_->setStyleSheet(kExportBtnStyle);
    exportCsvBtn_ = new QPushButton(tr("导出 CSV"));
    exportCsvBtn_->setStyleSheet(kExportBtnStyle);
    toolBar->addWidget(exportJsonBtn_);
    toolBar->addWidget(exportCsvBtn_);
    tableLayout->addLayout(toolBar);

    // 表格 + 详情 分割器
    tableSplitter_ = new QSplitter(Qt::Vertical);
    tableSplitter_->setHandleWidth(4);
    tableSplitter_->setChildrenCollapsible(false);

    // 工业级特征值+残差合并表 — 6 列，与 CLI formatResidualTable 一致
    resultTable_ = new QTableWidget();
    resultTable_->setColumnCount(6);
    resultTable_->setHorizontalHeaderLabels({
        tr("#"), tr("特征值"), tr("|λ|"),
        tr("相对残差"), tr("绝对残差"), tr("状态")
    });
    resultTable_->setStyleSheet(kDarkTableStyle);
    resultTable_->horizontalHeader()->setStretchLastSection(true);
    resultTable_->horizontalHeader()->setSectionResizeMode(5, QHeaderView::Stretch);
    resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable_->setAlternatingRowColors(true);
    resultTable_->verticalHeader()->setVisible(false);
    resultTable_->setSortingEnabled(false);
    tableSplitter_->addWidget(resultTable_);

    // 详情面板
    auto *detailWidget = new QWidget();
    auto *detailLayout = new QVBoxLayout(detailWidget);
    detailLayout->setContentsMargins(0, 4, 0, 0);
    detailLayout->setSpacing(0);

    auto *detailLabel = new QLabel(
        tr("  双击表格行查看该结果集的完整输出（概览卡片 + 特征值残差表 + 性能统计）"));
    detailLabel->setStyleSheet(
        "color: #cccccc; background: #2d2d30; padding: 4px 8px;");
    detailLayout->addWidget(detailLabel);

    detailText_ = new QTextEdit();
    detailText_->setReadOnly(true);
    detailText_->setFont(QFont("Consolas", 9));
    detailText_->setStyleSheet(kDarkTextEditStyle);
    detailText_->setMaximumHeight(300);
    detailLayout->addWidget(detailText_);

    tableSplitter_->addWidget(detailWidget);
    tableSplitter_->setSizes({400, 150});

    tableLayout->addWidget(tableSplitter_, 1);

    connect(exportJsonBtn_, &QPushButton::clicked, this, &OutputPanel::onExportJson);
    connect(exportCsvBtn_, &QPushButton::clicked, this, &OutputPanel::onExportCsv);
    connect(resultTable_, &QTableWidget::cellDoubleClicked, this, &OutputPanel::onResultRowClicked);
}

// ==================== 添加结果 — 工业级合并格式 ====================
void OutputPanel::addResult(const QString &caseName,
                            const QString &solverName,
                            const QEP::QuadraticEigenvalueResult &result,
                            double sigma)
{
    int nEig = static_cast<int>(result.eigenvalues_real.size());
    if (nEig <= 0)
        return;

    // 存储元数据
    ResultMeta meta;
    meta.caseName   = caseName;
    meta.solverName = solverName;
    meta.result     = result;
    meta.sigma      = sigma;
    int metaIndex   = resultMetas_.size();
    resultMetas_.append(meta);

    // 结果集分隔行（第一个结果集之后插入）
    if (metaIndex > 0) {
        int sepRow = resultTable_->rowCount();
        resultTable_->insertRow(sepRow);
        rowToResultIndex_.append(-1);

        QString sepText = QString("—— %1 | %2 ——")
            .arg(caseName).arg(solverName);
        auto *sepItem = new QTableWidgetItem(sepText);
        sepItem->setTextAlignment(Qt::AlignCenter);
        sepItem->setBackground(QColor("#2d2d30"));
        sepItem->setForeground(QColor("#569cd6"));
        QFont sepFont = sepItem->font();
        sepFont.setBold(true);
        sepFont.setPointSize(9);
        sepItem->setFont(sepFont);
        resultTable_->setItem(sepRow, 0, sepItem);
        resultTable_->setSpan(sepRow, 0, 1, 6);
        resultTable_->setRowHeight(sepRow, 26);
    }

    // 每个特征值一行，工业级 4 级残差状态
    for (int i = 0; i < nEig; ++i) {
        int row = resultTable_->rowCount();
        resultTable_->insertRow(row);
        rowToResultIndex_.append(metaIndex);

        double re = result.eigenvalues_real(i);
        double im = result.eigenvalues_imag(i);
        double absVal = std::sqrt(re * re + im * im);

        // 特征值字符串
        QString eigStr;
        if (std::abs(im) < 1e-8)
            eigStr = QString::number(re, 'g', 8);
        else
            eigStr = QString("%1%2%3i")
                .arg(QString::number(re, 'f', 6))
                .arg(im >= 0 ? "+" : "")
                .arg(QString::number(std::abs(im), 'f', 6));

        double absRes = (i < static_cast<int>(result.residuals_abs.size()))
            ? result.residuals_abs[i] : -1.0;
        double relRes = (i < static_cast<int>(result.residuals_rel.size()))
            ? result.residuals_rel[i] : -1.0;

        // 工业级 4 级残差状态分类
        QString statusStr;
        QColor statusColor;
        if (relRes < 0) {
            statusStr = "N/A";
            statusColor = QColor("#808080");
        } else if (relRes < 1e-8) {
            statusStr = "EXCELLENT";
            statusColor = QColor("#4ec9b0");
        } else if (relRes < 1e-6) {
            statusStr = "OK";
            statusColor = QColor("#6a9955");
        } else if (relRes < 1e-4) {
            statusStr = "ACCEPTABLE";
            statusColor = QColor("#dcdcaa");
        } else {
            statusStr = "WARNING";
            statusColor = QColor("#f44747");
        }

        resultTable_->setItem(row, 0, new QTableWidgetItem(QString::number(i + 1)));
        resultTable_->setItem(row, 1, new QTableWidgetItem(eigStr));
        resultTable_->setItem(row, 2, new QTableWidgetItem(QString::number(absVal, 'g', 6)));
        resultTable_->setItem(row, 3, new QTableWidgetItem(
            relRes >= 0 ? QString::number(relRes, 'e', 2) : "N/A"));
        resultTable_->setItem(row, 4, new QTableWidgetItem(
            absRes >= 0 ? QString::number(absRes, 'e', 2) : "N/A"));
        auto *si = new QTableWidgetItem(statusStr);
        si->setForeground(statusColor);
        resultTable_->setItem(row, 5, si);
    }

    updateSummaryBar(metaIndex);
    updateCountLabel();
    if (metaIndex == 0) {
        resultTable_->resizeColumnsToContents();
    }
    mainTab_->setCurrentWidget(tablePage_);
}

// ==================== 摘要栏更新 ====================
void OutputPanel::updateSummaryBar(int metaIndex)
{
    if (metaIndex < 0 || metaIndex >= resultMetas_.size()) return;

    const auto &meta = resultMetas_[metaIndex];
    const auto &res  = meta.result;
    int nEig = static_cast<int>(res.eigenvalues_real.size());

    double maxRel = -1.0;
    int excellentCount = 0, okCount = 0, acceptableCount = 0, warningCount = 0;
    for (size_t i = 0; i < res.residuals_rel.size(); ++i) {
        double r = res.residuals_rel[i];
        if (r > maxRel) maxRel = r;
        if (r < 1e-8)       excellentCount++;
        else if (r < 1e-6)  okCount++;
        else if (r < 1e-4)  acceptableCount++;
        else                warningCount++;
    }

    QString convergeIcon = res.eigensolver_fully_converged
        ? QString::fromUtf8("\xe2\x9c\x93")
        : QString::fromUtf8("\xe2\x9a\xa0");
    QString convergeColor = res.eigensolver_fully_converged
        ? "#4ec9b0" : "#dcdcaa";

    summaryLabel_->setText(
        QString("<span style='color:#569cd6;font-weight:bold;'>%1</span>"
                " &nbsp;|&nbsp;"
                "<span style='color:#cccccc;'>Solver: </span>"
                "<span style='color:#dcdcaa;'>%2</span>"
                " &nbsp;|&nbsp;"
                "<span style='color:#cccccc;'>Eigenvalues: </span>"
                "<span style='color:#b5cea8;'>%3</span>"
                " &nbsp;|&nbsp;"
                "<span style='color:%4;'>%5 Converged</span>"
                " &nbsp;|&nbsp;"
                "<span style='color:#cccccc;'>Max Rel Res: </span>"
                "<span style='color:#ce9178;'>%6</span>"
                " &nbsp;|&nbsp;"
                "<span style='color:#4ec9b0;'>E:%7</span>"
                " <span style='color:#6a9955;'>OK:%8</span>"
                " <span style='color:#dcdcaa;'>AC:%9</span>"
                " <span style='color:#f44747;'>WN:%10</span>")
            .arg(meta.caseName)
            .arg(meta.solverName)
            .arg(nEig)
            .arg(convergeColor)
            .arg(convergeIcon)
            .arg(maxRel >= 0 ? QString::number(maxRel, 'e', 2) : "N/A")
            .arg(excellentCount)
            .arg(okCount)
            .arg(acceptableCount)
            .arg(warningCount));
}

// ==================== 详情填充 ====================
void OutputPanel::fillDetailForRow(int row)
{
    detailText_->clear();
    if (row < 0 || row >= rowToResultIndex_.size())
        return;

    int metaIndex = rowToResultIndex_[row];
    if (metaIndex < 0 || metaIndex >= resultMetas_.size())
        return;

    const auto &meta = resultMetas_[metaIndex];
    const auto &res  = meta.result;

    QString detail;

    // 1. 概览卡片
    detail += QString::fromStdString(
        fmt::summaryCard(meta.caseName.toStdString(),
                         meta.solverName.toStdString(), res));
    detail += "\n\n";

    // 2. 特征值表
    int nEig = static_cast<int>(res.eigenvalues_real.size());
    {
        fmt::TableBuilder eigTbl;
        eigTbl.addCol("#", 3, fmt::Align::Right);
        eigTbl.addCol("Eigenvalue", 28, fmt::Align::Center);
        eigTbl.addCol("|λ|", 14, fmt::Align::Right);
        for (int i = 0; i < nEig; ++i) {
            double re = res.eigenvalues_real(i), im = res.eigenvalues_imag(i);
            char eigBuf[64], absBuf[32];
            if (std::abs(im) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6g", re);
            else if (std::abs(re) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6gi", im);
            else
                snprintf(eigBuf, sizeof(eigBuf), "%.6g%+.6gi", re, im);
            snprintf(absBuf, sizeof(absBuf), "%.6g", std::sqrt(re*re+im*im));
            eigTbl.addRow({std::to_string(i+1), eigBuf, absBuf});
        }
        detail += QString::fromStdString(eigTbl.render(0));
    }
    detail += "\n";

    // 3. 残差详情（使用预计算残差）
    {
        fmt::TableBuilder resTbl;
        resTbl.addCol("#", 3, fmt::Align::Right);
        resTbl.addCol("Eigenvalue", 26, fmt::Align::Center);
        resTbl.addCol("Abs. Residual", 14, fmt::Align::Right);
        resTbl.addCol("Rel. Residual", 14, fmt::Align::Right);
        resTbl.addCol("Status", 12, fmt::Align::Center);
        for (int i = 0; i < nEig; ++i) {
            double re = res.eigenvalues_real(i), im = res.eigenvalues_imag(i);
            char eigBuf[64];
            if (std::abs(im) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6g", re);
            else if (std::abs(re) < 1e-8)
                snprintf(eigBuf, sizeof(eigBuf), "%.6gi", im);
            else
                snprintf(eigBuf, sizeof(eigBuf), "%.6g%+.6gi", re, im);
            double absRes = i < (int)res.residuals_abs.size() ? res.residuals_abs[i] : -1;
            double relRes = i < (int)res.residuals_rel.size() ? res.residuals_rel[i] : -1;
            char absBuf[32], relBuf[32];
            snprintf(absBuf, sizeof(absBuf), "%.2e", absRes);
            snprintf(relBuf, sizeof(relBuf), "%.2e", relRes);
            resTbl.addRow({std::to_string(i+1), eigBuf,
                absRes >= 0 ? absBuf : "N/A",
                relRes >= 0 ? relBuf : "N/A",
                fmt::residualStatus(relRes)});
        }
        detail += QString::fromStdString(resTbl.render(0));
    }
    detail += "\n";

    // 4. 性能统计
    detail += QString::fromStdString(fmt::performanceStats(res));
    detail += "\n";

    // 5. 警告信息
    if (!res.warning_message.empty() || !res.warning_messages.empty()) {
        detail += "\n  === Warnings ===\n";
        if (!res.warning_message.empty())
            detail += QString("  %1\n").arg(
                QString::fromStdString(res.warning_message));
        for (const auto &w : res.warning_messages)
            detail += QString("  %1\n").arg(QString::fromStdString(w));
    }

    detailText_->setPlainText(detail);
}

// ==================== 交互处理 ====================
void OutputPanel::onResultRowClicked(int row, int col)
{
    if (row < 0 || row >= rowToResultIndex_.size()) return;
    if (rowToResultIndex_[row] < 0) return;
    fillDetailForRow(row);
}

void OutputPanel::appendLog(const QString &msg)
{
    textEdit_->moveCursor(QTextCursor::End);
    textEdit_->insertPlainText(msg);
    textEdit_->verticalScrollBar()->setValue(
        textEdit_->verticalScrollBar()->maximum());
}

void OutputPanel::clearLog()
{
    textEdit_->clear();
    resultTable_->setRowCount(0);
    resultMetas_.clear();
    rowToResultIndex_.clear();
    detailText_->clear();
    if (summaryLabel_) {
        summaryLabel_->setText(tr("就绪 — 尚未求解"));
    }
    updateCountLabel();
}

void OutputPanel::updateCountLabel()
{
    countLabel_->setText(tr("共 %1 条记录").arg(resultTable_->rowCount()));
}

void OutputPanel::onClearLog()
{
    auto ret = QMessageBox::question(this, tr("确认清空"),
        tr("确定要清空所有输出日志和结果吗？"),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        clearLog();
    }
}

void OutputPanel::onFilterTextChanged(const QString &text)
{
    QTextCursor cursor = textEdit_->textCursor();
    cursor.select(QTextCursor::Document);
    QTextCharFormat defaultFmt;
    defaultFmt.setForeground(QColor("#d4d4d4"));
    cursor.mergeCharFormat(defaultFmt);

    if (text.isEmpty()) return;

    QTextDocument *doc = textEdit_->document();
    QTextCursor highlight(doc);
    QTextCharFormat highlightFmt;
    highlightFmt.setBackground(QColor("#264f78"));

    while (!highlight.isNull() && !highlight.atEnd()) {
        highlight = doc->find(text, highlight, QTextDocument::FindCaseSensitively);
        if (!highlight.isNull())
            highlight.mergeCharFormat(highlightFmt);
    }
}

// ==================== 保存 / 导出 ====================
void OutputPanel::onSaveOutput()
{
    QString path = QFileDialog::getSaveFileName(
        this, tr("保存输出"), "qep_output.txt",
        tr("文本文件 (*.txt);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        f.write(textEdit_->toPlainText().toUtf8());
        QMessageBox::information(this, tr("保存成功"), tr("已保存到:\n%1").arg(path));
    } else {
        QMessageBox::warning(this, tr("保存失败"), tr("无法写入:\n%1").arg(path));
    }
}

void OutputPanel::onExportJson()
{
    if (resultTable_->rowCount() == 0) {
        QMessageBox::information(this, tr("提示"), tr("没有可导出的结果数据。"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("导出 JSON"), "qep_results.json",
        tr("JSON 文件 (*.json);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入:\n%1").arg(path));
        return;
    }

    f.write("{\n  \"results\": [\n");
    bool first = true;
    for (int row = 0; row < resultTable_->rowCount(); ++row) {
        if (row < rowToResultIndex_.size() && rowToResultIndex_[row] < 0)
            continue; // skip separator rows
        if (!first) f.write(",\n");
        first = false;
        f.write(QString("    {\n"
            "      \"index\": %1,\n"
            "      \"eigenvalue\": \"%2\",\n"
            "      \"abs_lambda\": \"%3\",\n"
            "      \"rel_residual\": \"%4\",\n"
            "      \"abs_residual\": \"%5\",\n"
            "      \"status\": \"%6\"\n"
            "    }")
            .arg(row + 1)
            .arg(resultTable_->item(row, 1)->text())
            .arg(resultTable_->item(row, 2)->text())
            .arg(resultTable_->item(row, 3)->text())
            .arg(resultTable_->item(row, 4)->text())
            .arg(resultTable_->item(row, 5)->text())
            .toUtf8());
    }
    f.write("\n  ]\n}\n");
    f.close();

    QMessageBox::information(this, tr("导出成功"), tr("已导出到:\n%1").arg(path));
}

void OutputPanel::onExportCsv()
{
    if (resultTable_->rowCount() == 0) {
        QMessageBox::information(this, tr("提示"), tr("没有可导出的结果数据。"));
        return;
    }

    QString path = QFileDialog::getSaveFileName(
        this, tr("导出 CSV"), "qep_results.csv",
        tr("CSV 文件 (*.csv);;所有文件 (*)"));
    if (path.isEmpty()) return;

    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("导出失败"), tr("无法写入:\n%1").arg(path));
        return;
    }

    f.write("#,Eigenvalue,AbsLambda,RelResidual,AbsResidual,Status\n");
    for (int row = 0; row < resultTable_->rowCount(); ++row) {
        if (row < rowToResultIndex_.size() && rowToResultIndex_[row] < 0)
            continue;
        auto escape = [](const QString &s) {
            QString copy = s;
            return "\"" + copy.replace("\"", "\"\"") + "\"";
        };
        QStringList fields;
        fields << QString::number(row + 1);
        for (int c = 1; c <= 5; ++c)
            fields << escape(resultTable_->item(row, c)->text());
        f.write(fields.join(',').toUtf8());
        f.write("\n");
    }
    f.close();

    QMessageBox::information(this, tr("导出成功"), tr("已导出到:\n%1").arg(path));
}
