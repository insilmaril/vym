#include <QApplication>
#include <QMessageBox>

#include <cstdlib>
#include <iostream>
using namespace std;

#include "command.h"
#include "findwidget.h"
#include "findresultwidget.h"
#include "flagrow.h"
#include "flagrowobj.h"
#include "headingeditor.h"
#include "macros.h"
#include "mainwindow.h"
#include "noteeditor.h"
#include "options.h"
#include "settings.h"
#include "scripteditor.h"
#include "shortcuts.h"
#include "taskeditor.h"
#include "taskmodel.h"
#include "version.h"

#if defined(VYM_DBUS)
#include <sys/types.h>		// To retrieve PID for use in DBUS
#endif

#if defined(Q_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define getpid GetCurrentProcessId
#else
#include <unistd.h>
#endif

#if defined(VYM_DBUS)
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusError>
#endif

QString vymName;
QString vymVersion;
QString vymHome;
QString vymBuildDate;
QString vymCodeName;
QString vymInstanceName;
QString vymPlatform;

QTextStream vout(stdout);        // vymout - Testing for now

bool bugzillaClientAvailable;	// openSUSE specific currently

TaskModel     *taskModel;
TaskEditor    *taskEditor;
ScriptEditor  *scriptEditor;
HeadingEditor *headingEditor;	    
NoteEditor    *noteEditor;	// used in Constr. of LinkableMapObj
// initialized in mainwindow
Main *mainWindow;		// used in BranchObj::select()		
FindWidget *findWidget;
FindResultWidget *findResultWidget;

Macros macros;

uint itemLastID=0;		// Unique ID for all items in all models

QString tmpVymDir;		// All temp files go there, created in mainwindow
QString clipboardDir;		// Clipboard used in all mapEditors
QString clipboardFile;		// Clipboard used in all mapEditors
QDir vymBaseDir;		// Containing all styles, scripts, images, ...
QDir lastImageDir;
QDir lastMapDir;
QDir lastExportDir;
#if defined(Q_OS_WIN32)
QDir vymInstallDir;
#endif
QString iconPath;		// Pointing to icons used for toolbars
QString flagsPath;		// Pointing to flags
QString macroPath;              // Pointing to macros

bool clipboardEmpty;		
bool debug;             // global debugging flag
bool testmode;			// Used to disable saving of autosave setting
FlagRow *systemFlagsMaster; 
FlagRow *standardFlagsMaster;	

Switchboard switchboard;

Settings settings ("InSilmaril","vym"); // Organization, Application name
QString zipToolPath;    // Platform dependant zip tool

QList <Command*> modelCommands;

Options options;
ImageIO imageIO;

int statusbarTime=10000;

int warningCount=0;
int criticalCount=0;
int fatalCount=0;

void msgHandler (QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type)
    {
    case QtDebugMsg:
        fprintf(stderr, "%s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        warningCount++;
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        criticalCount++;
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        fatalCount++;
        //abort();
    }
}

int main(int argc, char* argv[])
{
    QApplication app(argc,argv);

    vymName=__VYM_NAME;
    vymVersion=__VYM_VERSION;
    vymBuildDate=__VYM_BUILD_DATE;
    vymCodeName=__VYM_CODENAME;
    vymHome=__VYM_HOME;

    // Install our own handler for messages
    qInstallMessageHandler(msgHandler);

    // Reading and initializing options commandline options
    options.add ("batch", Option::Switch, "b", "batch");
    options.add ("commands", Option::Switch, "c", "commands");
    options.add ("commandslatex", Option::Switch, "cl", "commandslatex");
    options.add ("debug", Option::Switch, "d", "debug");
    options.add ("help", Option::Switch, "h", "help");
    options.add ("local", Option::Switch, "l", "local");
    options.add ("name", Option::String, "n", "name");
    options.add ("quit", Option::Switch, "q", "quit");
    options.add ("run", Option::String, "r", "run");
    options.add ("restore", Option::Switch, "R", "restore");
    options.add ("shortcuts", Option::Switch, "s", "shortcuts");
    options.add ("shortcutsLaTeX", Option::Switch, "sl", "shortcutsLaTeX");
    options.add ("testmode", Option::Switch, "t", "testmode");
    options.add ("version", Option::Switch, "v","version");
    options.setHelpText (
                "VYM - View Your Mind\n"
                "--------------------\n\n"
                "Information about vym can be found in vym.pdf,\n"
                "which should be part of the vym package.\n"
                "It is also available at the project homepage:\n\n"
                "http://www.InSilmaril.de/vym\n\n"
                "Usage: vym [OPTION]... [FILE]... \n"
                "Open FILEs with vym\n\n"
                "-b           batch       batch mode: hide windows\n"
                "-c           commands	  List all available commands\n"
                "-d           debug       Show debugging output\n"
                "-h           help        Show this help text\n"
                "-l           local       Run with ressources in current directory\n"
                "-n  STRING   name        Set name of instance for DBus access\n"
                "-q           quit        Quit immediatly after start for benchmarking\n"
                "-r  FILE     run         Run script\n"
                "-R           restore     Restore last session\n"
                "-s           shortcuts   Show Keyboard shortcuts on start\n"
                "-sl          LaTeX       Show Keyboard shortcuts in LaTeX format on start\n"
                "-t           testmode    Test mode, e.g. no autosave and changing of its setting\n"
                "-v           version     Show vym version\n"
                );

    if (options.parse())
    {
        cout << endl << qPrintable( options.getHelpText())<<endl;
        return 1;
    }

    if (options.isOn ("version"))
    {
        cout << "VYM - View Your Mind (c) 2004-"<< QDate::currentDate().year()<<" Uwe Drechsel "  << endl
             <<"   Version: "<<__VYM_VERSION <<endl
            <<"Build date: "<<__VYM_BUILD_DATE << endl
           <<"  "<<__VYM_CODENAME<<endl;

        return 0;
    }
    
    // Update some configurations, which were moved in 2.4.0
    // This code should be removed later, e.g. in 2.6.0...
    QStringList settingsChanged;
    settingsChanged  << "readerURL"
                     << "readerPDF"
                     << "autosave/use"
                     << "autosave/ms"
                     << "writeBackupFile"
                     << "printerName"
                     << "printerFormat"
                     << "printerFileName";
    foreach (QString s, settingsChanged)
    {
        if (settings.contains("/mainwindow/" + s))
        {
            settings.setValue("/system/" + s, settings.value("/mainwindow/" + s));
            settings.remove  ("/mainwindow/" + s);
        }
    }

    if (settings.contains( "/mainwindow/readerURL") )
        settings.setValue( "/system/readerURL", settings.value( "/mainwindow/readerURL"));

    taskModel = new TaskModel();

    debug=options.isOn ("debug");
    debug=true;
    testmode=options.isOn ("testmode");

    QString pidString=QString ("%1").arg(getpid());
    if (debug) qDebug()<< "vym PID="<<pidString;

#if defined(VYM_DBUS)
    // Register for DBUS
    QDBusConnection dbusConnection=QDBusConnection::sessionBus();
    if (!dbusConnection.registerService ("org.insilmaril.vym-"+pidString))
    {
        fprintf(stderr, "%s\n",
                qPrintable(QDBusConnection::sessionBus().lastError().message()));
        exit(1);
    }
#endif

    if (options.isOn ("name"))
        vymInstanceName=options.getArg ("name");
    else
        vymInstanceName=pidString;
    
    bool debugBuild=false;
#ifdef QT_DEBUG
    qDebug()<<"QT_DEBUG is set";
    debugBuild=true;
#endif

    // Use /usr/share/vym or /usr/local/share/vym or . ?
    // First try options
    if (options.isOn ("local") || debugBuild)
    {
        vymBaseDir.setPath (vymBaseDir.currentPath());
    } else
        // then look for environment variable
        if (getenv("VYMHOME")!=0)
        {
            vymBaseDir.setPath (getenv("VYMHOME"));
        } else
            // ok, let's find my way on my own
        {
#if defined (Q_OS_MACX)
            vymBaseDir.setPath(vymBaseDir.currentPath() +"/vym.app/Contents/Resources");

#elif defined (Q_OS_WIN32)
            QString basePath;

            wchar_t wbuf[512];
            if (GetModuleFileName(NULL, wbuf, 512))
            {
                QString mfn(QString::fromWCharArray(wbuf));
                mfn.replace('\\', '/');
                int i=mfn.lastIndexOf('/');
                if (i<0)
                {
                    QMessageBox::critical(0,
                        "Error",
                        "Couldn't setup vymBasePath");
                    return 0;
                }
                if (mfn.right(mfn.length() -i -1) != "vym.exe")
                {
                    QMessageBox::critical(0,
                        "Error",
                        "vym executable not known as vym.exe");
                    return 0;
                }
                basePath=mfn.left(i);
            }
            if (basePath.isEmpty())
                basePath = vymBaseDir.currentPath();

            vymInstallDir.setPath(basePath);
            vymBaseDir.setPath(basePath);
#else
            vymBaseDir.setPath ("/usr/share/vym");
            if (!vymBaseDir.exists())
            {
                vymBaseDir.setPath ("/usr/local/share/vym");
                if (!vymBaseDir.exists())
                    vymBaseDir.setPath(vymBaseDir.currentPath() );
            }
#endif
        }

#if defined(Q_OS_MACX)
    vymPlatform = "Mac";
#elif defined(Q_OS_WIN32)
    vymPlatform = "Win32";
    zipToolPath = settings.value("/system/zipToolPath", "c:\\Program Files\\7-Zip\\7z.exe").toString();
#elif defined(Q_OS_LINUX)
    QFile f("/etc/os-release");
    QString flavour="Unknown";
    if (f.exists())
    {
        QString s;
        bool ok = loadStringFromDisk( f.fileName(), s);
        if (ok)
        {
            QRegExp rx("PRETTY_NAME=.*\"(.*)\"");
            rx.setMinimal(true);
            int pos = rx.indexIn(s);
            if (pos > -1) flavour = rx.cap(1);
        }
    }
    vymPlatform = QString ("Linux (%1)").arg(flavour);
#else
    vymPlatform = "Unknown";
#endif
    iconPath=vymBaseDir.path()+"/icons/";
    flagsPath=vymBaseDir.path()+"/flags/";
    macroPath=vymBaseDir.path() + "/macros/";

    // Some directories
    QDir useDir;
    if (options.isOn ("local"))
        useDir=QDir().current();
    else
        useDir=QDir().home();
    lastImageDir=useDir;
    lastMapDir=useDir;
    lastExportDir=useDir;

    if (options.isOn ("help"))
    {
        cout << qPrintable (options.getHelpText())<<endl;
        return 0;
    }

    // Initialize translations
    QTranslator translator (0);
    //translator.load( QString("vym_")+QTextCodec::locale(), vymBaseDir.path() + "/lang");
    translator.load( QString("vym_")+QLocale().name(), vymBaseDir.path() + "/lang");
    app.installTranslator( &translator );

    // Initializing the master rows of flags
    systemFlagsMaster=new FlagRow;
    systemFlagsMaster->setName ("systemFlagsMaster");
    standardFlagsMaster=new FlagRow;
    standardFlagsMaster->setName ("standardFlagsMaster");

    // Initialize editors
    noteEditor = new NoteEditor(QObject::tr("Note Editor","Shortcut group"));
    noteEditor->setWindowIcon (QPixmap (":/vym-editor.png"));
    headingEditor = new HeadingEditor(QObject::tr("Heading Editor","Shortcut group"));

    // Check if there is a BugzillaClient
    QFileInfo fi(vymBaseDir.path()+"/scripts/BugzillaClient.pm");
    //bugzillaClientAvailable=fi.exists();
    bugzillaClientAvailable=true;   //FIXME-2 add real check again

    // Initialize mainwindow
#if defined(Q_OS_WIN32)
    Main m(0, Qt::Window | Qt::MSWindowsOwnDC);
#else
    Main m;
#endif

    // Check for zip tools (at least on windows...)
#if defined(Q_OS_WIN32)
    QFile zipTool(zipToolPath);
    if (!zipTool.exists() )
    {
        QMessageBox::critical( 0, QObject::tr( "Critical Error" ),
                               QObject::tr("Couldn't find tool to unzip data. "
                                           "Please download and install 7z and set "
                                           "path in Settings menu:\n ") +
                                           "http://www.7-zip.org/");
         m.settingsZipTool();
    }
#endif

    m.setWindowIcon (QPixmap (":/vym.png"));
    m.fileNew();

    if (options.isOn ("commands"))
    {
        cout << "Available commands:\n";
        cout << "==================:\n";
        foreach (Command* c, modelCommands)
            cout << c->getDescription().toStdString() << endl;
        return 0;
    }

    if (options.isOn ("commandslatex"))
    {
        foreach (Command* c, modelCommands)
            cout << c->getDescriptionLaTeX().toStdString() << endl;
        return 0;
    }

    if (options.isOn ("batch"))
        m.hide();
    else
    {
        // Paint Mainwindow first time
        qApp->processEvents();
        m.show();
    }

    // Show release notes, if not already done
    m.checkReleaseNotes();

    // Check for updates
    m.checkUpdates();

    if (options.isOn("shortcuts")) switchboard.printASCII();    //FIXME-3 global switchboard and exit after listing

    m.loadCmdLine();

    // For whatever reason tableView is not sorted initially
    taskEditor->sort();

    // Restore last session
    if (options.isOn ("restore"))
        m.fileRestoreSession();

    // Run script
    if (options.isOn ("run"))
    {
        QString script;
        QString fn=options.getArg ("run");
        if ( !fn.isEmpty() )
        {
            QFile f( fn );
            if ( !f.open( QFile::ReadOnly|QFile::Text ) )
            {
                QString error (QObject::tr("Error"));
                QString msg (QObject::tr("Couldn't open \"%1\"\n%2.").arg(fn).arg(f.errorString()));
                if (options.isOn("batch"))
                    qWarning ()<<error+": "+msg;
                else QMessageBox::warning(0, error,msg);
                return 0;
            }

            QTextStream in( &f );
            script=in.readAll();
            f.close();
            m.executeEverywhere (script);
            m.setScriptFile (fn);
        }
    }
    
    // For benchmarking we may want to quit instead of entering event loop
    if (options.isOn ("quit")) return 0;

    // Enable some last minute cleanup
    QObject::connect( &app, SIGNAL(lastWindowClosed()), &app, SLOT(quit()) );

    app.exec();

    int s=warningCount + criticalCount + fatalCount;
    if (s>0) qDebug()<<"vym exiting with:\n"<<warningCount<<" warning messages\n"<<criticalCount<<" critical messages\n"<<fatalCount<<" fatal messages";
    return s;
}
