#include <QApplication>

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
#include "taskeditor.h"
#include "taskmodel.h"
#include "version.h"

#include <sys/types.h>		// To retrieve PID for use in DBUS

#if defined(Q_OS_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define getpid GetCurrentProcessId
#else
#include <unistd.h>
#endif

// DBUS only available on Linux
#if defined(Q_OS_LINUX)
#include <QtDBus/QDBusConnection>
QDBusConnection dbusConnection= QDBusConnection::sessionBus();
#endif

QString vymName;
QString vymVersion;
QString vymHome;
QString vymBuildDate;
QString vymCodeName;
QString vymInstanceName;

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
bool clipboardEmpty;		
bool debug;			// global debugging flag
bool testmode;			// Used to disable saving of autosave setting
FlagRow *systemFlagsMaster; 
FlagRow *standardFlagsMaster;	

#if defined(Q_OS_WIN32)
#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")
    // Get path to settings ini file
    QString getINIPath()
    {
        char module_name_[256];
        GetModuleFileNameA(0, module_name_, sizeof(module_name_));
        PathRemoveFileSpecA( module_name_ );	
        QFileInfo filePath_;
        filePath_ = QString::fromLocal8Bit(module_name_);      
        
        return filePath_.filePath() + "/vym.ini"; 
    }
Settings settings ("InSilmaril",getINIPath()); // Organization, INI path
#else
Settings settings ("InSilmaril","vym"); // Organization, Application name
#endif

QList <Command*> modelCommands;

Options options;
ImageIO imageIO;

int statusbarTime=10000;

int warningCount=0;
int criticalCount=0;
int fatalCount=0;

void msgHandler (QtMsgType type, const char *msg)
{
    switch (type) 
    {
	case QtDebugMsg:
	    fprintf(stderr, "%s\n", msg);
	    break;
	case QtWarningMsg:
	    fprintf(stderr, "Warning: %s\n", msg);
	    warningCount++;
	    break;
	case QtCriticalMsg:
	    fprintf(stderr, "Critical: %s\n", msg);
	    criticalCount++;
	    break;
	case QtFatalMsg:
	    fprintf(stderr, "Fatal: %s\n", msg);
	    fatalCount++;
	    //abort();
    }
}

int main(int argc, char* argv[])
{
    //Q_INIT_RESOURCE (application);

    QApplication app(argc,argv);

    vymName=__VYM_NAME;
    vymVersion=__VYM_VERSION;
    vymBuildDate=__VYM_BUILD_DATE;
    vymCodeName=__VYM_CODENAME;
    vymHome=__VYM_HOME;

    // Install our own handler for messages
    qInstallMsgHandler(msgHandler);

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
    
    taskModel = new TaskModel();

    debug=options.isOn ("debug");
    testmode=options.isOn ("testmode");

    QString pidString=QString ("%1").arg(getpid());

#if defined(Q_OS_LINUX)
    // Register for DBUS
    if (debug) cout << "PID="<<getpid()<<endl;
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
    

    // Use /usr/share/vym or /usr/local/share/vym or . ?
    // First try options
    if (options.isOn ("local"))
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
                if (mfn.endsWith("/bin/vym.exe", Qt::CaseInsensitive))
                {
                    mfn.chop(12);
                    basePath = mfn;
                }
            }

            if (basePath.isEmpty())
                basePath = vymBaseDir.currentPath();

            vymInstallDir.setPath(basePath);
            vymBaseDir.setPath(basePath + "/share/vym");

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

    iconPath=vymBaseDir.path()+"/icons/";
    flagsPath=vymBaseDir.path()+"/flags/";

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
    noteEditor = new NoteEditor();
    noteEditor->setWindowIcon (QPixmap (iconPath+"vym-editor.png"));
    headingEditor = new HeadingEditor();

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

    m.setWindowIcon (QPixmap (iconPath+"vym.png"));
    m.fileNew();

    if (options.isOn ("commands"))
    {
	cout << "Available commands:\n";
	cout << "==================:\n";
	foreach (Command* c, modelCommands)
	    cout << c->getDescription().toStdString() << endl;
    }

    if (options.isOn ("commandslatex"))
    {
	foreach (Command* c, modelCommands)
	    cout << c->getDescriptionLaTeX().toStdString() << endl;
    }

    if (options.isOn ("batch"))
	m.hide();
    else	
    {
	// Paint Mainwindow first time
	qApp->processEvents();
	m.show();
    }

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
		else    
		    QMessageBox::warning(0, error,msg);
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
