//==============================================================================
//  src/gui/panels/SolverPanel.cpp  —  直接法/迭代法求解器复选框
//==============================================================================

#include "SolverPanel.h"
#include "config/ConfigManager.h"
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

SolverPanel::SolverPanel(QWidget *parent)
    : QGroupBox(tr("线性求解器"), parent)
{
    auto *layout = new QVBoxLayout(this);

    struct { QString key; QString display; QString registered; bool isDirect; } solvers[] = {
        {"PardisoLU",          tr("PardisoLU 直接法"),     "Pardiso直接法",     true},
        {"SparseLU",           tr("SparseLU 直接法"),       "LU直接法",          true},
        {"SimplicialLLT",      tr("SimplicialLLT 直接法"),  "LLT直接法",         true},
        {"ConjugateGradient",  tr("ConjugateGradient 迭代法"), "CG迭代法",      false},
        {"BiCGSTAB",           tr("BiCGSTAB 迭代法"),       "BiCGSTAB迭代法",     false},
        {"GMRES",              tr("GMRES 迭代法"),          "GMRES迭代法",        false}
    };

    bool firstIterative = true;
    for (auto &s : solvers) {
        if (!s.isDirect && firstIterative) {
            auto *sep = new QFrame();
            sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet("color: #ddd; margin: 4px 0;");
            layout->addWidget(sep);
            auto *label = new QLabel(tr("— 迭代法 —"));
            label->setStyleSheet("color: #888; font-size: 9pt; padding: 0 4px;");
            layout->addWidget(label);
            firstIterative = false;
        }
        auto *cb = new QCheckBox(s.display);
        connect(cb, &QCheckBox::toggled, [key = s.key](bool v) {
            auto &cfg = Config::ConfigManager::instance();
            cfg.data()["solver"]["enabled_solvers"][key.toStdString()] = v;
            cfg.save();
        });
        layout->addWidget(cb);
        entries_.append({s.key, s.display, s.registered, cb});
    }
    layout->addStretch();
}

void SolverPanel::setFromConfig()
{
    auto &j = Config::ConfigManager::instance().data();
    auto enabled = j.value("solver", nlohmann::json::object())
                       .value("enabled_solvers", nlohmann::json::object());

    for (auto &e : entries_) {
        e.checkbox->blockSignals(true);
        bool val = enabled.value(e.jsonKey.toStdString(), false);
        e.checkbox->setChecked(val);
        e.checkbox->blockSignals(false);
    }
}

QStringList SolverPanel::checkedSolvers() const
{
    QStringList names;
    for (auto &e : entries_) {
        if (e.checkbox->isChecked())
            names.append(e.registeredName);
    }
    return names;
}
