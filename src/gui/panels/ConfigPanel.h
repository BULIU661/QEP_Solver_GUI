//==============================================================================
//  src/gui/panels/ConfigPanel.h  —  参数配置面板接口（新架构：ViewModel 驱动）
//==============================================================================
#ifndef QEP_GUI_CONFIG_PANEL_H
#define QEP_GUI_CONFIG_PANEL_H

#include <QGroupBox>
#include <QTabWidget>
#include <QVector>

class QFormLayout;
class QFrame;
class QSpinBox; class QDoubleSpinBox; class QCheckBox; class QComboBox; class QLabel;
class ConfigPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit ConfigPanel(QWidget *parent = nullptr);

signals:
    void configChanged();

private slots:
    void onFieldChanged();
    void loadFromConfig();

private:
    void setupUI();
    QFrame *makeSolverTabPage();
    QFrame *makeLinearSolverTabPage();
    QFrame *makePerformanceTabPage();
    QFrame *makeFileConvertTabPage();
    QFrame *makeLoggingTabPage();
    QSpinBox *makeSpin(int min, int max, int val);
    QDoubleSpinBox *makeDoubleSpin(double min, double max, double val, int decimals);
    void blockAllSignals(bool block);

    QTabWidget *tabWidget_ = nullptr;

    QSpinBox *nevSpin_ = nullptr;
    QDoubleSpinBox *sigmaSpin_ = nullptr, *arnoldiTolSpin_ = nullptr, *checkTolSpin_ = nullptr;
    QSpinBox *arnoldiIterSpin_ = nullptr;
    QDoubleSpinBox *innerTolSpin_ = nullptr, *ilutDropTolSpin_ = nullptr;
    QSpinBox *innerIterSpin_ = nullptr, *ilutFillSpin_ = nullptr, *gmresRestartSpin_ = nullptr;
    QSpinBox *ompSpin_ = nullptr, *mklSpin_ = nullptr;
    QCheckBox *adaptiveCb_ = nullptr;
    QCheckBox *autoConvertCb_ = nullptr, *overwriteCb_ = nullptr;
    QCheckBox *printEigCb_ = nullptr, *printResCb_ = nullptr, *summaryCb_ = nullptr;
    QCheckBox *propCheckCb_ = nullptr, *condEstCb_ = nullptr, *solverDetailCb_ = nullptr;

    bool applyingPreset_ = false;
};

#endif
