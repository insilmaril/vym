#ifndef VYMLOCK_H
#define VYMLOCK_H

extern bool debug;

class VymLock
{
public:
    VymLock();
    VymLock( const QString &fn );
    ~VymLock();
    void init();
    bool tryLock();
    bool isLocked();
    void releaseLock();
    void setAuthor(const QString &s);
    QString getAuthor();
    void setHost(const QString &s);
    QString getHost();
    void setMapPath(const QString &s);
    QString getMapPath();

private:
    QWidget *parent;
    QString author;
    QString host;
    QString mapPath;
    bool isMyLockFile;
};

#endif
