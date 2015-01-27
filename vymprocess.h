#ifndef VYMPROCESS_H
#define VYMPROCESS_H

#include <QProcess>
#include <QString>

class VymProcess:public QProcess
{
    Q_OBJECT
public:
    VymProcess ();
    ~VymProcess ();
    void clear();
    void runScript( QString spath, QString fpath );
    QString getErrout();
    QString getStdout();

public slots:
    virtual void readProcErrout();
    virtual void readProcStdout();

private:
    QString errOut;
    QString stdOut;
};

#endif
