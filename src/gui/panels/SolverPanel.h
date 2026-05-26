//==============================================================================
//  src/gui/panels/SolverPanel.h  —  求解器选择面板（新架构：ViewModel 驱动）
//==============================================================================
#ifndef QEP_GUI_SOLVER_PANEL_H
#define QEP_GUI_SOLVER_PANEL_H

#include <QGroupBox>
#include <QList>

class QCheckBox;
class SolverPanel : public QGroupBox {
    Q_OBJECT
public:
    explicit SolverPanel(QWidget *parent = nullptr);
    QStringList checkedSolvers() const;

private:
    struct SolverEntry {
        QString jsonKey, displayName, registeredName;
        QCheckBox *checkbox;
    };
    QList<SolverEntry> entries_;
};

#endif
