#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>
#include <QString>

class Process:public QProcess
{
    Q_OBJECT
public:
    Process ();
    ~Process ();
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
