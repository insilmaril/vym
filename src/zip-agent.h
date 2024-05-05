#ifndef ZIPAGENT_H
#define ZIPAGENT_H

#include <QDir>
#include <QMessageBox>
#include <QProcess>

#include "file.h"

extern QString zipToolPath;

class ZipAgent : public QProcess {
    Q_OBJECT

  public:
    ZipAgent(QDir zipDir, QString zipName);
    ~ZipAgent();

  public:
    void startZip();

  signals:
    void finished();
    void zipError();

  private:
    QDir zipDirInt;
    QString zipNameInt;
};
#endif
