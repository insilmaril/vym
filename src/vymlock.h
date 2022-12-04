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
    bool releaseLock();
    bool removeLockForced();
    void setAuthor(const QString &s);
    QString getAuthor();
    void setHost(const QString &s);
    QString getHost();
    void setMapPath(const QString &path);
    QString getMapPath();

  private:
    QString author;
    QString host;
    QString mapPath;
    QString lockPath;
    LockState state;
};

#endif
