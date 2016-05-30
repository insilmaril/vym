#include "lockfile.h"
#include <QDebug>       
#include <QRegularExpression>       

#include "file.h"

LockFile::LockFile( const QString &fn )
{
    path = fn;
    isMyLockFile = false;
}

LockFile::~LockFile()
{
    if (isMyLockFile)
    {
        QFile lockfile( path );
        if (!lockfile.remove() )
        qWarning() << "Destructor LockFile:  Removing lockfile failed";
    }
}

bool LockFile::tryLock()
{
    QFile lockfile( path );
    if ( lockfile.exists() )
    {
        // File is already locked       
        if (debug) qDebug() << "Lockfile::tryLock  failed: lockfile exists";

        QString s;
        if (!loadStringFromDisk( path, s) )
            qWarning( "Failed to read from existing lockfile");
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

    if (!lockfile.open(QFile::WriteOnly | QFile::Text)) 
    {
        if (debug) qWarning() << QString("LockFile::tryLock failed: Cannot open lockfile %1\n%2")
                    .arg( path )
                    .arg( lockfile.errorString() );
        return false;
    }

    isMyLockFile = true;

    QString s;
    if (!author.isEmpty() ) s  = QString( "author: \"%1\"\n").arg( author );
    if (!host.isEmpty() )   s += QString( "host: \"%1\"\n").arg( host );

    if (!s.isEmpty() )
    {
        QTextStream out( &lockfile );
        out.setCodec( "UTF-8" );
        out << s;
    }

    lockfile.close();
}

bool LockFile::isLocked()
{
    QFile lockfile( path );
    return lockfile.exists();
}

void LockFile::setAuthor(const QString &s)
{
    author = s;
}

QString LockFile::getAuthor()
{
    return author;
}

void LockFile::setHost(const QString &s)
{
    host = s;
}

QString LockFile::getHost()
{
    return host;
}
