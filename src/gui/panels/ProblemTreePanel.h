//==============================================================================
//  src/gui/panels/ProblemTreePanel.h  —  测试问题浏览器面板接口
//==============================================================================

#ifndef QEP_GUI_PROBLEM_TREE_PANEL_H
#define QEP_GUI_PROBLEM_TREE_PANEL_H

#include <QGroupBox>
#include <QStringList>
#include <QPersistentModelIndex>
#include <QPoint>

class QTreeView;
class QStandardItemModel;
class QStandardItem;
class QPushButton;
class QFileSystemWatcher;
class QTimer;

class ProblemTreePanel : public QGroupBox {
    Q_OBJECT

public:
    explicit ProblemTreePanel(QWidget *parent = nullptr);

    QStringList checkedProblems() const;
    void scanProblems();
    void syncToConfig();

signals:
    void problemsChanged();
    void statusMessage(const QString &msg);

private slots:
    void onContextMenu(const QPoint &pos);
    void onNewProblem();
    void onDeleteProblem();
    void onConvertToBinary();

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    QStandardItem *addCategoryItem(const QString &name);
    QStandardItem *addProblemItem(QStandardItem *parent, const QString &name,
                                  const QStringList &files);
    QString problemBasePath() const;
    QString problemDirPath(const QStandardItem *item) const;

    QTreeView           *tree_              = nullptr;
    QStandardItemModel  *model_             = nullptr;
    QPushButton         *refreshBtn_        = nullptr;
    QStandardItem       *rootItem_          = nullptr;
    bool                 populating_        = false;
    QPersistentModelIndex contextMenuIndex_;  // 缓存右键点击的索引（持久化）
    QFileSystemWatcher    *watcher_       = nullptr;
    QTimer                *refreshTimer_  = nullptr;
};

#endif
