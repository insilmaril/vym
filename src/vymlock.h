#ifndef VYMLOCK_H
#define VYMLOCK_H

extern bool debug;

class VymLock {
  public:
    enum LockState { Undefined, LockedByMyself, LockedByOther, NotWritable };
    void operator==(const VymLock &);
    VymLock();
    VymLock(const QString &fn);
    ~VymLock();
    void init();
    bool tryLock();
    LockState getState();
    bool removeLock();
    bool releaseLock();
    bool rename(const QString &newMapPath);
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
    LockState state;
};

#endif
