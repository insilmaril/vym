#include <QDebug>
#include <QFile>
#include <QRegularExpression>

#include "file.h"

#include "vymlock.h"

void VymLock::operator==(const VymLock &other)
{
    author = other.author;
    host = other.host;
    mapPath = other.mapPath;
    lockPath = other.lockPath;
    state = other.state;
}

VymLock::VymLock() { init(); }

VymLock::VymLock(const QString &path)
{
    init();
    setMapPath(path);
}

VymLock::~VymLock()
{
}

void VymLock::init()
{
    state = Undefined;
}

bool VymLock::tryLock()
{
    QFile lockFile(lockPath);
    if (lockFile.exists()) {
        // File is already locked
        if (debug)
            qDebug() << QString("VymLock::tryLock  failed: LockFile exists: %1").arg(lockFile.fileName());

        QString s;
        if (!loadStringFromDisk(lockFile.fileName(), s))
            qWarning("Failed to read from existing lockFile");
        else {
            QRegularExpression re("^author:\\s\\\"(.*)\\\"$");
            re.setPatternOptions(QRegularExpression::MultilineOption);
            QRegularExpressionMatch match = re.match(s);
            if (match.hasMatch())
                author = match.captured(1);

            re.setPattern("^host:\\s\\\"(.*)\\\"$");
            match = re.match(s);
            if (match.hasMatch())
                host = match.captured(1);
        }
        state = LockedByOther;
        return false;
    }

    if (!lockFile.open(QFile::WriteOnly | QFile::Text)) {
        if (debug)
            qWarning()
                << QString(
                       "VymLock::tryLock failed: Cannot open lockFile %1\n%2")
                       .arg(lockFile.fileName())
                       .arg(lockFile.errorString());
        state = NotWritable;
        return false;
    }

    QString s;
    if (!author.isEmpty())
        s = QString("author: \"%1\"\n").arg(author);
    if (!host.isEmpty())
        s += QString("host: \"%1\"\n").arg(host);

    if (!s.isEmpty()) {
        QTextStream out(&lockFile);
        out.setCodec("UTF-8");
        out << s;
    }

    state = LockedByMyself;
    lockFile.close();

    return true;
}

VymLock::LockState VymLock::getState() { return state; }

bool VymLock::releaseLock()
{
    if (state == LockedByMyself) {
        QFile lockFile(lockPath);
        if (lockFile.remove()) {
            state = Undefined;
            return true;
        }
    }
    qWarning() << "VymLock::releaseLock  failed for " << lockPath;
    return false;
}

bool VymLock::removeLockForced()
{
    QFile lockFile(lockPath);
    if (lockFile.remove()) {
        state = Undefined;
        return true;
    }
    qWarning() << "VymLock::removeLockForced  failed for " << lockPath;
    return false;
}

void VymLock::setAuthor(const QString &s) { author = s; }

QString VymLock::getAuthor() { return author; }

void VymLock::setHost(const QString &s) { host = s; }

QString VymLock::getHost() { return host; }

void VymLock::setMapPath(const QString &path)
{
    mapPath = path;
    lockPath = path + ".lock";

    // Reset state for a new path
    state = Undefined;
}

QString VymLock::getMapPath()
{
    return mapPath;
}
