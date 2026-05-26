//==============================================================================
//  src/gui/workers/SolverWorker.h  —  后台求解工作线程接口
//==============================================================================

#ifndef QEP_GUI_SOLVER_WORKER_H
#define QEP_GUI_SOLVER_WORKER_H

#include <QObject>
#include <QStringList>
#include <vector>
#include <string>
#include "qep/QEP.h"

namespace Config { struct TestCase; }


class SolverWorker : public QObject {
    Q_OBJECT

public:
    struct SolveRequest {
        std::vector<Config::TestCase> cases;
        QStringList solverNames;
        int nev;
        double sigma;
    };

    void setRequest(const SolveRequest &req);

public slots:
    void run();

signals:
    void logMessage(const QString &msg);
    void caseStarted(const QString &caseName);
    void solverProgress(const QString &caseName, const QString &solverName);
    void progressChanged(int current, int total);
    void resultReady(const QString &caseName,
                     const QString &solverName,
                     const QEP::QuadraticEigenvalueResult &result);
    void finished();


private:
    SolveRequest request_;
};

#endif
