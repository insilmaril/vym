#include "mainwindow.h"

#include <iostream>
#include <typeinfo>

#ifndef Q_OS_WIN
#include <unistd.h>
#endif

#ifdef Q_OS_LINUX
#include "adaptorvym.h"
#endif

#include "aboutdialog.h"
#include "branchpropeditor.h"
#include "branchitem.h"
#include "command.h"
#include "exportoofiledialog.h"
#include "exports.h"
#include "file.h"
#include "findresultwidget.h"
#include "flagrow.h"
#include "headingeditor.h"
#include "historywindow.h"
#include "imports.h"
#include "lineeditdialog.h"
#include "macros.h"
#include "mapeditor.h"
#include "misc.h"
#include "options.h"
#include "process.h"
#include "scripteditor.h"
#include "settings.h"
#include "shortcuts.h"
#include "noteeditor.h"
#include "task.h"
#include "taskeditor.h"
#include "treeeditor.h"
#include "warningdialog.h"
#include "xlinkitem.h"

QPrinter *printer;

//#include <modeltest.h>    

#if defined(Q_OS_WIN32)
// Define only this structure as opposed to
// including full 'windows.h'. FindWindow
// clashes with the one in Win32 API.
typedef struct _PROCESS_INFORMATION
{
    long hProcess;
    long hThread;
    long dwProcessId;
    long dwThreadId;
} PROCESS_INFORMATION, *LPPROCESS_INFORMATION;
#endif

#if defined(Q_OS_LINUX)
#include <QDBusConnection>
extern QDBusConnection dbusConnection;
#endif

extern NoteEditor    *noteEditor;
extern HeadingEditor *headingEditor;
extern ScriptEditor  *scriptEditor;
extern Main *mainWindow;
extern FindResultWidget *findResultWidget;  
extern TaskEditor *taskEditor;
extern Macros macros;
extern QString tmpVymDir;
extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;
extern int statusbarTime;
extern FlagRow *standardFlagsMaster;	
extern FlagRow *systemFlagsMaster;
extern QString vymName;
extern QString vymVersion;
extern QString vymBuildDate;
extern bool debug;
extern bool testmode;
extern bool bugzillaClientAvailable;

extern QList <Command*> modelCommands;

QMenu* branchAddContextMenu;
QMenu* branchContextMenu;
QMenu* branchLinksContextMenu;
QMenu* branchRemoveContextMenu;
QMenu* branchXLinksContextMenuEdit;
QMenu* branchXLinksContextMenuFollow;
QMenu* canvasContextMenu;
QMenu* floatimageContextMenu;
QMenu* targetsContextMenu;
QMenu* taskContextMenu;
QMenu* fileLastMapsMenu;
QMenu* fileImportMenu;
QMenu* fileExportMenu;

extern Settings settings;
extern Options options;
extern ImageIO imageIO;

extern QDir vymBaseDir;
extern QDir lastImageDir;
extern QDir lastMapDir;
#if defined(Q_OS_WIN32)
extern QDir vymInstallDir;
#endif
extern QString iconPath;
extern QString flagsPath;


Main::Main(QWidget* parent, Qt::WFlags f) : QMainWindow(parent,f)
{
    mainWindow=this;

    setWindowTitle ("VYM - View Your Mind");

    // Load window settings
    #if defined(Q_OS_WIN32)
    if (settings.value("/mainwindow/geometry/maximized", false).toBool())
    {
	setWindowState(Qt::WindowMaximized);
    }
    else
    #endif
    {
	resize (settings.value("/mainwindow/geometry/size", QSize (1024,900)).toSize());
	move   (settings.value("/mainwindow/geometry/pos",  QPoint(50,50)).toPoint());
    }

    // Sometimes we may need to remember old selections
    prevSelection="";

    // Default color
    currentColor=Qt::black;

    // Create unique temporary directory
    bool ok;
    tmpVymDir=makeTmpDir (ok,"vym");
    if (!ok)
    {
	qWarning ("Mainwindow: Could not create temporary directory, failed to start vym");
	exit (1);
    }
    if (debug) qDebug ()<<"tmpDir="<<tmpVymDir;

    // Create direcctory for clipboard
    clipboardDir=tmpVymDir+"/clipboard";
    clipboardFile="map.xml";
    QDir d(clipboardDir);
    d.mkdir (clipboardDir);
    makeSubDirs (clipboardDir);
    clipboardEmpty=true;

    // Remember PID of our friendly webbrowser
    browserPID=new qint64;
    *browserPID=0;

    // Define commands in API (used globally)
    setupAPI();

    // Dock widgets /////////////////////////////////////////////// /
    // FIXME-4 Missing call to updateAction, when widget is closed
    QDockWidget *dw;
    dw = new QDockWidget (tr("Note Editor"));
    dw->setWidget (noteEditor);
    dw->setObjectName ("NoteEditor");
    dw->hide();
    noteEditorDW=dw;
    addDockWidget (Qt::LeftDockWidgetArea,dw);

    dw = new QDockWidget (tr("Heading Editor"));
    dw->setWidget (headingEditor);
    dw->setObjectName ("HeadingEditor");
    dw->hide();
    headingEditorDW=dw;
    addDockWidget (Qt::BottomDockWidgetArea,dw);

    dw = new QDockWidget (tr("Script Editor"));
    dw->setWidget (scriptEditor);
    dw->setObjectName ("ScriptEditor");
    dw->hide();
    scriptEditorDW=dw;
    addDockWidget (Qt::LeftDockWidgetArea,dw);

    findResultWidget=new FindResultWidget ();
    dw= new QDockWidget (tr ("Search results list","FindResultWidget"));
    dw->setWidget (findResultWidget);
    dw->setObjectName ("FindResultWidget");
    dw->hide();	
    addDockWidget (Qt::RightDockWidgetArea,dw);
    connect (
	findResultWidget, SIGNAL (noteSelected (QString, int)),
	this, SLOT (selectInNoteEditor (QString, int)));
    connect (
	findResultWidget, SIGNAL (findPressed (QString) ), 
	this, SLOT (editFindNext(QString) ) );

    taskEditor = new TaskEditor ();
    dw= new QDockWidget (tr ("Task list","TaskEditor"));
    dw->setWidget (taskEditor);
    dw->setObjectName ("TaskEditor");
    dw->hide();	
    addDockWidget (Qt::TopDockWidgetArea,dw);
    connect (dw, SIGNAL (visibilityChanged(bool ) ), this, SLOT (updateActions()));

    scriptEditor = new ScriptEditor(this);
    dw= new QDockWidget (tr ("Script Editor","ScriptEditor"));
    dw->setWidget (scriptEditor);
    dw->setObjectName ("ScriptEditor");
    dw->hide();	
    addDockWidget (Qt::LeftDockWidgetArea,dw);

    branchPropertyEditor = new BranchPropertyEditor();
    dw = new QDockWidget (tr("Property Editor","PropertyEditor"));
    dw->setWidget (branchPropertyEditor);
    dw->setObjectName ("PropertyEditor");
    dw->hide();
    addDockWidget (Qt::LeftDockWidgetArea,dw);

    historyWindow=new HistoryWindow();
    dw = new QDockWidget (tr("History window","HistoryWidget"));
    dw->setWidget (historyWindow);
    dw->setObjectName ("HistoryWidget");
    dw->hide();
    addDockWidget (Qt::RightDockWidgetArea,dw);
    connect (dw, SIGNAL (visibilityChanged(bool ) ), this, SLOT (updateActions()));

    // Connect NoteEditor, so that we can update flags if text changes
    connect (noteEditor, SIGNAL (textHasChanged() ), this, SLOT (updateNoteFlag()));
    connect (noteEditor, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

    // Connect heading editor
    connect (headingEditor, SIGNAL (textHasChanged() ), this, SLOT (updateHeading()));

    connect( scriptEditor, SIGNAL( runScript ( QString ) ), 
	this, SLOT( execute( QString ) ) );

    // Initialize some settings, which are platform dependant
    QString p,s;

	// application to open URLs
	p="/mainwindow/readerURL";
	#if defined(Q_OS_LINUX)
	    s=settings.value (p,"xdg-open").toString();
	#else
	    #if defined(Q_OS_MACX)
		s=settings.value (p,"/usr/bin/open").toString();

	    #else
		#if defined(Q_OS_WIN32)
		    // Assume that system has been set up so that
		    // Explorer automagically opens up the URL
		    // in the user's preferred browser.
		    s=settings.value (p,"explorer").toString();
		#else
		    s=settings.value (p,"mozilla").toString();
		#endif
	    #endif
	#endif
	settings.setValue( p,s);

	// application to open PDFs
	p="/mainwindow/readerPDF";
	#if defined(Q_OS_LINUX)
	    s=settings.value (p,"xdg-open").toString();
	#else
	    #if defined(Q_OS_MACX)
		s=settings.value (p,"/usr/bin/open").toString();
	    #elif defined(Q_OS_WIN32)
		s=settings.value (p,"acrord32").toString();
	    #else
		s=settings.value (p,"acroread").toString();
	    #endif
	#endif
	settings.setValue( p,s);

    // width of xLinksMenu
    xLinkMenuWidth=60;

    // Create Layout
    QWidget* centralWidget = new QWidget (this);
    QVBoxLayout *layout=new QVBoxLayout (centralWidget);
    setCentralWidget(centralWidget);	

    // Create tab widget which holds the maps
    tabWidget= new QTabWidget (centralWidget);
    connect(tabWidget, SIGNAL( currentChanged( QWidget * ) ), 
            this, SLOT( editorChanged( QWidget * ) ) );

    // Allow closing of tabs (introduced in Qt 4.5)
    tabWidget->setTabsClosable( true ); 
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), 
            this, SLOT( closeTab(int) ));

    layout->addWidget (tabWidget);

    setupFileActions();
    setupEditActions();
    setupSelectActions();
    setupFormatActions();
    setupViewActions();
    setupModeActions();
    setupNetworkActions();
    setupSettingsActions();
    setupContextMenus();
    setupMacros();
    setupToolbars();
    setupFlagActions();

    if (options.isOn("shortcuts")) switchboard.printASCII();
    if (options.isOn("shortcutsLaTeX")) switchboard.printLaTeX();

    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) setupTestActions();
    setupHelpActions();

    // Status bar and progress bar there
    statusBar();
    progressMax=0;
    progressCounter=0;
    progressCounterTotal=0;

    progressDialog.setAutoReset(false);
    progressDialog.setAutoClose(false);
    progressDialog.setMinimumWidth (600);
    //progressDialog.setWindowModality (Qt::WindowModal);   // That forces mainwindo to update and slows down
    progressDialog.setCancelButton (NULL);

    restoreState (settings.value("/mainwindow/state",0).toByteArray());

    // Global Printer
    printer=new QPrinter (QPrinter::HighResolution );	

    updateGeometry();

#if defined(Q_OS_LINUX)
    // Announce myself on DBUS
    new AdaptorVym (this);    // Created and not deleted as documented in Qt
    if (!QDBusConnection::sessionBus().registerObject ("/vym",this))
	qWarning ("MainWindow: Couldn't register DBUS object!");
#endif    
}

Main::~Main()
{
    // Save Settings

    if (!testmode) 
    {
	#if defined(Q_OS_WIN32)
	settings.setValue ("/mainwindow/geometry/maximized", isMaximized());
	#endif
	settings.setValue ("/mainwindow/geometry/size", size());
	settings.setValue ("/mainwindow/geometry/pos", pos());
	settings.setValue ("/mainwindow/state",saveState(0));

	settings.setValue ("/mainwindow/view/AntiAlias",actionViewToggleAntiAlias->isChecked());
	settings.setValue ("/mainwindow/view/SmoothPixmapTransform",actionViewToggleSmoothPixmapTransform->isChecked());
	settings.setValue( "/version/version", vymVersion );
	settings.setValue( "/version/builddate", vymBuildDate );
	settings.setValue( "/mainwindow/autosave/use",actionSettingsAutosaveToggle->isChecked() );
	settings.setValue ("/mainwindow/autosave/ms", settings.value("/mainwindow/autosave/ms",60000)); 
	settings.setValue ("/mainwindow/autoLayout/use",actionSettingsAutoLayoutToggle->isChecked() );
	settings.setValue( "/mapeditor/editmode/autoSelectNewBranch",actionSettingsAutoSelectNewBranch->isChecked() );
	settings.setValue( "/mainwindow/writeBackupFile",actionSettingsWriteBackupFile->isChecked() );
	settings.setValue( "/mapeditor/editmode/autoSelectText",actionSettingsAutoSelectText->isChecked() );
	settings.setValue( "/mapeditor/editmode/autoEditNewBranch",actionSettingsAutoEditNewBranch->isChecked() );
	settings.setValue( "/mapeditor/editmode/useFlagGroups",actionSettingsUseFlagGroups->isChecked() );
	settings.setValue( "/export/useHideExport",actionSettingsUseHideExport->isChecked() );
    }
    //FIXME-4 save scriptEditor settings

    // call the destructors
    delete noteEditor;	    // FIXME-4 shouldn't this be done in main?
    delete historyWindow;
    delete branchPropertyEditor;

    // Remove temporary directory
    removeDir (QDir(tmpVymDir));
}

void Main::loadCmdLine()
{
    QStringList flist=options.getFileList();
    QStringList::Iterator it=flist.begin();

    initProgressCounter (flist.count());
    while (it !=flist.end() )
    {
	FileType type=getMapType (*it);
	fileLoad (*it, NewMap,type);
	*it++;
    }	
    removeProgressCounter();
}


void Main::statusMessage(const QString &s)
{
    // Surpress messages while progressdialog during 
    // load is active
    statusBar()->showMessage( s,statusbarTime);
}

void Main::setProgressMaximum (int max)	{
    if (progressCounter==0)
    {
	// Init range only on first time, when progressCounter still 0
	// Normalize range to 1000
	progressDialog.setRange (0,1000);
	progressDialog.setValue (0);
    }
    progressCounter++;	// Another map is loaded

    progressMax=max*1000;
    //qDebug() << "Main  max="<<max<<"  v="<<progressDialog.value();
    if (!options.isOn("batch"))
	progressDialog.show();
    else	
	progressDialog.hide();
}

void Main::addProgressValue (float v) 

{
/*
    qDebug() << "addVal v="<<v
	 <<"  cur="<<progressDialog.value()
	 <<"  pCounter="<<progressCounter
	 <<"  pCounterTotal="<<progressCounterTotal
	 ;
	 */
    if (!options.isOn("batch"))
	progressDialog.show();
    else	
	progressDialog.hide();
    progressDialog.setValue ( (v + progressCounter -1)*1000/progressCounterTotal );
    progressDialog.repaint();
}

void Main::initProgressCounter(uint n)
{
    progressCounterTotal=n;
}

void Main::removeProgressCounter()
{
    // Hide dialog again
    progressCounter=0;
    progressCounterTotal=0;
    progressDialog.reset();
    progressDialog.hide();
}

void Main::closeEvent (QCloseEvent* event)
{
    if (fileExitVYM())
	event->ignore();
    else	
	event->accept();
}

// Define commands for models
void Main::setupAPI()
{
    Command *c = new Command ("addBranch",Command::Branch);
    c->addPar (Command::Int, true, "Index of new branch");
    modelCommands.append(c);

    c=new Command ("addBranchBefore",Command::Branch);
    modelCommands.append(c);

    c=new Command ("addMapCenter",Command::Any);
    c->addPar (Command::Double,false, "Position x");
    c->addPar (Command::Double,false, "Position y");
    modelCommands.append(c);

    c=new Command ("addMapInsert",Command::Any);
    c->addPar (Command::String,false, "Filename of map to load");
    c->addPar (Command::Int,true, "Index where map is inserted");
    c->addPar (Command::Int,true, "Content filter");
    modelCommands.append(c);

    c=new Command ("addMapReplace",Command::Branch);
    c->addPar (Command::String,false, "Filename of map to load");
    modelCommands.append(c);

    c=new Command ("addSlide",Command::Branch);
    modelCommands.append(c);

    c=new Command ("addXLink",Command::BranchLike);
    c->addPar (Command::String, false, "Begin of XLink");
    c->addPar (Command::String, false, "End of XLink");
    c->addPar (Command::Int,    true, "Width of XLink");
    c->addPar (Command::Color,  true, "Color of XLink");
    c->addPar (Command::String, true, "Penstyle of XLink");
    modelCommands.append(c);

    c=new Command ("branchCount",Command::BranchLike);
    modelCommands.append(c);

    c=new Command ("centerOnID",Command::Any);
    c->addPar (Command::String,false, "UUID of object to center on");
    modelCommands.append(c);

    c=new Command ("clearFlags",Command::BranchLike);
    modelCommands.append(c);

    c=new Command ("colorBranch",Command::Branch);
    c->addPar (Command::Color,true, "New color");
    modelCommands.append(c);

    c=new Command ("colorSubtree",Command::Branch);
    c->addPar (Command::Color,true, "New color");
    modelCommands.append(c);

    c=new Command ("copy",Command::BranchOrImage);
    modelCommands.append(c);

    c=new Command ("cut",Command::BranchOrImage);
    modelCommands.append(c);

    c=new Command ("cycleTask",Command::BranchOrImage);
    c->addPar (Command::Bool,true, "True, if cycling in reverse order");
    modelCommands.append(c);

    c=new Command ("delete",Command::TreeItem);
    modelCommands.append(c);

    c=new Command ("deleteChildren",Command::Branch);
    modelCommands.append(c);

    c=new Command ("deleteKeepChildren",Command::Branch);
    modelCommands.append(c);

    c=new Command ("deleteSlide",Command::Any);
    c->addPar (Command::Int,false,"Index of slide to delete");
    modelCommands.append(c);

    c=new Command ("exportAO",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportASCII",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportHTML",Command::Any);
    c->addPar (Command::String,false,"Path used for export");
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportImage",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    c->addPar (Command::String,true,"Image format");
    modelCommands.append(c);

    c=new Command ("exportLast",Command::Any);
    modelCommands.append(c);

    c=new Command ("exportLaTeX",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportImpress",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    c->addPar (Command::String,false,"Configuration file for export");
    modelCommands.append(c);

    c=new Command ("exportPDF",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportPDF",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportSVG",Command::Any);
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("exportXML",Command::Any);
    c->addPar (Command::String,false,"Path used for export");
    c->addPar (Command::String,false,"Filename for export");
    modelCommands.append(c);

    c=new Command ("getDestPath",Command::Any);
    modelCommands.append(c);

    c=new Command ("getFileDir",Command::Any);
    modelCommands.append(c);

    c=new Command ("getFrameType",Command::Branch);
    modelCommands.append(c);

    c=new Command ("getHeading",Command::TreeItem);
    modelCommands.append(c);

    c=new Command ("getMapAuthor",Command::Any);
    modelCommands.append(c);

    c=new Command ("getMapComment",Command::Any);
    modelCommands.append(c);

    c=new Command ("getMapTitle",Command::Any);
    modelCommands.append(c);

    c=new Command ("getSelectString",Command::TreeItem);
    modelCommands.append(c);

    c=new Command ("getTaskSleepDays",Command::Branch);
    modelCommands.append(c);

    c=new Command ("getURL",Command::TreeItem); 
    modelCommands.append(c);

    c=new Command ("getVymLink",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("getXLinkColor",Command::XLinkItem);
    modelCommands.append(c);

    c=new Command ("getXLinkWidth",Command::XLinkItem);
    modelCommands.append(c);

    c=new Command ("getXLinkPenStyle",Command::XLinkItem);
    modelCommands.append(c);

    c=new Command ("hasActiveFlag",Command::TreeItem);
    c->addPar (Command::String,false,"Name of flag");
    modelCommands.append(c);

    c=new Command ("hasTask",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("importDir",Command::Branch);
    c->addPar (Command::String,false,"Directory name to import");
    modelCommands.append(c);

    c=new Command ("isScrolled",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("loadImage",Command::Branch); 
    c->addPar (Command::String,false,"Filename of image");
    modelCommands.append(c);

    c=new Command ("loadNote",Command::Branch); 
    c->addPar (Command::String,false,"Filename of note");
    modelCommands.append(c);


    c=new Command ("moveDown",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("moveUp",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("moveSlideDown",Command::Any); 
    modelCommands.append(c);

    c=new Command ("moveSlideUp",Command::Any); 
    modelCommands.append(c);

    c=new Command ("move",Command::BranchOrImage); 
    c->addPar (Command::Double,false,"Position x");
    c->addPar (Command::Double,false,"Position y");
    modelCommands.append(c);

    c=new Command ("moveRel",Command::BranchOrImage); 
    c->addPar (Command::Double,false,"Position x");
    c->addPar (Command::Double,false,"Position y");
    modelCommands.append(c);

    c=new Command ("nop",Command::Any); 
    modelCommands.append(c);

    c=new Command ("note2URLs",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("paste",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("redo",Command::Any); 
    modelCommands.append(c);

    c=new Command ("relinkTo",Command::TreeItem);   // FIXME different number of parameters for Image or Branch
    c->addPar (Command::String,false,"Selection string of parent");
    c->addPar (Command::Int,false,"Index position");
    c->addPar (Command::Double,true,"Position x");
    c->addPar (Command::Double,true,"Position y");
    modelCommands.append(c);

    c=new Command ("saveImage",Command::Image); 
    c->addPar (Command::String,false,"Filename of image to save");
    c->addPar (Command::String,false,"Format of image to save");
    modelCommands.append(c);

    c=new Command ("saveNote",Command::Branch); 
    c->addPar (Command::String,false,"Filename of note to save");
    modelCommands.append(c);

    c=new Command ("scroll",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("select",Command::Any); 
    c->addPar (Command::String,false,"Selection string");
    modelCommands.append(c);

    c=new Command ("selectID",Command::Any); 
    c->addPar (Command::String,false,"Unique ID");
    modelCommands.append(c);

    c=new Command ("selectLastBranch",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("selectLastImage",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("selectLatestAdded",Command::Any); 
    modelCommands.append(c);

    c=new Command ("setFlag",Command::TreeItem); 
    c->addPar (Command::String,false,"Name of flag");
    modelCommands.append(c);

    c=new Command ("setTaskSleep",Command::Branch); 
    c->addPar (Command::String,false,"Days to sleep");
    modelCommands.append(c);

    c=new Command ("setFrameIncludeChildren",Command::BranchOrImage); 
    c->addPar (Command::Bool,false,"Include or don't include children in frame");
    modelCommands.append(c);

    c=new Command ("setFrameType",Command::BranchOrImage); 
    c->addPar (Command::String,false,"Type of frame");
    modelCommands.append(c);

    c=new Command ("setFramePenColor",Command::BranchOrImage); 
    c->addPar (Command::Color,false,"Color of frame border line");
    modelCommands.append(c);

    c=new Command ("setFrameBrushColor",Command::BranchOrImage); 
    c->addPar (Command::Color,false,"Color of frame background");
    modelCommands.append(c);

    c=new Command ("setFramePadding",Command::BranchOrImage); 
    c->addPar (Command::Int,false,"Padding around frame");
    modelCommands.append(c);

    c=new Command ("setFrameBorderWidth",Command::BranchOrImage); 
    c->addPar (Command::Int,false,"Width of frame borderline");
    modelCommands.append(c);

    c=new Command ("setHeading",Command::TreeItem); 
    c->addPar (Command::String,false,"New heading");
    modelCommands.append(c);

    c=new Command ("setHideExport",Command::BranchOrImage); 
    c->addPar (Command::Bool,false,"Set if item should be visible in export");
    modelCommands.append(c);

    c=new Command ("setIncludeImagesHorizontally",Command::Branch); 
    c->addPar (Command::Bool,false,"Set if images should be included horizontally in parent branch");
    modelCommands.append(c);

    c=new Command ("setIncludeImagesVertically",Command::Branch); 
    c->addPar (Command::Bool,false,"Set if images should be included vertically in parent branch");
    modelCommands.append(c);

    c=new Command ("setHideLinksUnselected",Command::BranchOrImage); 
    c->addPar (Command::Bool,false,"Set if links of items should be visible for unselected items");
    modelCommands.append(c);

    c=new Command ("setMapAnimCurve",Command::Any); 
    c->addPar (Command::Int,false,"EasingCurve used in animation in MapEditor");
    modelCommands.append(c);

    c=new Command ("setMapAuthor",Command::Any); 
    c->addPar (Command::String,false,"");
    modelCommands.append(c);

    c=new Command ("setMapAnimDuration",Command::Any); 
    c->addPar (Command::Int,false,"Duration of animation in MapEditor in milliseconds");
    modelCommands.append(c);

    c=new Command ("setMapBackgroundColor",Command::Any); 
    c->addPar (Command::Color,false,"Color of map background");
    modelCommands.append(c);

    c=new Command ("setMapComment",Command::Any); 
    c->addPar (Command::String,false,"");
    modelCommands.append(c);

    c=new Command ("setMapTitle",Command::Any); 
    c->addPar (Command::String,false,"");
    modelCommands.append(c);

    c=new Command ("setMapDefLinkColor",Command::Any); 
    c->addPar (Command::Color,false,"Default color of links");
    modelCommands.append(c);

    c=new Command ("setMapLinkStyle",Command::Any); 
    c->addPar (Command::String,false,"Link style in map");
    modelCommands.append(c);

    c=new Command ("setMapRotation",Command::Any); 
    c->addPar (Command::Double,false,"Rotation of map");
    modelCommands.append(c);

    c=new Command ("setMapTitle",Command::Any); 
    c->addPar (Command::String,false,"");
    modelCommands.append(c);

    c=new Command ("setMapZoom",Command::Any); 
    c->addPar (Command::Double,false,"Zoomfactor of map");
    modelCommands.append(c);

    c=new Command ("setNote",Command::Branch); 
    c->addPar (Command::String,false,"Note of branch");
    modelCommands.append(c);

    c=new Command ("setScale",Command::Image); 
    c->addPar (Command::Double,false,"Scale image x");
    c->addPar (Command::Double,false,"Scale image y");
    modelCommands.append(c);

    c=new Command ("setSelectionColor",Command::Any); 
    c->addPar (Command::Color,false,"Color of selection box");
    modelCommands.append(c);

    c=new Command ("setURL",Command::TreeItem); 
    c->addPar (Command::String,false,"URL of TreeItem");
    modelCommands.append(c);

    c=new Command ("setVymLink",Command::Branch); 
    c->addPar (Command::String,false,"Vymlink of branch");
    modelCommands.append(c);

    c=new Command ("sleep",Command::Any); 
    c->addPar (Command::Int,false,"Sleep (seconds)");
    modelCommands.append(c);

    c=new Command ("sortChildren",Command::Branch); 
    c->addPar (Command::Bool,true,"Sort children of branch in revers order if set");
    modelCommands.append(c);

    c=new Command ("toggleFlag",Command::Branch); 
    c->addPar (Command::String,false,"Name of flag to toggle");
    modelCommands.append(c);

    c=new Command ("toggleFrameIncludeChildren",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("toggleScroll",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("toggleTarget",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("toggleTask",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("undo",Command::Any); 
    modelCommands.append(c);

    c=new Command ("unscroll",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("unscrollChildren",Command::Branch); 
    modelCommands.append(c);

    c=new Command ("unsetFlag",Command::Branch); 
    c->addPar (Command::String,false,"Name of flag to unset");
    modelCommands.append(c);
}

// File Actions
void Main::setupFileActions()
{
    QMenu *fileMenu = menuBar()->addMenu ( tr ("&Map","Toolbar for file actions") );

    QAction *a;
    a = new QAction(QPixmap( iconPath+"filenew.png"), tr( "&New map","File menu" ),this);
    a->setShortcut ( Qt::CTRL + Qt::Key_N );
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNew() ) );
    actionFileNew=a;

    a = new QAction(QPixmap( iconPath+"filenewcopy.png"), tr( "&Copy to new map","File menu" ),this);
    a->setShortcut ( Qt::CTRL +Qt::SHIFT + Qt::Key_N );	 
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNewCopy() ) );
    actionFileNewCopy=a;

    a = new QAction( QPixmap( iconPath+"fileopen.png"), tr( "&Open..." ,"File menu"),this);
    a->setShortcut ( Qt::CTRL + Qt::Key_L );	 
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileLoad() ) );
    actionFileOpen=a;

    a = new QAction( tr( "&Restore last session" ,"File menu"),this);
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileRestoreSession() ) );

    fileLastMapsMenu = fileMenu->addMenu (tr("Open Recent","File menu"));
    fileMenu->addSeparator();

    a = new QAction( QPixmap( iconPath+"filesave.png"), tr( "&Save...","File menu" ), this);
    a->setShortcut (Qt::CTRL + Qt::Key_S );	 
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
    actionListMap.append (a);
    actionFileSave=a;

    a = new QAction( QPixmap(iconPath+"filesaveas.png"), tr( "Save &As...","File menu" ), this);
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );

    fileMenu->addSeparator();

    fileImportMenu = fileMenu->addMenu (tr("Import","File menu"));

    a = new QAction(tr("KDE 4 Bookmarks","Import filters"), this);
    switchboard.addConnection(fileImportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportKDE4Bookmarks() ) );

    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) 
    {
	a = new QAction( QPixmap(), tr("Firefox Bookmarks","Import filters"),this);
	switchboard.addConnection(fileImportMenu, a,tr("File","Shortcut group"));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFirefoxBookmarks() ) );
    }	

    a = new QAction("Freemind...",this);
    switchboard.addConnection(fileImportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFreemind() ) );

    a = new QAction("Mind Manager...",this);
    switchboard.addConnection(fileImportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportMM() ) );

    a = new QAction( tr( "Import Dir%1","Import Filters").arg("... (experimental)"), this);
    switchboard.addConnection(fileImportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportDir() ) );

    fileExportMenu = fileMenu->addMenu (tr("Export","File menu"));

    a = new QAction( QPixmap(iconPath+"file-document-export.png"),tr("Repeat last export (%1)").arg("-"), this);
    a->setShortcut (Qt::ALT + Qt::Key_E);	    
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportLast() ) );
    actionFileExportLast=a;

    a = new QAction(  "Webpage (HTML)...",this );
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportHTML() ) );

    a = new QAction( "Text (ASCII)...", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportASCII() ) );

    a = new QAction( "Text (A&O report)...", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportAO() ) );

    a = new QAction( tr("Image%1","File export menu").arg("..."), this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportImage() ) );

    a = new QAction( tr("PDF%1","File export menu").arg("..."), this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportPDF() ) );

    a = new QAction( tr("SVG%1","File export menu").arg("..."), this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportSVG() ) );

    a = new QAction( "Open Office...", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportImpress() ) );

    a = new QAction( "XML..." , this );
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportXML() ) );

    a = new QAction( tr("KDE 4 Bookmarks","File menu"), this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportKDE4Bookmarks() ) );

    a = new QAction( "Spreadsheet (CSV)... (experimental)", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportCSV() ) );

    a = new QAction( "Taskjuggler... (experimental)", this );
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportTaskjuggler() ) );

    a = new QAction( "OrgMode... (experimental)", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportOrgMode() ) );

    a = new QAction( "LaTeX... (experimental)", this);
    switchboard.addConnection(fileExportMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportLaTeX() ) );

    fileMenu->addSeparator();

    a = new QAction(QPixmap( iconPath+"fileprint.png"), tr( "&Print")+QString("..."), this);
    a->setShortcut (Qt::CTRL + Qt::Key_P );
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( filePrint() ) );
    actionListMap.append (a);
    actionFilePrint=a;

    a = new QAction( QPixmap(iconPath+"fileclose.png"), tr( "&Close Map","File menu" ), this);
    a->setShortcut (Qt::CTRL + Qt::Key_W );	 
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileCloseMap() ) );

    a = new QAction(QPixmap(iconPath+"exit.png"), tr( "E&xit","File menu")+" "+vymName, this);
    a->setShortcut (Qt::CTRL + Qt::Key_Q );	  
    switchboard.addConnection(fileMenu, a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExitVYM() ) );
}


//Edit Actions
void Main::setupEditActions()
{
    QMenu *editMenu = menuBar()->addMenu( tr("E&dit","Edit menu") );

    QAction *a;
    a = new QAction( QPixmap( iconPath+"undo.png"), tr( "&Undo","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUndo() ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_Z );	  
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    actionListMap.append (a);
    actionUndo=a;

    a = new QAction( QPixmap( iconPath+"redo.png"), tr( "&Redo","Edit menu" ), this); 
    a->setShortcut (Qt::CTRL + Qt::Key_Y );	  
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editRedo() ) );
    actionListMap.append (a);
    actionRedo=a;

    editMenu->addSeparator();
    a = new QAction(QPixmap( iconPath+"editcopy.png"), tr( "&Copy","Edit menu" ), this);
    a->setShortcut (Qt::CTRL + Qt::Key_C );	 
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editCopy() ) );
    actionListMap.append(a);
    actionCopy=a;

    a = new QAction(QPixmap( iconPath+"editcut.png" ), tr( "Cu&t","Edit menu" ), this);
    a->setShortcut (Qt::CTRL + Qt::Key_X );	  
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editCut() ) );
    actionListMap.append(a);
    actionCut=a;

    a = new QAction(QPixmap( iconPath+"editpaste.png"), tr( "&Paste","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editPaste() ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_V );	  
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    actionListMap.append(a);
    actionPaste=a;

    // Shortcut to delete selection
    a = new QAction( tr( "Delete Selection","Edit menu" ),this);
    a->setShortcut ( Qt::Key_Delete);		 
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteSelection() ) );
    actionListItems.append (a);
    actionDelete=a;

    // Shortcut to add attribute
    a= new QAction(tr( "Add attribute" ), this);
    if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
    {
	//a->setShortcut ( Qt::Key_Q);	
	a->setShortcutContext (Qt::WindowShortcut);
	switchboard.addConnection(a,tr("Edit","Shortcut group"));
    }
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddAttribute() ) );
    actionAddAttribute= a;


    // Shortcut to add mapcenter
    a= new QAction(QPixmap(iconPath+"newmapcenter.png"),tr( "Add mapcenter","Canvas context menu" ), this);
    a->setShortcut ( Qt::Key_C);    
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddMapCenter() ) );
    actionListMap.append (a);
    actionAddMapCenter = a;


    // Shortcut to add branch
    a = new QAction(QPixmap(iconPath+"newbranch.png"), tr( "Add branch as child","Edit menu" ), this);
    a->setShortcut (Qt::Key_A);		
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranch() ) );
    actionListBranches.append(a);
    actionAddBranch=a;


    // Add branch by inserting it at selection
    a = new QAction(tr( "Add branch (insert)","Edit menu" ),this);
    a->setShortcut ( Qt::ALT + Qt::Key_A );	 
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBefore() ) );
    actionListBranches.append(a);
    actionAddBranchBefore=a;

    // Add branch above
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
    a->setShortcut (Qt::SHIFT+Qt::Key_Insert );	  
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionAddBranchAbove=a;
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
    a->setShortcut (Qt::SHIFT+Qt::Key_A );	 
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
    actionListBranches.append(a);

    // Add branch below 
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
    a->setShortcut (Qt::CTRL +Qt::Key_Insert );	  
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionAddBranchBelow=a;
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
    a->setShortcut (Qt::CTRL +Qt::Key_A );	 
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
    actionListBranches.append(a);

    a = new QAction(QPixmap(iconPath+"up.png" ), tr( "Move up","Edit menu" ), this);
    a->setShortcut (Qt::Key_PageUp );		
    a->setShortcutContext (Qt::WidgetShortcut);
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveUp() ) );
    actionListBranches.append (a);
    actionMoveUp=a;

    a = new QAction( QPixmap( iconPath+"down.png"), tr( "Move down","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveDown() ) );
    a->setShortcut ( Qt::Key_PageDown );	  
    a->setShortcutContext (Qt::WidgetShortcut);
    a->setEnabled (false);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    actionListBranches.append (a);
    actionMoveDown=a;

    a = new QAction(QPixmap(), tr( "&Detach","Context menu" ),this);
    a->setStatusTip ( tr( "Detach branch and use as mapcenter","Context menu" ) );
    a->setShortcut ( Qt::Key_D );		 
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editDetach() ) );
    actionListBranches.append (a);
    actionDetach=a;

    a = new QAction( QPixmap(iconPath+"editsort.png" ), tr( "Sort children","Edit menu" ), this );
    connect( a, SIGNAL( activated() ), this, SLOT( editSortChildren() ) );
    a->setEnabled (true);
    a->setShortcut ( Qt::Key_O );		  
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    actionListBranches.append (a);
    actionSortChildren=a;

    a = new QAction( QPixmap(iconPath+"editsortback.png" ), tr( "Sort children backwards","Edit menu" ), this );
    connect( a, SIGNAL( activated() ), this, SLOT( editSortBackChildren() ) );
    a->setEnabled (true);
    a->setShortcut ( Qt::SHIFT + Qt::Key_O );		
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    actionListBranches.append (a);
    actionSortBackChildren=a;	

    a = new QAction( QPixmap(flagsPath+"flag-scrolled-right.png"), tr( "Scroll branch","Edit menu" ), this);
    a->setShortcut ( Qt::Key_S );		  
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleScroll() ) );
    a->setEnabled (false);
    a->setCheckable(true);
    addAction (a);
    actionListBranches.append(a);
    actionToggleScroll=a;

    a = new QAction( QPixmap(), tr( "Expand all branches","Edit menu" ), this);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editExpandAll() ) );
    actionExpandAll=a;
    actionExpandAll->setEnabled (false);
    actionExpandAll->setCheckable(false);
    actionListBranches.append(actionExpandAll);
    addAction (a);

    a = new QAction( QPixmap(), tr( "Expand one level","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Greater );	    
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editExpandOneLevel() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    addAction (a);
    actionListBranches.append(a);
    actionExpandOneLevel=a;

    a = new QAction( QPixmap(), tr( "Collapse one level","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Less + Qt::CTRL);	
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editCollapseOneLevel() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    actionListBranches.append(a);
    addAction (a);
    actionCollapseOneLevel=a;

    a = new QAction( QPixmap(), tr( "Collapse unselected levels","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Less);	  
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editCollapseUnselected() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    actionListBranches.append(a);
    addAction (a);
    actionCollapseUnselected=a;

    a = new QAction( tr( "Unscroll children","Edit menu" ), this);
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editUnscrollChildren() ) );
    actionListBranches.append (a);

    a = new QAction( tr( "Grow selection","Edit menu" ), this);
    a->setShortcut ( Qt::CTRL + Qt::Key_Plus);	  
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editGrowSelectionSize() ) );
    actionListItems.append (a);
    actionGrowSelectionSize=a;

    a = new QAction( tr( "Shrink selection","Edit menu" ), this);
    a->setShortcut ( Qt::CTRL + Qt::Key_Minus);	   
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editShrinkSelectionSize() ) );
    actionListItems.append (a);
    actionShrinkSelectionSize=a;

    a = new QAction( tr( "Reset selection size","Edit menu" ), this);
    a->setShortcut ( Qt::CTRL + Qt::Key_0);	    
    switchboard.addConnection(editMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editResetSelectionSize() ) );
    actionListItems.append (a);
    actionResetSelectionSize=a;

    editMenu->addSeparator();

    a = new QAction( QPixmap(flagsPath+"flag-url.png"), tr( "Open URL","Edit menu" ), this);
    a->setShortcut (Qt::SHIFT + Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURL() ) );
    actionListBranches.append (a);
    actionOpenURL=a;

    a = new QAction( tr( "Open URL in new tab","Edit menu" ), this);
    //a->setShortcut (Qt::CTRL+Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURLTab() ) );
    actionListBranches.append (a);
    actionOpenURLTab=a;

    a = new QAction( tr( "Open all URLs in subtree (including scrolled branches)","Edit menu" ), this);
    a->setShortcut ( Qt::CTRL + Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleVisURLTabs() ) );
    actionListBranches.append(a);
    actionOpenMultipleVisURLTabs=a;

    a = new QAction( tr( "Open all URLs in subtree","Edit menu" ), this);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleURLTabs() ) );
    actionListBranches.append(a);
    actionOpenMultipleURLTabs=a;

    a = new QAction(QPixmap(), tr( "Extract URLs from note","Edit menu"), this);
    a->setShortcut ( Qt::SHIFT + Qt::Key_N );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNote2URLs() ) );
    actionListBranches.append(a);
    actionGetURLsFromNote=a;

    a = new QAction(QPixmap(flagsPath+"flag-urlnew.png"), tr( "Edit URL...","Edit menu"), this);
    a->setShortcut ( Qt::Key_U );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editURL() ) );
    actionListBranches.append(a);
    actionURLNew=a;

    a = new QAction(QPixmap(), tr( "Edit local URL...","Edit menu"), this);
    //a->setShortcut (Qt::SHIFT +  Qt::Key_U );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLocalURL() ) );
    actionListBranches.append(a);
    actionLocalURL=a;

    a = new QAction( tr( "Use heading for URL","Edit menu" ), this);
    a->setShortcut ( Qt::ALT + Qt::Key_U );
    a->setShortcutContext (Qt::ApplicationShortcut);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading2URL() ) );
    actionListBranches.append(a);
    actionHeading2URL=a;

    a = new QAction(tr( "Create URL to Novell Bugzilla","Edit menu" ), this);
    a->setEnabled (false);
    actionListBranches.append(a);
    a->setShortcut ( Qt::Key_B );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editBugzilla2URL() ) );
    actionListBranches.append(a);
    actionBugzilla2URL=a;

    a = new QAction(tr( "Get data from Novell Bugzilla","Edit menu" ), this);
    a->setShortcut ( Qt::Key_B + Qt::SHIFT);
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( getBugzillaData() ) );
    actionListBranches.append(a);
    actionGetBugzillaData=a;

    a = new QAction(tr( "Get data from Novell Bugzilla for subtree","Edit menu" ), this);
    a->setShortcut ( Qt::Key_B + Qt::CTRL);
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( getBugzillaDataSubtree() ) );
    actionListBranches.append(a);
    actionGetBugzillaDataSubtree=a;

    a = new QAction(tr( "Create URL to Novell FATE","Edit menu" ), this);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editFATE2URL() ) );
    actionListBranches.append(a);
    actionFATE2URL=a;

    a = new QAction(QPixmap(flagsPath+"flag-vymlink.png"), tr( "Open linked map","Edit menu" ), this);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenVymLink() ) );
    actionListBranches.append (a);
    actionOpenVymLink=a;

    a = new QAction(QPixmap(), tr( "Open all vym links in subtree","Edit menu" ), this);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleVymLinks() ) );
    actionListBranches.append(a);
    actionOpenMultipleVymLinks=a;


    a = new QAction(QPixmap(flagsPath+"flag-vymlinknew.png"), tr( "Edit vym link...","Edit menu" ), this);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editVymLink() ) );
    actionListBranches.append(a);
    actionEditVymLink=a;

    a = new QAction(tr( "Delete vym link","Edit menu" ),this);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteVymLink() ) );
    actionListBranches.append(a);
    actionDeleteVymLink=a;

    a = new QAction(QPixmap(flagsPath+"flag-hideexport.png"), tr( "Hide in exports","Edit menu" ), this);
    a->setShortcut (Qt::Key_H );
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(true);
    a->setEnabled (false);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleHideExport() ) );
    actionListItems.append (a);
    actionToggleHideExport=a;

    a = new QAction(QPixmap(flagsPath+"flag-task.png"), tr( "Toggle task","Edit menu" ), this);
    a->setShortcut (Qt::Key_W + Qt::SHIFT);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(true);
    a->setEnabled (false);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleTask() ) );
    actionListBranches.append (a);
    actionToggleTask=a;

    a = new QAction(QPixmap(), tr( "Cycle task status","Edit menu" ), this);
    a->setShortcut (Qt::Key_W );
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editCycleTaskStatus() ) );
    actionListBranches.append (a);
    actionCycleTaskStatus=a;

    a = new QAction(QPixmap(), tr( "Reset sleep","Task sleep" ), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (0);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep0=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 days","Task sleep" ).arg("n")+"...", this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setShortcut (Qt::Key_Q + Qt::SHIFT);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (-1);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleepN=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 day","Task sleep" ).arg(1), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (1);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep1=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 days","Task sleep" ).arg(2), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (2);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep2=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 days","Task sleep" ).arg(3), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (3);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep3=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 days","Task sleep" ).arg(5), this); 
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (5);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep5=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 days","Task sleep" ).arg(7), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (7);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep7=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 weeks","Task sleep" ).arg(2), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (14);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep14=a;

    a = new QAction(QPixmap(), tr( "Sleep %1 weeks","Task sleep" ).arg(4), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable(false);
    a->setEnabled (false);
    a->setData (28);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editTaskSleepN() ) );
    actionListBranches.append (a);
    actionTaskSleep28=a;

    a = new QAction(tr( "Add timestamp","Edit menu" ), this);
    a->setEnabled (false);
    actionListBranches.append(a);
    a->setShortcut ( Qt::Key_T ); 
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddTimestamp() ) );
    actionListBranches.append (a);
    actionAddTimestamp=a;

    a = new QAction(tr( "Edit Map Info...","Edit menu" ),this);
    a->setEnabled (true);
    switchboard.addConnection(NULL, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editMapInfo() ) );
    actionListMap.append (a);
    actionMapInfo=a;

    // Import at selection (adding to selection)
    a = new QAction( tr( "Add map (insert)","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportAdd() ) );
    a->setEnabled (false);
    switchboard.addConnection(NULL, a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    actionImportAdd=a;

    // Import at selection (replacing selection)
    a = new QAction( tr( "Add map (replace)","Edit menu" ), this);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportReplace() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionImportReplace=a;

    // Save selection 
    a = new QAction( tr( "Save selection","Edit menu" ), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editSaveBranch() ) );
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    actionSaveBranch=a;

    // Only remove branch, not its children
    a = new QAction(tr( "Remove only branch and keep its children ","Edit menu" ), this);
    a->setShortcut (Qt::ALT + Qt::Key_X );
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteKeepChildren() ) );
    a->setEnabled (false);
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    actionDeleteKeepChildren=a;

    // Only remove children of a branch
    a = new QAction( tr( "Remove children","Edit menu" ), this);
    a->setShortcut (Qt::SHIFT + Qt::Key_X );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteChildren() ) );
    a->setEnabled (false);
    addAction (a);
    actionListBranches.append(a);
    actionDeleteChildren=a;

    a = new QAction( tr( "Add Image...","Edit menu" ), this);
    a->setShortcutContext (Qt::WindowShortcut);
    a->setShortcut (Qt::Key_I + Qt::SHIFT);    
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLoadImage() ) );
    actionLoadImage=a;

    a = new QAction( tr( "Property window","Dialog to edit properties of selection" )+QString ("..."), this);
    a->setShortcut ( Qt::Key_P );	 
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable (true);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( windowToggleProperty() ) );
    actionViewTogglePropertyEditor=a;
}

// Select Actions
void Main::setupSelectActions()
{
    QMenu *selectMenu = menuBar()->addMenu( tr("Select","Select menu") );
    QAction *a;
    a = new QAction( QPixmap(flagsPath + "flag-target"), tr( "Toggle target...","Edit menu"), this);
    a->setShortcut (Qt::SHIFT + Qt::Key_T );		
    a->setCheckable(true);
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleTarget() ) );
    actionListBranches.append (a);
    actionToggleTarget=a;

    a = new QAction( QPixmap(flagsPath + "flag-target"), tr( "Goto target...","Edit menu"), this);
    a->setShortcut (Qt::Key_G );		
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editGoToTarget() ) );
    actionListBranches.append (a);
    actionGoToTarget=a;

    a = new QAction( QPixmap(flagsPath + "flag-target"), tr( "Move to target...","Edit menu"), this);
    a->setShortcut (Qt::Key_M );		
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveToTarget() ) );
    actionListBranches.append (a);
    actionMoveToTarget=a;

    a = new QAction( QPixmap(iconPath + "selectprevious.png"), tr( "Select previous","Edit menu"), this);
    a->setShortcut (Qt::CTRL+ Qt::Key_O );	
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editSelectPrevious() ) );
    actionListMap.append (a);
    actionSelectPrevious=a;

    a = new QAction( QPixmap(iconPath + "selectnext.png"), tr( "Select next","Edit menu"), this);
    a->setShortcut (Qt::CTRL + Qt::Key_I );
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editSelectNext() ) );
    actionListMap.append (a);
    actionSelectNext=a;

    a = new QAction( tr( "Unselect all","Edit menu"), this);
    //a->setShortcut (Qt::CTRL + Qt::Key_I );
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editSelectNothing() ) );
    actionListMap.append (a);
    actionSelectNothing=a;

    a = new QAction( QPixmap(iconPath+"find.png"), tr( "Find...","Edit menu"), this);
    a->setShortcut (Qt::CTRL + Qt::Key_F );	
    switchboard.addConnection(selectMenu, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenFindResultWidget() ) );
    actionListMap.append(a);
    actionFind=a;

    a = new QAction( tr( "Find duplicate URLs","Edit menu"), this);
    a->setShortcut (Qt::SHIFT + Qt::Key_F);	
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    if (settings.value( "/mainwindow/showTestMenu",false).toBool() ) 
	selectMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editFindDuplicateURLs() ) );

}

// Format Actions
void Main::setupFormatActions()
{
    QMenu *formatMenu = menuBar()->addMenu (tr ("F&ormat","Format menu"));

    QAction *a;
    QPixmap pix( 16,16);
    pix.fill (Qt::black);
    a= new QAction(pix, tr( "Set &Color" )+QString("..."), this);
    switchboard.addConnection(formatMenu, a,tr("Format shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectColor() ) );
    actionFormatColor=a;

    a= new QAction( QPixmap(iconPath+"formatcolorpicker.png"), tr( "Pic&k color","Edit menu" ), this);
    a->setShortcut (Qt::CTRL + Qt::Key_K );
    switchboard.addConnection(formatMenu, a,tr("Format shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatPickColor() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionFormatPickColor=a;

    a= new QAction(QPixmap(iconPath+"formatcolorbranch.png"), tr( "Color &branch","Edit menu" ), this);
    //a->setShortcut (Qt::CTRL + Qt::Key_B + Qt::SHIFT);
    switchboard.addConnection(formatMenu, a,tr("Format shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorBranch() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionFormatColorBranch=a;

    a= new QAction(QPixmap(iconPath+"formatcolorsubtree.png"), tr( "Color sub&tree","Edit menu" ), this);
    //a->setShortcut (Qt::CTRL + Qt::Key_B);	// Color subtree
    switchboard.addConnection(formatMenu, a,tr("Format shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorSubtree() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionFormatColorSubtree=a;

    formatMenu->addSeparator();

    a= new QAction( tr( "Select default font","Branch attribute" )+"...",  this);
    a->setCheckable(false);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectFont() ) );
    formatMenu->addAction (a);
    actionFormatFont=a;

    formatMenu->addSeparator();

    actionGroupFormatLinkStyles=new QActionGroup ( this);
    actionGroupFormatLinkStyles->setExclusive (true);
    a= new QAction( tr( "Linkstyle Line" ), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleLine() ) );
    formatMenu->addAction (a);
    actionFormatLinkStyleLine=a;
    a= new QAction( tr( "Linkstyle Curve" ), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleParabel() ) );
    formatMenu->addAction (a);
    actionFormatLinkStyleParabel=a;
    a= new QAction( tr( "Linkstyle Thick Line" ), actionGroupFormatLinkStyles );
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyLine() ) );
    formatMenu->addAction (a);
    actionFormatLinkStylePolyLine=a;
    a= new QAction( tr( "Linkstyle Thick Curve" ), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    a->setChecked (true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyParabel() ) );
    formatMenu->addAction (a);
    formatMenu->addSeparator();
    actionFormatLinkStylePolyParabel=a;

    a = new QAction( tr( "Hide link if object is not selected","Branch attribute" ), this);
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatHideLinkUnselected() ) );
    actionListBranches.append (a);
    actionFormatHideLinkUnselected=a;

    a= new QAction( tr( "&Use color of heading for link","Branch attribute" ),  this);
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatToggleLinkColorHint() ) );
    formatMenu->addAction (a);
    actionFormatLinkColorHint=a;

    pix.fill (Qt::white);
    a= new QAction( pix, tr( "Set &Link Color")+"..." , this  );
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectLinkColor() ) );
    actionFormatLinkColor=a;

    a= new QAction( pix, tr( "Set &Selection Color")+"...", this  );
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectSelectionColor() ) );
    actionFormatSelectionColor=a;

    a= new QAction( pix, tr( "Set &Background Color" )+"...", this );
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackColor() ) );
    actionFormatBackColor=a;

    a= new QAction( pix, tr( "Set &Background image" )+"...", this );
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackImage() ) );
    actionFormatBackImage=a;
}

// View Actions
void Main::setupViewActions()
{
    QMenu *viewMenu = menuBar()->addMenu ( tr( "&View" ));
    toolbarsMenu=viewMenu->addMenu (tr("Toolbars","Toolbars overview in view menu") );
    viewMenu->addSeparator();	

    QAction *a;
    a = new QAction( QPixmap(iconPath+"viewmag+.png"), tr( "Zoom in","View action" ), this);
    a->setShortcut(Qt::Key_Plus);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomIn() ) );
    actionListMap.append (a);
    actionZoomIn=a;

    a = new QAction( QPixmap(iconPath+"viewmag-.png"), tr( "Zoom out","View action" ), this);
    a->setShortcut(Qt::Key_Minus);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewZoomOut() ) );
    actionListMap.append (a);
    actionZoomOut=a;

    a = new QAction( QPixmap(iconPath+"rotate-ccw.png"), tr( "Rotate counterclockwise","View action" ), this);
    a->setShortcut( Qt::SHIFT + Qt::Key_R);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewRotateCounterClockwise() ) );
    actionListMap.append (a);
    actionRotateCounterClockwise=a;

    a = new QAction( QPixmap(iconPath+"rotate-cw.png"), tr( "Rotate rclockwise","View action" ), this);
    a->setShortcut(Qt::Key_R);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewRotateClockwise() ) );
    actionListMap.append (a);
    actionRotateClockwise=a;

    a = new QAction(QPixmap(iconPath+"viewmag-reset.png"), tr( "reset Zoom","View action" ), this);
    a->setShortcut (Qt::Key_Comma);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomReset() ) );
    actionListMap.append (a);
    actionZoomReset=a;

    a = new QAction( QPixmap(iconPath+"viewshowsel.png"), tr( "Center on selection","View action" ), this);
    a->setShortcut(Qt::Key_Period);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewCenter() ) );
    actionListMap.append (a);
    actionCenterOn=a;

    viewMenu->addSeparator();	

    a=noteEditorDW->toggleViewAction();
    a->setShortcut ( Qt::Key_N );  
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    a->setIcon (QPixmap(flagsPath+"flag-note.png"));
    viewMenu->addAction (a);
    actionViewToggleNoteEditor=a;

    a=headingEditorDW->toggleViewAction();
    a->setShortcut ( Qt::Key_E );  
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    a->setIcon (QPixmap(iconPath+"headingeditor.png"));
    viewMenu->addAction (a);
    actionViewToggleHeadingEditor=a;

    // Original icon is "category" from KDE
    a = new QAction(QPixmap(iconPath+"treeeditor.png"), tr( "Tree editor","View action" ),this);
    a->setShortcut ( Qt::CTRL + Qt::Key_T );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleTreeEditor() ) );
    actionViewToggleTreeEditor=a;

    a = new QAction(QPixmap(iconPath+"taskeditor.png"), tr( "Task editor","View action" ),this);
    a->setShortcut ( Qt::Key_Q );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleTaskEditor() ) );
    actionViewToggleTaskEditor=a;

    a = new QAction(QPixmap(iconPath+"slideeditor.png"), tr( "Slide editor","View action" ),this);
    a->setShortcut ( Qt::SHIFT + Qt::Key_S );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleSlideEditor() ) );
    actionViewToggleSlideEditor=a;

    a = new QAction(QPixmap(iconPath+"scripteditor.png"), tr("Script editor","View action"), this);
    a->setShortcut ( Qt::ALT + Qt::Key_S );
    a->setCheckable(true);
    viewMenu->addAction (a);
    switchboard.addConnection(a, tr("View shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( windowToggleScriptEditor() ) );
    actionViewToggleScriptEditor=a;

    a = new QAction(QPixmap(iconPath+"history.png"),  tr( "History Window","View action" ),this );
    a->setShortcut ( Qt::CTRL + Qt::Key_H  );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleHistory() ) );
    actionViewToggleHistoryWindow=a;

    viewMenu->addAction (actionViewTogglePropertyEditor);

    viewMenu->addSeparator();	

    a = new QAction(tr( "Antialiasing","View action" ),this );
    a->setCheckable(true);
    a->setChecked (settings.value("/mainwindow/view/AntiAlias",true).toBool());
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleAntiAlias() ) );
    actionViewToggleAntiAlias=a;

    a = new QAction(tr( "Smooth pixmap transformations","View action" ),this );
    a->setStatusTip (a->text());
    a->setCheckable(true);
    a->setChecked (settings.value("/mainwindow/view/SmoothPixmapTransformation",true).toBool());
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleSmoothPixmap() ) );
    actionViewToggleSmoothPixmapTransform=a;

    a = new QAction(tr( "Next Map","View action" ), this);
    a->setStatusTip (a->text());
    a->setShortcut (Qt::SHIFT+ Qt::Key_Right );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowNextEditor() ) );

    a = new QAction (tr( "Previous Map","View action" ), this );
    a->setStatusTip (a->text());
    a->setShortcut (Qt::SHIFT+ Qt::Key_Left );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowPreviousEditor() ) );

    a = new QAction (tr( "Next slide","View action" ), this );
    a->setStatusTip (a->text());
    a->setShortcut (Qt::Key_Space);
    switchboard.addConnection(a,tr("Next slide","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(nextSlide() ) );

    a = new QAction (tr( "Previous slide","View action" ), this );
    a->setStatusTip (a->text());
    a->setShortcut (Qt::Key_Backspace);
    switchboard.addConnection(a,tr("Previous  slide","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(previousSlide() ) );
}

// Mode Actions
void Main::setupModeActions()
{
    //QPopupMenu *menu = new QPopupMenu( this );
    //menuBar()->insertItem( tr( "&Mode (using modifiers)" ), menu );

    QAction *a;
    actionGroupModModes=new QActionGroup ( this);
    actionGroupModModes->setExclusive (true);
    a= new QAction( QPixmap(iconPath+"modecolor.png"), tr( "Use modifier to color branches","Mode modifier" ), actionGroupModModes);
    a->setShortcut (Qt::Key_J);
    switchboard.addConnection(this,a,tr("Modes","Shortcut group"));
    a->setCheckable(true);
    a->setChecked(true);
    actionListMap.append (a);
    actionModModeColor=a;

    a->setShortcut( Qt::Key_K); 
    switchboard.addConnection(this,a,tr("Modes","Shortcut group"));
    a->setCheckable(true);
    actionListMap.append (a);
    actionModModeCopy=a;

    a= new QAction(QPixmap(iconPath+"modelink.png"), tr( "Use modifier to draw xLinks","Mode modifier" ), actionGroupModModes );
    a->setShortcut (Qt::Key_L);
    switchboard.addConnection(this,a,tr("Modes","Shortcut group"));
    a->setCheckable(true);
    actionListMap.append (a);
    actionModModeXLink=a;
}

// Flag Actions
void Main::setupFlagActions()
{
    // Create System Flags
    Flag *flag;

    // Tasks
    // Origin: ./share/icons/oxygen/48x48/status/task-reject.png
    flag=new Flag(flagsPath+"flag-task-new.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-new",tr("Note","SystemFlag"));
    flag=new Flag(flagsPath+"flag-task-new-morning.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-new-morning",tr("Note","SystemFlag"));
    flag=new Flag(flagsPath+"flag-task-new-sleeping.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-new-sleeping",tr("Note","SystemFlag"));
    // Origin: ./share/icons/oxygen/48x48/status/task-reject.png
    flag=new Flag(flagsPath+"flag-task-wip.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-wip",tr("Note","SystemFlag"));
    flag=new Flag(flagsPath+"flag-task-wip-morning.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-wip-morning",tr("Note","SystemFlag"));
    flag=new Flag(flagsPath+"flag-task-wip-sleeping.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-wip-sleeping",tr("Note","SystemFlag"));
    // Origin: ./share/icons/oxygen/48x48/status/task-complete.png
    flag=new Flag(flagsPath+"flag-task-finished.png");
    flag->setGroup("system-tasks");
    setupFlag (flag,NULL,"system-task-finished",tr("Note","SystemFlag"));

    flag=new Flag(flagsPath+"flag-note.png");
    setupFlag (flag,NULL,"system-note",tr("Note","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url.png");
    setupFlag (flag,NULL,"system-url",tr("URL to Document ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url-bugzilla-novell.png");
    setupFlag (flag,NULL,"system-url-bugzilla-novell",tr("URL to Bugzilla ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url-bugzilla-novell-closed.png");
    setupFlag (flag,NULL,"system-url-bugzilla-novell-closed",tr("URL to Bugzilla ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-target.png");
    setupFlag (flag,NULL,"system-target",tr("Map target","SystemFlag"));

    flag=new Flag(flagsPath+"flag-vymlink.png");
    setupFlag (flag,NULL,"system-vymLink",tr("Link to another vym map","SystemFlag"));

    flag=new Flag(flagsPath+"flag-scrolled-right.png");
    setupFlag (flag,NULL,"system-scrolledright",tr("subtree is scrolled","SystemFlag"));

    flag=new Flag(flagsPath+"flag-tmpUnscrolled-right.png");
    setupFlag (flag,NULL,"system-tmpUnscrolledRight",tr("subtree is temporary scrolled","SystemFlag"));

    flag=new Flag(flagsPath+"flag-hideexport.png");
    setupFlag (flag,NULL,"system-hideInExport",tr("Hide object in exported maps","SystemFlag"));

    addToolBarBreak();

    // Create Standard Flags
    standardFlagsToolbar=addToolBar (tr ("Standard Flags toolbar","Standard Flag Toolbar"));
    standardFlagsToolbar->setObjectName ("standardFlagTB");
    standardFlagsMaster->setToolBar (standardFlagsToolbar);

    // Add entry now, to avoid chicken and egg problem and position toolbar 
    // after all others:
    toolbarsMenu->addAction (standardFlagsToolbar->toggleViewAction() );

    flag=new Flag(flagsPath+"flag-stopsign.png");
    setupFlag (flag,standardFlagsToolbar,"stopsign",tr("This won't work!","Standardflag"),Qt::Key_1);
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-hook-green.png");
    flag->setGroup("standard-status");
    setupFlag (flag,standardFlagsToolbar,"hook-green",tr("Status - ok,done","Standardflag"),Qt::Key_2);

    flag=new Flag(flagsPath+"flag-wip.png");
    flag->setGroup("standard-status");
    setupFlag (flag,standardFlagsToolbar,"wip",tr("Status - work in progress","Standardflag"),Qt::Key_3);

    flag=new Flag(flagsPath+"flag-cross-red.png");
    flag->setGroup("standard-status");
    setupFlag (flag,standardFlagsToolbar,"cross-red",tr("Status - missing, not started","Standardflag"),Qt::Key_4);

    flag=new Flag(flagsPath+"flag-exclamationmark.png");
    flag->setGroup("standard-mark");
    setupFlag (flag,standardFlagsToolbar,"exclamationmark",tr("Take care!","Standardflag"),Qt::Key_Exclam);

    flag=new Flag(flagsPath+"flag-questionmark.png");
    flag->setGroup("standard-mark");
    setupFlag (flag,standardFlagsToolbar,"questionmark",tr("Really?","Standardflag"),Qt::Key_Question);

    flag=new Flag(flagsPath+"flag-smiley-good.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,standardFlagsToolbar,"smiley-good",tr("Good","Standardflag"),Qt::Key_ParenRight);

    flag=new Flag(flagsPath+"flag-smiley-sad.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,standardFlagsToolbar,"smiley-sad",tr("Bad","Standardflag"),Qt::Key_ParenLeft);

    flag=new Flag(flagsPath+"flag-smiley-omb.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,standardFlagsToolbar,"smiley-omb",tr("Oh no!","Standardflag"));
    // Original omg.png (in KDE emoticons)
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-clock.png");
    setupFlag (flag,standardFlagsToolbar,"clock",tr("Time critical","Standardflag"));

    flag=new Flag(flagsPath+"flag-phone.png");
    setupFlag (flag,standardFlagsToolbar,"phone",tr("Call...","Standardflag"));

    flag=new Flag(flagsPath+"flag-lamp.png");
    setupFlag (flag,standardFlagsToolbar,"lamp",tr("Idea!","Standardflag"),Qt::Key_Asterisk);

    flag=new Flag(flagsPath+"flag-arrow-up.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,standardFlagsToolbar,"arrow-up",tr("Important","Standardflag"),Qt::SHIFT + Qt::Key_PageUp);

    flag=new Flag(flagsPath+"flag-arrow-down.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,standardFlagsToolbar,"arrow-down",tr("Unimportant","Standardflag"),Qt::SHIFT + Qt::Key_PageDown);

    flag=new Flag(flagsPath+"flag-2arrow-up.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,standardFlagsToolbar,"2arrow-up",tr("Very important!","Standardflag"),Qt::SHIFT + +Qt::CTRL + Qt::Key_PageUp);

    flag=new Flag(flagsPath+"flag-2arrow-down.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,standardFlagsToolbar,"2arrow-down",tr("Very unimportant!","Standardflag"),Qt::SHIFT + Qt::CTRL + Qt::Key_PageDown);
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-thumb-up.png");
    flag->setGroup("standard-thumb");
    setupFlag (flag,standardFlagsToolbar,"thumb-up",tr("I like this","Standardflag"),Qt::Key_BracketRight);

    flag=new Flag(flagsPath+"flag-thumb-down.png");
    flag->setGroup("standard-thumb");
    setupFlag (flag,standardFlagsToolbar,"thumb-down",tr("I do not like this","Standardflag"),Qt::Key_BracketLeft);
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-rose.png");
    setupFlag (flag,standardFlagsToolbar,"rose",tr("Rose","Standardflag"));

    flag=new Flag(flagsPath+"flag-heart.png");
    setupFlag (flag,standardFlagsToolbar,"heart",tr("I just love...","Standardflag"));

    flag=new Flag(flagsPath+"flag-present.png");
    setupFlag (flag,standardFlagsToolbar,"present",tr("Surprise!","Standardflag"));

    flag=new Flag(flagsPath+"flag-flash.png");
    setupFlag (flag,standardFlagsToolbar,"flash",tr("Dangerous","Standardflag"));

    // Original: xsldbg_output.png
    flag=new Flag(flagsPath+"flag-info.png");
    setupFlag (flag,standardFlagsToolbar,"info",tr("Info","Standardflag"),Qt::Key_I);

    // Original khelpcenter.png
    flag=new Flag(flagsPath+"flag-lifebelt.png");
    setupFlag (flag,standardFlagsToolbar,"lifebelt",tr("This will help","Standardflag"));

    // Freemind flags
    flag=new Flag(flagsPath+"freemind/warning.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,  "freemind-warning",tr("Important","Freemind-Flag"));

    for (int i=1; i<8; i++)
    {
	flag=new Flag(flagsPath+QString("freemind/priority-%1.png").arg(i));
	flag->setVisible(false);
	flag->setGroup ("Freemind-priority");
	setupFlag (flag,standardFlagsToolbar, QString("freemind-priority-%1").arg(i),tr("Priority","Freemind-Flag"));
    }

    flag=new Flag(flagsPath+"freemind/back.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-back",tr("Back","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/forward.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-forward",tr("forward","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/attach.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-attach",tr("Look here","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/clanbomber.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-clanbomber",tr("Dangerous","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/desktopnew.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-desktopnew",tr("Don't flagrget","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/flag.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-flag",tr("Flag","Freemind-Flag"));


    flag=new Flag(flagsPath+"freemind/gohome.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-gohome",tr("Home","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/kaddressbook.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-kaddressbook",tr("Telephone","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/knotify.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-knotify",tr("Music","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/korn.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-korn",tr("Mailbox","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/mail.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-mail",tr("Maix","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/password.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-password",tr("Password","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/pencil.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-pencil",tr("To be improved","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/stop.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-stop",tr("Stop","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/wizard.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-wizard",tr("Magic","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/xmag.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-xmag",tr("To be discussed","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/bell.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-bell",tr("Reminder","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/bookmark.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-bookmark",tr("Excellent","Freemind-Flag"));

    flag= new Flag(flagsPath+"freemind/penguin.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-penguin",tr("Linux","Freemind-Flag"));

    flag=new Flag (flagsPath+"freemind/licq.png");
    flag->setVisible(false);
    setupFlag (flag,standardFlagsToolbar,"freemind-licq",tr("Sweet","Freemind-Flag"));
}

void Main::setupFlag (Flag *flag, QToolBar *tb, const QString &name, const QString &tooltip, const QKeySequence &shortcut)
{
    flag->setName(name);
    flag->setToolTip (tooltip);
    QAction *a;
    if (tb)
    {
	a=new QAction (flag->getPixmap(),name,this);
	// StandardFlag
	flag->setAction (a);
	a->setVisible (flag->isVisible());
	a->setCheckable(true);
	a->setObjectName(name);
	a->setToolTip(tooltip);
	a->setShortcut (shortcut);
	connect (a, SIGNAL( triggered() ), this, SLOT( standardFlagChanged() ) );
	standardFlagsMaster->addFlag (flag);	
	switchboard.addConnection (tb, a, tr("Flags toolbar"));
    } else
    {
	// SystemFlag
	systemFlagsMaster->addFlag (flag);  
    }
}

// Network Actions
void Main::setupNetworkActions()
{
    if (!settings.value( "/mainwindow/showTestMenu",false).toBool() ) 
	return;
    QMenu *netMenu = menuBar()->addMenu(  "Network" );

    QAction *a;

    a = new QAction(  "Start TCPserver for MapEditor",this);
    //a->setShortcut ( Qt::ALT + Qt::Key_T );	 
    switchboard.addConnection(netMenu, a,tr("Network shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( networkStartServer() ) );

    a = new QAction(  "Connect MapEditor to server",this);
    a->setShortcut ( Qt::ALT + Qt::Key_C );
    switchboard.addConnection(netMenu, a,tr("Network shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( networkConnect() ) );
}

// Settings Actions
void Main::setupSettingsActions()
{
    QMenu *settingsMenu = menuBar()->addMenu( tr( "Settings" ));

    QAction *a;

    a = new QAction( tr( "Set application to open pdf files","Settings action"), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsPDF() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set application to open external links","Settings action"), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsURL() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set path for macros","Settings action")+"...", this);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsMacroDir() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set number of undo levels","Settings action")+"...", this);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsUndoLevels() ) );
    settingsMenu->addAction (a);

    settingsMenu->addSeparator();

    a = new QAction( tr( "Autosave","Settings action"), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mainwindow/autosave/use",true).toBool());
    settingsMenu->addAction (a);
    actionSettingsAutosaveToggle=a;

    a = new QAction( tr( "Autosave time","Settings action")+"...", this);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutosaveTime() ) );
    settingsMenu->addAction (a);
    actionSettingsAutosaveTime=a;

    // Disable certain actions during testing
    if (testmode)
    {
	actionSettingsAutosaveToggle->setChecked (false);
	actionSettingsAutosaveToggle->setEnabled (false);
	actionSettingsAutosaveTime->setEnabled (false);
    }

    a = new QAction( tr( "Write backup file on save","Settings action"), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mainwindow/writeBackupFile",false).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsWriteBackupFileToggle() ) );
    settingsMenu->addAction (a);
    actionSettingsWriteBackupFile=a;

    settingsMenu->addSeparator();

    a = new QAction( tr( "Edit branch after adding it","Settings action" ), this );
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoEditNewBranch",true).toBool());
    settingsMenu->addAction (a);
    actionSettingsAutoEditNewBranch=a;

    a= new QAction( tr( "Select branch after adding it","Settings action" ), this );
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoSelectNewBranch",false).toBool() );
    settingsMenu->addAction (a);
    actionSettingsAutoSelectNewBranch=a;

    a= new QAction(tr( "Select existing heading","Settings action" ), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoSelectText",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsAutoSelectText=a;

    a= new QAction( tr( "Exclusive flags","Settings action" ), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/useFlagGroups",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsUseFlagGroups=a;

    a= new QAction( tr( "Use hide flags","Settings action" ), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/export/useHideExport",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsUseHideExport=a;

    settingsMenu->addSeparator();

    a = new QAction( tr( "Animation","Settings action"), this);
    a->setCheckable(true);
    a->setChecked (settings.value("/animation/use",true).toBool() );
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleAnimation() ) );
    settingsMenu->addAction (a);
    actionSettingsUseAnimation=a;

    a = new QAction( tr( "Automatic layout","Settings action"), this);
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mainwindow/autoLayout/use",true).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutoLayoutToggle() ) );
    settingsMenu->addAction (a);
    actionSettingsAutoLayoutToggle=a;

}

// Test Actions
void Main::setupTestActions()
{
    QMenu *testMenu = menuBar()->addMenu( tr( "Test" ));

    QAction *a;
    a = new QAction( "Test function 1" , this);
    a->setShortcut (Qt::ALT + Qt::Key_T); 
    switchboard.addConnection(testMenu, a, tr("Test shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction1() ) );

    a = new QAction( "Test function 2" , this);
    switchboard.addConnection(testMenu, a, tr("Test shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction2() ) );

    a = new QAction( "Toggle hide export mode" , this);
    a->setCheckable (true);
    a->setChecked (false);
    switchboard.addConnection(testMenu, a, tr("Test shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( toggleHideExport() ) );
    actionToggleHideMode=a;

    a = new QAction( "Toggle winter mode" , this);
    a->setShortcut (Qt::ALT + Qt::Key_Asterisk); 
    switchboard.addConnection(testMenu, a, "Toggle Winter mode");
    connect( a, SIGNAL( triggered() ), this, SLOT( toggleWinter() ) );
    actionToggleWinter=a;
}

// Help Actions
void Main::setupHelpActions()
{
    QMenu *helpMenu = menuBar()->addMenu ( tr( "&Help","Help menubar entry" ));

    QAction *a;
    a = new QAction(  tr( "Open VYM Documentation (pdf) ","Help action" ), this );
    switchboard.addConnection(helpMenu, a,tr("Help shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDoc() ) );

    a = new QAction(  tr( "Open VYM example maps ","Help action" ), this );
    switchboard.addConnection(helpMenu, a,tr("Help shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDemo() ) );

    a = new QAction(  tr( "Show keyboard shortcuts","Help action" ), this );
    switchboard.addConnection(helpMenu, a,tr("Help shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpShortcuts() ) );

    a = new QAction( tr( "About VYM","Help action" ), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAbout() ) );
    helpMenu->addAction (a);

    a = new QAction( tr( "About QT","Help action" ), this);
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAboutQT() ) );
    helpMenu->addAction (a);
}

// Context Menus
void Main::setupContextMenus()
{
    // Context menu for goto/move targets  (populated on demand)
    targetsContextMenu = new QMenu (this);

    // Context Menu for branch or mapcenter
    branchContextMenu =new QMenu (this);
    branchContextMenu->addAction (actionViewTogglePropertyEditor);
    branchContextMenu->addSeparator();	

	// Submenu "Add"
	branchAddContextMenu =branchContextMenu->addMenu (tr("Add"));
	branchAddContextMenu->addAction (actionPaste );
	branchAddContextMenu->addAction ( actionAddMapCenter );
	branchAddContextMenu->addAction ( actionAddBranch );
	branchAddContextMenu->addAction ( actionAddBranchBefore );
	branchAddContextMenu->addAction ( actionAddBranchAbove);
	branchAddContextMenu->addAction ( actionAddBranchBelow );
	branchAddContextMenu->addSeparator();	
	branchAddContextMenu->addAction ( actionImportAdd );
	branchAddContextMenu->addAction ( actionImportReplace );

	// Submenu "Remove"
	branchRemoveContextMenu =branchContextMenu->addMenu (tr ("Remove","Context menu name"));
	branchRemoveContextMenu->addAction (actionCut);
	branchRemoveContextMenu->addAction ( actionDelete );
	branchRemoveContextMenu->addAction ( actionDeleteKeepChildren );
	branchRemoveContextMenu->addAction ( actionDeleteChildren );
	

    branchContextMenu->addAction(actionSaveBranch);
    branchContextMenu->addAction( actionFileNewCopy);
    branchContextMenu->addAction(actionDetach);

    branchContextMenu->addSeparator();	
    branchContextMenu->addAction ( actionLoadImage);
    if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
	branchContextMenu->addAction ( actionAddAttribute);


    branchContextMenu->addSeparator();  

    // Context menu for tasks
    taskContextMenu = branchContextMenu->addMenu (tr("Tasks","Context menu"));
	taskContextMenu->addAction (actionToggleTask);
	taskContextMenu->addAction (actionCycleTaskStatus);
	taskContextMenu->addSeparator();
	taskContextMenu->addAction (actionTaskSleep0);
	taskContextMenu->addAction (actionTaskSleepN);
	taskContextMenu->addAction (actionTaskSleep1);
	taskContextMenu->addAction (actionTaskSleep2);
	taskContextMenu->addAction (actionTaskSleep3);
	taskContextMenu->addAction (actionTaskSleep5);
	taskContextMenu->addAction (actionTaskSleep7);
	taskContextMenu->addAction (actionTaskSleep14);
	taskContextMenu->addAction (actionTaskSleep28);

    // Submenu for Links (URLs, vymLinks)
    branchLinksContextMenu =new QMenu (this);

	branchLinksContextMenu=branchContextMenu->addMenu(tr("References (URLs, vymLinks, ...)","Context menu name"));	
	branchLinksContextMenu->addAction ( actionOpenURL );
	branchLinksContextMenu->addAction ( actionOpenURLTab );
	branchLinksContextMenu->addAction ( actionOpenMultipleVisURLTabs );
	branchLinksContextMenu->addAction ( actionOpenMultipleURLTabs );
	branchLinksContextMenu->addAction ( actionURLNew );
	branchLinksContextMenu->addAction ( actionLocalURL );
	branchLinksContextMenu->addAction ( actionGetURLsFromNote );
	branchLinksContextMenu->addAction ( actionHeading2URL );
	branchLinksContextMenu->addAction ( actionBugzilla2URL );
	branchLinksContextMenu->addAction ( actionGetBugzillaData );
	branchLinksContextMenu->addAction ( actionGetBugzillaDataSubtree );
	if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
	    branchLinksContextMenu->addAction ( actionFATE2URL );
	branchLinksContextMenu->addSeparator();	
	branchLinksContextMenu->addAction ( actionOpenVymLink );
	branchLinksContextMenu->addAction ( actionOpenMultipleVymLinks );
	branchLinksContextMenu->addAction ( actionEditVymLink );
	branchLinksContextMenu->addAction ( actionDeleteVymLink );
	

    // Context Menu for XLinks in a branch menu
    // This will be populated "on demand" in updateActions
    branchContextMenu->addSeparator();	
    branchXLinksContextMenuEdit =branchContextMenu->addMenu (tr ("Edit XLink","Context menu name"));
    connect( 
	branchXLinksContextMenuEdit, SIGNAL( triggered(QAction *) ), 
	this, SLOT( editEditXLink(QAction * ) ) );
    QAction *a;
    a = new QAction( tr("Follow XLink","Context menu") , this);
    a->setShortcut (Qt::Key_F); 
    switchboard.addConnection(this, a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( popupFollowXLink() ) );

    branchXLinksContextMenuFollow =branchContextMenu->addMenu (tr ("Follow XLink","Context menu name"));
    connect( 
	branchXLinksContextMenuFollow, SIGNAL( triggered(QAction *) ), 
	this, SLOT( editFollowXLink(QAction * ) ) );



    // Context menu for floatimage
    floatimageContextMenu =new QMenu (this);
    a= new QAction (tr ("Save image","Context action"),this);
    connect (a, SIGNAL (triggered()), this, SLOT (editSaveImage()));
    floatimageContextMenu->addAction (a);

    floatimageContextMenu->addSeparator();  
    floatimageContextMenu->addAction(actionCopy);
    floatimageContextMenu->addAction(actionCut);

    floatimageContextMenu->addSeparator();  
    floatimageContextMenu->addAction ( actionFormatHideLinkUnselected );

    // Context menu for canvas
    canvasContextMenu =new QMenu (this);
    canvasContextMenu->addAction (actionAddMapCenter);
    canvasContextMenu->addAction (actionMapInfo);   //FIXME-4 move to File menu?
    canvasContextMenu->insertSeparator(actionMapInfo);   

    canvasContextMenu->addAction(actionFormatFont);
    canvasContextMenu->insertSeparator(actionFormatFont);

    canvasContextMenu->addActions(actionGroupFormatLinkStyles->actions() );
    canvasContextMenu->insertSeparator(actionGroupFormatLinkStyles->actions().first() );   
    canvasContextMenu->addAction(actionFormatLinkColorHint);
    canvasContextMenu->insertSeparator(actionFormatLinkColorHint);
    canvasContextMenu->addAction(actionFormatLinkColor);
    canvasContextMenu->addAction(actionFormatSelectionColor);
    canvasContextMenu->addAction(actionFormatBackColor);
    // actionFormatBackImage->addTo( canvasContextMenu );  //FIXME-4 makes vym too slow: postponed for later version 

    // Menu for last opened files
    // Create actions
    for (int i = 0; i < MaxRecentFiles; ++i) 
    {
	recentFileActions[i] = new QAction(this);
	recentFileActions[i]->setVisible(false);
	fileLastMapsMenu->addAction(recentFileActions[i]);
	connect(recentFileActions[i], SIGNAL(triggered()),
		this, SLOT(fileLoadRecent()));
    }
    setupRecentMapsMenu();
}

void Main::setupRecentMapsMenu()
{
    QStringList files = settings.value("/mainwindow/recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
	QString text = QString("&%1 %2").arg(i + 1).arg(files[i]);
	recentFileActions[i]->setText(text);
	recentFileActions[i]->setData(files[i]);
	recentFileActions[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
	recentFileActions[j]->setVisible(false);
}

void Main::setupMacros()
{
    for (int i = 0; i <= 11; i++) 
    {
	macroActions[i] = new QAction(this);
	macroActions[i]->setData(i);
	addAction (macroActions[i]);
	//switchboard.addConnection(macroActions[i],tr("Macro shortcuts","Shortcut group"));
	connect(macroActions[i], SIGNAL(triggered()),
		this, SLOT(callMacro()));
    }		
    macroActions[0]->setShortcut ( Qt::Key_F1 );
    macroActions[1]->setShortcut ( Qt::Key_F2 );
    macroActions[2]->setShortcut ( Qt::Key_F3 );
    macroActions[3]->setShortcut ( Qt::Key_F4 );
    macroActions[4]->setShortcut ( Qt::Key_F5 );
    macroActions[5]->setShortcut ( Qt::Key_F6 );
    macroActions[6]->setShortcut ( Qt::Key_F7 );
    macroActions[7]->setShortcut ( Qt::Key_F8 );
    macroActions[8]->setShortcut ( Qt::Key_F9 );
    macroActions[9]->setShortcut ( Qt::Key_F10 );
    macroActions[10]->setShortcut ( Qt::Key_F11 );
    macroActions[11]->setShortcut ( Qt::Key_F12 );
}

void Main::setupToolbars()
{
    // File actions
    fileToolbar = addToolBar( tr ("File actions toolbar","Toolbar for file actions"));
    fileToolbar->setObjectName ("fileTB");
    fileToolbar->addAction(actionFileNew);
    fileToolbar->addAction(actionFileOpen);
    fileToolbar->addAction(actionFileSave);
    fileToolbar->addAction(actionFileExportLast);
    fileToolbar->addAction(actionFilePrint);

    // Undo/Redo and clipboard
    clipboardToolbar =addToolBar( tr ("Undo and clipboard toolbar","Toolbar for redo/undo and clipboard"));
    clipboardToolbar->setObjectName ("clipboard toolbar");
    clipboardToolbar->addAction (actionUndo);
    clipboardToolbar->addAction (actionRedo);
    clipboardToolbar->addAction (actionCopy);
    clipboardToolbar->addAction (actionCut);
    clipboardToolbar->addAction (actionPaste);

    // Basic edits
    editActionsToolbar = addToolBar( tr ("Edit actions toolbar","Toolbar name") );
    editActionsToolbar->setObjectName ("basic edit actions TB");
    editActionsToolbar->addAction (actionAddMapCenter);
    editActionsToolbar->addAction (actionAddBranch);
    editActionsToolbar->addAction (actionMoveUp);
    editActionsToolbar->addAction (actionMoveDown);
    editActionsToolbar->addAction (actionSortChildren);
    editActionsToolbar->addAction (actionSortBackChildren);
    editActionsToolbar->addAction (actionToggleScroll);
    editActionsToolbar->addAction (actionToggleHideExport);
    editActionsToolbar->addAction (actionToggleTask);
    //editActionsToolbar->addAction (actionExpandAll);
    //editActionsToolbar->addAction (actionExpandOneLevel);
    //editActionsToolbar->addAction (actionCollapseOneLevel);
    //editActionsToolbar->addAction (actionCollapseUnselected);

    // Selections
    selectionToolbar = addToolBar( tr ("Selection toolbar","Toolbar name") );
    selectionToolbar->setObjectName ("toolbar for selecting items");
    selectionToolbar->addAction (actionToggleTarget);
    selectionToolbar->addAction (actionSelectPrevious);
    selectionToolbar->addAction (actionSelectNext);
    selectionToolbar->addAction (actionFind);

    // URLs and vymLinks
    referencesToolbar=addToolBar( tr ("URLs and vymLinks toolbar","Toolbar for URLs and vymlinks"));
    referencesToolbar->setObjectName ("URLs and vymlinks toolbar");
    referencesToolbar->addAction (actionOpenURL);
    referencesToolbar->addAction (actionURLNew);
    referencesToolbar->addAction (actionOpenVymLink);
    referencesToolbar->addAction (actionEditVymLink);

    // Format and colors
    colorsToolbar = addToolBar( tr("Colors toolbar","Colors toolbar name"));
    colorsToolbar->setObjectName ("colorsTB");
    colorsToolbar->addAction(actionFormatColor);
    colorsToolbar->addAction(actionFormatPickColor);
    colorsToolbar->addAction(actionFormatColorBranch);
    colorsToolbar->addAction(actionFormatColorSubtree);

    // Zoom
    zoomToolbar = addToolBar( tr("Zoom toolbar","View Toolbar name") );
    zoomToolbar->setObjectName ("viewTB");
    zoomToolbar->addAction(actionZoomIn);
    zoomToolbar->addAction(actionZoomOut);
    zoomToolbar->addAction(actionZoomReset);
    zoomToolbar->addAction(actionCenterOn);
    zoomToolbar->addAction(actionRotateCounterClockwise);
    zoomToolbar->addAction(actionRotateClockwise);

    // Editors
    editorsToolbar = addToolBar( tr("Editors toolbar","Editor Toolbar name") );
    editorsToolbar->setObjectName ("editorsTB");
    editorsToolbar->addAction (actionViewToggleNoteEditor);
    editorsToolbar->addAction (actionViewToggleHeadingEditor);
    editorsToolbar->addAction (actionViewToggleTreeEditor);
    editorsToolbar->addAction (actionViewToggleTaskEditor);
    editorsToolbar->addAction (actionViewToggleSlideEditor);
    editorsToolbar->addAction (actionViewToggleScriptEditor);
    editorsToolbar->addAction (actionViewToggleHistoryWindow);


    // Modifier modes
    modModesToolbar = addToolBar( tr ("Modifier modes toolbar","Modifier Toolbar name") );
    modModesToolbar->setObjectName ("modesTB");
    modModesToolbar->addAction(actionModModeColor);
    modModesToolbar->addAction(actionModModeCopy);
    modModesToolbar->addAction(actionModModeXLink);
    
    // Add all toolbars to View menu
    toolbarsMenu->addAction (fileToolbar->toggleViewAction() );
    toolbarsMenu->addAction (clipboardToolbar->toggleViewAction() );
    toolbarsMenu->addAction (editActionsToolbar->toggleViewAction() );
    toolbarsMenu->addAction (selectionToolbar->toggleViewAction() );
    toolbarsMenu->addAction (colorsToolbar->toggleViewAction() );
    toolbarsMenu->addAction (zoomToolbar->toggleViewAction() );
    toolbarsMenu->addAction (modModesToolbar->toggleViewAction() );
    toolbarsMenu->addAction (referencesToolbar->toggleViewAction() );
    toolbarsMenu->addAction (editorsToolbar->toggleViewAction() );

    // Default visibility to not overload new users
    fileToolbar->show();
    clipboardToolbar->show();
    editActionsToolbar->show();
    selectionToolbar->hide();
    colorsToolbar->show();
    zoomToolbar->show();
    modModesToolbar->hide();
    referencesToolbar->hide();
    editorsToolbar->hide();

}

VymView* Main::currentView() const
{
    if ( tabWidget->currentWidget() )
    {
	int i=tabWidget->currentIndex();
	if (i>=0 && i< vymViews.count() ) return vymViews.at(i);
    }
    return NULL;
}

MapEditor* Main::currentMapEditor() const
{
    if ( tabWidget->currentWidget())
	return vymViews.at(tabWidget->currentIndex())->getMapEditor();
    return NULL;    
}

uint  Main::currentModelID() const
{
    VymModel *m=currentModel();
    if (m)
	return m->getModelID();
    else
	return 0;    
}

VymModel* Main::currentModel() const
{
    VymView *vv=currentView();
    if (vv) 
	return vv->getModel();
    else
	return NULL;    
}

VymModel* Main::getModel(uint id) const	
{
    // Used in BugAgent
    for (int i=0; i<vymViews.count();i++)
	if (vymViews.at(i)->getModel()->getModelID()==id)
	    return vymViews.at(i)->getModel();
    return NULL;    
}

void Main::gotoModel (VymModel *m)
{
    for (int i=0; i<vymViews.count();i++)
	if (vymViews.at(i)->getModel()==m)
	{
	    tabWidget->setCurrentIndex (i);
	    return;
	}
}

int Main::modelCount()
{
    return vymViews.count();
}

void Main::editorChanged(QWidget *)
{
    VymModel *vm=currentModel();
    if (vm) 
    {	
	updateNoteEditor (vm->getSelectedIndex() );
	updateQueries (vm);

	taskEditor->setMapName (vm->getMapName() );
    }	

    // Update actions to in menus and toolbars according to editor
    updateActions();
}

void Main::fileNew()
{
    VymModel *vm=new VymModel;

    /////////////////////////////////////
//  new ModelTest(vm, this);	
    /////////////////////////////////////

    VymView *vv=new VymView (vm);
    vymViews.append (vv);

    tabWidget->addTab (vv,tr("unnamed","MainWindow: name for new and empty file"));
    tabWidget->setCurrentIndex (vymViews.count() );
    vv->initFocus();

    // Create MapCenter for empty map
    vm->addMapCenter(false);
    vm->makeDefault();

    // For the very first map we do not have flagrows yet...
    vm->select("mc:");

    // Switch to new tab
    tabWidget->setCurrentIndex (tabWidget->count() -1);
}

void Main::fileNewCopy() 
{
    QString fn="unnamed";
    VymModel *srcModel=currentModel();
    if (srcModel)
    {
	srcModel->copy();
	fileNew();
	VymModel *dstModel=vymViews.last()->getModel();
	if (dstModel->select("mc:0"))
	    dstModel->loadMap (clipboardDir+"/"+clipboardFile,ImportReplace);
	else
	    qWarning ()<<"Main::fileNewCopy couldn't select mapcenter";
    }
}

File::ErrorCode Main::fileLoad(QString fn, const LoadMode &lmode, const FileType &ftype) 
{
    File::ErrorCode err=File::Success;

    // fn is usually the archive, mapfile the file after uncompressing
    QString mapfile;

    // Make fn absolute (needed for unzip)
    fn=QDir (fn).absolutePath();

    VymModel *vm;

    if (lmode==NewMap)
    {
	// Check, if map is already loaded
	int i=0;
	while (i<=tabWidget->count() -1)
	{
	    if (vymViews.at(i)->getModel()->getFilePath() == fn)
	    {
		// Already there, ask for confirmation
		QMessageBox mb( vymName,
		    tr("The map %1\nis already opened."
		    "Opening the same map in multiple editors may lead \n"
		    "to confusion when finishing working with vym."
		    "Do you want to").arg(fn),
		    QMessageBox::Warning,
		    QMessageBox::Yes | QMessageBox::Default,
		    QMessageBox::Cancel | QMessageBox::Escape,
		    QMessageBox::NoButton);
		mb.setButtonText( QMessageBox::Yes, tr("Open anyway") );
		mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
		switch( mb.exec() ) 
		{
		    case QMessageBox::Yes:
			// end loop and load anyway
			i=tabWidget->count();
			break;
		    case QMessageBox::Cancel:
			// do nothing
			return File::Aborted;
			break;
		}
	    }
	    i++;
	}
    }

    int tabIndex=tabWidget->currentIndex();

    // Try to load map
    if ( !fn.isEmpty() )
    {
	vm = currentModel();
	// Check first, if mapeditor exists
	// If it is not default AND we want a new map, 
	// create a new mapeditor in a new tab
	if ( lmode==NewMap && (!vm || !vm->isDefault() )  )
	{
	    vm=new VymModel;
	    VymView *vv=new VymView (vm);
	    vymViews.append (vv);

	    tabWidget->addTab (vv,fn);
	    tabIndex=tabWidget->count()-1;
	    //tabWidget->setCurrentIndex (tabIndex);
	    vv->initFocus();
	}
	
	// Check, if file exists (important for creating new files
	// from command line
	if (!QFile(fn).exists() )
	{
	    QMessageBox mb( vymName,
		tr("This map does not exist:\n  %1\nDo you want to create a new one?").arg(fn),
		QMessageBox::Question,
		QMessageBox::Yes ,
		QMessageBox::Cancel | QMessageBox::Default,
		QMessageBox::NoButton );

	    mb.setButtonText( QMessageBox::Yes, tr("Create"));
	    mb.setButtonText( QMessageBox::No, tr("Cancel"));
	    switch( mb.exec() ) 
	    {
		case QMessageBox::Yes:
		    // Create new map
		    currentMapEditor()->getModel()->setFilePath(fn);
		    tabWidget->setTabText (tabIndex,
			currentMapEditor()->getModel()->getFileName() );
		    statusBar()->showMessage( "Created " + fn , statusbarTime );
		    return File::Success;
			
		case QMessageBox::Cancel:
		    // don't create new map
		    statusBar()->showMessage( "Loading " + fn + " failed!", statusbarTime );
		    int cur=tabWidget->currentIndex();
		    tabWidget->setCurrentIndex (tabWidget->count()-1);
		    fileCloseMap();
		    tabWidget->setCurrentIndex (cur);
		    return File::Aborted;
	    }
	}   

	//tabWidget->setCurrentIndex (tabIndex);

	if (err!=File::Aborted)
	{
	    // Save existing filename in case  we import
	    QString fn_org=vm->getFilePath();

	    // Finally load map into mapEditor
	    progressDialog.setLabelText (tr("Loading: %1","Progress dialog while loading maps").arg(fn));
	    vm->setFilePath (fn);
	    vm->saveStateBeforeLoad (lmode,fn);
	    err=vm->loadMap(fn,lmode,ftype);

	    // Restore old (maybe empty) filepath, if this is an import
	    if (lmode!=NewMap)
		vm->setFilePath (fn_org);
	}   

	// Finally check for errors and go home
	if (err==File::Aborted) 
	{
	    if (lmode==NewMap) fileCloseMap();
	    statusBar()->showMessage( "Could not load " + fn, statusbarTime );
	} else 
	{
	    if (lmode==NewMap)
	    {
		vm->setFilePath (fn);
		tabWidget->setTabText (tabIndex, vm->getFileName());
		if (!isInTmpDir (fn))
		{
		    // Only append to lastMaps if not loaded from a tmpDir
		    // e.g. imported bookmarks are in a tmpDir
		    addRecentMap(vm->getFilePath() );
		}
		actionFilePrint->setEnabled (true);
	    }	
	    editorChanged(this);
	    vm->emitShowSelection();
	    statusBar()->showMessage( "Loaded " + fn, statusbarTime );
	}   
    }
    return err;
}


void Main::fileLoad(const LoadMode &lmode)
{
    QString caption;
    switch (lmode)
    {
	case NewMap:
	    caption=vymName+ " - " +tr("Load vym map");
	    break;
	case ImportAdd:
	    caption=vymName+ " - " +tr("Import: Add vym map to selection");
	    break;
	case ImportReplace:
	    caption=vymName+ " - " +tr("Import: Replace selection with vym map");
	    break;
    }

    QString filter;
    filter+="VYM map " + tr("or","File Dialog") +" Freemind map" + " (*.vym *.vyp *.mm);;";
    filter+="VYM map (*.vym *.vyp);;";
    filter+="VYM Backups (*.vym~);;";
    filter+="Freemind map (*.mm);;";
    filter+="XML (*.xml);;";
    filter+="All (* *.*)";
    QStringList fns=QFileDialog::getOpenFileNames( 
	    this,
	    caption,
	    lastMapDir.path(), 
	    filter);

    if (!fns.isEmpty() )
    {
	lastMapDir.setPath(fns.first().left(fns.first().lastIndexOf ("/")) );
	foreach (QString fn, fns)
	    fileLoad(fn, lmode, getMapType (fn) );		   
    }
    removeProgressCounter();

    fileSaveSession();
}

void Main::fileLoad()
{
    fileLoad (NewMap);
}

void Main::fileSaveSession()
{
    QStringList flist;
    for (int i=0;i<vymViews.count(); i++)
	flist.append (vymViews.at(i)->getModel()->getFilePath() );
    settings.setValue("/mainwindow/sessionFileList", flist);
}

void Main::fileRestoreSession()
{
    QStringList flist= settings.value("/mainwindow/sessionFileList").toStringList();
    QStringList::Iterator it=flist.begin();

    initProgressCounter (flist.count());
    while (it !=flist.end() )
    {
	FileType type=getMapType (*it);
	fileLoad (*it, NewMap,type);
	*it++;
    }	
    removeProgressCounter();
}

void Main::fileLoadRecent()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
	initProgressCounter ();
        QString fn=action->data().toString();
	FileType type=getMapType (fn);
        fileLoad (fn, NewMap,type);
	removeProgressCounter();
    }
}

void Main::addRecentMap (const QString &fileName)
{

    QStringList files = settings.value("/mainwindow/recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("/mainwindow/recentFileList", files);

    setupRecentMapsMenu();
}

void Main::fileSave(VymModel *m, const SaveMode &savemode)
{
    if (!m) return;

    if ( m->getFilePath().isEmpty() ) 
    {
	// We have  no filepath yet,
	// call fileSaveAs() now, this will call fileSave() 
	// again.
	// First switch to editor
	fileSaveAs(savemode);
    }

    if (m->save (savemode)==File::Success)
    {
	statusBar()->showMessage( 
	    tr("Saved  %1").arg(m->getFilePath()), 
	    statusbarTime );
	addRecentMap (m->getFilePath() );
	fileSaveSession();
    } else	
	statusBar()->showMessage( 
	    tr("Couldn't save ").arg(m->getFilePath()), 
	    statusbarTime );
}

void Main::fileSave()
{
    fileSave (currentModel(), CompleteMap);
}

void Main::fileSave(VymModel *m)
{
    fileSave (m,CompleteMap);
}

void Main::fileSaveAs(const SaveMode& savemode)
{
    if (currentMapEditor())
    {
	QString filter;
	if (savemode==CompleteMap)
	    filter="VYM map (*.vym)";
	else    
	    filter="VYM part of map (*vyp)";
	filter+=";;All (* *.*)";

	QString fn=QFileDialog::getSaveFileName (
	    this,
	    tr("Save map as"),
	    lastMapDir.path(),
	    filter,
	    NULL,
	    QFileDialog::DontConfirmOverwrite);
	if (!fn.isEmpty() )    
	{
	    // Check for existing file
	    if (QFile (fn).exists())
	    {
		QMessageBox mb( vymName,
		    tr("The file %1\nexists already. Do you want to").arg(fn),
		    QMessageBox::Warning,
		    QMessageBox::Yes | QMessageBox::Default,
		    QMessageBox::Cancel | QMessageBox::Escape,
		    QMessageBox::NoButton);
		mb.setButtonText( QMessageBox::Yes, tr("Overwrite") );
		mb.setButtonText( QMessageBox::Cancel, tr("Cancel"));
		switch( mb.exec() ) 
		{
		    case QMessageBox::Yes:
			// save 
			break;
		    case QMessageBox::Cancel:
			// do nothing
			return;
			break;
		}
		lastMapDir.setPath(fn.left(fn.lastIndexOf ("/")) );
	    } else
	    {
		// New file, add extension to filename, if missing
		// This is always .vym or .vyp, depending on savemode
		if (savemode==CompleteMap)
		{
		    if (!fn.contains (".vym") && !fn.contains (".xml"))
			fn +=".vym";
		} else	    
		{
		    if (!fn.contains (".vyp") && !fn.contains (".xml"))
			fn +=".vyp";
		}
	    }
    
	    // Save now
	    VymModel *m=currentModel();
	    QString fn_org=m->getFilePath(); // Restore fn later, if savemode != CompleteMap
	    m->setFilePath(fn);
	    fileSave(m, savemode);

	    // Set name of tab, assuming current tab is the one we just saved
	    if (savemode==CompleteMap)
		tabWidget->setTabText (tabWidget->currentIndex(), m->getFileName() );
	    else
		m->setFilePath (fn_org);
	    return; 
	}
    }
}

void Main::fileSaveAs()
{
    fileSaveAs (CompleteMap);
}

void Main::fileImportKDE4Bookmarks()
{
    ImportKDE4Bookmarks im;
    im.transform();
    if (File::Aborted!=fileLoad (im.getTransformedFile(),NewMap,VymMap) && currentMapEditor() )
	currentMapEditor()->getModel()->setFilePath ("");
}

void Main::fileImportFirefoxBookmarks()
{
    QFileDialog fd;
    fd.setDirectory (vymBaseDir.homePath()+"/.mozilla/firefox");
    fd.setFileMode (QFileDialog::ExistingFiles);
    QStringList filters;
    filters<<"Firefox "+tr("Bookmarks")+" (*.html)";
    fd.setFilters(filters);
    fd.setAcceptMode (QFileDialog::AcceptOpen);
    fd.setWindowTitle(tr("Import")+" "+"Firefox "+tr("Bookmarks"));

    if ( fd.exec() == QDialog::Accepted )
    {
	ImportFirefoxBookmarks im;
	QStringList flist = fd.selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    im.setFile (*it);
	    if (im.transform() && 
		File::Aborted!=fileLoad (im.getTransformedFile(),NewMap,FreemindMap) && 
		currentMapEditor() )
		currentMapEditor()->getModel()->setFilePath ("");
	    ++it;
	}
    }
}

void Main::fileImportFreemind()
{
    QStringList filters;
    filters <<"Freemind map (*.mm)"<<"All files (*)";
    QFileDialog fd;
    fd.setDirectory (lastMapDir);
    fd.setFileMode (QFileDialog::ExistingFiles);
    fd.setFilters (filters);
    fd.setWindowTitle(vymName+ " - " +tr("Load Freemind map"));
    fd.setAcceptMode (QFileDialog::AcceptOpen);

    QString fn;
    if ( fd.exec() == QDialog::Accepted )
    {
	lastMapDir=fd.directory().path();
	QStringList flist = fd.selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    fn = *it;
	    if ( fileLoad (fn,NewMap, FreemindMap)  )
	    {	
		currentMapEditor()->getModel()->setFilePath ("");
	    }	
	    ++it;
	}
    }
}


void Main::fileImportMM()
{
    ImportMM im;

    QFileDialog fd;
    fd.setDirectory (lastMapDir);
    fd.setFileMode (QFileDialog::ExistingFiles);
    QStringList filters;
    filters<<"Mind Manager (*.mmap)";
    fd.setFilters (filters);
    fd.setAcceptMode (QFileDialog::AcceptOpen);
    fd.setWindowTitle(tr("Import")+" "+"Mind Manager");

    if ( fd.exec() == QDialog::Accepted )
    {
	lastMapDir=fd.directory();
	QStringList flist = fd.selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    im.setFile (*it);
	    if (im.transform() && 
		File::Success==fileLoad (im.getTransformedFile(),NewMap,VymMap) && 
		currentMapEditor() )
		currentMapEditor()->getModel()->setFilePath ("");
	    ++it;
	}
    }
}

void Main::fileImportDir()
{
    VymModel *m=currentModel();
    if (m) m->importDir();
}

void Main::fileExportXML()  
{
    VymModel *m=currentModel();
    if (m) m->exportXML();
}

void Main::fileExportHTML() 
{
    VymModel *m=currentModel();
    if (m) m->exportHTML();
}

void Main::fileExportImage()	
{
    VymModel *m=currentModel();
    if (m) m->exportImage();
}

void Main::fileExportPDF()	
{
    VymModel *m=currentModel();
    if (m) m->exportPDF();
}

void Main::fileExportSVG()	
{
    VymModel *m=currentModel();
    if (m) m->exportSVG();
}

void Main::fileExportAO()
{
    VymModel *m=currentModel();
    if (m) m->exportAO();
}

void Main::fileExportASCII()
{
    VymModel *m=currentModel();
    if (m) m->exportASCII();
}

void Main::fileExportCSV()  //FIXME-3 not scriptable yet
{
    VymModel *m=currentModel();
    if (m)
    {
	ExportCSV ex;
	ex.setModel (m);
	ex.addFilter ("CSV (*.csv)");
	ex.setDirPath(lastImageDir.absolutePath());
	ex.setWindowTitle(vymName+ " -" +tr("Export as CSV")+" "+tr("(still experimental)"));
	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }
}

void Main::fileExportLaTeX()
{
    VymModel *m=currentModel();
    if (m) m->exportLaTeX();
}

void Main::fileExportOrgMode()	//FIXME-3 not scriptable yet
{
    VymModel *m=currentModel();
    if (m)
    {
	ExportOrgMode ex;
	ex.setModel (m);
	ex.addFilter ("org-mode (*.org)");
	ex.setDirPath (lastImageDir.absolutePath());
	ex.setWindowTitle(vymName+ " -" +tr("Export as org-mode")+" "+tr("(still experimental)"));
	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }
}

void Main::fileExportKDE4Bookmarks()	//FIXME-3 not scriptable yet
{
    ExportKDE4Bookmarks ex;
    VymModel *m=currentModel();
    if (m)
    {
	ex.setModel (m);
	ex.doExport();
    }	
}

void Main::fileExportTaskjuggler()  //FIXME-3 not scriptable yet
{
    ExportTaskjuggler ex;
    VymModel *m=currentModel();
    if (m)
    {
	ex.setModel (m);
	ex.setWindowTitle ( vymName+" - "+tr("Export to")+" Taskjuggler"+tr("(still experimental)"));
	ex.setDirPath (lastImageDir.absolutePath());
	ex.addFilter ("Taskjuggler (*.tjp)");

	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }	
}

void Main::fileExportImpress()	
{
    ExportOOFileDialog fd;
    // TODO add preview in dialog
    fd.setWindowTitle(vymName+" - "+tr("Export to")+" Open Office");
    fd.setDirectory (QDir().current());
    fd.setAcceptMode (QFileDialog::AcceptSave);
    fd.setFileMode (QFileDialog::AnyFile);
    if (fd.foundConfig())
    {
	if ( fd.exec() == QDialog::Accepted )
	{
	    if (!fd.selectedFiles().isEmpty())
	    {
		QString fn=fd.selectedFiles().first();
		if (!fn.contains (".odp")) fn +=".odp";

		//lastImageDir=fn.left(fn.findRev ("/"));
		VymModel *m=currentModel();
		if (m) m->exportImpress (fn,fd.selectedConfig());	
	    }
	}
    } else
    {
	QMessageBox::warning(0, 
	tr("Warning"),
	tr("Couldn't find configuration for export to Open Office\n"));
    }
}

void Main::fileExportLast()
{
    VymModel *m=currentModel();
    if (m) m->exportLast();
}

bool Main::closeTab(int i)
{
    // Find model
    VymModel *m=vymViews.at(i)->getModel();
    if (!m) return true;

    vymViews.removeAt (i);
    tabWidget->removeTab (i);

    delete (m->getMapEditor()); 
    delete (m); 

    updateActions();
    return false;
}

bool Main::fileCloseMap()   
{
    VymModel *m=currentModel();
    if (m)
    {
	if (m->hasChanged())
	{
	    QMessageBox mb( vymName,
		tr("The map %1 has been modified but not saved yet. Do you want to").arg(m->getFileName()),
		QMessageBox::Warning,
		QMessageBox::Yes | QMessageBox::Default,
		QMessageBox::No,
		QMessageBox::Cancel | QMessageBox::Escape );
	    mb.setButtonText( QMessageBox::Yes, tr("Save modified map before closing it") );
	    mb.setButtonText( QMessageBox::No, tr("Discard changes"));
	    mb.setModal (true);
	    mb.show();
	    switch( mb.exec() ) 
	    {
		case QMessageBox::Yes:
		    // save and close
		    fileSave(m, CompleteMap);
		    break;
		case QMessageBox::No:
		// close  without saving
		    break;
		case QMessageBox::Cancel:
		    // do nothing
		    return true;
	    }
	} 
        return closeTab(tabWidget->currentIndex());
    }
    return true; // Better don't exit vym if there is no currentModel()...
}

void Main::filePrint()
{
    if (currentMapEditor())
	currentMapEditor()->print();
}

bool Main::fileExitVYM()    
{
    fileSaveSession();

    // Check if one or more editors have changed
    int i=0;
    while (vymViews.count()>0)
    {
	tabWidget->setCurrentIndex(i);
	if (fileCloseMap()) return true;
    } 
    qApp->quit();
    return false;
}

void Main::editUndo()
{
    VymModel *m=currentModel();
    if (m) m->undo();
}

void Main::editRedo()	   
{
    VymModel *m=currentModel();
    if (m) m->redo();
}

void Main::gotoHistoryStep (int i)     
{
    VymModel *m=currentModel();
    if (m) m->gotoHistoryStep(i);
}

void Main::editCopy()
{
    VymModel *m=currentModel();
    if (m) m->copy();
}

void Main::editPaste()
{
    VymModel *m=currentModel();
    if (m) m->paste();
}

void Main::editCut()
{
    VymModel *m=currentModel();
    if (m) m->cut();
}

bool Main::openURL(const QString &url)
{
    if (url.isEmpty()) return false;

    QString browser=settings.value("/mainwindow/readerURL" ).toString();
    QStringList args;
    args<<url;
    if (!QProcess::startDetached(browser,args,QDir::currentPath(),browserPID))
    {
        // try to set path to browser
        QMessageBox::warning(0, 
            tr("Warning"),
            tr("Couldn't find a viewer to open %1.\n").arg(url)+
            tr("Please use Settings->")+tr("Set application to open an URL"));
        settingsURL() ; 
        return false;
    }   
    return true;
}

void Main::openTabs(QStringList urls)
{
<<<<<<< HEAD
    if (!urls.isEmpty())
    {	
	bool success=true;
	QStringList args;
	QString browser=settings.value("/mainwindow/readerURL" ).toString();

#if defined(Q_OS_LINUX)
	//qDebug ()<<"Services: "<<QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
	if (*browserPID==0 ||
	    (browser.contains("konqueror") &&
	     !QDBusConnection::sessionBus().interface()->registeredServiceNames().value().contains (QString("org.kde.konqueror-%1").arg(*browserPID)))
	   )	 
	{
	    // Start a new browser, if there is not one running already or
	    // if a previously started konqueror is gone.
	    if (debug) qDebug() <<"Main::openTabs no konqueror-"<<*browserPID<<" found";
	    QString u=urls.takeFirst();
	    args<<u;
	    QString workDir=QDir::currentPath();
	    if (!QProcess::startDetached(browser,args,workDir,browserPID))
	    {
		// try to set path to browser
		QMessageBox::warning(0, 
		    tr("Warning"),
		    tr("Couldn't find a viewer to open %1.\n").arg(u)+
		    tr("Please use Settings->")+tr("Set application to open an URL"));
		return;
	    }
	    if (debug) qDebug() << "Main::openTabs  Started konqueror-"<<*browserPID;
=======
    if (urls.isEmpty()) return;
    	
    bool success=true;
    QStringList args;
    QString browser=settings.value("/mainwindow/readerURL" ).toString();
    if ( browser.contains("konqueror") && 
            (browserPID==0 || !QDBusConnection::sessionBus().interface()->registeredServiceNames().value().contains (QString("org.kde.konqueror-%1").arg(*browserPID)))
       )	 
    {
        // Start a new browser, if there is not one running already or
        // if a previously started konqueror is gone.
        if (debug) qDebug() <<"Main::openTabs no konqueror with PID "<<*browserPID<<" found";
        openURL(urls.takeFirst());
        if (debug) qDebug() << "Main::openTabs Started konqueror, new PID is "<<*browserPID;
>>>>>>> master
#if defined(Q_OS_WIN32)
        // There's no sleep in VCEE, replace it with Qt's QThread::wait().
        this->thread()->wait(3000);
#else
        sleep (3);	//needed to start first konqueror
#endif
    }

<<<<<<< HEAD
	if (browser.contains("konqueror"))
	{
	    for (int i=0; i<urls.size(); i++)
	    {
		// Open new browser
		// Try to open new tab in existing konqueror started previously by vym
		args.clear();

		args<< QString("org.kde.konqueror-%1").arg(*browserPID)<<
		    "/konqueror/MainWindow_1"<<
		    "newTab" <<
		    urls.at(i)<<
		    "false";
		if (debug) qDebug() << "MainWindow::openURLs  args="<<args.join(" ");
		if (!QProcess::startDetached ("qdbus",args))
		    success=false;
	    }
	    if (!success)
		QMessageBox::warning(0, 
		    tr("Warning"),
		    tr("Couldn't start %1 to open a new tab in %2.").arg("dcop").arg("konqueror"));
	    return;	
	}
#endif
        if (browser.contains ("firefox") || browser.contains ("mozilla") )
	{
	    for (int i=0; i<urls.size(); i++)
	    {
		// Try to open new tab in firefox
		args<< "-remote"<< QString("openurl(%1,new-tab)").arg(urls.at(i));
		if (!QProcess::startDetached (browser,args))
		    success=false;
	    }		
	    if (!success)
		QMessageBox::warning(0, 
		    tr("Warning"),
		    tr("Couldn't start %1 to open a new tab").arg(browser));
	    return;	
	}	    
	QMessageBox::warning(0, 
	    tr("Warning"),
	    tr("Sorry, currently only Konqueror supports integrated tabbed browsing.","Mainwindow, open URL")+
	    tr("Currently vym is using %1 to open external links.\n(Change in Settings menu)","Mainwindow, open URL")
		.arg(settings.value("/mainwindow/readerURL" ).toString()));
    }	
=======
    if (browser.contains("konqueror"))
    {
        foreach (QString u, urls) 
        {
            // Open new browser
            // Try to open new tab in existing konqueror started previously by vym
            args.clear();

            args<< QString("org.kde.konqueror-%1").arg(*browserPID)<<
                "/konqueror/MainWindow_1"<<
                "newTab" << 
                u <<
                "false";
            if (!QProcess::startDetached ("qdbus",args))
                success=false;
        }
        if (!success)
            QMessageBox::warning(0, 
                tr("Warning"),
                tr("Couldn't start %1 to open a new tab in %2.").arg("qdbus").arg("konqueror"));
        return;	
    } 
    // Other browser, e.g. xdg-open
    // Just open all urls and leave it to the system to cope with it
    foreach (QString u, urls) openURL(u);
>>>>>>> master
}

void Main::editOpenURL()
{
    // Open new browser
    VymModel *m=currentModel();
    if (m)
    {	
	QString url=m->getURL();
	if (url=="") return;
        openURL(url);
    }	
}
void Main::editOpenURLTab()
{
    VymModel *m=currentModel();
    if (m)
    {	
	QStringList urls;
	urls.append(m->getURL());
	openTabs (urls);
    }	
}

void Main::editOpenMultipleVisURLTabs(bool ignoreScrolled)
{
    VymModel *m=currentModel();
    if (m)
    {	
	QStringList urls;
	urls=m->getURLs(ignoreScrolled);
	openTabs (urls);
    }	
}

void Main::editOpenMultipleURLTabs()
{
    editOpenMultipleVisURLTabs (false);
}

void Main::editNote2URLs()
{
    VymModel *m=currentModel();
    if (m) m->note2URLs();
}

void Main::editURL()
{
    VymModel *m=currentModel();
    if (m) 
    {
	QInputDialog *dia=new QInputDialog (this);
	dia->setLabelText (tr("Enter URL:"));
	dia->setWindowTitle (vymName);
	dia->setInputMode (QInputDialog::TextInput);
	TreeItem *selti=m->getSelectedItem();
	if (selti) dia->setTextValue (selti->getURL());
	dia->resize(width()*0.6,80);
        centerDialog(dia);

	if ( dia->exec() ) m->setURL (dia->textValue() );
        delete dia;
    }
}

void Main::editLocalURL()
{
    VymModel *m=currentModel();
    if (m) 
    {
	TreeItem *selti=m->getSelectedItem();
	if (selti)
	{	    
	    QString filter;
	    filter+="All files (*);;";
	    filter+=tr("HTML","Filedialog") + " (*.html,*.htm);;";
	    filter+=tr("Text","Filedialog") + " (*.txt);;";
	    filter+=tr("Spreadsheet","Filedialog") + " (*.odp,*.sxc);;";
	    filter+=tr("Textdocument","Filedialog") +" (*.odw,*.sxw);;";
	    filter+=tr("Images","Filedialog") + " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";

	    QString fn=QFileDialog::getOpenFileName( 
		this,
		vymName+" - " +tr("Set URL to a local file"), 
		lastMapDir.path(), 
		filter);

	    if (!fn.isEmpty() )
	    {
		lastMapDir.setPath(fn.left(fn.lastIndexOf ("/")) );
		if (!fn.startsWith("file://") ) 
		    fn="file://" + fn;
		m->setURL (fn);
	    }
	}
    }
}

void Main::editHeading2URL()
{
    VymModel *m=currentModel();
    if (m) m->editHeading2URL();
}

void Main::editBugzilla2URL()
{
    VymModel *m=currentModel();
    if (m) m->editBugzilla2URL();
}

void Main::getBugzillaData()
{
    VymModel *m=currentModel();
    /*
    QProgressDialog progress ("Doing stuff","cancl",0,10,this);
    progress.setWindowModality(Qt::WindowModal);
    //progress.setCancelButton (NULL);
    progress.show();
    progress.setMinimumDuration (0);
    progress.setValue (1);
    progress.setValue (5);
    progress.update();
    */
    /*
    QProgressBar *pb=new QProgressBar;
    pb->setMinimum (0);
    pb->setMaximum (0);
    pb->show();
    pb->repaint();
    */
    if (m) m->getBugzillaData(false);
}

void Main::getBugzillaDataSubtree()
{
    VymModel *m=currentModel();
    if (m) m->getBugzillaData(true);
}

void Main::editFATE2URL()
{
    VymModel *m=currentModel();
    if (m) m->editFATE2URL();
}

void Main::editHeading()
{
    MapEditor *me=currentMapEditor();
    if (me) me->editHeading();
}

void Main::editHeadingFinished(VymModel *m)
{
    if (m)
    {
	if (!actionSettingsAutoSelectNewBranch->isChecked() && 
	    !prevSelection.isEmpty()) 
	    m->select(prevSelection);
	prevSelection="";
    }
}

void Main::openVymLinks(const QStringList &vl)
{
    QStringList vlmin;
    int index=-1;
    for (int j=0; j<vl.size(); ++j)
    {
	// compare path with already loaded maps
	int i;
	index=-1;
	for (i=0;i<=vymViews.count() -1;i++)
	{
	    if (vl.at(j)==vymViews.at(i)->getModel()->getFilePath() )
	    {
		index=i;
		break;
	    }
	}   
	if (index<0) vlmin.append (vl.at(j));
    }

    
    progressCounterTotal=vlmin.size();
    for (int j=0; j<vlmin.size(); j++)
    {
	// Load map
	if (!QFile(vlmin.at(j)).exists() )
	    QMessageBox::critical( 0, tr( "Critical Error" ),
	       tr("Couldn't open map %1").arg(vlmin.at(j)));
	else
	{
	    fileLoad (vlmin.at(j), NewMap,VymMap);
	    tabWidget->setCurrentIndex (tabWidget->count()-1);	
	}
    }	    
    // Go to tab containing the map
    if (index>=0)
	tabWidget->setCurrentIndex (index);	
    removeProgressCounter();
}

void Main::editOpenVymLink()
{
    VymModel *m=currentModel();
    if (m)
    {
	QStringList vl;
	vl.append(m->getVymLink()); 
	openVymLinks (vl);
    }
}

void Main::editOpenMultipleVymLinks()
{
    QString currentVymLink;
    VymModel *m=currentModel();
    if (m)
    {
	QStringList vl=m->getVymLinks();
	openVymLinks (vl);
    }
}

void Main::editVymLink()
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *bi=m->getSelectedBranch();
	if (bi)
	{	    
	    QStringList filters;
	    filters <<"VYM map (*.vym)";
	    QFileDialog fd;
	    fd.setWindowTitle (vymName+" - " +tr("Link to another map"));
	    fd.setFilters (filters);
	    fd.setWindowTitle(vymName+" - " +tr("Link to another map"));
	    fd.setDirectory (lastMapDir);
	    fd.setAcceptMode (QFileDialog::AcceptOpen);
	    if (! bi->getVymLink().isEmpty() )
		fd.selectFile( bi->getVymLink() );
	    fd.show();

	    QString fn;
	    if ( fd.exec() == QDialog::Accepted &&!fd.selectedFiles().isEmpty() )
	    {
		QString fn=fd.selectedFiles().first();
		lastMapDir=QDir (fd.directory().path());
		m->setVymLink (fn);
	    }
	}
    }
}

void Main::editDeleteVymLink()
{
    VymModel *m=currentModel();
    if (m) m->deleteVymLink();	
}

void Main::editToggleHideExport()
{
    VymModel *m=currentModel();
    if (m) m->toggleHideExport();   
}

void Main::editToggleTask()
{
    VymModel *m=currentModel();
    if (m) m->toggleTask();   
}

void Main::editCycleTaskStatus()
{
    VymModel *m=currentModel();
    if (m) m->cycleTaskStatus();   
}

void Main::editTaskSleepN()
{
    VymModel *m=currentModel();
    if (m) 
    {
	int n=((QAction*)sender())->data().toInt();
	Task *task=m->getSelectedTask();
	if (task)
	{
	    bool ok=true;
            QString s;
	    if (n<0)
            {
                n=task->getDaysSleep();
                if (n<=0) n=0;

                LineEditDialog *dia=new LineEditDialog(this);
                dia->setLabel(tr("Enter sleep time (number of days or date YYYY-MM-DD or DD.MM[.YYYY]","task sleep time dialog"));
                dia->setText(QString("%1").arg(n));
                centerDialog (dia);
                if (dia->exec() == QDialog::Accepted)
                {
                    ok=true;
                    s=dia->getText();
                } else
                    ok=false;

                delete dia;
            } else
                s=QString("%1").arg(n);

            if (ok && !m->setTaskSleep(s) )
                QMessageBox::warning(0, 
                    tr("Warning"),
                    tr("Couldn't set sleep time to %1.\n").arg(s));
	}
    }
}

void Main::editAddTimestamp()
{
    VymModel *m=currentModel();
    if (m) m->addTimestamp();	
}

void Main::editMapInfo()    
{
    VymModel *m=currentModel();
    if (!m) return;

    ExtraInfoDialog dia;
    dia.setMapName (m->getFileName() );
    dia.setMapTitle (m->getTitle() );
    dia.setAuthor (m->getAuthor() );
    dia.setComment(m->getComment() );

    // Calc some stats
    QString stats;
    stats+=tr("%1 items on map\n","Info about map").arg (m->getScene()->items().size(),6);

    uint b=0;
    uint f=0;
    uint n=0;
    uint xl=0;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    m->nextBranch(cur,prev);
    while (cur) 
    {
	if (!cur->getNote().isEmpty() ) n++;
	f+= cur->imageCount();
	b++;
	xl+=cur->xlinkCount();
	m->nextBranch(cur,prev);
    }

    stats+=QString ("%1 branches\n").arg (m->branchCount(),6);
    stats+=QString ("%1 notes\n").arg (n,6);
    stats+=QString ("%1 images\n").arg (f,6);
    stats+=QString ("%1 tasks\n").arg (m->taskCount(),6 );;
    stats+=QString ("%1 slides\n").arg (m->slideCount(),6 );;
    stats+=QString ("%1 xLinks \n").arg (xl/2,6);
    dia.setStats (stats);

    // Finally show dialog
    if (dia.exec() == QDialog::Accepted)
    {
	m->setAuthor (dia.getAuthor() );
	m->setComment (dia.getComment() );
	m->setTitle (dia.getMapTitle() );
    }
}

void Main::editMoveUp()
{
    VymModel *m=currentModel();
    if (m) m->moveUp();
}

void Main::editMoveDown()
{
    VymModel *m=currentModel();
    if (m) m->moveDown();
}

void Main::editDetach()
{
    VymModel *m=currentModel();
    if (m) m->detach();
}

void Main::editSortChildren()
{
    VymModel *m=currentModel();
    if (m) m->sortChildren(false);
}

void Main::editSortBackChildren()
{
    VymModel *m=currentModel();
    if (m) m->sortChildren(true);
}

void Main::editToggleScroll()
{
    VymModel *m=currentModel();
    if (m) m->toggleScroll();
}

void Main::editExpandAll()
{
    VymModel *m=currentModel();
    if (m) m->emitExpandAll();
}

void Main::editExpandOneLevel()
{
    VymModel *m=currentModel();
    if (m) m->emitExpandOneLevel();
}

void Main::editCollapseOneLevel()
{
    VymModel *m=currentModel();
    if (m) m->emitCollapseOneLevel();
}

void Main::editCollapseUnselected()
{
    VymModel *m=currentModel();
    if (m) m->emitCollapseUnselected();
}

void Main::editUnscrollChildren()
{
    VymModel *m=currentModel();
    if (m) m->unscrollChildren();
}

void Main::editGrowSelectionSize()
{
    VymModel *m=currentModel();
    if (m) m->growSelectionSize();
}

void Main::editShrinkSelectionSize()
{
    VymModel *m=currentModel();
    if (m) m->shrinkSelectionSize();
}

void Main::editResetSelectionSize()
{
    VymModel *m=currentModel();
    if (m) m->resetSelectionSize();
}

void Main::editAddAttribute()
{
    VymModel *m=currentModel();
    if (m) 
    {

	m->addAttribute();
    }
}

void Main::editAddMapCenter() 
{
    VymModel *m=currentModel();
    if (m) 
    {
	m->select (m->addMapCenter ());
	MapEditor *me=currentMapEditor();
	if (me) 
	{
	    m->setHeading("");
	    me->editHeading();
	}    
    }
}

void Main::editNewBranch()
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *bi=m->addNewBranch();
	if (!bi) return;

	if (actionSettingsAutoEditNewBranch->isChecked() 
	     && !actionSettingsAutoSelectNewBranch->isChecked() )
	    prevSelection=m->getSelectString();
	else	
	    prevSelection=QString();

	if (actionSettingsAutoSelectNewBranch->isChecked()  
	    || actionSettingsAutoEditNewBranch->isChecked()) 
	{
	    m->select (bi);
	    if (actionSettingsAutoEditNewBranch->isChecked())
		currentMapEditor()->editHeading();
	}
    }	
}

void Main::editNewBranchBefore()
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *bi=m->addNewBranchBefore();

	if (bi) 
	    m->select (bi);
	else
	    return;

	if (actionSettingsAutoEditNewBranch->isChecked())
	{
	    if (!actionSettingsAutoSelectNewBranch->isChecked())
		prevSelection=m->getSelectString(bi); 
	    currentMapEditor()->editHeading();
	}
    }	
}

void Main::editNewBranchAbove()	
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *selbi=m->getSelectedBranch();
	if (selbi)
	{
	    BranchItem *bi=m->addNewBranch(selbi,-3);

	    if (bi) 
		m->select (bi);
	    else
		return;

	    if (actionSettingsAutoEditNewBranch->isChecked())
	    {
		if (!actionSettingsAutoSelectNewBranch->isChecked())
		    prevSelection=m->getSelectString (bi);	
		currentMapEditor()->editHeading();
	    }
	}
    }	
}

void Main::editNewBranchBelow()
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *selbi=m->getSelectedBranch();
	if (selbi)
	{
	    BranchItem *bi=m->addNewBranch(selbi,-1);

	    if (bi) 
		m->select (bi);
	    else
		return;

	    if (actionSettingsAutoEditNewBranch->isChecked())
	    {
		if (!actionSettingsAutoSelectNewBranch->isChecked())
		    prevSelection=m->getSelectString(bi);
		currentMapEditor()->editHeading();
	    }
	}
    }	
}

void Main::editImportAdd()
{
    fileLoad (ImportAdd);
}

void Main::editImportReplace()
{
    fileLoad (ImportReplace);
}

void Main::editSaveBranch() 
{
    fileSaveAs (PartOfMap);
}

void Main::editDeleteKeepChildren()
{
    VymModel *m=currentModel();
     if (m) m->deleteKeepChildren();
}

void Main::editDeleteChildren()
{
    VymModel *m=currentModel();
    if (m) m->deleteChildren();
}

void Main::editDeleteSelection()
{
    VymModel *m=currentModel();
    if (m) m->deleteSelection();
}

void Main::editLoadImage()
{
    VymModel *m=currentModel();
    if (m) m->loadImage();
}

void Main::editSaveImage()
{
    VymModel *m=currentModel();
    if (m) m->saveImage();
}

void Main::editEditXLink(QAction *a)
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *selbi=m->getSelectedBranch();
	if (selbi)
	{
	    Link *l=selbi->getXLinkItemNum(branchXLinksContextMenuEdit->actions().indexOf(a))->getLink();
	    if (l && m->select (l->getBeginLinkItem() ) )
		m->editXLink();
	}    
    }	
}

void Main::popupFollowXLink()
{
    branchXLinksContextMenuFollow->exec( QCursor::pos());
}

void Main::editFollowXLink(QAction *a)
{
    VymModel *m=currentModel();

    if (m)
	m->followXLink(branchXLinksContextMenuFollow->actions().indexOf(a));
}

void Main::editToggleTarget()  
{
    VymModel *m=currentModel();
    if (m) m->toggleTarget();
}

void Main::editGoToTarget()  
{
    VymModel *m=currentModel();
    if (m) 
    {
	targetsContextMenu->clear();

	ItemList targets=m->getTargets();
	QMap<uint,QString>::const_iterator i = targets.constBegin();
	while (i != targets.constEnd()) 
	{
	    (targetsContextMenu->addAction (i.value() ) )->setData (i.key());
	    ++i;
	}
	QAction *a=targetsContextMenu->exec (QCursor::pos());
	if (a) m->select (m->findID (a->data().toUInt() ) );
    }
}

void Main::editMoveToTarget()  
{
    VymModel *m=currentModel();
    if (m) 
    {
	targetsContextMenu->clear();

	ItemList targets=m->getTargets();
	QMap<uint,QString>::const_iterator i = targets.constBegin();
	while (i != targets.constEnd()) 
	{
	    (targetsContextMenu->addAction (i.value() ) )->setData (i.key());
	    ++i;
	}
	QAction *a=targetsContextMenu->exec (QCursor::pos());
	if (a) 
	{
	    TreeItem *ti=m->findID (a->data().toUInt());
	    BranchItem *selbi=m->getSelectedBranch();
	    if (!selbi) return;

	    if (ti && ti->isBranchLikeType() && selbi)
	    {
		BranchItem *pi =selbi->parentBranch();
		// If branch below exists, select that one
		// Makes it easier to quickly resort using the MoveTo function
		BranchItem *below=pi->getBranchNum(selbi->num()+1);
		LinkableMapObj *lmo=selbi->getLMO();
		QPointF orgPos;
		if (lmo) orgPos=lmo->getAbsPos();

		if (m->relinkBranch ( selbi, (BranchItem*)ti,-1,true,orgPos) )
		{
		    if (below) 
			m->select (below);
		    else    
			if (pi) m->select (pi);
		}    
	    }	    
	}
    }
}

void Main::editSelectPrevious()  
{
    VymModel *m=currentModel();
    if (m) m->selectPrevious();
}

void Main::editSelectNext()  
{
    VymModel *m=currentModel();
    if (m) m->selectNext();
}

void Main::editSelectNothing()  
{
    VymModel *m=currentModel();
    if (m) m->unselectAll();
}

void Main::editOpenFindResultWidget()  
{
    if (!findResultWidget->parentWidget()->isVisible())
    {
//	findResultWidget->parentWidget()->show();
	findResultWidget->popup();
    } else 
	findResultWidget->parentWidget()->hide();
}

#include "findwidget.h" // FIXME-4 Integrated FRW and FW
void Main::editFindNext(QString s)  
{
    Qt::CaseSensitivity cs=Qt::CaseInsensitive;
    VymModel *m=currentModel();
    if (m) 
    {
	if (m->findAll (findResultWidget->getResultModel(),s,cs) )
	    findResultWidget->setStatus (FindWidget::Success);
	else
	    findResultWidget->setStatus (FindWidget::Failed);
    }
}

void Main::editFindDuplicateURLs() //FIXME-4 feature: use FindResultWidget for display
{
    VymModel *m=currentModel();
    if (m) m->findDuplicateURLs();
}

void Main::updateQueries (VymModel* ) //FIXME-4 disabled for now to avoid selection in FRW
{
    return;
/*
    qDebug() << "MW::updateQueries m="<<m<<"   cM="<<currentModel();
    if (m && currentModel()==m)
    {
	QString s=findResultWidget->getFindText();
	if (!s.isEmpty() ) editFindNext (s);
    }	
*/
}

void Main::formatPickColor()
{
    VymModel *m=currentModel();
    if (m)
	setCurrentColor ( m->getCurrentHeadingColor() );
}

QColor Main::getCurrentColor()
{
    return currentColor;
}

void Main::setCurrentColor(QColor c)
{
    QPixmap pix( 16, 16 );
    pix.fill( c );
    actionFormatColor->setIcon( pix );
    currentColor=c;
}

void Main::formatSelectColor()
{
    QColor col = QColorDialog::getColor((currentColor ), this );
    if ( !col.isValid() ) return;
    setCurrentColor( col );
}

void Main::formatColorBranch()
{
    VymModel *m=currentModel();
    if (m) m->colorBranch(currentColor);
}

void Main::formatColorSubtree()
{
    VymModel *m=currentModel();
    if (m) m->colorSubtree (currentColor);
}

void Main::formatLinkStyleLine()
{
    VymModel *m=currentModel();
    if (m)
    {
	m->setMapLinkStyle("StyleLine");
        actionFormatLinkStyleLine->setChecked(true);
    }
}

void Main::formatLinkStyleParabel()
{
    VymModel *m=currentModel();
    if (m)
    {
	m->setMapLinkStyle("StyleParabel");
        actionFormatLinkStyleParabel->setChecked(true);
    }
}

void Main::formatLinkStylePolyLine()
{
    VymModel *m=currentModel();
    if (m)
    {
	m->setMapLinkStyle("StylePolyLine");
        actionFormatLinkStylePolyLine->setChecked(true);
    }
}

void Main::formatLinkStylePolyParabel()
{
    VymModel *m=currentModel();
    if (m)
    {
	m->setMapLinkStyle("StylePolyParabel");
        actionFormatLinkStylePolyParabel->setChecked(true);
    }
}

void Main::formatSelectBackColor()
{
    VymModel *m=currentModel();
    if (m) m->selectMapBackgroundColor();
}

void Main::formatSelectBackImage()
{
    VymModel *m=currentModel();
    if (m)
	m->selectMapBackgroundImage();
}

void Main::formatSelectLinkColor()
{
    VymModel *m=currentModel();
    if (m)
    {
	QColor col = QColorDialog::getColor( m->getMapDefLinkColor(), this );
	m->setMapDefLinkColor( col );
    }
}

void Main::formatSelectSelectionColor()
{
    VymModel *m=currentModel();
    if (m)
    {
	QColor col = QColorDialog::getColor( m->getMapDefLinkColor(), this );
	m->setSelectionColor (col);
    }

}

void Main::formatSelectFont()
{
    VymModel *m=currentModel();
    if (m) 
    {
	bool ok;
	QFont font = QFontDialog::getFont( &ok, m->getMapDefaultFont(), this);
	if (ok) m->setMapDefaultFont (font);
    }
}

void Main::formatToggleLinkColorHint()
{
    VymModel *m=currentModel();
    if (m) m->toggleMapLinkColorHint();
}

void Main::formatHideLinkUnselected()	//FIXME-4 get rid of this with imagepropertydialog
{
    VymModel *m=currentModel();
    if (m)
	m->setHideLinkUnselected(actionFormatHideLinkUnselected->isChecked());
}

void Main::viewZoomReset()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setViewCenterTarget();
}

void Main::viewZoomIn()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setZoomFactorTarget (me->getZoomFactorTarget()*1.15);
}

void Main::viewZoomOut()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setZoomFactorTarget (me->getZoomFactorTarget()*0.85);
}

void Main::viewRotateCounterClockwise()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setAngleTarget (me->getAngleTarget()-10);
}

void Main::viewRotateClockwise()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setAngleTarget (me->getAngleTarget()+10);
}

void Main::viewCenter()
{
    VymModel *m=currentModel();
    if (m) m->emitShowSelection();
}

void Main::networkStartServer()
{
    VymModel *m=currentModel();
    if (m) m->newServer();
}

void Main::networkConnect()
{
    VymModel *m=currentModel();
    if (m) m->connectToServer();
}

bool Main::settingsPDF()
{
    // Default browser is set in constructor
    bool ok;
    QString text = QInputDialog::getText(
	this,
	"VYM", tr("Set application to open PDF files")+":", QLineEdit::Normal,
	settings.value("/mainwindow/readerPDF").toString(), &ok);
    if (ok)
	settings.setValue ("/mainwindow/readerPDF",text);
    return ok;
}


bool Main::settingsURL()
{
    // Default browser is set in constructor
    bool ok;
    QString text = QInputDialog::getText(
	this,
	"VYM", tr("Set application to open an URL")+":", QLineEdit::Normal,
	settings.value("/mainwindow/readerURL").toString()
	, &ok);
    if (ok)
	settings.setValue ("/mainwindow/readerURL",text);
    return ok;
}

void Main::settingsMacroDir()
{
    QDir defdir(vymBaseDir.path() + "/macros");
    if (!defdir.exists())
	defdir=vymBaseDir;
    QDir dir=QFileDialog::getExistingDirectory (
	this,
	tr ("Directory with vym macros:"), 
	settings.value ("/macros/macroDir",defdir.path()).toString()
    );
    if (dir.exists())
	settings.setValue ("/macros/macroDir",dir.absolutePath());
}

void Main::settingsUndoLevels()	    
{
    bool ok;
    int i = QInputDialog::getInteger(
	this, 
	"QInputDialog::getInteger()",
	tr("Number of undo/redo levels:"), settings.value("/history/stepsTotal",1000).toInt(), 0, 100000, 1, &ok);
    if (ok)
    {
	settings.setValue ("/history/stepsTotal",i);
	QMessageBox::information( this, tr( "VYM -Information:" ),
	   tr("Settings have been changed. The next map opened will have \"%1\" undo/redo levels").arg(i)); 
   }	
}

bool Main::useAutosave()
{
    return actionSettingsAutosaveToggle->isChecked();
}

void Main::setAutosave(bool b)
{
    actionSettingsAutosaveToggle->setChecked(b);
}

void Main::settingsAutosaveTime()
{
    bool ok;
    int i = QInputDialog::getInteger(
	this, 
	"QInputDialog::getInteger()",
	tr("Number of seconds before autosave:"), settings.value("/mainwindow/autosave/ms").toInt() / 1000, 10, 60000, 1, &ok);
    if (ok)
	settings.setValue ("/mainwindow/autosave/ms",i * 1000);
}

void Main::settingsAutoLayoutToggle()
{
    settings.setValue ("/mainwindow/autoLayout/use",actionSettingsAutosaveToggle->isChecked() );
}

void Main::settingsWriteBackupFileToggle()
{
    settings.setValue ("/mainwindow/writeBackupFile",actionSettingsWriteBackupFile->isChecked() );
}

void Main::settingsToggleAnimation()
{
    settings.setValue ("/animation/use",actionSettingsUseAnimation->isChecked() );
}

void Main::windowToggleNoteEditor()
{
    if (noteEditorDW->isVisible() )
	noteEditorDW->hide();
    else
	noteEditorDW->show();
}

void Main::windowToggleTreeEditor()
{
    if ( tabWidget->currentWidget())
	vymViews.at(tabWidget->currentIndex())->toggleTreeEditor();
}

void Main::windowToggleTaskEditor()
{
    if (taskEditor->parentWidget()->isVisible() )
    {
	taskEditor->parentWidget()->hide();
	actionViewToggleTaskEditor->setChecked (false);
    } else
    {
	taskEditor->parentWidget()->show();
	actionViewToggleTaskEditor->setChecked (true);
    }
}

void Main::windowToggleSlideEditor()
{
    if ( tabWidget->currentWidget())
	vymViews.at(tabWidget->currentIndex())->toggleSlideEditor();
}

void Main::windowToggleScriptEditor()
{
    if (scriptEditor->parentWidget()->isVisible() )
    {
	scriptEditor->parentWidget()->hide();
	actionViewToggleScriptEditor->setChecked (false);
    } else
    {
	scriptEditor->parentWidget()->show();
	actionViewToggleScriptEditor->setChecked (true);
    }
}

void Main::windowToggleHistory()
{
    if (historyWindow->isVisible())
	historyWindow->parentWidget()->hide();
    else    
	historyWindow->parentWidget()->show();
}

void Main::windowToggleProperty()
{
    if (branchPropertyEditor->parentWidget()->isVisible())
	branchPropertyEditor->parentWidget()->hide();
    else    
	branchPropertyEditor->parentWidget()->show();
    branchPropertyEditor->setModel (currentModel() );
}

void Main::windowToggleAntiAlias()
{
    bool b=actionViewToggleAntiAlias->isChecked();
    MapEditor *me;
    for (int i=0;i<vymViews.count();i++)
    {
	me=vymViews.at(i)->getMapEditor();
	if (me) me->setAntiAlias(b);
    }	

}

bool Main::isAliased()
{
    return actionViewToggleAntiAlias->isChecked();
}

bool Main::hasSmoothPixmapTransform()
{
    return actionViewToggleSmoothPixmapTransform->isChecked();
}

void Main::windowToggleSmoothPixmap()
{
    bool b=actionViewToggleSmoothPixmapTransform->isChecked();
    MapEditor *me;
    for (int i=0;i<vymViews.count();i++)
    {
	
	me=vymViews.at(i)->getMapEditor();
	if (me) me->setSmoothPixmap(b);
    }	
}

void Main::updateHistory(SimpleSettings &undoSet)
{
    historyWindow->update (undoSet);
}

void Main::updateHeading()
{
    VymModel *m=currentModel();
    if (m) m->setHeading (headingEditor->getText() );
}

void Main::updateNoteFlag() 
{
    // this slot is connected to noteEditor::textHasChanged()
    VymModel *m=currentModel();
    if (m) m->updateNoteFlag();
}

void Main::updateNoteEditor(QModelIndex index ) //FIXME-4 maybe change to TreeItem as parameter?
{
    if (index.isValid() )
    {
	TreeItem *ti=((VymModel*) QObject::sender())->getItem(index);
	/*
	qDebug()<< "Main::updateNoteEditor model="<<sender() 
		<< "  item="<<ti->getHeadingStd()<<" ("<<ti<<")";
	*/
	if (ti) noteEditor->setNote (ti->getNoteObj() );
    }
}

void Main::selectInNoteEditor(QString s,int i)
{
    // TreeItem is already selected at this time, therefor
    // the note is already in the editor
    noteEditor->findText (s,0,i);
}

void Main::changeSelection (VymModel *model, const QItemSelection &newsel, const QItemSelection &)
{
    branchPropertyEditor->setModel (model ); 

    if (model && model==currentModel() )
    {
	TreeItem *ti;
	if (!newsel.indexes().isEmpty() )
	{
	    ti=model->getItem(newsel.indexes().first());
	    if (!ti->hasEmptyNote() )
		noteEditor->setNote(ti->getNoteObj() );
	    else
		noteEditor->setNote(NoteObj() );    //FIXME-5 maybe add a clear() to TE
	    // Show URL and link in statusbar	
	    QString status;
	    QString s=ti->getURL();
	    if (!s.isEmpty() ) status+="URL: "+s+"  ";
	    s=ti->getVymLink();
	    if (!s.isEmpty() ) status+="Link: "+s;
	    if (!status.isEmpty() ) statusMessage (status);

	    headingEditor->setText (ti->getHeading() );

	    // Select in TaskEditor, if necessary 
	    if (ti->isBranchLikeType() )
		taskEditor->select ( ((BranchItem*)ti)->getTask() );
	} else
	    noteEditor->setInactive();

	updateActions();
    }
}

void Main::updateActions()
{
    // updateActions is also called when satellites are closed	
    actionViewToggleNoteEditor->setChecked (noteEditor->parentWidget()->isVisible());
    actionViewToggleTaskEditor->setChecked (taskEditor->parentWidget()->isVisible());
    actionViewToggleHistoryWindow->setChecked (historyWindow->parentWidget()->isVisible());
    actionViewTogglePropertyEditor->setChecked (branchPropertyEditor->parentWidget()->isVisible());
    actionViewToggleScriptEditor->setChecked (scriptEditor->parentWidget()->isVisible());
    VymView *vv=currentView();
    if (vv)
    {
	actionViewToggleTreeEditor->setChecked ( vv->treeEditorIsVisible() );
	actionViewToggleSlideEditor->setChecked( vv->slideEditorIsVisible() );
    } else	
    {
	actionViewToggleTreeEditor->setChecked  ( false );
	actionViewToggleSlideEditor->setChecked ( false );
    }

    VymModel  *m =currentModel();
    if (m)
    {
	// Enable all map actions first
	for (int i=0; i<actionListMap.size(); ++i)	
	    actionListMap.at(i)->setEnabled(true);

	// Disable other actions for now
	for (int i=0; i<actionListBranches.size(); ++i) 
	    actionListBranches.at(i)->setEnabled(false);

	for (int i=0; i<actionListItems.size(); ++i) 
	    actionListItems.at(i)->setEnabled(false);

	// Link style in context menu
	switch (m->getMapLinkStyle())
	{
	    case LinkableMapObj::Line: 
		actionFormatLinkStyleLine->setChecked(true);
		break;
	    case LinkableMapObj::Parabel:
		actionFormatLinkStyleParabel->setChecked(true);
		break;
	    case LinkableMapObj::PolyLine:  
		actionFormatLinkStylePolyLine->setChecked(true);
		break;
	    case LinkableMapObj::PolyParabel:	
		actionFormatLinkStylePolyParabel->setChecked(true);
		break;
	    default:
		break;
	}	

	// Update colors
	QPixmap pix( 16, 16 );
	pix.fill( m->getMapBackgroundColor() );
	actionFormatBackColor->setIcon( pix );
	pix.fill( m->getSelectionColor() );
	actionFormatSelectionColor->setIcon( pix );
	pix.fill( m->getMapDefLinkColor() );
	actionFormatLinkColor->setIcon( pix );

	// Selection history
	if (!m->canSelectPrevious() )
	    actionSelectPrevious->setEnabled(false);

	if (!m->canSelectNext() )
	    actionSelectNext->setEnabled(false);
	    
	if (!m->getSelectedItem() )
	    actionSelectNothing->setEnabled (false);

	// Save
	if (! m->hasChanged() )
	    actionFileSave->setEnabled( false);

	// Undo/Redo
	if (! m->isUndoAvailable())
	    actionUndo->setEnabled( false);

	if (! m->isRedoAvailable())
	    actionRedo->setEnabled( false);

	// History window
	historyWindow->setWindowTitle (vymName + " - " +tr("History for %1","Window Caption").arg(m->getFileName()));


	// Expanding/collapsing
	actionExpandAll->setEnabled (true);
	actionExpandOneLevel->setEnabled (true);
	actionCollapseOneLevel->setEnabled (true);
	actionCollapseUnselected->setEnabled (true);

	if (m->getMapLinkColorHint()==LinkableMapObj::HeadingColor) 
	    actionFormatLinkColorHint->setChecked(true);
	else    
	    actionFormatLinkColorHint->setChecked(false);

	// Export last
	QString s, t, u, v;
	if (m && m->exportLastAvailable(s,t,u, v) )
	    actionFileExportLast->setEnabled (true);
	else
	{
	    actionFileExportLast->setEnabled (false);
	    t=u="";
	    s=" - ";
	}	
	actionFileExportLast->setText( tr( "Export in last used format (%1) to: %2","status tip" ).arg(s).arg(u));

	TreeItem *selti=m->getSelectedItem();
	BranchItem *selbi=m->getSelectedBranch();

	if (selti)
	{   // Tree Item selected
	    actionToggleTarget->setChecked (selti->isTarget() );
	    actionDelete->setEnabled (true);
	    actionDeleteChildren->setEnabled (true);

	    if (selbi || selti->getType()==TreeItem::Image)
	    {
		actionFormatHideLinkUnselected->setChecked (((MapItem*)selti)->getHideLinkUnselected());
		actionFormatHideLinkUnselected->setEnabled (true);
	    }

	    if (selbi)	
	    {	// Branch Item selected
		for (int i=0; i<actionListBranches.size(); ++i)	
		    actionListBranches.at(i)->setEnabled(true);

		actionHeading2URL->setEnabled (true);  

		// Note
		actionGetURLsFromNote->setEnabled (!selbi->getNote().isEmpty());

		standardFlagsMaster->setEnabled (true);

		// Take care of xlinks  
		// FIXME-4 similar code in mapeditor mousePressEvent
		int b=selbi->xlinkCount();
		branchXLinksContextMenuEdit->setEnabled(b);
		branchXLinksContextMenuFollow->setEnabled(b);
		branchXLinksContextMenuEdit->clear();
		branchXLinksContextMenuFollow->clear();
		if (b)
		{
		    BranchItem *bi;
		    QString s;
		    for (int i=0; i<selbi->xlinkCount();++i)
		    {
			bi=selbi->getXLinkItemNum(i)->getPartnerBranch();
			if (bi)
			{
			    s=bi->getHeading();
			    if (s.length()>xLinkMenuWidth)
				s=s.left(xLinkMenuWidth)+"...";
			    branchXLinksContextMenuEdit->addAction (s);
			    branchXLinksContextMenuFollow->addAction (s);
			}   
		    }
		}
		//Standard Flags
		standardFlagsMaster->updateToolBar (selbi->activeStandardFlagNames() );

		// System Flags
		actionToggleScroll->setEnabled (true);
		if ( selbi->isScrolled() )
		    actionToggleScroll->setChecked(true);
		else	
		    actionToggleScroll->setChecked(false);

		actionGetBugzillaDataSubtree->setEnabled (bugzillaClientAvailable);
		if ( selti->getURL().isEmpty() )
		{
		    actionOpenURL->setEnabled (false);
		    actionOpenURLTab->setEnabled (false);
		    actionGetBugzillaData->setEnabled (false);
		}   
		else	
		{
		    actionOpenURL->setEnabled (true);
		    actionOpenURLTab->setEnabled (true);
		    actionGetBugzillaData->setEnabled (
			selti->getURL().contains("bugzilla") && bugzillaClientAvailable);
		}
		if ( selti->getVymLink().isEmpty() )
		{
		    actionOpenVymLink->setEnabled (false);
		    actionDeleteVymLink->setEnabled (false);
		} else	
		{
		    actionOpenVymLink->setEnabled (true);
		    actionDeleteVymLink->setEnabled (true);
		}   

		if (!selbi->canMoveUp()) 
		    actionMoveUp->setEnabled (false);
		if (!selbi->canMoveDown()) 
		    actionMoveDown->setEnabled (false);

		if (selbi->branchCount() <2 )
		{
		    actionSortChildren->setEnabled (false);
		    actionSortBackChildren->setEnabled (false);
		}

		actionToggleHideExport->setEnabled (true);  
		actionToggleHideExport->setChecked (selbi->hideInExport() );	

		actionToggleTask->setEnabled (true);  
		if (!selbi->getTask() )
		    actionToggleTask->setChecked (false);
		else
		    actionToggleTask->setChecked (true);

		if (!clipboardEmpty)
		    actionPaste->setEnabled (true); 
		else	
		    actionPaste->setEnabled (false);	

		actionToggleTarget->setEnabled (true);
		return;
	    }	// end of BranchItem

	    if ( selti->getType()==TreeItem::Image)
	    {
		for (int i=0; i<actionListBranches.size(); ++i)	
		    actionListBranches.at(i)->setEnabled(false);

		standardFlagsMaster->setEnabled (false);

		actionOpenURL->setEnabled (false);
		actionOpenVymLink->setEnabled (false);
		actionDeleteVymLink->setEnabled (false);    
		actionToggleHideExport->setEnabled (true);  
		actionToggleHideExport->setChecked (selti->hideInExport() );	

		actionToggleTarget->setEnabled (true);

		actionPaste->setEnabled (false); 
		actionDelete->setEnabled (true);

		actionGrowSelectionSize->setEnabled (true);
		actionShrinkSelectionSize->setEnabled (true);
		actionResetSelectionSize->setEnabled (true);
	    }	// Image
	    return;
	} // TreeItem 
	
	// Check (at least for some) multiple selection //FIXME-4
	QList <TreeItem*> selItems=m->getSelectedItems();
	if (selItems.count()>0 )
	{
	    actionDelete->setEnabled (true);
	    actionToggleHideExport->setEnabled (true);  
	    actionToggleHideExport->setChecked (false);	
	}

	QList <BranchItem*> selbis=m->getSelectedBranches();
	if (selbis.count()>0 )
	    actionFormatColorBranch->setEnabled (true);

	return;
    } 

    // No map available 
    for (int i=0; i<actionListMap.size(); ++i)	
	actionListMap.at(i)->setEnabled(false);

    // Disable standard flags toolbar
    standardFlagsMaster->setEnabled (false);
}

Main::ModMode Main::getModMode()
{
    if (actionModModeColor->isChecked()) return ModModeColor;
    if (actionModModeCopy->isChecked()) return ModModeCopy;
    if (actionModModeXLink->isChecked()) return ModModeXLink;
    return ModModeNone;
}

bool Main::autoEditNewBranch()
{
    return actionSettingsAutoEditNewBranch->isChecked();
}

bool Main::autoSelectNewBranch()
{
    return actionSettingsAutoSelectNewBranch->isChecked();
}

void Main::setScriptFile (const QString &fn)
{
    scriptEditor->setScriptFile (fn);
}

QVariant Main::execute (const QString &script)
{
    VymModel *m=currentModel();
    if (m) return m->execute (script);
    return QVariant();
}

void Main::executeEverywhere (const QString &script)
{
    foreach (VymView *vv,vymViews)
    {
	VymModel *m=vv->getModel();
	if (m) m->execute (script);
    }
}

void Main::gotoWindow (const int &n)
{
    if (n < tabWidget->count() && n>=0 )
	tabWidget->setCurrentIndex (n);
}

void Main::windowNextEditor()
{
    if (tabWidget->currentIndex() < tabWidget->count())
	tabWidget->setCurrentIndex (tabWidget->currentIndex() +1);
}

void Main::windowPreviousEditor()
{
    if (tabWidget->currentIndex() >0)
	tabWidget->setCurrentIndex (tabWidget->currentIndex() -1);
}

void Main::nextSlide()
{
    VymView *cv=currentView();
    if (cv) cv->nextSlide();
}

void Main::previousSlide()
{
    VymView *cv=currentView();
    if (cv) cv->previousSlide();
}

void Main::standardFlagChanged()
{
    if (currentModel())
    {
	if ( actionSettingsUseFlagGroups->isChecked() )
	    currentModel()->toggleStandardFlag(sender()->objectName(),standardFlagsMaster);
	else	
	    currentModel()->toggleStandardFlag(sender()->objectName());
	updateActions();    
    }
}

void Main::testFunction1()
{
    if (!currentMapEditor()) return;
    currentMapEditor()->testFunction1();
}

void Main::testFunction2()
{
    if (!currentMapEditor()) return;
    currentMapEditor()->testFunction2();
}

void Main::toggleWinter()
{
    if (!currentMapEditor()) return;
    currentMapEditor()->toggleWinter();
}

void Main::toggleHideExport()
{
    VymModel *m=currentModel();
    if (!m) return;
    if (actionToggleHideMode->isChecked() )
	m->setHideTmpMode (TreeItem::HideExport);
    else
	m->setHideTmpMode (TreeItem::HideNone);
}

void Main::testCommand()
{
    if (!currentMapEditor()) return;
    scriptEditor->show();
}

void Main::helpDoc()
{
    QString locale = QLocale::system().name();
    QString docname;
    if (locale.left(2)=="es")
	docname="vym_es.pdf";
    else    
	docname="vym.pdf";

    QStringList searchList;
    QDir docdir;
    #if defined(Q_OS_MACX)
	searchList << "./vym.app/Contents/Resources/doc";
    #elif defined(Q_OS_WIN32)
        searchList << vymInstallDir.path() + "/share/doc/packages/vym";
    #else
	#if defined(VYM_DOCDIR)
	    searchList << VYM_DOCDIR;
	#endif
	// default path in SUSE LINUX
	searchList << "/usr/share/doc/packages/vym";
    #endif

    searchList << "doc";    // relative path for easy testing in tarball
    searchList << "/usr/share/doc/vym";	// Debian
    searchList << "/usr/share/doc/packages";// Knoppix

    bool found=false;
    QFile docfile;
    for (int i=0; i<searchList.count(); ++i)
    {
	docfile.setFileName(searchList.at(i)+"/"+docname);
	if (docfile.exists())
	{
	    found=true;
	    break;
	}   
    }

    if (!found)
    {
	QMessageBox::critical(0, 
	    tr("Critcal error"),
	    tr("Couldn't find the documentation %1 in:\n%2").arg(searchList.join("\n")));
	return;
    }	

    QStringList args;
    Process *pdfProc = new Process();
    args << QDir::toNativeSeparators(docfile.fileName());

    if (!pdfProc->startDetached( settings.value("/mainwindow/readerPDF").toString(),args) )
    {
	// error handling
	QMessageBox::warning(0, 
	    tr("Warning"),
	    tr("Couldn't find a viewer to open %1.\n").arg(docfile.fileName())+
	    tr("Please use Settings->")+tr("Set application to open PDF files"));
	settingsPDF();	
	return;
    }
}


void Main::helpDemo()
{
    QStringList filters;
    filters <<"VYM example map (*.vym)";
    QFileDialog fd;
    #if defined(Q_OS_MACX)
	fd.setDirectory (QDir("./vym.app/Contents/Resources/demos"));
    #else
	// default path in SUSE LINUX
	fd.setDirectory (QDir(vymBaseDir.path()+"/demos"));
    #endif

    fd.setFileMode (QFileDialog::ExistingFiles);
    fd.setFilters (filters);
    fd.setWindowTitle (vymName+ " - " +tr("Load vym example map"));
    fd.setAcceptMode (QFileDialog::AcceptOpen);

    QString fn;
    if ( fd.exec() == QDialog::Accepted )
    {
	lastMapDir=fd.directory().path();
	QStringList flist = fd.selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    fn = *it;
	    fileLoad(*it, NewMap,VymMap);		   
	    ++it;
	}
    }
}

void Main::helpShortcuts()
{
    ShowTextDialog dia;
    dia.setText( switchboard.getASCII() );
    dia.exec();
}

void Main::helpAbout()
{
    AboutDialog ad;
    ad.setMinimumSize(500,500);
    ad.resize (QSize (500,500));
    ad.exec();
}

void Main::helpAboutQT()
{
    QMessageBox::aboutQt( this, "Qt Application Example" );
}

void Main::callMacro ()
{
    QAction *action = qobject_cast<QAction *>(sender());
    int i=-1;
    if (action)
    {
        i=action->data().toInt();
        QString s=macros.getMacro (i);
        if (!s.isEmpty())
	{
	    VymModel *m=currentModel();
	    if (m) m->execute(s);
	}   
    }	
}


