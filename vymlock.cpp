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
    if ( state == lockedByMyself && !releaseLock()) 
        qWarning() << "Destructor VymLock:  Removing LockFile failed";
}

void VymLock::init()
{
    state = undefined;
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
        state = lockedByOther;
        return false; 
    }

    if (!lockFile.open(QFile::WriteOnly | QFile::Text))
    {
        if (debug) qWarning() << QString("VymLock::tryLock failed: Cannot open lockFile %1\n%2")
                    .arg( mapPath + ".lock")
                    .arg( lockFile.errorString() );
        state = notWritable;
        return false;
    }

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
    state = lockedByMyself;

    return true;
}

VymLock::LockState VymLock::getState()
{
    return state;
}

bool VymLock::removeLock() 
{
    QFile LockFile( mapPath + ".lock" );
    if (LockFile.remove() )
        return true;
    else
        return false;
}

bool VymLock::releaseLock() 
{
    if (state == lockedByMyself)
    {
        QFile LockFile( mapPath + ".lock" );
        if (LockFile.remove() )
            return true;
    }
    return false;
}

bool VymLock::rename( const QString &newMapPath)
{
    QFile lockFile( mapPath + ".lock" );

    if ( lockFile.rename( newMapPath + ".lock") )
    {
        mapPath = newMapPath;
        return true;
    }
    return false;
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
