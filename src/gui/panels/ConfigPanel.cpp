//==============================================================================
//  src/gui/panels/ConfigPanel.cpp  —  求解设置、线性求解器、性能优化、文件转换选项卡
//==============================================================================
#include "ConfigPanel.h"
#include "config/ConfigManager.h"
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
#include <nlohmann/json.hpp>

// ==================== 样式定义 ====================
static const char *kCardStyle = R"(
    QFrame#configCard {
        background: #ffffff;
        border: 1px solid #e8e8e8;
        border-radius: 6px;
    }
    QFrame#configCard:hover {
        border-color: #3498db;
    }
)";

static const char *kTabStyle = R"(
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

static const char *kPresetStyle = R"(
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

static const char *kGroupTitleStyle = R"(
    QLabel {
        color: #2c3e50;
        font-weight: bold;
        font-size: 11pt;
        padding: 8px 12px;
        background: #f8f9fa;
        border-bottom: 1px solid #e8e8e8;
    }
)";

static const char *kTooltipStyle = R"(
    QLabel#hintLabel {
        color: #7f8c8d;
        font-size: 9pt;
        padding-left: 4px;
    }
)";

// ==================== 构造函数 ====================
ConfigPanel::ConfigPanel(QWidget *parent)
    : QGroupBox(tr("参数配置"), parent)
{
    setupUI();
    loadPresets();
    setFromConfig();
}

// ==================== 界面布局 ====================
void ConfigPanel::setupUI()
{
    auto *mainLay = new QVBoxLayout(this);
    mainLay->setContentsMargins(4, 4, 4, 4);
    mainLay->setSpacing(8);

    // ----- 预设选择器 -----
    auto *presetBar = new QHBoxLayout();
    presetBar->addWidget(new QLabel(tr("预设配置:")));
    presetCombo_ = new QComboBox();
    presetCombo_->setStyleSheet(kPresetStyle);
    presetCombo_->setMinimumWidth(180);
    connect(presetCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ConfigPanel::onPresetChanged);
    presetBar->addWidget(presetCombo_);
    presetBar->addStretch();
    mainLay->addLayout(presetBar);

    // ----- Tab 控件 -----
    tabWidget_ = new QTabWidget();
    tabWidget_->setStyleSheet(kTabStyle);

    // 第1页：求解设置
    tabWidget_->addTab(makeSolverTabPage(), "求解设置");
    // 第2页：线性求解器
    tabWidget_->addTab(makeLinearSolverTabPage(), "线性求解器");
    // 第3页：性能优化
    tabWidget_->addTab(makePerformanceTabPage(), "性能优化");
    // 第4页：文件转换
    tabWidget_->addTab(makeFileConvertTabPage(), "文件转换");

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
    title->setStyleSheet(kGroupTitleStyle);
    lay->addWidget(title);

    // 参数表单
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
    connect(nevSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(sigmaSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(arnoldiTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(arnoldiIterSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(checkTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);

    return page;
}

QFrame *ConfigPanel::makeLinearSolverTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    auto *title = new QLabel(tr("内层线性求解器参数"));
    title->setStyleSheet(kGroupTitleStyle);
    lay->addWidget(title);

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

    connect(innerTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(innerIterSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(ilutFillSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(ilutDropTolSpin_, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(gmresRestartSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);

    return page;
}

QFrame *ConfigPanel::makePerformanceTabPage()
{
    auto *page = new QFrame();
    auto *lay = new QVBoxLayout(page);
    lay->setContentsMargins(12, 12, 12, 12);
    lay->setSpacing(16);

    auto *title = new QLabel(tr("性能与并行设置"));
    title->setStyleSheet(kGroupTitleStyle);
    lay->addWidget(title);

    auto *form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignRight);
    form->setSpacing(10);
    form->setContentsMargins(0, 12, 0, 0);

    ompSpin_ = makeSpin(1, 256, 12);
    mklSpin_ = makeSpin(1, 256, 12);
    adaptiveCb_ = new QCheckBox(tr("启用自适应参数（根据矩阵条件数自动调整）"));

    // AVX2 是编译期特性，显示当前编译状态，不可运行时切换
#ifdef EIGEN_VECTORIZE_AVX
    useAvxCb_ = new QCheckBox(tr("AVX2 指令集 (编译时已启用)"));
    useAvxCb_->setChecked(true);
#else
    useAvxCb_ = new QCheckBox(tr("AVX2 指令集 (编译时未启用)"));
    useAvxCb_->setChecked(false);
#endif
    useAvxCb_->setEnabled(false);
    useAvxCb_->setToolTip(tr("AVX2 由编译选项控制，无法在运行时切换"));

    form->addRow(tr("OpenMP 线程数:"), ompSpin_);
    form->addRow(tr("MKL 线程数:"), mklSpin_);
    form->addRow("", adaptiveCb_);
    form->addRow("", useAvxCb_);

    lay->addLayout(form);
    lay->addStretch();

    connect(ompSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(mklSpin_, QOverload<int>::of(&QSpinBox::valueChanged), this, &ConfigPanel::updateJSONFromUI);
    connect(adaptiveCb_, &QCheckBox::toggled, this, &ConfigPanel::updateJSONFromUI);
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
    title->setStyleSheet(kGroupTitleStyle);
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

    connect(autoConvertCb_, &QCheckBox::toggled, this, &ConfigPanel::updateJSONFromUI);
    connect(overwriteCb_, &QCheckBox::toggled, this, &ConfigPanel::updateJSONFromUI);

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

// ==================== 预设管理 ====================
void ConfigPanel::loadPresets()
{
    presetCombo_->clear();

    // 预设1：标准
    Preset standard;
    standard.name = "标准";
    standard.description = "适用于大多数问题的默认配置";
    standard.values = {
        {"solver.default_nev", 10},
        {"solver.default_sigma", 0.0},
        {"solver.arnoldi_tolerance", 1e-6},
        {"solver.arnoldi_max_iterations", 1000},
        {"solver.inner_tolerance", 1e-8},
        {"solver.inner_max_iterations", 1000},
        {"solver.enable_adaptive_parameters", false}
    };
    presets_.append(standard);

    // 预设2：高精度
    Preset highPrecision;
    highPrecision.name = "高精度";
    highPrecision.description = "更严格容差，适合病态问题";
    highPrecision.values = {
        {"solver.default_nev", 10},
        {"solver.default_sigma", 0.0},
        {"solver.arnoldi_tolerance", 1e-10},
        {"solver.arnoldi_max_iterations", 2000},
        {"solver.inner_tolerance", 1e-12},
        {"solver.inner_max_iterations", 2000},
        {"solver.enable_adaptive_parameters", true}
    };
    presets_.append(highPrecision);

    // 预设3：快速
    Preset fast;
    fast.name = "快速";
    fast.description = "放宽容差，追求速度";
    fast.values = {
        {"solver.default_nev", 10},
        {"solver.default_sigma", 0.0},
        {"solver.arnoldi_tolerance", 1e-4},
        {"solver.arnoldi_max_iterations", 500},
        {"solver.inner_tolerance", 1e-4},
        {"solver.inner_max_iterations", 300},
        {"solver.enable_adaptive_parameters", false}
    };
    presets_.append(fast);

    // 预设4：自定义（当前配置）
    Preset custom;
    custom.name = "自定义";
    custom.description = "使用当前界面配置的值";
    custom.values = {};
    presets_.append(custom);

    for (const auto &p : presets_) {
        presetCombo_->addItem(p.name + " - " + p.description);
    }
}

void ConfigPanel::onPresetChanged(int index)
{
    if (index < 0 || index >= presets_.size()) return;
    if (index == presets_.size() - 1) return; // 自定义预设不应用

    applyingPreset_ = true;

    const auto &preset = presets_[index];

    // 阻止信号以避免循环
    blockAllSignals(true);

    for (const auto &pair : preset.values) {
        QString key = pair.first;
        QVariant val = pair.second;

        // 解析嵌套键
        QStringList parts = key.split('.');
        if (parts.size() >= 2) {
            auto &j = Config::ConfigManager::instance().data();
            if (parts[0] == "solver" && parts[1] == "default_nev") {
                if (nevSpin_) nevSpin_->setValue(val.toInt());
            } else if (parts[0] == "solver" && parts[1] == "default_sigma") {
                if (sigmaSpin_) sigmaSpin_->setValue(val.toDouble());
            } else if (parts[0] == "solver" && parts[1] == "arnoldi_tolerance") {
                if (arnoldiTolSpin_) arnoldiTolSpin_->setValue(val.toDouble());
            } else if (parts[0] == "solver" && parts[1] == "arnoldi_max_iterations") {
                if (arnoldiIterSpin_) arnoldiIterSpin_->setValue(val.toInt());
            } else if (parts[0] == "solver" && parts[1] == "inner_tolerance") {
                if (innerTolSpin_) innerTolSpin_->setValue(val.toDouble());
            } else if (parts[0] == "solver" && parts[1] == "inner_max_iterations") {
                if (innerIterSpin_) innerIterSpin_->setValue(val.toInt());
            } else if (parts[0] == "solver" && parts[1] == "enable_adaptive_parameters") {
                if (adaptiveCb_) adaptiveCb_->setChecked(val.toBool());
            }
        }
    }

    blockAllSignals(false);
    updateJSONFromUI();
    applyingPreset_ = false;
    emit configChanged();
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
}

// ==================== JSON 同步 ====================
void ConfigPanel::updateJSONFromUI()
{
    // 用户手动修改参数 → 自动切到"自定义"预设
    if (!applyingPreset_ && presetCombo_ && presets_.size() > 0) {
        int customIdx = presets_.size() - 1;
        if (presetCombo_->currentIndex() != customIdx) {
            presetCombo_->blockSignals(true);
            presetCombo_->setCurrentIndex(customIdx);
            presetCombo_->blockSignals(false);
        }
    }

    auto &j = Config::ConfigManager::instance().data();

    // 求解设置
    j["solver"]["default_nev"] = nevSpin_->value();
    j["solver"]["default_sigma"] = sigmaSpin_->value();
    j["solver"]["arnoldi_tolerance"] = arnoldiTolSpin_->value();
    j["solver"]["arnoldi_max_iterations"] = arnoldiIterSpin_->value();
    j["solver"]["check_tolerance"] = checkTolSpin_->value();

    // 线性求解器
    j["solver"]["inner_tolerance"] = innerTolSpin_->value();
    j["solver"]["inner_max_iterations"] = innerIterSpin_->value();
    j["solver"]["ilut_fill_factor"] = ilutFillSpin_->value();
    j["solver"]["ilut_drop_tol"] = ilutDropTolSpin_->value();
    j["solver"]["gmres_restart"] = gmresRestartSpin_->value();

    // 性能
    j["parallel"]["omp_num_threads"] = ompSpin_->value();
    j["parallel"]["mkl_num_threads"] = mklSpin_->value();
    j["solver"]["enable_adaptive_parameters"] = adaptiveCb_->isChecked();

    // 文件转换
    j["binary_conversion"]["auto_convert"] = autoConvertCb_->isChecked();
    j["binary_conversion"]["overwrite"] = overwriteCb_->isChecked();

    emit configChanged();
}

// ==================== 从配置加载 ====================
void ConfigPanel::setFromConfig()
{
    auto &j = Config::ConfigManager::instance().data();

    blockAllSignals(true);

    // 求解设置
    nevSpin_->setValue(j.value("solver", nlohmann::json::object()).value("default_nev", 10));
    sigmaSpin_->setValue(j.value("solver", nlohmann::json::object()).value("default_sigma", 0.0));
    arnoldiTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("arnoldi_tolerance", 1e-6));
    arnoldiIterSpin_->setValue(j.value("solver", nlohmann::json::object()).value("arnoldi_max_iterations", 1000));
    checkTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("check_tolerance", 1e-6));

    // 线性求解器
    innerTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("inner_tolerance", 1e-8));
    innerIterSpin_->setValue(j.value("solver", nlohmann::json::object()).value("inner_max_iterations", 1000));
    ilutFillSpin_->setValue(j.value("solver", nlohmann::json::object()).value("ilut_fill_factor", 10));
    ilutDropTolSpin_->setValue(j.value("solver", nlohmann::json::object()).value("ilut_drop_tol", 1e-4));
    gmresRestartSpin_->setValue(j.value("solver", nlohmann::json::object()).value("gmres_restart", 30));

    // 性能
    ompSpin_->setValue(j.value("parallel", nlohmann::json::object()).value("omp_num_threads", 12));
    mklSpin_->setValue(j.value("parallel", nlohmann::json::object()).value("mkl_num_threads", 12));
    adaptiveCb_->setChecked(j.value("solver", nlohmann::json::object()).value("enable_adaptive_parameters", false));
    // 文件转换
    autoConvertCb_->setChecked(j.value("binary_conversion", nlohmann::json::object()).value("auto_convert", false));
    overwriteCb_->setChecked(j.value("binary_conversion", nlohmann::json::object()).value("overwrite", false));

    blockAllSignals(false);
}