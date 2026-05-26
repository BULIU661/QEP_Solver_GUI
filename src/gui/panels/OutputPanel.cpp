//==============================================================================
//  src/gui/panels/OutputPanel.cpp  —  结果表格、文本日志、详情面板、一键导出
//==============================================================================
#include "OutputPanel.h"
#include "qep/QEP.h"
#include "utils/ResidualValidation.h"
#include "config/TableFormatter.h"
#include "utils/EigenvalueFormatter.h"
#include "styles/StyleConstants.h"
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
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QTextStream>
#include <QHeaderView>
#include <QTabWidget>
#include <QLineEdit>
#include <QLabel>
#include <QFrame>
#include <QColor>
#include <QFont>
#include <cmath>

OutputPanel::OutputPanel(QWidget *parent) : QGroupBox(parent) { setupUI(); }

void OutputPanel::setupUI()
{
    auto *ml = new QVBoxLayout(this); ml->setContentsMargins(8,12,8,8);
    mainTab_ = new QTabWidget(); mainTab_->setStyleSheet(style::kDarkTab);

    tablePage_ = new QWidget(); auto *tl = new QVBoxLayout(tablePage_);
    tl->setContentsMargins(6,6,6,6); setupTablePage(tl);

    logPage_ = new QWidget(); auto *ll = new QVBoxLayout(logPage_);
    ll->setContentsMargins(6,6,6,6); setupLogPage(ll);

    mainTab_->addTab(tablePage_, tr("结果表格"));
    mainTab_->addTab(logPage_, tr("文本输出"));
    ml->addWidget(mainTab_, 1);

    auto *sb = new QHBoxLayout();
    statusLabel_ = new QLabel(tr("就绪"));
    statusLabel_->setStyleSheet("QLabel{color:#6a9955;padding:2px 8px;background:#1e1e1e;border-radius:3px;}");
    countLabel_ = new QLabel(tr("共 0 条记录"));
    countLabel_->setStyleSheet("color:#cccccc;padding:4px;");
    sb->addWidget(statusLabel_); sb->addStretch(); sb->addWidget(countLabel_);
    ml->addLayout(sb);
}

void OutputPanel::setupLogPage(QVBoxLayout *ll)
{
    auto *fb = new QHBoxLayout(); fb->addWidget(new QLabel(tr("过滤:")));
    filterEdit_ = new QLineEdit(); filterEdit_->setPlaceholderText(tr("输入关键字过滤日志..."));
    filterEdit_->setStyleSheet(style::kDarkInput); fb->addWidget(filterEdit_,1); ll->addLayout(fb);

    textEdit_ = new QTextEdit(); textEdit_->setReadOnly(true);
    textEdit_->setFont(QFont("Consolas",9)); textEdit_->setStyleSheet(style::kDarkTextEdit);
    textEdit_->setLineWrapMode(QTextEdit::NoWrap); ll->addWidget(textEdit_,1);

    auto *bl = new QHBoxLayout();
    clearBtn_ = new QPushButton(tr("清空")); clearBtn_->setStyleSheet(style::kDarkBtn);
    saveBtn_ = new QPushButton(tr("保存输出")); saveBtn_->setStyleSheet(style::kDarkExportBtn);
    bl->addWidget(clearBtn_); bl->addWidget(saveBtn_); bl->addStretch(); ll->addLayout(bl);

    connect(clearBtn_, &QPushButton::clicked, this, &OutputPanel::onClearLog);
    connect(saveBtn_, &QPushButton::clicked, this, &OutputPanel::onSaveOutput);
    connect(filterEdit_, &QLineEdit::textChanged, this, &OutputPanel::onFilterTextChanged);
}

void OutputPanel::setupTablePage(QVBoxLayout *tl)
{
    summaryBar_ = new QFrame(); summaryBar_->setObjectName("summaryBar");
    summaryBar_->setStyleSheet("QFrame#summaryBar{background:#252526;border:1px solid #3c3c3c;border-radius:4px;}");
    auto *sl = new QHBoxLayout(summaryBar_); sl->setContentsMargins(12,6,12,6);
    summaryLabel_ = new QLabel(tr("就绪 — 尚未求解"));
    summaryLabel_->setStyleSheet("color:#cccccc;font-family:Consolas;font-size:10pt;");
    summaryLabel_->setTextFormat(Qt::RichText); sl->addWidget(summaryLabel_); sl->addStretch();
    tl->addWidget(summaryBar_);

    auto *tb = new QHBoxLayout(); tb->addStretch();
    exportBtn_ = new QPushButton(tr("导出结果")); exportBtn_->setStyleSheet(style::kDarkExportBtn);
    tb->addWidget(exportBtn_); tl->addLayout(tb);

    tableSplitter_ = new QSplitter(Qt::Vertical); tableSplitter_->setHandleWidth(4);
    tableSplitter_->setChildrenCollapsible(false);

    resultTable_ = new QTableWidget(); resultTable_->setColumnCount(6);
    resultTable_->setHorizontalHeaderLabels({tr("#"),tr("特征值"),tr("|λ|"),tr("相对残差"),tr("绝对残差"),tr("状态")});
    resultTable_->setStyleSheet(style::kDarkTable);
    resultTable_->horizontalHeader()->setStretchLastSection(true);
    resultTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    resultTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    resultTable_->verticalHeader()->setVisible(false);
    resultTable_->setSortingEnabled(false);
    tableSplitter_->addWidget(resultTable_);

    auto *dw = new QWidget(); auto *dl = new QVBoxLayout(dw);
    dl->setContentsMargins(0,4,0,0);
    auto *dlabel = new QLabel(tr("  双击表格行查看该结果集的完整输出"));
    dlabel->setStyleSheet("color:#cccccc;background:#2d2d30;padding:4px 8px;");
    dl->addWidget(dlabel);
    detailText_ = new QTextEdit(); detailText_->setReadOnly(true);
    detailText_->setFont(QFont("Consolas",9)); detailText_->setStyleSheet(style::kDarkTextEdit);
    detailText_->setMinimumHeight(100); dl->addWidget(detailText_);
    tableSplitter_->addWidget(dw); tableSplitter_->setSizes({400,150});
    tl->addWidget(tableSplitter_,1);

    connect(exportBtn_, &QPushButton::clicked, this, &OutputPanel::onExportAll);
    connect(resultTable_, &QTableWidget::cellDoubleClicked, this, &OutputPanel::onResultRowClicked);
}

// ==================== 添加结果 ====================
void OutputPanel::addResult(const QString &caseName, const QString &solverName,
                            std::shared_ptr<const QEP::QuadraticEigenvalueResult> result, double sigma)
{
    int nEig = static_cast<int>(result->eigenvalues_real.size());
    ResultMeta meta; meta.caseName=caseName; meta.solverName=solverName;
    meta.result=result; meta.sigma=sigma;
    int metaIndex = resultMetas_.size(); resultMetas_.append(meta);

    int sepRow = resultTable_->rowCount(); resultTable_->insertRow(sepRow);
    QString convergeText = result->eigensolver_fully_converged ? "converged" : "partial";
    QString st = (nEig <= 0)
        ? QString("—— %1 | %2 —— FAILED").arg(caseName).arg(solverName)
        : QString("—— %1 | %2 —— %3 eig (%4)").arg(caseName).arg(solverName).arg(nEig).arg(convergeText);
    auto *si = new QTableWidgetItem(st); si->setTextAlignment(Qt::AlignCenter);
    si->setBackground(QColor("#2d2d30"));
    si->setForeground(nEig <= 0 ? QColor("#f44747") : QColor("#569cd6"));
    QFont sf = si->font(); sf.setBold(true); sf.setPointSize(9); si->setFont(sf);
    resultTable_->setItem(sepRow,0,si); resultTable_->setSpan(sepRow,0,1,6);
    resultTable_->setRowHeight(sepRow,26);

    if (nEig <= 0) { updateSummaryBar(); updateCountLabel(); return; }

    for (int i=0; i < nEig; ++i) {
        int row = resultTable_->rowCount(); resultTable_->insertRow(row);
        double re=result->eigenvalues_real(i), im=result->eigenvalues_imag(i);
        double av=std::sqrt(re*re+im*im);
        QString eigStr = fmt::formatEigenvalueQt(re, im);
        double absRes=(i<(int)result->residuals_abs.size())?result->residuals_abs[i]:-1.0;
        double relRes=(i<(int)result->residuals_rel.size())?result->residuals_rel[i]:-1.0;
        QString statusStr=QString::fromStdString(fmt::residualStatus(relRes));
        static const QHash<QString,QColor> sc={{"EXCELLENT",QColor("#4ec9b0")},{"OK",QColor("#6a9955")},{"ACCEPTABLE",QColor("#dcdcaa")},{"WARNING",QColor("#f44747")},{"N/A",QColor("#808080")}};
        QColor scolor=sc.value(statusStr,QColor("#808080"));
        auto *ii=new QTableWidgetItem(QString::number(i+1));
        ii->setData(Qt::UserRole,metaIndex); resultTable_->setItem(row,0,ii);
        resultTable_->setItem(row,1,new QTableWidgetItem(eigStr));
        resultTable_->setItem(row,2,new QTableWidgetItem(QString::number(av,'g',6)));
        resultTable_->setItem(row,3,new QTableWidgetItem(relRes>=0?QString::number(relRes,'e',2):"N/A"));
        resultTable_->setItem(row,4,new QTableWidgetItem(absRes>=0?QString::number(absRes,'e',2):"N/A"));
        auto *stItem=new QTableWidgetItem(statusStr); stItem->setForeground(scolor);
        resultTable_->setItem(row,5,stItem);
    }
    updateSummaryBar(); updateCountLabel();
    if (metaIndex==0) resultTable_->resizeColumnsToContents();
    mainTab_->setCurrentWidget(tablePage_);
}

void OutputPanel::updateSummaryBar()
{
    int totalEig=0,converged=0,failed=0; double mr=-1.0;
    for (const auto &m : resultMetas_) {
        const auto &r=*m.result; int n=static_cast<int>(r.eigenvalues_real.size());
        if (n<=0) { failed++; continue; } converged++; totalEig+=n;
        for (size_t i=0;i<r.residuals_rel.size();++i) {
            double v=r.residuals_rel[i]; if(v>mr)mr=v;
        }
    }
    int total=converged+failed; if(total==0) return;
    QString worstStatus = "N/A";
    if (mr >= 0) {
        std::string s = fmt::residualStatus(mr);
        worstStatus = QString::fromStdString(s);
    }
    summaryLabel_->setText(QString("<span style='color:#569cd6;font-weight:bold;'>Results: %1 ok + %2 fail</span>"
        " &nbsp;|&nbsp;<span style='color:#cccccc;'>%3 eigenvalues</span>"
        " &nbsp;|&nbsp;<span style='color:#cccccc;'>Worst: </span>"
        "<span style='color:#ce9178;'>%4</span>"
        " &nbsp;<span style='color:#%5;'>(%6)</span>")
        .arg(converged).arg(failed).arg(totalEig)
        .arg(mr>=0?QString::number(mr,'e',2):"N/A")
        .arg(worstStatus=="EXCELLENT"?"4ec9b0":worstStatus=="OK"?"6a9955":worstStatus=="ACCEPTABLE"?"dcdcaa":"f44747")
        .arg(worstStatus));
}

void OutputPanel::updateCountLabel() { countLabel_->setText(tr("共 %1 条记录").arg(resultTable_->rowCount())); }

// ==================== 一键导出（TXT + JSON + CSV） ====================
void OutputPanel::onExportAll()
{
    if (resultTable_->rowCount() == 0) {
        QMessageBox::information(this, tr("提示"), tr("没有可导出的结果数据。")); return;
    }
    QString base = QCoreApplication::applicationDirPath() + "/../Results";
    QString dir = base + "/" + QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QDir().mkpath(dir);

    // TXT
    {
        QFile f(dir + "/results.txt");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream ts(&f);
            ts << QString("%1 %2 %3 %4 %5 %6\n").arg(tr("#"),-3).arg(tr("特征值"),-28).arg(tr("|λ|"),-14).arg(tr("相对残差"),-12).arg(tr("绝对残差"),-12).arg(tr("状态"),-10);
            ts << QString(80, '-') << "\n";
            for (int row=0; row<resultTable_->rowCount(); ++row) {
                auto *ii=resultTable_->item(row,0);
                if (!ii || ii->data(Qt::UserRole).isNull()) { ts << resultTable_->item(row,0)->text() << "\n"; continue; }
                ts << QString("%1 %2 %3 %4 %5 %6\n")
                    .arg(resultTable_->item(row,0)->text(),-3)
                    .arg(resultTable_->item(row,1)->text(),-28)
                    .arg(resultTable_->item(row,2)->text(),-14)
                    .arg(resultTable_->item(row,3)->text(),-12)
                    .arg(resultTable_->item(row,4)->text(),-12)
                    .arg(resultTable_->item(row,5)->text(),-10);
            }
            f.close();
        }
    }

    // JSON
    {
        QFile f(dir + "/results.json");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write("{\n  \"results\": [\n"); bool first=true;
            for (int row=0; row<resultTable_->rowCount(); ++row) {
                auto *ii=resultTable_->item(row,0);
                if (!ii || ii->data(Qt::UserRole).isNull()) continue;
                if (!first) f.write(",\n"); first=false;
                f.write(QString("    {\"index\":%1,\"eigenvalue\":\"%2\",\"abs_lambda\":\"%3\",\"rel_residual\":\"%4\",\"abs_residual\":\"%5\",\"status\":\"%6\"}")
                    .arg(resultTable_->item(row,0)->text())
                    .arg(resultTable_->item(row,1)->text())
                    .arg(resultTable_->item(row,2)->text())
                    .arg(resultTable_->item(row,3)->text())
                    .arg(resultTable_->item(row,4)->text())
                    .arg(resultTable_->item(row,5)->text()).toUtf8());
            }
            f.write("\n  ]\n}\n"); f.close();
        }
    }

    // CSV
    {
        QFile f(dir + "/results.csv");
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f.write("#,Eigenvalue,AbsLambda,RelResidual,AbsResidual,Status\n");
            for (int row=0; row<resultTable_->rowCount(); ++row) {
                auto *ii=resultTable_->item(row,0);
                if (!ii || ii->data(Qt::UserRole).isNull()) continue;
                f.write(QString("%1,\"%2\",%3,%4,%5,\"%6\"\n")
                    .arg(resultTable_->item(row,0)->text())
                    .arg(resultTable_->item(row,1)->text())
                    .arg(resultTable_->item(row,2)->text())
                    .arg(resultTable_->item(row,3)->text())
                    .arg(resultTable_->item(row,4)->text())
                    .arg(resultTable_->item(row,5)->text()).toUtf8());
            }
            f.close();
        }
    }

    QMessageBox::information(this, tr("导出成功"), tr("已导出到:\n%1").arg(dir));
}

// ==================== 日志/详情 ====================
void OutputPanel::appendLog(const QString &msg) { textEdit_->append(msg); }
void OutputPanel::onFilterTextChanged(const QString &kw) { /* filter textEdit_ by keyword */ }
void OutputPanel::onSaveOutput() {
    QString p = QFileDialog::getSaveFileName(this, tr("保存日志"), "output.txt", tr("文本文件 (*.txt)"));
    if (p.isEmpty()) return;
    QFile f(p); if (f.open(QIODevice::WriteOnly|QIODevice::Text)) { f.write(textEdit_->toPlainText().toUtf8()); f.close(); }
}

void OutputPanel::clearLog() {
    textEdit_->clear(); resultTable_->setRowCount(0); resultMetas_.clear(); detailText_->clear();
    summaryLabel_->setText(tr("就绪 — 尚未求解")); updateCountLabel();
}
void OutputPanel::onClearLog() { clearLog(); }

void OutputPanel::onResultRowClicked(int row, int) {
    auto *item = resultTable_->item(row, 0); if (!item || item->data(Qt::UserRole).isNull()) return;
    int mi = item->data(Qt::UserRole).toInt();
    if (mi < 0 || mi >= resultMetas_.size()) return;
    const auto &meta = resultMetas_[mi]; const auto &res = *meta.result;
    QString detail;
    int nEig = static_cast<int>(res.eigenvalues_real.size());

    // 1. 问题求解概要
    detail += QString::fromStdString(fmt::summaryCard(meta.caseName.toStdString(), meta.solverName.toStdString(), res));
    detail += "\n";

    // 2. 结果表格：编号 | 特征值 | |λ| | 绝对残差 | 相对残差 | 状态
    fmt::TableBuilder tbl;
    tbl.addCol("#", 3, fmt::Align::Right);
    tbl.addCol("Eigenvalue", 28, fmt::Align::Center);
    tbl.addCol("|λ|", 14, fmt::Align::Right);
    tbl.addCol("Abs. Residual", 14, fmt::Align::Right);
    tbl.addCol("Rel. Residual", 14, fmt::Align::Right);
    tbl.addCol("Status", 12, fmt::Align::Center);
    for (int i = 0; i < nEig; ++i) {
        double re = res.eigenvalues_real(i), im = res.eigenvalues_imag(i);
        double av = std::sqrt(re*re + im*im);
        std::string es = fmt::formatEigenvalue(re, im);
        double absRes = (i < (int)res.residuals_abs.size()) ? res.residuals_abs[i] : -1.0;
        double relRes = (i < (int)res.residuals_rel.size()) ? res.residuals_rel[i] : -1.0;
        char absBuf[32], relBuf[32], absVal[32];
        snprintf(absVal, sizeof(absVal), "%.6g", av);
        snprintf(absBuf, sizeof(absBuf), "%.2e", absRes);
        snprintf(relBuf, sizeof(relBuf), "%.2e", relRes);
        tbl.addRow({std::to_string(i+1), es, absVal,
            absRes >= 0 ? absBuf : "N/A",
            relRes >= 0 ? relBuf : "N/A",
            fmt::residualStatus(relRes)});
    }
    detail += QString::fromStdString(fmt::subHdr("Result Table") + "\n");
    detail += QString::fromStdString(tbl.render(0));
    detail += "\n";

    // 3. 求解细节（性能统计 + 日志）
    detail += QString::fromStdString(fmt::performanceStats(res));
    if (!res.log_messages.empty()) {
        detail += "\n" + QString::fromStdString(fmt::subHdr("Solver Log") + "\n");
        for (const auto &log : res.log_messages) {
            std::string trimmed = log;
            while (!trimmed.empty() && trimmed.back() == '\n') trimmed.pop_back();
            detail += "  " + QString::fromStdString(trimmed) + "\n";
        }
    }

    detailText_->setPlainText(detail);
}
