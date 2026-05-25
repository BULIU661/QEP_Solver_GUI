//==============================================================================
//  src/gui/MainWindow.h  —  主窗口接口声明
//==============================================================================

#ifndef QEP_GUI_MAINWINDOW_H
#define QEP_GUI_MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

class QLabel;
class QProgressBar;
class QPushButton;
class QTabWidget;
class ConfigPanel;
class ProblemTreePanel;
class SolverPanel;
class OutputPanel;
class SolverWorker;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onLoadConfig();
    void onSaveConfig();
    void onSolveClicked();
    void onSolverFinished();
    void appendLog(const QString &msg);

private:
    void createMenuBar();
    void createCentralWidget();
    void createStatusBar();
    void loadConfigToUI();

    ConfigPanel       *configPanel_       = nullptr;
    ProblemTreePanel  *problemTreePanel_  = nullptr;
    SolverPanel       *solverPanel_       = nullptr;
    OutputPanel       *outputPanel_       = nullptr;
    QPushButton       *solveBtn_          = nullptr;
    QTabWidget        *tabWidget_         = nullptr;

    QThread           *solverThread_      = nullptr;

    QProgressBar      *progressBar_       = nullptr;
    QLabel            *statusLabel_       = nullptr;
};

#endif
