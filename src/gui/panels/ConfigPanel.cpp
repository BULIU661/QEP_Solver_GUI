//==============================================================================
//  src/gui/panels/ConfigPanel.cpp  —  求解设置、线性求解器、性能优化、文件转换选项卡
//  所有权审计: QWidget 子控件通过 parent 参数或 layout->addWidget 管理生命周期. PASSED.
//==============================================================================
#include "ConfigPanel.h"
#include "config/ConfigManager.h"
#include "styles/StyleConstants.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QFrame>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QMessageBox>
#include <QCoreApplication>
#include <QDir>
#include <nlohmann/json.hpp>

// ==================== 构造函数 ====================
ConfigPanel::ConfigPanel(QWidget *parent)
    : QGroupBox(parent)
{
    applyingPreset_ = true;
    setupUI();
    blockAllSignals(true);
    loadFromConfig();
    blockAllSignals(false);
    applyingPreset_ = false;
}

// ==================== 界面布局 ====================
void ConfigPanel::setupUI()
{
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(4, 4, 4, 4);
    mainLay->setSpacing(8);

    // ----- Tab 控件 -----
    tabWidget_ = new QTabWidget();
    tabWidget_->setStyleSheet(style::kConfigTabStyle);

    // 第1页：求解设置
    tabWidget_->addTab(makeSolverTabPage(), "求解设置");
    // 第2页：线性求解器
    tabWidget_->addTab(makeLinearSolverTabPage(), "线性求解器");
    // 第3页：性能优化
    tabWidget_->addTab(makePerformanceTabPage(), "性能优化");
    // 第4页：文件转换
    tabWidget_->addTab(makeFileConvertTabPage(), "文件转换");
    tabWidget_->addTab(makeLoggingTabPage(), "日志诊断");

    mainLay->addWidget(tabWidget_, 1);
}

// ==================== 各 Tab 页面 ====================
QFrame *ConfigPanel::makeSolverTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    // 标题
    auto *title = new QLabel(tr("外层特征值求解参数"));
    title->setStyleSheet(style::kGroupTitleStyle);
    lay->addWidget(title);
    auto *desc = new QLabel(tr("配置 Arnoldi 特征值求解器的外层参数，包括求解个数、位移点和收敛容差。"));
    desc->setWordWrap(true); desc->setStyleSheet("color: #7f8c8d; padding: 4px 8px;");
    lay->addWidget(desc);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->setContentsMargins(0, 12, 0, 0);

    nevSpin_ = makeSpin(1, 500, 10);
    sigmaSpin_ = makeDoubleSpin(-1e6, 1e6, 0.0, 6);
    arnoldiTolSpin_ = makeDoubleSpin(1e-12, 1e-2, 1e-6, 10);
    arnoldiIterSpin_ = makeSpin(10, 10000, 1000);
    checkTolSpin_ = makeDoubleSpin(1e-12, 1e-2, 1e-6, 10);

    form->addRow(tr("特征值个数 (NEV):"), nevSpin_);
    form->addRow(tr("位移点 (σ):"), sigmaSpin_);
    form->addRow(tr("Arnoldi 容差:"), arnoldiTolSpin_);
    form->addRow(tr("Arnoldi 最大迭代:"), arnoldiIterSpin_);
    form->addRow(tr("残差检验容差:"), checkTolSpin_);

    lay->addLayout(form);
    lay->addStretch();

    // 统一信号连接
    connect(nevSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(sigmaSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(arnoldiTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(arnoldiIterSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(checkTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);

    return page;
}

QFrame *ConfigPanel::makeLinearSolverTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    auto *title = new QLabel(tr("内层线性求解器参数"));
    title->setStyleSheet(style::kGroupTitleStyle);
    lay->addWidget(title);
    auto *desc = new QLabel(tr("配置位移-逆矩阵的内层线性求解参数，直接影响每次 Arnoldi 迭代的速度与精度。"));
    desc->setWordWrap(true); desc->setStyleSheet("color: #7f8c8d; padding: 4px 8px;");
    lay->addWidget(desc);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->setContentsMargins(0, 12, 0, 0);

    innerTolSpin_ = makeDoubleSpin(1e-15, 1e-2, 1e-8, 10);
    innerIterSpin_ = makeSpin(1, 10000, 1000);
    ilutFillSpin_ = makeSpin(0, 100, 10);
    ilutDropTolSpin_ = makeDoubleSpin(1e-10, 1e-2, 1e-4, 8);
    gmresRestartSpin_ = makeSpin(5, 500, 30);

    form->addRow(tr("内层容差:"), innerTolSpin_);
    form->addRow(tr("内层最大迭代:"), innerIterSpin_);
    form->addRow(tr("ILUT 填充因子:"), ilutFillSpin_);
    form->addRow(tr("ILUT 丢弃容差:"), ilutDropTolSpin_);
    form->addRow(tr("GMRES 重启步数:"), gmresRestartSpin_);

    lay->addLayout(form);
    lay->addStretch();

    connect(innerTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(innerIterSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(ilutFillSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(ilutDropTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(gmresRestartSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);

    return page;
}

QFrame *ConfigPanel::makePerformanceTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    auto *title = new QLabel(tr("性能与并行设置"));
    title->setStyleSheet(style::kGroupTitleStyle);
    lay->addWidget(title);
    auto *desc = new QLabel(tr("控制 OpenMP/MKL 线程数以优化多核计算性能，可启用自适应参数根据矩阵条件数自动调整求解策略。"));
    desc->setWordWrap(true); desc->setStyleSheet("color: #7f8c8d; padding: 4px 8px;");
    lay->addWidget(desc);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->setContentsMargins(0, 12, 0, 0);

    ompSpin_ = makeSpin(1, 256, 12);
    mklSpin_ = makeSpin(1, 256, 12);
    adaptiveCb_ = new QCheckBox(tr("启用自适应参数（根据矩阵条件数自动调整）（未完善）"));

    form->addRow(tr("OpenMP 线程数:"), ompSpin_);
    form->addRow(tr("MKL 线程数:"), mklSpin_);
    form->addRow("", adaptiveCb_);

    lay->addLayout(form);
    lay->addStretch();

    connect(ompSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(mklSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::onFieldChanged);
    connect(adaptiveCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    // 自适应参数逻辑：启用时禁用手动调节的内层参数
    connect(adaptiveCb_, &QCheckBox::toggled, [this](bool checked) {
        innerTolSpin_->setEnabled(!checked);
        innerIterSpin_->setEnabled(!checked);
        ilutFillSpin_->setEnabled(!checked);
        ilutDropTolSpin_->setEnabled(!checked);
        gmresRestartSpin_->setEnabled(!checked);
    });

    return page;
}

QFrame *ConfigPanel::makeFileConvertTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    auto *title = new QLabel(tr("文件格式转换"));
    title->setStyleSheet(style::kGroupTitleStyle);
    lay->addWidget(title);

    auto *desc = new QLabel(
        tr("自动将文本格式 (.txt) 的矩阵文件转换为二进制格式 (.bin)，")
        + "\n" +
        tr("二进制格式读取速度更快，适合大规模问题。"));
    desc->setWordWrap(true);
    desc->setStyleSheet("color: #7f8c8d; padding: 8px;");
    lay->addWidget(desc);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->setContentsMargins(0, 12, 0, 0);

    autoConvertCb_ = new QCheckBox(tr("自动转换 .txt → .bin"));
    overwriteCb_ = new QCheckBox(tr("覆盖已存在的 .bin 文件"));

    form->addRow("", autoConvertCb_);
    form->addRow("", overwriteCb_);

    lay->addLayout(form);
    lay->addStretch();

    connect(autoConvertCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(overwriteCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);

    return page;
}

QFrame *ConfigPanel::makeLoggingTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12); lay->setSpacing(16);
    auto *title = new QLabel(tr("日志与诊断设置"));
    title->setStyleSheet(style::kGroupTitleStyle);
    lay->addWidget(title);
    auto *desc = new QLabel(tr("控制求解过程中输出哪些信息到文本日志面板，包括特征值表、残差验证表、汇总报告及求解器内部详细日志。"));
    desc->setWordWrap(true); desc->setStyleSheet("color: #7f8c8d; padding: 4px 8px;");
    lay->addWidget(desc);
    auto *form = new QFormLayout();
    form->setSpacing(10); form->setContentsMargins(0, 12, 0, 0);
    printEigCb_ = new QCheckBox(tr("打印特征值表"));
    printResCb_ = new QCheckBox(tr("打印残差验证表"));
    summaryCb_ = new QCheckBox(tr("启用汇总报告"));
    propCheckCb_ = new QCheckBox(tr("启用矩阵属性检查（未完善）"));
    condEstCb_ = new QCheckBox(tr("启用条件数估计（未完善）"));
    solverDetailCb_ = new QCheckBox(tr("打印求解器详细日志"));
    form->addRow("", printEigCb_); form->addRow("", printResCb_);
    form->addRow("", summaryCb_); form->addRow("", propCheckCb_);
    form->addRow("", condEstCb_); form->addRow("", solverDetailCb_);
    lay->addLayout(form); lay->addStretch();
    connect(printEigCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(printResCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(summaryCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(propCheckCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(condEstCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    connect(solverDetailCb_, &QCheckBox::toggled, this, &ConfigPanel::onFieldChanged);
    return page;
}

// ==================== 辅助函数 ====================
QSpinBox *ConfigPanel::makeSpin(int min, int max, int val)
{
    auto *s = new QSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    s->setAlignment(Qt::AlignRight);
    return s;
}

QDoubleSpinBox *ConfigPanel::makeDoubleSpin(double min, double max, double val, int decimals)
{
    auto *s = new QDoubleSpinBox();
    s->setRange(min, max);
    s->setValue(val);
    s->setDecimals(decimals);
    s->setAlignment(Qt::AlignRight);
    return s;
}

void ConfigPanel::blockAllSignals(bool block)
{
    if (nevSpin_) nevSpin_->blockSignals(block);
    if (sigmaSpin_) sigmaSpin_->blockSignals(block);
    if (arnoldiTolSpin_) arnoldiTolSpin_->blockSignals(block);
    if (arnoldiIterSpin_) arnoldiIterSpin_->blockSignals(block);
    if (checkTolSpin_) checkTolSpin_->blockSignals(block);
    if (innerTolSpin_) innerTolSpin_->blockSignals(block);
    if (innerIterSpin_) innerIterSpin_->blockSignals(block);
    if (ilutFillSpin_) ilutFillSpin_->blockSignals(block);
    if (ilutDropTolSpin_) ilutDropTolSpin_->blockSignals(block);
    if (gmresRestartSpin_) gmresRestartSpin_->blockSignals(block);
    if (ompSpin_) ompSpin_->blockSignals(block);
    if (mklSpin_) mklSpin_->blockSignals(block);
    if (adaptiveCb_) adaptiveCb_->blockSignals(block);
    if (autoConvertCb_) autoConvertCb_->blockSignals(block);
    if (overwriteCb_) overwriteCb_->blockSignals(block);
    if (printEigCb_) printEigCb_->blockSignals(block);
    if (printResCb_) printResCb_->blockSignals(block);
    if (summaryCb_) summaryCb_->blockSignals(block);
    if (propCheckCb_) propCheckCb_->blockSignals(block);
    if (condEstCb_) condEstCb_->blockSignals(block);
    if (solverDetailCb_) solverDetailCb_->blockSignals(block);
}

// ==================== 直接读写 ConfigManager（与 CLI 共享底层） ====================
void ConfigPanel::onFieldChanged()
{
    if (applyingPreset_) return;
    auto &j = Config::ConfigManager::instance().data();

    j["solver"]["default_nev"] = nevSpin_->value();
    j["solver"]["default_sigma"] = sigmaSpin_->value();
    j["solver"]["arnoldi_tolerance"] = arnoldiTolSpin_->value();
    j["solver"]["arnoldi_max_iterations"] = arnoldiIterSpin_->value();
    j["solver"]["check_tolerance"] = checkTolSpin_->value();
    j["solver"]["inner_tolerance"] = innerTolSpin_->value();
    j["solver"]["inner_max_iterations"] = innerIterSpin_->value();
    j["solver"]["ilut_fill_factor"] = ilutFillSpin_->value();
    j["solver"]["ilut_drop_tol"] = ilutDropTolSpin_->value();
    j["solver"]["gmres_restart"] = gmresRestartSpin_->value();
    j["solver"]["enable_adaptive_parameters"] = adaptiveCb_->isChecked();
    j["parallel"]["omp_num_threads"] = ompSpin_->value();
    j["parallel"]["mkl_num_threads"] = mklSpin_->value();
    j["binary_conversion"]["auto_convert"] = autoConvertCb_->isChecked();
    j["binary_conversion"]["overwrite"] = overwriteCb_->isChecked();
    j["logging"]["print_eigenvalues"] = printEigCb_->isChecked();
    j["logging"]["print_residuals"] = printResCb_->isChecked();
    j["logging"]["enable_summary_report"] = summaryCb_->isChecked();
    j["logging"]["enable_matrix_property_check"] = propCheckCb_->isChecked();
    j["logging"]["enable_condition_estimation"] = condEstCb_->isChecked();
    j["logging"]["print_solver_details"] = solverDetailCb_->isChecked();

    Config::ConfigManager::instance().save();
    emit configChanged();
}

void ConfigPanel::loadFromConfig()
{
    auto &j = Config::ConfigManager::instance().data();

    nevSpin_->setValue(j.value("solver", nlohmann::json::object()).value("default_nev", 10));
    sigmaSpin_->setValue(j.value("solver", nlohmann::json::object()).value("default_sigma", 0.0));
    arnoldiTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("arnoldi_tolerance", 1e-6));
    arnoldiIterSpin_->setValue(j.value("solver", nlohmann::json::object()).value("arnoldi_max_iterations", 1000));
    checkTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("check_tolerance", 1e-6));
    innerTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("inner_tolerance", 1e-8));
    innerIterSpin_->setValue(j.value("solver", nlohmann::json::object()).value("inner_max_iterations", 1000));
    ilutFillSpin_->setValue(j.value("solver", nlohmann::json::object()).value("ilut_fill_factor", 10));
    ilutDropTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("ilut_drop_tol", 1e-4));
    gmresRestartSpin_->setValue(j.value("solver", nlohmann::json::object()).value("gmres_restart", 30));
    ompSpin_->setValue(j.value("parallel", nlohmann::json::object()).value("omp_num_threads", 12));
    mklSpin_->setValue(j.value("parallel", nlohmann::json::object()).value("mkl_num_threads", 12));
    adaptiveCb_->setChecked(j.value("solver", nlohmann::json::object()).value("enable_adaptive_parameters", false));
    autoConvertCb_->setChecked(j.value("binary_conversion", nlohmann::json::object()).value("auto_convert", false));
    overwriteCb_->setChecked(j.value("binary_conversion", nlohmann::json::object()).value("overwrite", false));
    auto lg = j.value("logging", nlohmann::json::object());
    printEigCb_->setChecked(lg.value("print_eigenvalues", true));
    printResCb_->setChecked(lg.value("print_residuals", true));
    summaryCb_->setChecked(lg.value("enable_summary_report", false));
    propCheckCb_->setChecked(lg.value("enable_matrix_property_check", false));
    condEstCb_->setChecked(lg.value("enable_condition_estimation", false));
    solverDetailCb_->setChecked(lg.value("print_solver_details", false));
}