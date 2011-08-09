#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPixmap>
#include <QLabel>
#include <QTextStream>

#include "file.h"
#include "process.h"

#if defined(Q_OS_WIN32)
#include "mkdtemp.h"
#include <windows.h>
#endif

QString maskPath(QString p)
{
    // Change " " to "\ " to enable blanks in filenames
    p=p.replace(QChar('&'),"\\&");
    return p.replace(QChar(' '),"\\ ");
}

QString convertToRel (const QString &src, const QString &dst)
{
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

QString makeTmpDir (bool &ok, QString prefix)
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
    // Create unique directory e.g. for s="/tmp/vym-XXXXXX"

    // Convert Separators
    s=QDir::convertSeparators(s);

    // Convert QString to string 
    ok=true;
    char *p;
    int bytes=s.length();
    p=(char*) malloc (bytes+1);
    int i;
    for (i=0;i<bytes;i++)
	p[i]=s.at(i).unicode();
    p[bytes]=0;	

    QString r=mkdtemp (p);
    if (r.isEmpty()) ok=false;
    free (p);
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

void copyDir (QDir src, QDir dst)   //FIXME-3 don't use system call
{
    system (QString ("cp -r "+src.path()+"/* "+dst.path()).toUtf8() );

    /*
    ErrorCode err=success;

    Process *cpProc=new Process ();
    QStringList args;
    cpProc->setWorkingDirectory (src.path());
    args <<"-r";
    args <<src.path();
    args <<dst.path();

    cpProc->start ("cp",args);
    if (!cpProc->waitForStarted() )
    {	
	// zip could not be started
	QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
		       QObject::tr("Couldn't start zip to compress data."));
	err=aborted;
    } else
    {
	// zip could be started
	cpProc->waitForFinished();
	if (cpProc->exitStatus()!=QProcess::NormalExit )
	{
	    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
			   QObject::tr("cp didn't exit normally")+
			   "\n" + cpProc->getErrout());
	    err=aborted;
	} else
	{
	    if (cpProc->exitCode()>0)
	    {
		QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
			   QString("cp exit code:  %1").arg(cpProc->exitCode() )+
			   "\n" + cpProc->getErrout() );
		err=aborted;
	    }
	}
    }	// cp could be started
    */
}

void makeSubDirs (const QString &s)
{
    QDir d(s);
    d.mkdir(s);
    d.mkdir ("images");	
    d.mkdir ("flags");	
}

ErrorCode zipDir (const QDir &zipDir, const QString &zipName)
{
    ErrorCode err=success;
    
    // zip the temporary directory
    QStringList args;
    Process *zipProc=new Process ();
    zipProc->setWorkingDirectory (zipDir.path());
    args <<"-r";
    args <<zipName;
    args <<".";

    zipProc->start ("zip",args);
    if (!zipProc->waitForStarted() )
    {	
	// zip could not be started
	QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
		       QObject::tr("Couldn't start zip to compress data."));
	err=aborted;
    } else
    {
	// zip could be started
	zipProc->waitForFinished();
	if (zipProc->exitStatus()!=QProcess::NormalExit )
	{
	    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
			   QObject::tr("zip didn't exit normally")+
			   "\n" + zipProc->getErrout());
	    err=aborted;
	} else
	{
	    if (zipProc->exitCode()>0)
	    {
		QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
			   QString("zip exit code:  %1").arg(zipProc->exitCode() )+
			   "\n" + zipProc->getErrout() );
		err=aborted;
	    }
	}
    }	// zip could be started
    return err;	
}

ErrorCode unzipDir (const QDir &zipDir, const QString &zipName)
{
    ErrorCode err=success;

    // Try to unzip file
#if !defined(Q_OS_WIN32)
    QStringList args;
    Process *zipProc=new Process ();
    zipProc->setWorkingDirectory (zipDir.path());
    args << "-o";   // overwrite existing files!
    args << zipName ;
    args << "-d";
    args << zipDir.path();

    zipProc->start ("unzip",args);
    if (!zipProc->waitForStarted() )
    {
	QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
		       QObject::tr("Couldn't start unzip to decompress data."));
	err=aborted;
	
    } else
    {
	zipProc->waitForFinished();
	if (zipProc->exitStatus()!=QProcess::NormalExit )
	{
	    QMessageBox::critical( 0,QObject::tr( "Critical Error" ),
			   QObject::tr("unzip didn't exit normally") +
			   zipProc->getErrout() );
	    err=aborted;
	} else
	{
	    if (zipProc->exitCode()>0)
	    {
		if (zipProc->exitCode()==9)
		    // no zipped file, but maybe .xml or old version? Try again.
		    err=nozip;
		else	
		{
		    QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
				   QString("unzip exit code:  %1").arg(zipProc->exitCode() ) +
				   zipProc->getErrout() );
		    err=aborted;
		}
	    } 
	}
    }
#else
    // Do this process creation using Win32 API.
    //! Create process.
    PROCESS_INFORMATION piProcInfo;
    STARTUPINFO siStartInfo;

    // Initialize members of the PROCESS_INFORMATION structure.
    ::ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

    // Set up members of the STARTUPINFO structure.
    ::ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
    siStartInfo.cb = sizeof(STARTUPINFO);

    // Create command line.
    QString argv("unzip -o ");
    argv.append(QDir::convertSeparators(zipName));
    argv.append(" -d ");
    argv.append(QDir::convertSeparators(zipDir.path()));

    // Create the child process.
    if( !::CreateProcess(NULL, 
        (LPWSTR)argv.unicode(), // command line
        NULL, // process security attributes
        NULL, // primary thread security attributes
        TRUE, // handles are inherited
        0, // creation flags
        NULL, // use parent's environment
        NULL, // use parent's current directory
        &siStartInfo, // STARTUPINFO pointer
        &piProcInfo) ) // receives PROCESS_INFORMATION
    {
        err = aborted;
    }
    else
    {
        // Wait for it to finish.
        ::WaitForSingleObject( piProcInfo.hProcess, 10000 );
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
    if (!file.open(QFile::WriteOnly | QFile::Text)) {
         qWarning()<<QString("saveStringToDisk: Cannot write file %1:\n%2.")
	      .arg(fname)
	      .arg(file.errorString());
         return false;
    }

    QTextStream out(&file);
    out << s;

    return true;
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


