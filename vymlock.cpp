#include <QDebug>       
#include <QFile>       
#include <QRegularExpression>       

#include "file.h"

#include "vymlock.h"

VymLock::VymLock() 
{
    init();
}

VymLock::VymLock( const QString &fn )
{
    init();
    mapPath = fn;
}

VymLock::~VymLock()
{
    if (isMyLockFile)
    {
        QFile LockFile( mapPath + ".lock" );
        if (!LockFile.remove() )
        qWarning() << "Destructor VymLock:  Removing LockFile failed";
    }
}

void VymLock::init()
{
    isMyLockFile = false;
}

bool VymLock::tryLock()
{
    QFile lockFile( mapPath + ".lock");
    if ( lockFile.exists() )
    {
        // File is already locked       
        if (debug) qDebug() << "VymLock::tryLock  failed: LockFile exists";

        QString s;
        if (!loadStringFromDisk( mapPath, s) )
            qWarning( "Failed to read from existing lockFile");
        else
        {
            QRegularExpression re("^author:\\s\\\"(.*)\\\"$");
            re.setPatternOptions( QRegularExpression::MultilineOption );
            QRegularExpressionMatch match = re.match( s );
            if ( match.hasMatch() ) author = match.captured(1);

            re.setPattern("^host:\\s\\\"(.*)\\\"$");
            match = re.match( s );
            if ( match.hasMatch() ) host = match.captured(1);
        }
        return false; 
    }

    if (!lockFile.open(QFile::WriteOnly | QFile::Text))
    {
        if (debug) qWarning() << QString("VymLock::tryLock failed: Cannot open lockFile %1\n%2")
                    .arg( mapPath + ".lock")
                    .arg( lockFile.errorString() );
        return false;
    }

    isMyLockFile = true;

    QString s;
    if (!author.isEmpty() ) s  = QString( "author: \"%1\"\n").arg( author );
    if (!host.isEmpty() )   s += QString( "host: \"%1\"\n").arg( host );

    if (!s.isEmpty() )
    {
        QTextStream out( &lockFile );
        out.setCodec( "UTF-8" );
        out << s;
    }

    lockFile.close();
    return true;
}

bool VymLock::isLocked()
{
    QFile lockFile( mapPath + ".lock" );
    return lockFile.exists();
}

void VymLock::releaseLock() // FIXME-2 missing
{
}

void VymLock::setAuthor(const QString &s)
{
    author = s;
}

QString VymLock::getAuthor()
{
    return author;
}

void VymLock::setHost(const QString &s)
{
    host = s;
}

QString VymLock::getHost()
{
    return host;
}
void VymLock::setMapPath(const QString &s)
{
    mapPath = s;
}

QString VymLock::getMapPath()
{
    return mapPath;
}
