#ifndef LOCKFILE_H
#define LOCKFILE_H

#include <QFile>

extern bool debug;

class LockFile
{
public:
    LockFile( const QString &fn );
    ~LockFile();
    bool tryLock();
    bool isLocked();
    void setAuthor(const QString &s);
    QString getAuthor();
    void setHost(const QString &s);
    QString getHost();

private:    
    QWidget *parent;
    QString path;
    QString author;
    QString host;
    bool isMyLockFile;
};

#endif
