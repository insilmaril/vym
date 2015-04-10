#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QLabel>
#include <QTextStream>
#include <iostream>
#include <cstdlib>

#include "file.h"
#include "vymprocess.h"

#if defined(Q_OS_WIN32)
    #include "mkdtemp.h"
    #include <windows.h>
#endif

using namespace File;

extern QString zipToolPath;

QString convertToRel (const QString &src, const QString &dst)
{
    // Creates a relative path pointing from src to dst

    QString s=src;
    QString d=dst;
    int i;

    if (s==d)
    {
        // Special case, we just need the name of the file,
        // not the complete path
        i=d.lastIndexOf ("/");
        d=d.right (d.length()-i-1);
    } else
    {
        // remove identical left parts
        while (s.section("/",0,0) == d.section("/",0,0) )
        {
            i=s.indexOf ("/");
            s=s.right (s.length()-i-1);
            d=d.right (d.length()-i-1);
        }

        // Now take care of paths where we have to go back first
        int srcsep=s.count("/");
        while (srcsep > 0 )
        {
            d="../"+d;
            srcsep--;
        }
    }
    return d;
}

QString convertToAbs (const QString &src, const QString &dst)
{
    // Creates a relative path pointing from src to dst
    QDir dd(src);
    return dd.absoluteFilePath(dst);
}


QString basename(const QString &path)
{
    return path.section ('/', -1);
}

QString dirname(const QString &path)
{
    return path.section('/', 0, -2);
}

extern QString vymName;
bool reallyWriteDirectory(const QString &dir)
{
    QStringList eList = QDir(dir).entryList();
    if (eList.first() ==".")  eList.pop_front();    // remove "."
    if (eList.first() =="..") eList.pop_front();    // remove "."
    if (!eList.isEmpty())
    {
        QMessageBox mb( vymName,
                        QObject::tr("The directory %1 is not empty.\nDo you risk to overwrite its contents?","write directory").arg(dir),
                        QMessageBox::Warning,
                        QMessageBox::Yes ,
                        QMessageBox::Cancel | QMessageBox::Default,
                        QMessageBox::NoButton );

        mb.setButtonText( QMessageBox::Yes, QObject::tr("Overwrite") );
        mb.setButtonText( QMessageBox::No, QObject::tr("Cancel"));
        switch( mb.exec() )
        {
        case QMessageBox::Yes:
            // save
            return true;
        case QMessageBox::Cancel:
            // do nothing
            return false;
        }
    }
    return true;
}

QString makeTmpDir (bool &ok, QString prefix)   //FIXME-3 use QTemporaryDir
{
    bool b;
    QString path=makeUniqueDir (b,QDir::tempPath()+"/"+prefix+"-XXXXXX");
    ok=b;
    return path;
}

bool isInTmpDir(QString fn)
{
    QString temp=QDir::tempPath();
    int l=temp.length();
    return fn.left(l)==temp;
}

QString makeUniqueDir (bool &ok,QString s)
{
    ok=true;

    QString r;

#if defined(Q_OS_WIN32)
    r=mkdtemp (s);
#else
    // On Linux and friends use cstdlib
    
    // Convert QString to string 
    ok=true;
    char *p;
    int bytes=s.length();
    p=(char*) malloc (bytes+1);
    int i;
    for (i=0;i<bytes;i++)
	p[i]=s.at(i).unicode();
    p[bytes]=0;	

    r=mkdtemp (p);
    free (p);
#endif

    if (r.isEmpty()) ok=false;
    return r;
}

void removeDir(QDir d)
{
    // This check should_ not be necessary, but proved to be useful ;-)
    if (!isInTmpDir(d.path()))
    {
        qWarning ()<<"file.cpp::removeDir should remove "+d.path()+" - aborted.";
        return;
    }

    // Traverse directories
    d.setFilter( QDir::Dirs| QDir::Hidden | QDir::NoSymLinks );
    QFileInfoList list = d.entryInfoList();
    QFileInfo fi;

    for (int i = 0; i < list.size(); ++i)
    {
        fi=list.at(i);
        if (fi.fileName() != "." && fi.fileName() != ".." )
        {
            if ( !d.cd(fi.fileName()) )
                qWarning ()<<"removeDir() cannot find the directory "+fi.fileName();
            else
            {
                // Recursively remove subdirs
                removeDir (d);
                d.cdUp();
            }
        }
    }

    // Traverse files
    d.setFilter( QDir::Files| QDir::Hidden | QDir::NoSymLinks );
    list = d.entryInfoList();

    for (int i = 0; i < list.size(); ++i)
    {
        fi=list.at(i);
        QFile (fi.filePath()).remove();
    }

    if (!d.rmdir(d.path()))
        qWarning ()<<"removeDir("+d.path()+") failed!";
}	

bool copyDir (QDir src, QDir dst, const bool &override)   
{
    QStringList dirs  = src.entryList(QDir::AllDirs | QDir::Hidden | QDir::NoDotAndDotDot);
    QStringList files = src.entryList(QDir::Files );

    // Check if dst is a subdir of src, which would cause endless recursion...
    if (dst.absolutePath().contains(src.absolutePath())) return false;
    
    // Traverse directories
    QList<QString>::iterator d,f;
    for (d = dirs.begin(); d != dirs.end(); ++d) 
    {
        if (!QFileInfo(src.path() + "/" + (*d)).isDir()) continue;

        QDir cdir(dst.path() + "/" + (*d));
        cdir.mkpath(cdir.path());

        if (!copyDir (QDir(src.path() + "/" + (*d)), QDir(dst.path() + "/" + (*d)), override)) 
            return false;
    }

    // Traverse files
    for (f = files.begin(); f != files.end(); ++f) 
    {
        QFile cfile(src.path() + "/" + (*f));
        QFile destFile(dst.path()+ "/" + src.relativeFilePath(cfile.fileName()));
        if (destFile.exists() && override) 
            destFile.remove();

        if (!cfile.copy(dst.path() + "/" + src.relativeFilePath(cfile.fileName()))) 
            return false;
    }
    return true;
}

void makeSubDirs (const QString &s)
{
    QDir d(s);
    d.mkdir(s);
    d.mkdir ("images");	
    d.mkdir ("flags");	
}

ErrorCode zipDir ( QDir zipInputDir, QString zipName)
{
    ErrorCode err = Success;

    QString symLinkTarget;

    QString newName;
    // Move existing file away
    QFile file(zipName);
    if (file.exists() )
    {
        symLinkTarget = file.symLinkTarget();

        newName = zipName + ".tmp";
        int n=0;
        while (!file.rename (newName) && n<5)
        {
            newName = newName + QString().setNum(n);
            n++;
        }
        if (n>=5)
        {
            QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                                   QObject::tr("Couldn't move existing file out of the way before saving."));
            return Aborted;
        }
    }

    // zip the temporary directory
    VymProcess *zipProc=new VymProcess ();
    zipProc->setWorkingDirectory (zipInputDir.path());

#if defined(Q_OS_WIN32)
    QByteArray result;
    zipProc->start("cmd");
    if (!zipProc->waitForStarted())
    {
        QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
            QObject::tr("Couldn't start tool to decompress data."));
        err=Aborted;

    }
    zipProc->write(QString("\"%1\" a \"%2\" -r %3\\*\n").arg(zipToolPath).arg(zipName).arg(zipInputDir.path()).toUtf8());
    zipProc->closeWriteChannel();   //done Writing

    while(zipProc->state()!=QProcess::NotRunning){
        zipProc->waitForReadyRead();
        result = zipProc->readAll();
        //vout << result << flush;
    }
    //vout << zipProc->getStdout()<<flush;
#else
    QStringList args;
    args <<"-r";
    args <<zipName;
    args <<".";

    zipProc->start ("zip",args);
    if (!zipProc->waitForStarted() )
    {
        // zip could not be started
        QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                               QObject::tr("Couldn't start zip to compress data."));
        err=Aborted;
    } else
    {
        // zip could be started
        zipProc->waitForFinished();
        if (zipProc->exitStatus()!=QProcess::NormalExit )
        {
            QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                                   QObject::tr("zip didn't exit normally")+
                                   "\n" + zipProc->getErrout());
            err=Aborted;
        } else
        {
            if (zipProc->exitCode()>0)
            {
                QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                                       QString("zip exit code:  %1").arg(zipProc->exitCode() )+
                                       "\n" + zipProc->getErrout() );
                err=Aborted;
            }
        }
    }
#endif
    // Try to restore previous file, if zipping failed
    if (err == Aborted && !newName.isEmpty() && !file.rename (zipName) )
	QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
	   QObject::tr("Couldn't rename %1 back to %2").arg(newName).arg(zipName) );
    else
    {
        // Take care of symbolic link
        if (!symLinkTarget.isEmpty() )
        {
            if (!QFile(symLinkTarget).remove() )
            {
                QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                   QObject::tr("Couldn't remove target of old symbolic link %1").arg(symLinkTarget));
                err = Aborted;
                return err;
            }

            if (!QFile(zipName).rename(symLinkTarget) )
            {
                QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                   QObject::tr("Couldn't rename output to target of old symbolic link %1").arg(symLinkTarget));
                err = Aborted;
                return err;
            }
            if (!QFile(symLinkTarget).link(zipName) )
            {
                QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                   QObject::tr("Couldn't link from %1 to target of old symbolic link %2").arg(zipName).arg(symLinkTarget));
                err = Aborted;
                return err;
            }
        }

	// Remove temporary file
	if (!newName.isEmpty()  && !file.remove() )
	    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
	       QObject::tr("Saved %1, but couldn't remove %2").arg(zipName).arg(newName));
    }

    return err;	
}

File::ErrorCode unzipDir ( QDir zipOutputDir, QString zipName)
{
    ErrorCode err=Success;

    // Try to unzip file

    VymProcess *zipProc=new VymProcess ();
    zipProc->setWorkingDirectory (zipOutputDir.path());

#if defined(Q_OS_WIN32)
    QByteArray result;
    zipProc->start("cmd");
    if (!zipProc->waitForStarted())
    {
        QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                               QObject::tr("Couldn't start tool to decompress data."));
        err=Aborted;
    }
    zipProc->write(QString("\"%1\" -o%2 x \"%3\"\n").arg(zipToolPath).arg(zipOutputDir.path()).arg(zipName).toUtf8());
    zipProc->closeWriteChannel();   //done Writing

    while(zipProc->state()!=QProcess::NotRunning){
        zipProc->waitForReadyRead();
        result = zipProc->readAll();
        //vout << result << flush;
    }
    //vout << zipProc->getStdout()<<flush;
#else
    QStringList args;
    args << "-o";   // overwrite existing files!
    args << zipName ;
    args << "-d";
    args << zipOutputDir.path();

    zipProc->start ("unzip",args);
    if (!zipProc->waitForStarted() )
    {
        QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                               QObject::tr("Couldn't start unzip to decompress data."));
        err=Aborted;


    } else
    {
        zipProc->waitForFinished();
        if (zipProc->exitStatus()!=QProcess::NormalExit )
        {
            QMessageBox::critical( 0,QObject::tr( "Critical Error" ),
                                   QObject::tr("unzip didn't exit normally") +
                                   zipProc->getErrout() );
            err=Aborted;
        } else
        {
            if (zipProc->exitCode()>0)
            {
                if (zipProc->exitCode()==9)
                    // no zipped file, but maybe .xml or old version? Try again.
                    err=NoZip;
                else
                {
                    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                                           QString("unzip exit code:  %1").arg(zipProc->exitCode() ) +
                                           zipProc->getErrout() );
                    err=Aborted;
                }
            }
        }
    }
#endif
    return err;
}

bool loadStringFromDisk (const QString &fname, QString &s)
{
    s="";
    QFile file(fname);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        qWarning()<<QString("loadStringFromDisk: Cannot read file %1\n%2")
                    .arg(fname)
                    .arg(file.errorString());
        return false;
    }

    QTextStream in(&file);
    s=in.readAll();
    return true;
}

bool saveStringToDisk (const QString &fname, const QString &s)
{
    QFile file(fname);
    // Write as binary (default), QFile::Text would convert linebreaks
    if (!file.open(QFile::WriteOnly  )) {
        qWarning()<<QString("saveStringToDisk: Cannot write file %1:\n%2.")
                    .arg(fname)
                    .arg(file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out << s;

    return true;
}

FileType getMapType (const QString &fn)
{
    int i=fn.lastIndexOf(".");
    if (i>=0)
    {
        QString postfix=fn.mid(i+1);
        if (postfix=="vym" || postfix=="vyp" || postfix=="xml") return VymMap;
        if (postfix=="mm") return FreemindMap;
    }
    return UnknownMap;
}

ImageIO::ImageIO ()
{
    // Create list with supported image types
    // foreach (QByteArray format, QImageWriter::supportedImageFormats())
    // imageTypes.append( tr("%1...").arg(QString(format).toUpper()));
    imageFilters.append ("Images (*.png *.jpg *.jpeg *.bmp *.bmp *.ppm *.xpm *.xbm)");
    imageTypes.append ("PNG");
    imageFilters.append ("Portable Network Graphics (*.png)");
    imageTypes.append ("PNG");
    imageFilters.append ("Joint Photographic Experts Group (*.jpg)");
    imageTypes.append ("JPG");
    imageFilters.append ("Joint Photographic Experts Group (*.jpeg)");
    imageTypes.append ("JPG");
    imageFilters.append ("Windows Bitmap (*.bmp)");
    imageTypes.append ("BMP");
    imageFilters.append ("Portable Pixmap (*.ppm)");
    imageTypes.append ("PPM");
    imageFilters.append ("X11 Bitmap (*.xpm)");
    imageTypes.append ("XPM");
    imageFilters.append ("X11 Bitmap (*.xbm)");
    imageTypes.append ("XBM");
}

QStringList ImageIO::getFilters()
{
    return imageFilters;
}

QString ImageIO::getType(QString filter)
{
    for (int i=0;i<imageFilters.count()+1;i++)
        if (imageFilters.at(i)==filter) return imageTypes.at(i);
    return QString();
}

QString ImageIO::guessType(QString fn)
{
    int i=fn.lastIndexOf(".");
    if (i>=0)
    {
        QString postfix=fn.mid(i+1);
        for (int i=1;i<imageFilters.count();i++)
            if (imageFilters.at(i).contains(postfix)) return imageTypes.at(i);
    }
    return QString();
}

