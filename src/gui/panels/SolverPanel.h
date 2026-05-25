//==============================================================================
//  src/gui/panels/SolverPanel.h  —  线性求解器选择面板接口
//==============================================================================

#ifndef QEP_GUI_SOLVER_PANEL_H
#define QEP_GUI_SOLVER_PANEL_H

#include <QGroupBox>

class QCheckBox;

class SolverPanel : public QGroupBox {
    Q_OBJECT

public:
    explicit SolverPanel(QWidget *parent = nullptr);
    void setFromConfig();
    QStringList checkedSolvers() const;

private:
    struct SolverEntry {
        QString jsonKey;
        QString displayName;
        QString registeredName;
        QCheckBox *checkbox;
    };
    QList<SolverEntry> entries_;
};

#endif
