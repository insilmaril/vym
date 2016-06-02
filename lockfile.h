#ifndef LOCKFILE_H
#define LOCKFILE_H

extern bool debug;

class VymLockFile
{
public:
    VymLockFile( const QString &fn );
    ~VymLockFile();
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
