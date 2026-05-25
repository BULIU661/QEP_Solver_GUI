//==============================================================================
//  src/gui/panels/ProblemTreePanel.cpp  —  目录树、右键菜单、文件监控
//==============================================================================

#include "ProblemTreePanel.h"
#include "config/ConfigManager.h"
#include "qep/QEP.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QDialog>
#include <QFormLayout>
#include <QComboBox>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QItemSelectionModel>
#include <QSet>
#include <QFileSystemWatcher>
#include <QTimer>
#include <filesystem>

ProblemTreePanel::ProblemTreePanel(QWidget *parent)
    : QGroupBox(tr("测试问题"), parent)
{
    setAcceptDrops(true);
    setupUI();
}

void ProblemTreePanel::setupUI()
{
    auto *layout = new QVBoxLayout(this);

    model_ = new QStandardItemModel(this);
    rootItem_ = model_->invisibleRootItem();

    tree_ = new QTreeView();
    tree_->setModel(model_);
    tree_->setHeaderHidden(true);
    tree_->setContextMenuPolicy(Qt::CustomContextMenu);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setAnimated(true);
    tree_->setIndentation(18);

    auto *btnLayout = new QHBoxLayout();
    refreshBtn_ = new QPushButton(tr("刷新"));
    refreshBtn_->setMaximumWidth(80);
    btnLayout->addWidget(refreshBtn_);
    btnLayout->addStretch();

    layout->addWidget(tree_);
    layout->addLayout(btnLayout);

    connect(tree_, &QTreeView::customContextMenuRequested,
            this, &ProblemTreePanel::onContextMenu);
    connect(refreshBtn_, &QPushButton::clicked, this, [this]() {
        scanProblems();
        emit statusMessage(tr("列表已刷新"));
    });

    // 文件系统监控：目录变化时自动刷新
    watcher_ = new QFileSystemWatcher(this);
    refreshTimer_ = new QTimer(this);
    refreshTimer_->setSingleShot(true);
    connect(refreshTimer_, &QTimer::timeout, this, &ProblemTreePanel::scanProblems);
    connect(watcher_, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        refreshTimer_->start(500);  // 500ms 防抖
    });
}

QString ProblemTreePanel::problemBasePath() const
{
    return QString::fromStdString(
        Config::ConfigManager::instance().problemBasePath());
}

QString ProblemTreePanel::problemDirPath(const QStandardItem *item) const
{
    auto *parent = item->parent();
    if (!parent) return {};
    QString base = problemBasePath();
    // Case B: 子目录结构 base/category/problem/
    QString subDir = base + "/" + parent->text() + "/" + item->text();
    QDir sd(subDir);
    if (sd.exists() && !sd.entryList({"M.*", "C.*", "K.*"}, QDir::Files).isEmpty())
        return subDir;
    // Case A: 扁平结构 base/category/
    QString flatDir = base + "/" + parent->text();
    QDir fd(flatDir);
    if (fd.exists() && !fd.entryList({"M.*", "C.*", "K.*"}, QDir::Files).isEmpty())
        return flatDir;
    return subDir;
}

QStandardItem *ProblemTreePanel::addCategoryItem(const QString &name)
{
    auto *item = new QStandardItem(name);
    item->setFlags(Qt::ItemIsEnabled);
    item->setEditable(false);
    rootItem_->appendRow(item);
    return item;
}

QStandardItem *ProblemTreePanel::addProblemItem(QStandardItem *parent,
    const QString &name, const QStringList &files)
{
    auto *item = new QStandardItem(name);
    item->setCheckable(true);
    item->setCheckState(Qt::Unchecked);
    item->setEditable(false);
    item->setToolTip(files.join(", "));
    parent->appendRow(item);
    return item;
}

static bool hasAllThree(QDir &dir, const QString &ext)
{
    return dir.exists("M." + ext) && dir.exists("C." + ext) && dir.exists("K." + ext);
}

void ProblemTreePanel::scanProblems()
{
    // 保存当前勾选状态
    QSet<QString> checkedNames;
    for (int ci = 0; ci < rootItem_->rowCount(); ++ci) {
        auto *catItem = rootItem_->child(ci);
        for (int pi = 0; pi < catItem->rowCount(); ++pi) {
            auto *probItem = catItem->child(pi);
            if (probItem->checkState() == Qt::Checked)
                checkedNames.insert(probItem->text());
        }
    }

    populating_ = true;
    model_->clear();
    rootItem_ = model_->invisibleRootItem();

    QString base = problemBasePath();
    QDir baseDir(base);
    if (!baseDir.exists()) {
        populating_ = false;
        emit statusMessage(tr("未找到 Problems 目录"));
        return;
    }

    int totalProblems = 0;
    auto catDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    for (auto &cat : catDirs) {
        QDir catDir(base + "/" + cat);
        auto problemDirs = catDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);

        auto *catItem = addCategoryItem(cat);

        // Case A: Flat directory — .bin/.txt files directly under category
        bool hasFlat = hasAllThree(catDir, "bin") || hasAllThree(catDir, "txt");
        if (hasFlat) {
            QStringList files;
            for (auto &f : catDir.entryList({"M.*", "C.*", "K.*"}, QDir::Files))
                files << f;
            auto *flatItem = addProblemItem(catItem, cat, files);
            // 视觉区分：斜体 + 提示
            auto font = flatItem->font();
            font.setItalic(true);
            flatItem->setFont(font);
            flatItem->setToolTip(tr("矩阵文件位于 %1（扁平结构）").arg(cat));
            totalProblems++;
        }

        // Case B: Subdirectory structure (category/problem_name/)
        QString flatName = cat;
        for (auto &prob : problemDirs) {
            // 跳过与扁平问题同名的子目录（避免重复显示）
            if (hasFlat && prob == flatName) continue;
            QDir probDir(catDir.absoluteFilePath(prob));
            QStringList files;
            if (probDir.exists("M.bin")) files << "M.bin";
            if (probDir.exists("C.bin")) files << "C.bin";
            if (probDir.exists("K.bin")) files << "K.bin";
            if (probDir.exists("M.txt")) files << "M.txt";
            if (probDir.exists("C.txt")) files << "C.txt";
            if (probDir.exists("K.txt")) files << "K.txt";

            if (files.size() >= 2) {
                addProblemItem(catItem, prob, files);
                totalProblems++;
            }
        }

        // 若分类下无任何问题，移除空的分类节点
        if (catItem->rowCount() == 0)
            rootItem_->removeRow(catItem->row());
    }

    tree_->expandAll();

    // 恢复勾选状态
    if (!checkedNames.isEmpty()) {
        for (int ci = 0; ci < rootItem_->rowCount(); ++ci) {
            auto *catItem = rootItem_->child(ci);
            for (int pi = 0; pi < catItem->rowCount(); ++pi) {
                auto *probItem = catItem->child(pi);
                if (checkedNames.contains(probItem->text()))
                    probItem->setCheckState(Qt::Checked);
            }
        }
    }

    if (!populating_) { populating_ = false; return; }

    // Only sync config if we found problems (avoid wiping config on empty scan)
    if (totalProblems > 0) {
        syncToConfig();
    } else {
        emit statusMessage(tr("未发现任何问题"));
    }
    populating_ = false;

    // 更新文件监控目录列表
    if (watcher_ && !watcher_->directories().isEmpty())
        watcher_->removePaths(watcher_->directories());
    if (watcher_) {
        QString base = problemBasePath();
        watcher_->addPath(base);
        QDir baseDir(base);
        for (auto &cat : baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            watcher_->addPath(base + "/" + cat);
            QDir catDir(base + "/" + cat);
            for (auto &prob : catDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                watcher_->addPath(catDir.absoluteFilePath(prob));
        }
    }
}

void ProblemTreePanel::syncToConfig()
{
    auto &j = Config::ConfigManager::instance().data();
    nlohmann::json arr = nlohmann::json::array();

    for (int ci = 0; ci < rootItem_->rowCount(); ++ci) {
        auto *catItem = rootItem_->child(ci);
        QString cat = catItem->text();
        for (int pi = 0; pi < catItem->rowCount(); ++pi) {
            auto *probItem = catItem->child(pi);
            QString prob = probItem->text();
            nlohmann::json entry;
            entry["name"] = prob.toStdString();

            // Determine if flat or subdirectory structure
            QString prefix = cat + "/" + prob;
            QString base = problemBasePath();
            QDir probDir(base + "/" + prefix);
            QDir flatDir(base + "/" + cat);

            // Detect actual file extensions (.bin or .txt)
            auto detectFile = [&](const QString &name) -> QString {
                if (probDir.exists(name + ".bin")) return prefix + "/" + name + ".bin";
                if (probDir.exists(name + ".txt")) return prefix + "/" + name + ".txt";
                if (flatDir.exists(name + ".bin")) return cat + "/" + name + ".bin";
                if (flatDir.exists(name + ".txt")) return cat + "/" + name + ".txt";
                return prefix + "/" + name + ".bin"; // fallback
            };

            entry["M_file"] = detectFile("M").toStdString();
            entry["C_file"] = detectFile("C").toStdString();
            entry["K_file"] = detectFile("K").toStdString();
            entry["is_sparse"] = true;
            arr.push_back(entry);
        }
    }

    if (!arr.empty())
        j["test_cases"] = arr;

    emit problemsChanged();
}

QStringList ProblemTreePanel::checkedProblems() const
{
    QStringList result;
    for (int ci = 0; ci < rootItem_->rowCount(); ++ci) {
        auto *catItem = rootItem_->child(ci);
        for (int pi = 0; pi < catItem->rowCount(); ++pi) {
            auto *probItem = catItem->child(pi);
            if (probItem->checkState() == Qt::Checked)
                result.append(probItem->text());
        }
    }
    return result;
}

void ProblemTreePanel::onContextMenu(const QPoint &pos)
{
    contextMenuIndex_ = tree_->indexAt(pos);
    auto *item = model_->itemFromIndex(contextMenuIndex_);

    QMenu menu;
    menu.setStyleSheet(
        "QMenu { font-size: 13pt; padding: 8px; }"
        "QMenu::item { padding: 8px 36px 8px 16px; }"
        "QMenu::separator { height: 1px; margin: 6px 12px; background: #dcdcdc; }");

    if (!item) {
        menu.addAction(tr("新建问题"), this, &ProblemTreePanel::onNewProblem);
        menu.addSeparator();
        menu.addAction(tr("刷新"), this, [this]() { scanProblems(); });
        menu.exec(tree_->viewport()->mapToGlobal(pos));
        return;
    }

    bool isProblem = item->isCheckable();
    bool isCategory = !isProblem && item->parent() == nullptr && item != rootItem_;

    auto *newAct = menu.addAction(tr("新建问题"));
    if (isProblem) {
        menu.addSeparator();
        menu.addAction(tr("转换为二进制"));
        menu.addAction(tr("在资源管理器中打开"));
        menu.addSeparator();
        menu.addAction(tr("删除问题"));
    } else if (isCategory) {
        menu.addSeparator();
        menu.addAction(tr("在资源管理器中打开"));
    }

    auto *selected = menu.exec(tree_->viewport()->mapToGlobal(pos));
    if (!selected) return;

    if (selected == newAct) {
        onNewProblem();
    } else if (selected->text() == tr("转换为二进制")) {
        onConvertToBinary();
    } else if (selected->text() == tr("删除问题")) {
        onDeleteProblem();
    } else if (selected->text() == tr("在资源管理器中打开")) {
        QString path;
        if (isProblem)
            path = problemDirPath(item);
        else if (isCategory)
            path = problemBasePath() + "/" + item->text();
        if (!path.isEmpty())
            QDesktopServices::openUrl(QUrl::fromLocalFile(path));
    }
}

void ProblemTreePanel::onNewProblem()
{
    QString base = problemBasePath();
    QDir baseDir(base);

    // Choose category
    QStringList cats;
    for (int ci = 0; ci < rootItem_->rowCount(); ++ci)
        cats << rootItem_->child(ci)->text();
    if (cats.isEmpty()) {
        // Use first directory on disk
        auto catDirs = baseDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        if (!catDirs.isEmpty())
            cats = catDirs;
        else
            cats << "small_demo";
    }

    // 自定义对话框：分类 + 名称在一个窗口中
    QDialog infoDlg(this);
    infoDlg.setWindowTitle(tr("新建问题"));
    auto *form = new QFormLayout(&infoDlg);

    auto *catCombo = new QComboBox();
    catCombo->addItems(cats);
    catCombo->setMinimumWidth(360);
    form->addRow(tr("分类:"), catCombo);

    auto *nameEdit = new QLineEdit();
    nameEdit->setMinimumWidth(360);
    form->addRow(tr("问题名称 (文件夹名):"), nameEdit);

    auto *btnBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(btnBox, &QDialogButtonBox::accepted, &infoDlg, &QDialog::accept);
    QObject::connect(btnBox, &QDialogButtonBox::rejected, &infoDlg, &QDialog::reject);
    form->addRow(btnBox);

    infoDlg.setWindowFlags(infoDlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    infoDlg.setMinimumSize(520, 180);
    if (infoDlg.exec() != QDialog::Accepted) return;
    QString cat = catCombo->currentText();
    QString name = nameEdit->text().trimmed();
    if (cat.isEmpty() || name.isEmpty()) return;

    // File selection — QFileDialog detail mode + large size
    QString lastDir = base;
    auto openMatrixFile = [&](const QString &title) -> QString {
        QFileDialog dlg(this, title, lastDir,
            tr("Matrix Files (*.txt *.bin);;All Files (*)"));
        dlg.setOption(QFileDialog::DontUseNativeDialog);
        dlg.setFileMode(QFileDialog::ExistingFile);
        dlg.setViewMode(QFileDialog::Detail);
        dlg.setMinimumSize(900, 650);
        if (dlg.exec() == QDialog::Accepted && !dlg.selectedFiles().isEmpty()) {
            QString result = dlg.selectedFiles().first();
            lastDir = QFileInfo(result).absolutePath();
            return result;
        }
        return {};
    };
    QString mFile = openMatrixFile(tr("选择 M 矩阵文件"));
    QString cFile = openMatrixFile(tr("选择 C 矩阵文件"));
    QString kFile = openMatrixFile(tr("选择 K 矩阵文件"));

    if (mFile.isEmpty() && cFile.isEmpty() && kFile.isEmpty()) {
        QMessageBox::warning(this, tr("提示"), tr("未选择任何文件"));
        return;
    }

    // Create directory and copy files
    QString destDir = base + "/" + cat + "/" + name;
    if (!QDir().mkpath(destDir)) {
        QMessageBox::warning(this, tr("创建失败"),
            tr("无法创建目录:\n%1").arg(destDir));
        return;
    }

    auto copyFile = [&](const QString &src) {
        if (src.isEmpty()) return;
        QFileInfo fi(src);
        QString dst = destDir + "/" + fi.fileName();
        if (src == dst) return;
        if (!QFile::copy(src, dst)) {
            QMessageBox::warning(this, tr("复制失败"),
                tr("无法复制 %1 → %2").arg(fi.fileName(), dst));
        }
    };
    copyFile(mFile);
    copyFile(cFile);
    copyFile(kFile);

    // Add to binary_convert_list for .txt files
    auto &j = Config::ConfigManager::instance().data();
    auto &binList = j["binary_convert_list"];
    if (!binList.is_array()) binList = nlohmann::json::array();

    auto addBin = [&](const QString &src) {
        if (src.isEmpty()) return;
        QFileInfo fi(src);
        if (fi.suffix().toLower() != "txt") return;
        QString rel = cat + "/" + name + "/" + fi.fileName();
        QString binRel = cat + "/" + name + "/" + fi.completeBaseName() + ".bin";
        for (auto &e : binList)
            if (e.value("text_file", "") == rel.toStdString()) return;
        nlohmann::json entry;
        entry["text_file"] = rel.toStdString();
        entry["bin_file"] = binRel.toStdString();
        binList.push_back(entry);
    };
    addBin(mFile); addBin(cFile); addBin(kFile);

    scanProblems();
    Config::ConfigManager::instance().save();

    // 在树中找到并选中新创建的问题
    for (int ci = 0; ci < rootItem_->rowCount(); ++ci) {
        auto *catItem = rootItem_->child(ci);
        if (catItem->text() != cat) continue;
        tree_->expand(catItem->index());
        for (int pi = 0; pi < catItem->rowCount(); ++pi) {
            auto *probItem = catItem->child(pi);
            if (probItem->text() == name) {
                tree_->selectionModel()->select(probItem->index(),
                    QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
                tree_->scrollTo(probItem->index());
                break;
            }
        }
        break;
    }
    emit statusMessage(tr("已创建: %1").arg(name));
}

void ProblemTreePanel::onDeleteProblem()
{
    auto index = contextMenuIndex_.isValid() ? contextMenuIndex_ : tree_->currentIndex();
    auto *item = model_->itemFromIndex(index);
    if (!item || !item->isCheckable()) return;

    auto *parent = item->parent();
    if (!parent) return;

    QString path = problemDirPath(item);
    if (path.isEmpty()) return;

    // 检测扁平模式：problemDirPath 返回分类目录而非子目录
    QString catPath = problemBasePath() + "/" + parent->text();
    bool isFlat = (QDir::cleanPath(path) == QDir::cleanPath(catPath));

    auto reply = QMessageBox::question(this, tr("确认删除"),
        tr("确定删除问题「%1」及其所有文件？\n\n路径: %2").arg(item->text(), path),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    QDir dir(path);
    if (isFlat) {
        // 扁平模式：只删除 M.* C.* K.* 文件，保留目录及子目录问题
        QStringList toRemove;
        for (auto &f : dir.entryList({"M.*", "C.*", "K.*"}, QDir::Files))
            toRemove << f;
        for (auto &f : toRemove) {
            if (!dir.remove(f)) {
                QMessageBox::warning(this, tr("删除失败"),
                    tr("无法删除:\n%1/%2").arg(path, f));
                return;
            }
        }
    } else {
        if (!dir.removeRecursively()) {
            QMessageBox::warning(this, tr("删除失败"), tr("无法删除目录:\n%1").arg(path));
            return;
        }
    }

    // 清理 binary_convert_list 中属于该问题的条目
    QString prefix = isFlat
        ? parent->text() + "/"         // e.g. "test/"
        : parent->text() + "/" + item->text() + "/";  // e.g. "test/test1/"
    auto &j = Config::ConfigManager::instance().data();
    auto &binList = j["binary_convert_list"];
    if (binList.is_array()) {
        nlohmann::json filtered = nlohmann::json::array();
        for (auto &e : binList) {
            std::string tf = e.value("text_file", "");
            std::string bf = e.value("bin_file", "");
            if (tf.find(prefix.toStdString()) == 0 || bf.find(prefix.toStdString()) == 0)
                continue;
            filtered.push_back(e);
        }
        j["binary_convert_list"] = filtered;
    }

    scanProblems();
    Config::ConfigManager::instance().save();
    emit statusMessage(tr("已删除: %1").arg(item->text()));
}

void ProblemTreePanel::onConvertToBinary()
{
    auto index = contextMenuIndex_.isValid() ? contextMenuIndex_ : tree_->currentIndex();
    auto *item = model_->itemFromIndex(index);
    if (!item || !item->isCheckable()) return;

    QString dirPath = problemDirPath(item);
    if (dirPath.isEmpty()) return;

    QDir dir(dirPath);
    auto txtFiles = dir.entryList({"*.txt"}, QDir::Files);
    if (txtFiles.isEmpty()) {
        QMessageBox::information(this, tr("提示"), tr("该目录下无可转换的 .txt 文件"));
        return;
    }

    // 检查是否有 .bin 文件已存在，给出覆盖提示
    QStringList existingBins;
    for (auto &f : txtFiles) {
        QString binName = f.left(f.length() - 4) + ".bin";
        if (dir.exists(binName))
            existingBins << binName;
    }
    if (!existingBins.isEmpty()) {
        auto reply = QMessageBox::question(this, tr("确认覆盖"),
            tr("以下 .bin 文件已存在，是否覆盖？\n%1")
                .arg(existingBins.join(", ")),
            QMessageBox::Yes | QMessageBox::No);
        if (reply != QMessageBox::Yes) return;
    }

    int count = 0;
    QStringList failedList;
    for (auto &f : txtFiles) {
        QString txtPath = dirPath + "/" + f;
        QString binPath = dirPath + "/" + f.left(f.length() - 4) + ".bin";
        try {
            QEP::convertTextCSRtoBinary(txtPath.toStdString(), binPath.toStdString());
            count++;
        } catch (const std::exception &e) {
            failedList << (f + ": " + e.what());
        }
    }

    scanProblems();
    Config::ConfigManager::instance().save();

    // 构建 GUI 反馈消息
    QString resultMsg = tr("成功转换 %1 个文件").arg(count);
    if (!failedList.isEmpty())
        resultMsg += "\n\n" + tr("失败 %1 个:\n%2")
            .arg(failedList.size()).arg(failedList.join("\n"));

    QMessageBox::information(this, tr("转换完成"), resultMsg);
    emit statusMessage(tr("已转换 %1 个文件").arg(count));
}

void ProblemTreePanel::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void ProblemTreePanel::dropEvent(QDropEvent *event)
{
    const auto urls = event->mimeData()->urls();
    if (urls.isEmpty()) return;

    auto index = tree_->indexAt(tree_->viewport()->mapFrom(this, event->pos()));
    auto *item = model_->itemFromIndex(index);
    QString cat = "small_demo";
    if (item) {
        if (item->isCheckable() && item->parent())
            cat = item->parent()->text();
        else if (!item->isCheckable() && item != rootItem_)
            cat = item->text();
    }

    QStringList files;
    for (auto &url : urls)
        files << url.toLocalFile();

    // 自定义对话框以控制尺寸
    QDialog dlg(this);
    dlg.setWindowTitle(tr("新建问题 — 拖入 %1").arg(cat));
    auto *dlgLayout = new QFormLayout(&dlg);
    auto *nameEdit = new QLineEdit();
    nameEdit->setMinimumWidth(360);
    dlgLayout->addRow(tr("问题名称:"), nameEdit);
    auto *dlgBtns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    QObject::connect(dlgBtns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(dlgBtns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    dlgLayout->addRow(dlgBtns);
    dlg.setWindowFlags(dlg.windowFlags() & ~Qt::WindowContextHelpButtonHint);
    dlg.setMinimumSize(480, 120);
    if (dlg.exec() != QDialog::Accepted) return;
    QString name = nameEdit->text().trimmed();
    if (name.isEmpty()) return;

    QString destDir = problemBasePath() + "/" + cat + "/" + name;
    QDir().mkpath(destDir);
    for (auto &f : files) {
        QFileInfo fi(f);
        QString dst = destDir + "/" + fi.fileName();
        if (!QFile::copy(f, dst)) {
            QMessageBox::warning(this, tr("复制失败"),
                tr("无法复制 %1").arg(fi.fileName()));
        }
    }

    scanProblems();
    emit statusMessage(tr("已创建: %1").arg(name));
}
