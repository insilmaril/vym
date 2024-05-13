#ifndef ZIPAGENT_H
#define ZIPAGENT_H

#include <QDir>
#include <QMessageBox>
#include <QProcess>

#include "file.h"

class ZipAgent : public QProcess {
    Q_OBJECT

  public:
    ZipAgent(QDir zipDir, QString zipName);
    ~ZipAgent();

  public:
    static bool checkZipTool();
    static bool checkUnzipTool();

    void setBackgroundProcess(bool);
    void startZip();
    void startUnzip();

  signals:
    void zipFinished();
    void zipError();

  private slots:
    void zipProcessFinished (int exitCode, QProcess::ExitStatus exitStatus);

  private:
    QDir zipDirInt;
    QString zipNameInt;

    bool isBackgroundProcessInt;
    QStringList args;
};
#endif
