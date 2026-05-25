//==============================================================================
//  src/gui/panels/ConfigPanel.h  —  参数配置面板接口
//==============================================================================
#ifndef QEP_GUI_CONFIG_PANEL_H
#define QEP_GUI_CONFIG_PANEL_H

#include <QGroupBox>
#include <QTabWidget>
#include <QVector>

class QFormLayout;
class QFrame;
class QSpinBox;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QLabel;

class ConfigPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit ConfigPanel(QWidget *parent = nullptr);
    void setFromConfig();

signals:
    void configChanged();

private slots:
    void onPresetChanged(int index);
    void updateJSONFromUI();

private:
    void setupUI();
    QFrame *makeCard(const QString &title);
    QFrame *makeTabPage(const QString &title, const QString &icon);
    QFrame *makeSolverTabPage();
    QFrame *makeLinearSolverTabPage();
    QFrame *makePerformanceTabPage();
    QFrame *makeFileConvertTabPage();
    QSpinBox *makeSpin(int min, int max, int val);
    QDoubleSpinBox *makeDoubleSpin(double min, double max, double val, int decimals);
    void addLabeledControl(QFormLayout *form, const QString &label, QWidget *control, const QString &tooltip = "");
    void blockAllSignals(bool block);

    // 参数预设
    void loadPresets();
    void applyPreset(int presetIndex);

    // UI 组件
    QTabWidget *tabWidget_ = nullptr;

    // 预设选择
    QComboBox *presetCombo_ = nullptr;

    // 求解设置页
    QSpinBox *nevSpin_ = nullptr;
    QDoubleSpinBox *sigmaSpin_ = nullptr;
    QDoubleSpinBox *arnoldiTolSpin_ = nullptr;
    QSpinBox *arnoldiIterSpin_ = nullptr;
    QDoubleSpinBox *checkTolSpin_ = nullptr;

    // 线性求解器页
    QDoubleSpinBox *innerTolSpin_ = nullptr;
    QSpinBox *innerIterSpin_ = nullptr;
    QSpinBox *ilutFillSpin_ = nullptr;
    QDoubleSpinBox *ilutDropTolSpin_ = nullptr;
    QSpinBox *gmresRestartSpin_ = nullptr;

    // 性能优化页
    QSpinBox *ompSpin_ = nullptr;
    QSpinBox *mklSpin_ = nullptr;
    QCheckBox *adaptiveCb_ = nullptr;
    QCheckBox *useAvxCb_ = nullptr;

    // 二进制转换页
    QCheckBox *autoConvertCb_ = nullptr;
    QCheckBox *overwriteCb_ = nullptr;

    // 预设定义
    struct Preset {
        QString name;
        QString description;
        QVector<QPair<QString, QVariant>> values;
    };
    QVector<Preset> presets_;
    bool applyingPreset_ = false;
};

#endif // QEP_GUI_CONFIG_PANEL_H