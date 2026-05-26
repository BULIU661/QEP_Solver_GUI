//==============================================================================
//  src/gui/panels/SolverPanel.cpp  —  求解器选择（直接读写 ConfigManager）
//==============================================================================
#include "SolverPanel.h"
#include "config/ConfigManager.h"
#include "config/SolverRegistry.h"
#include <QVBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QFrame>

SolverPanel::SolverPanel(QWidget *parent) : QGroupBox(tr("线性求解器"), parent)
{
    auto *layout = new QVBoxLayout(this);
    bool firstDirect = true, firstIterative = true;
    for (auto &s : Config::kSolverDescriptors) {
        if (s.isDirect && firstDirect) {
            auto *label = new QLabel(tr("— 直接法 —"));
            label->setStyleSheet("color: #888; font-size: 9pt; padding: 0 4px;");
            layout->addWidget(label);
            firstDirect = false;
        }
        if (!s.isDirect && firstIterative) {
            auto *sep = new QFrame(); sep->setFrameShape(QFrame::HLine);
            sep->setStyleSheet("color: #ddd; margin: 4px 0;");
            layout->addWidget(sep);
            auto *lbl = new QLabel(tr("— 迭代法 —"));
            lbl->setStyleSheet("color: #888; font-size: 9pt; padding: 0 4px;");
            layout->addWidget(lbl);
            firstIterative = false;
        }
        auto *cb = new QCheckBox(tr(s.displayName));
        std::string key = s.jsonKey;
        connect(cb, &QCheckBox::toggled, [key](bool v) {
            auto &cfg = Config::ConfigManager::instance();
            cfg.data()["solver"]["enabled_solvers"][key] = v;
            cfg.save();
        });
        layout->addWidget(cb);
        entries_.append({QString::fromUtf8(s.jsonKey), tr(s.displayName),
                         QString::fromUtf8(s.registeredName), cb});
    }
    layout->addStretch();

    // Set initial checkbox states from config
    auto &j = Config::ConfigManager::instance().data();
    auto enabled = j.value("solver", nlohmann::json::object())
                       .value("enabled_solvers", nlohmann::json::object());
    for (auto &e : entries_) {
        e.checkbox->blockSignals(true);
        e.checkbox->setChecked(enabled.value(e.jsonKey.toStdString(), false));
        e.checkbox->blockSignals(false);
    }
}

QStringList SolverPanel::checkedSolvers() const {
    QStringList names;
    for (auto &e : entries_)
        if (e.checkbox->isChecked())
            names.append(e.registeredName);
    return names;
}
