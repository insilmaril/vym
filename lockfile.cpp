#include <QDebug>       
#include <QFile>       
#include <QRegularExpression>       

#include "file.h"

#include "lockfile.h"

VymLockFile::VymLockFile( const QString &fn )
{
    path = fn;
    isMyLockFile = false;
}

VymLockFile::~VymLockFile()
{
    if (isMyLockFile)
    {
        QFile LockFile( path );
        if (!LockFile.remove() )
        qWarning() << "Destructor VymLockFile:  Removing LockFile failed";
    }
}

bool VymLockFile::tryLock()
{
    QFile lockFile( path );
    if ( lockFile.exists() )
    {
        // File is already locked       
        if (debug) qDebug() << "VymLockFile::tryLock  failed: LockFile exists";

        QString s;
        if (!loadStringFromDisk( path, s) )
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
        if (debug) qWarning() << QString("VymLockFile::tryLock failed: Cannot open lockFile %1\n%2")
                    .arg( path )
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

bool VymLockFile::isLocked()
{
    QFile lockFile( path );
    return lockFile.exists();
}

void VymLockFile::setAuthor(const QString &s)
{
    author = s;
}

QString VymLockFile::getAuthor()
{
    return author;
}

void VymLockFile::setHost(const QString &s)
{
    host = s;
}

QString VymLockFile::getHost()
{
    return host;
}
