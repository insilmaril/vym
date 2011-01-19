#include "mainwindow.h"

#include <QtGui>

#include <iostream>
#include <typeinfo>

#include "aboutdialog.h"
#include "branchpropwindow.h"
#include "branchitem.h"
#include "exportoofiledialog.h"
#include "exports.h"
#include "file.h"
#include "findresultwidget.h"
#include "flagrow.h"
#include "headingeditor.h"
#include "historywindow.h"
#include "imports.h"
#include "mapeditor.h"
#include "misc.h"
#include "options.h"
#include "process.h"
#include "settings.h"
#include "shortcuts.h"
#include "noteeditor.h"
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

extern NoteEditor    *noteEditor;
extern HeadingEditor *headingEditor;
extern Main *mainWindow;
extern FindResultWidget *findResultWidget;
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
extern bool bugzillaClientAvailable;

QMenu* branchContextMenu;
QMenu* branchAddContextMenu;
QMenu* branchRemoveContextMenu;
QMenu* branchLinksContextMenu;
QMenu* branchXLinksContextMenuEdit;
QMenu* branchXLinksContextMenuFollow;
QMenu* floatimageContextMenu;
QMenu* canvasContextMenu;
QMenu* fileLastMapsMenu;
QMenu* fileImportMenu;
QMenu* fileExportMenu;


extern Settings settings;
extern Options options;
extern ImageIO imageIO;

extern QDir vymBaseDir;
extern QDir lastImageDir;
extern QDir lastFileDir;
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
    if (debug) qDebug ()<<QString("vym tmpDir=%1").arg(tmpVymDir) ;

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

    // Dock widgets ///////////////////////////////////////////////
    QDockWidget *dw;
    if (settings.value("/satellite/noteeditor/isDockWindow",true).toBool())
    {
	dw = new QDockWidget (tr("Note Editor"),this);
	dw->setWidget (noteEditor);
	dw->setObjectName ("NoteEditor");
	dw->hide();
	noteEditorDW=dw;
	addDockWidget (Qt::LeftDockWidgetArea,dw);
    } else
	noteEditorDW=NULL;

    dw = new QDockWidget (tr("Heading Editor"),this);
    dw->setWidget (headingEditor);
    dw->setObjectName ("HeadingEditor");
    dw->hide();
    headingEditorDW=dw;
    addDockWidget (Qt::BottomDockWidgetArea,dw);

    findResultWidget=new FindResultWidget ();
    dw= new QDockWidget (tr ("Search results list","FindResultWidget"),this);
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

    // Satellite windows //////////////////////////////////////////
    // history window
    historyWindow=new HistoryWindow();
    connect (historyWindow, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

    // properties window
    branchPropertyWindow = new BranchPropertyWindow();
    branchPropertyWindow->hide();
    connect (branchPropertyWindow, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

    // Connect NoteEditor, so that we can update flags if text changes
    connect (noteEditor, SIGNAL (textHasChanged() ), this, SLOT (updateNoteFlag()));
    connect (noteEditor, SIGNAL (windowClosed() ), this, SLOT (updateActions()));

    // Connect heading editor
    connect (headingEditor, SIGNAL (textHasChanged() ), this, SLOT (updateHeading()));

    // Initialize script editor
    scriptEditor = new SimpleScriptEditor();
    scriptEditor->move (50,50);

    connect( scriptEditor, SIGNAL( runScript ( QString ) ), 
	this, SLOT( runScript( QString ) ) );

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
    connect( tabWidget, SIGNAL( currentChanged( QWidget * ) ), 
	this, SLOT( editorChanged( QWidget * ) ) );

    layout->addWidget (tabWidget);

    setupFileActions();
    setupEditActions();
    setupFormatActions();
    setupViewActions();
    setupModeActions();
    setupFlagActions();
    setupNetworkActions();
    setupSettingsActions();
    setupContextMenus();
    setupMacros();
    if (options.isOn("shortcuts")) switchboard.print();
    if (options.isOn("shortcutsLaTeX")) switchboard.printLaTeX();

    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) setupTestActions();
    setupHelpActions();

    // Status bar and progress bar there
    statusBar();
    progressMax=0;
    progressCounter=0;
    progressCounterTotal=0;

    progressDialog.setLabelText (tr("Loading maps","Mainwindow"));
    progressDialog.setAutoReset(false);
    progressDialog.setAutoClose(false);
    //progressDialog.setWindowModality (Qt::WindowModal);   // That forces mainwindo to update and slows down
    //progressDialog.setCancelButton (NULL);

    restoreState (settings.value("/mainwindow/state",0).toByteArray());

    // Global Printer
    printer=new QPrinter (QPrinter::HighResolution );	

    updateGeometry();
}

Main::~Main()
{
    // Save Settings
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
    settings.setValue( "/mapeditor/editmode/useDelKey",actionSettingsUseDelKey->isChecked() );
    settings.setValue( "/mapeditor/editmode/useFlagGroups",actionSettingsUseFlagGroups->isChecked() );
    settings.setValue( "/export/useHideExport",actionSettingsUseHideExport->isChecked() );
    settings.setValue( "/satellite/noteeditor/isDockWindow",actionSettingsNoteEditorIsDockWindow->isChecked());

    //TODO save scriptEditor settings

    // call the destructors
    delete noteEditor;	    // FIXME-3 shouldn't this be done in main?
    delete historyWindow;
    delete branchPropertyWindow;

    // Remove temporary directory
    removeDir (QDir(tmpVymDir));
}

void Main::loadCmdLine()
{
    /* TODO draw some kind of splashscreen while loading...
    if (qApp->argc()>1)
    {
    }
    */

    QStringList flist=options.getFileList();
    QStringList::Iterator it=flist.begin();

    progressCounterTotal=flist.count();
    while (it !=flist.end() )
    {
	fileLoad (*it, NewMap);
	*it++;
    }	
    removeProgressCounter();
}


void Main::statusMessage(const QString &s)
{
    // Surpress messages while progressdialog during 
    // load is active
    //if (progressMax==0)   //FIXME-3 needed?
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
    //cout << "Main  max="<<max<<"  v="<<progressDialog.value()<<endl;
    if (!options.isOn("batch"))
	progressDialog.show();
    else	
	progressDialog.hide();
}

void Main::addProgressValue (float v) 

{
/*
    cout << "addVal v="<<v
	 <<"  cur="<<progressDialog.value()
	 <<"  pCounter="<<progressCounter
	 <<"  pCounterTotal="<<progressCounterTotal
	 <<endl;
	 */
    if (!options.isOn("batch"))
	progressDialog.show();
    else	
	progressDialog.hide();
    progressDialog.setValue ( (v + progressCounter -1)*1000/progressCounterTotal );
    progressDialog.repaint();
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

// File Actions
void Main::setupFileActions()
{
    QMenu *fileMenu = menuBar()->addMenu ( tr ("&Map","Toolbar for file actions") );
    QToolBar *tb = addToolBar( tr ("File actions toolbar","Toolbar for file actions"));
    tb->setObjectName ("fileTB");

    QAction *a;
    a = new QAction(QPixmap( iconPath+"filenew.png"), tr( "&New map","File menu" ),this);
    a->setStatusTip ( tr( "New map","Status tip File menu" ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_N );	    //New map
    switchboard.addConnection(a,tr("File","Shortcut group"));
    tb->addAction(a);
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNew() ) );

    a = new QAction(QPixmap( iconPath+"filenewcopy.png"), tr( "&Copy to new map","File menu" ),this);
    a->setStatusTip ( tr( "Copy selection to mapcenter of a new map","Status tip File menu" ) );
    a->setShortcut ( Qt::CTRL +Qt::SHIFT + Qt::Key_N );	    //New map
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileNewCopy() ) );
    actionFileNewCopy=a;

    a = new QAction( QPixmap( iconPath+"fileopen.png"), tr( "&Open..." ,"File menu"),this);
    a->setStatusTip (tr( "Open","Status tip File menu" ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_O );	    //Open map
    switchboard.addConnection(a,tr("File","Shortcut group"));
    tb->addAction(a);
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileLoad() ) );

    fileLastMapsMenu = fileMenu->addMenu (tr("Open Recent","File menu"));
    fileMenu->addSeparator();

    a = new QAction( QPixmap( iconPath+"filesave.png"), tr( "&Save...","File menu" ), this);
    a->setStatusTip ( tr( "Save","Status tip file menu" ));
    a->setShortcut (Qt::CTRL + Qt::Key_S );	    //Save map
    switchboard.addConnection(a,tr("File","Shortcut group"));
    tb->addAction(a);
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSave() ) );
    actionFileSave=a;

    a = new QAction( QPixmap(iconPath+"filesaveas.png"), tr( "Save &As...","File menu" ), this);
    a->setStatusTip (tr( "Save &As","Status tip file menu" ) );
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileSaveAs() ) );

    fileMenu->addSeparator();

    fileImportMenu = fileMenu->addMenu (tr("Import","File menu"));

    a = new QAction(tr("KDE 3 Bookmarks"), this);
    a->setStatusTip ( tr( "Import %1","Status tip file menu" ).arg(tr("KDE 3 bookmarks")));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileImportMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportKDE3Bookmarks() ) );

    a = new QAction(tr("KDE 4 Bookmarks"), this);
    a->setStatusTip ( tr( "Import %1","Status tip file menu" ).arg(tr("KDE 4 bookmarks")));
    fileImportMenu->addAction (a);
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportKDE4Bookmarks() ) );

    if (settings.value( "/mainwindow/showTestMenu",false).toBool()) 
    {
	a = new QAction( QPixmap(), tr("Firefox Bookmarks","File menu"),this);
	a->setStatusTip (tr( "Import %1","Status tip file menu").arg(tr("Firefox Bookmarks" ) ));
	fileImportMenu->addAction (a);
	switchboard.addConnection(a,tr("File","Shortcut group"));
	connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFirefoxBookmarks() ) );
    }	

    a = new QAction("Freemind...",this);
    a->setStatusTip ( tr( "Import %1","status tip file menu").arg(" Freemind")  );
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileImportMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportFreemind() ) );

    a = new QAction("Mind Manager...",this);
    a->setStatusTip ( tr( "Import %1","status tip file menu").arg(" Mind Manager")  );
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileImportMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportMM() ) );

    a = new QAction( tr( "Import Dir%1","File menu").arg("..."), this);
    a->setStatusTip (tr( "Import directory structure (experimental)","status tip file menu" ) );
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileImportMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileImportDir() ) );

    fileExportMenu = fileMenu->addMenu (tr("Export","File menu"));

    a = new QAction( tr("Image%1","File export menu").arg("..."), this);
    a->setStatusTip( tr( "Export map as image","status tip file menu" ));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportImage() ) );
    fileExportMenu->addAction (a);

    a = new QAction( tr("PDF%1","File export menu").arg("..."), this);
    a->setStatusTip( tr( "Export map as PDF","status tip file menu" ));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportPDF() ) );
    fileExportMenu->addAction (a);

    a = new QAction( tr("SVG%1","File export menu").arg("..."), this);
    a->setStatusTip( tr( "Export map as SVG","status tip file menu" ));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportSVG() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "Open Office...", this);
    a->setStatusTip( tr( "Export in Open Document Format used e.g. in Open Office ","status tip file menu" ));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportOOPresentation() ) );
    fileExportMenu->addAction (a);

    a = new QAction( QPixmap(iconPath+"file-document-export.png"),tr("Repeat last export (%1)").arg("-"), this);
    a->setShortcut (Qt::ALT + Qt::Key_E);	    
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportLast() ) );
    tb->addAction(a);
    actionFileExportLast=a;
    fileExportMenu->addAction (a);

    a = new QAction(  "Webpage (HTML)...",this );
    a->setStatusTip ( tr( "Export as %1","status tip file menu").arg(tr(" webpage (HTML)","status tip file menu")));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportHTML() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "Text (A&O report)...", this);
    a->setStatusTip ( tr( "Export as %1").arg("A&O report "+tr("(still experimental)" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportAO() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "Text (ASCII)...", this);
    a->setStatusTip ( tr( "Export as %1").arg("ASCII "+tr("(still experimental)" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportASCII() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "Spreadsheet (CSV)...", this);
    a->setStatusTip ( tr( "Export as %1").arg("CSV "+tr("(still experimental)" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportCSV() ) );
    fileExportMenu->addAction (a);

    a = new QAction( tr("KDE 3 Bookmarks","File menu"), this);
    a->setStatusTip( tr( "Export as %1").arg(tr("KDE 3 Bookmarks" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportKDE3Bookmarks() ) );
    fileExportMenu->addAction (a);

    a = new QAction( tr("KDE 4 Bookmarks","File menu"), this);
    a->setStatusTip( tr( "Export as %1").arg(tr("KDE 4 Bookmarks" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportKDE4Bookmarks() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "Taskjuggler...", this );
    a->setStatusTip( tr( "Export as %1").arg("Taskjuggler "+tr("(still experimental)" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportTaskjuggler() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "LaTeX...", this);
    a->setStatusTip( tr( "Export as %1").arg("LaTeX "+tr("(still experimental)" )));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportLaTeX() ) );
    fileExportMenu->addAction (a);

    a = new QAction( "XML..." , this );
    a->setStatusTip (tr( "Export as %1").arg("XML"));
    switchboard.addConnection(a,tr("File","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExportXML() ) );
    fileExportMenu->addAction (a);

    fileMenu->addSeparator();

    a = new QAction(QPixmap( iconPath+"fileprint.png"), tr( "&Print")+QString("..."), this);
    a->setStatusTip ( tr( "Print" ,"File menu") );
    a->setShortcut (Qt::CTRL + Qt::Key_P );	    //Print map
    tb->addAction(a);
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( filePrint() ) );
    actionFilePrint=a;

    a = new QAction( QPixmap(iconPath+"fileclose.png"), tr( "&Close Map","File menu" ), this);
    a->setStatusTip (tr( "Close Map" ) );
    a->setShortcut (Qt::CTRL + Qt::Key_W );	    //Close map
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileCloseMap() ) );

    a = new QAction(QPixmap(iconPath+"exit.png"), tr( "E&xit","File menu")+" "+vymName, this);
    a->setStatusTip ( tr( "Exit")+" "+vymName );
    a->setShortcut (Qt::CTRL + Qt::Key_Q );	    //Quit vym
    switchboard.addConnection(a,tr("File","Shortcut group"));
    fileMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( fileExitVYM() ) );
}


//Edit Actions
void Main::setupEditActions()
{
    QToolBar *tb = addToolBar( tr ("Edit actions toolbar","Toolbar name") );
    tb->setObjectName ("actionsTB");
    QMenu *editMenu = menuBar()->addMenu( tr("&Edit","Edit menu") );

    QAction *a;
    a = new QAction( QPixmap( iconPath+"undo.png"), tr( "&Undo","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUndo() ) );
    a->setStatusTip (tr( "Undo" ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_Z );	    //Undo last action
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    editMenu->addAction (a);
    actionUndo=a;

    a = new QAction( QPixmap( iconPath+"redo.png"), tr( "&Redo","Edit menu" ), this); 
    a->setStatusTip (tr( "Redo" ));
    a->setShortcut (Qt::CTRL + Qt::Key_Y );	    //Redo last action
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editRedo() ) );
    actionRedo=a;

    editMenu->addSeparator();
    a = new QAction(QPixmap( iconPath+"editcopy.png"), tr( "&Copy","Edit menu" ), this);
    a->setStatusTip ( tr( "Copy" ) );
    a->setShortcut (Qt::CTRL + Qt::Key_C );	    //Copy
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editCopy() ) );
    actionCopy=a;

    a = new QAction(QPixmap( iconPath+"editcut.png" ), tr( "Cu&t","Edit menu" ), this);
    a->setStatusTip ( tr( "Cut" ) );
    a->setShortcut (Qt::CTRL + Qt::Key_X );	    //Cut
    a->setEnabled (false);
    tb->addAction (a);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    actionCut=a;
    connect( a, SIGNAL( triggered() ), this, SLOT( editCut() ) );

    a = new QAction(QPixmap( iconPath+"editpaste.png"), tr( "&Paste","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editPaste() ) );
    a->setStatusTip ( tr( "Paste" ) );
    a->setShortcut ( Qt::CTRL + Qt::Key_V );	    //Paste
    a->setEnabled (false);
    tb->addAction (a);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    actionPaste=a;

    // Shortcut to delete selection
    a = new QAction( tr( "Delete Selection","Edit menu" ),this);
    a->setStatusTip (tr( "Delete Selection" ));
    a->setShortcut ( Qt::Key_Delete);		    //Delete selection
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteSelection() ) );
    actionDelete=a;

    // Shortcut to add attribute
    a= new QAction(tr( "Add attribute" ), this);
    if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
    {
	a->setShortcut ( Qt::Key_Q);	
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
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddMapCenter() ) );
    //actionListBranches.append(a);
    tb->addAction (a);
    actionAddMapCenter = a;


    // Shortcut to add branch
    a = new QAction(QPixmap(iconPath+"newbranch.png"), tr( "Add branch as child","Edit menu" ), this);
    a->setStatusTip ( tr( "Add a branch as child of selection" ));
    a->setShortcut (Qt::Key_A);			//Add branch
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranch() ) );
    actionListBranches.append(a);
    actionAddBranch=a;
    editMenu->addAction (actionAddBranch);
    tb->addAction (actionAddBranch);


    // Add branch by inserting it at selection
    a = new QAction(tr( "Add branch (insert)","Edit menu" ),this);
    a->setStatusTip ( tr( "Add a branch by inserting and making selection its child" ));
    a->setShortcut ( Qt::ALT + Qt::Key_A );	    //Insert branch
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBefore() ) );
    actionListBranches.append(a);
    actionAddBranchBefore=a;

    // Add branch above
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
    a->setStatusTip ( tr( "Add a branch above selection" ));
    a->setShortcut (Qt::SHIFT+Qt::Key_Insert );	    //Add branch above
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionAddBranchAbove=a;
    a = new QAction(tr( "Add branch above","Edit menu" ), this);
    a->setStatusTip ( tr( "Add a branch above selection" ));
    a->setShortcut (Qt::SHIFT+Qt::Key_A );	    //Add branch above
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchAbove() ) );
    actionListBranches.append(a);

    // Add branch below 
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
    a->setStatusTip ( tr( "Add a branch below selection" ));
    a->setShortcut (Qt::CTRL +Qt::Key_Insert );	    //Add branch below
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionAddBranchBelow=a;
    a = new QAction(tr( "Add branch below","Edit menu" ), this);
    a->setStatusTip ( tr( "Add a branch below selection" ));
    a->setShortcut (Qt::CTRL +Qt::Key_A );	    // Add branch below
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editNewBranchBelow() ) );
    actionListBranches.append(a);

    a = new QAction(QPixmap(iconPath+"up.png" ), tr( "Move up","Edit menu" ), this);
    a->setStatusTip ( tr( "Move branch up" ) );
    a->setShortcut (Qt::Key_PageUp );		    // Move branch up	
    a->setShortcutContext (Qt::WidgetShortcut);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveUp() ) );
    actionMoveUp=a;

    a = new QAction( QPixmap( iconPath+"down.png"), tr( "Move down","Edit menu" ),this);
    connect( a, SIGNAL( triggered() ), this, SLOT( editMoveDown() ) );
    a->setStatusTip (tr( "Move branch down" ) );
    a->setShortcut ( Qt::Key_PageDown );	    // Move branch down
    a->setShortcutContext (Qt::WidgetShortcut);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    editMenu->addAction (a);
    actionMoveDown=a;

    a = new QAction(QPixmap(), tr( "&Detach","Context menu" ),this);
    a->setStatusTip ( tr( "Detach branch and use as mapcenter","Context menu" ) );
    a->setShortcut ( Qt::Key_D );		    // Detach branch
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editDetach() ) );
    actionDetach=a;

    a = new QAction( QPixmap(iconPath+"editsort.png" ), tr( "Sort children","Edit menu" ), this );
    connect( a, SIGNAL( activated() ), this, SLOT( editSortChildren() ) );
    a->setEnabled (true);
    a->setShortcut ( Qt::Key_O );		    // Detach branch
    tb->addAction(a);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    actionSortChildren=a;

    a = new QAction( QPixmap(iconPath+"editsortback.png" ), tr( "Sort children backwards","Edit menu" ), this );
    connect( a, SIGNAL( activated() ), this, SLOT( editSortBackChildren() ) );
    a->setEnabled (true);
    a->setShortcut ( Qt::SHIFT + Qt::Key_O );		    // Detach branch
    tb->addAction(a);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    actionSortBackChildren=a;	

    a = new QAction( QPixmap(flagsPath+"flag-scrolled-right.png"), tr( "Scroll branch","Edit menu" ), this);
    a->setShortcut ( Qt::Key_S );		    // Scroll branch
    a->setStatusTip (tr( "Scroll branch" )); 
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleScroll() ) );
    a->setEnabled (false);
    a->setCheckable(true);
    actionToggleScroll=a;
    tb->addAction (actionToggleScroll);
    editMenu->addAction ( actionToggleScroll);
    editMenu->addAction (actionToggleScroll);
    addAction (a);
    actionListBranches.append(actionToggleScroll);

    a = new QAction( QPixmap(), tr( "Expand all branches","Edit menu" ), this);
    a->setShortcut ( Qt::SHIFT + Qt::Key_X );	    // Expand all branches 
    a->setStatusTip (tr( "Expand all branches" )); 
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editExpandAll() ) );
    actionExpandAll=a;
    actionExpandAll->setEnabled (false);
    actionExpandAll->setCheckable(false);
    //tb->addAction (actionExpandAll);
    editMenu->addAction ( actionExpandAll);
    addAction (a);
    actionListBranches.append(actionExpandAll);

    a = new QAction( QPixmap(), tr( "Expand one level","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Greater );	    // Expand one level in tree editor
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    a->setStatusTip (tr( "Expand one level in tree editor" )); 
    connect( a, SIGNAL( triggered() ), this, SLOT( editExpandOneLevel() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    actionExpandOneLevel=a;
    //tb->addAction (a);
    editMenu->addAction ( a);
    addAction (a);
    actionListBranches.append(a);

    a = new QAction( QPixmap(), tr( "Collapse one level","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Less + Qt::CTRL);		// Collapse one level in tree editor
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    a->setStatusTip (tr( "Collapse one level in tree editor" )); 
    connect( a, SIGNAL( triggered() ), this, SLOT( editCollapseOneLevel() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    actionCollapseOneLevel=a;
    //tb->addAction (a);
    editMenu->addAction ( a);
    addAction (a);
    actionListBranches.append(a);

    a = new QAction( QPixmap(), tr( "Collapse unselected levels","Edit menu" ), this);
    a->setShortcut ( Qt::Key_Less);	    // Collapse all branches with greater depth than current selection
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    a->setStatusTip (tr( "Collapse unselected levels in tree editor" )); 
    connect( a, SIGNAL( triggered() ), this, SLOT( editCollapseUnselected() ) );
    a->setEnabled (false);
    a->setCheckable(false);
    actionCollapseUnselected=a;
    //tb->addAction (a);
    editMenu->addAction ( a);
    addAction (a);
    actionListBranches.append(a);

    a = new QAction( tr( "Unscroll children","Edit menu" ), this);
    a->setStatusTip (tr( "Unscroll all scrolled branches in selected subtree" ));
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editUnscrollChildren() ) );

    a = new QAction( tr( "Grow selection","Edit menu" ), this);
    a->setStatusTip (tr( "Grow selection, e.g. make image larger" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_Plus);	    // Grow selection
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editGrowSelectionSize() ) );

    a = new QAction( tr( "Shrink selection","Edit menu" ), this);
    a->setStatusTip (tr( "Shrink selection, e.g. make image smaller" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_Minus);	    // Shrink selection
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editShrinkSelectionSize() ) );

    a = new QAction( tr( "Reset selection size","Edit menu" ), this);
    a->setStatusTip (tr( "Reset selection size to original" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_0);	    // Shrink selection
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editResetSelectionSize() ) );

    editMenu->addSeparator();

    a = new QAction( QPixmap(iconPath+"find.png"), tr( "Find...","Edit menu"), this);
    a->setStatusTip (tr( "Find" ) );
    a->setShortcut (Qt::CTRL + Qt::Key_F );		//Find
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenFindResultWidget() ) );

    a = new QAction( tr( "Find duplicate URLs","Edit menu"), this);
    //a->setStatusTip (tr( "Find" ) );
    a->setShortcut (Qt::SHIFT + Qt::Key_F);		//Find duplicate URLs
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    if (settings.value( "/mainwindow/showTestMenu",false).toBool() ) 
	editMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editFindDuplicateURLs() ) );

    editMenu->addSeparator();

    a = new QAction( QPixmap(flagsPath+"flag-url.png"), tr( "Open URL","Edit menu" ), this);
    a->setShortcut (Qt::SHIFT + Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    tb->addAction (a);
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURL() ) );
    actionOpenURL=a;

    a = new QAction( tr( "Open URL in new tab","Edit menu" ), this);
    a->setStatusTip (tr( "Open URL in new tab" ));
    //a->setShortcut (Qt::CTRL+Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenURLTab() ) );
    actionOpenURLTab=a;

    a = new QAction( tr( "Open all URLs in subtree (including scrolled branches)","Edit menu" ), this);
    a->setStatusTip (tr( "Open all URLs in subtree (including scrolled branches)" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_U );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleVisURLTabs() ) );
    actionOpenMultipleVisURLTabs=a;

    a = new QAction( tr( "Open all URLs in subtree","Edit menu" ), this);
    a->setStatusTip (tr( "Open all URLs in subtree" ));
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleURLTabs() ) );
    actionOpenMultipleURLTabs=a;

    a = new QAction(QPixmap(), tr( "Edit URL...","Edit menu"), this);
    a->setStatusTip ( tr( "Edit URL" ) );
    a->setShortcut ( Qt::Key_U );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editURL() ) );
    actionURL=a;

    a = new QAction(QPixmap(), tr( "Edit local URL...","Edit menu"), this);
    a->setStatusTip ( tr( "Edit local URL" ) );
    //a->setShortcut (Qt::SHIFT +  Qt::Key_U );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLocalURL() ) );
    actionLocalURL=a;

    a = new QAction( tr( "Use heading for URL","Edit menu" ), this);
    a->setStatusTip ( tr( "Use heading of selected branch as URL" ));
    a->setShortcut ( Qt::ALT + Qt::Key_U );
    a->setShortcutContext (Qt::ApplicationShortcut);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading2URL() ) );
    actionHeading2URL=a;

    a = new QAction(tr( "Create URL to Novell Bugzilla","Edit menu" ), this);
    a->setStatusTip ( tr( "Create URL to Novell Bugzilla" ));
    a->setEnabled (false);
    actionListBranches.append(a);
    a->setShortcut ( Qt::Key_B );
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editBugzilla2URL() ) );
    actionBugzilla2URL=a;

    a = new QAction(tr( "Get data from Novell Bugzilla","Edit menu" ), this);
    a->setStatusTip ( tr( "Get data from Novell Bugzilla" ));
    a->setShortcut ( Qt::Key_B + Qt::SHIFT);
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    a->setEnabled (bugzillaClientAvailable);
    connect( a, SIGNAL( triggered() ), this, SLOT( getBugzillaData() ) );
    actionGetBugzillaData=a;

    a = new QAction(tr( "Get data from Novell Bugzilla for subtree","Edit menu" ), this);
    a->setStatusTip ( tr( "Update all Novell bugs in subtree" ));
    a->setShortcut ( Qt::Key_B + Qt::CTRL);
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    a->setEnabled (bugzillaClientAvailable);
    connect( a, SIGNAL( triggered() ), this, SLOT( getBugzillaDataSubtree() ) );
    actionGetBugzillaDataSubtree=a;

    a = new QAction(tr( "Create URL to Novell FATE","Edit menu" ), this);
    a->setStatusTip ( tr( "Create URL to Novell FATE" ));
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editFATE2URL() ) );
    actionFATE2URL=a;

    a = new QAction(QPixmap(flagsPath+"flag-vymlink.png"), tr( "Open linked map","Edit menu" ), this);
    a->setStatusTip ( tr( "Jump to another vym map, if needed load it first" ));
    tb->addAction (a);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenVymLink() ) );
    actionOpenVymLink=a;

    a = new QAction(QPixmap(), tr( "Open all vym links in subtree","Edit menu" ), this);
    a->setStatusTip ( tr( "Open all vym links in subtree" ));
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editOpenMultipleVymLinks() ) );
    actionOpenMultipleVymLinks=a;


    a = new QAction(tr( "Edit vym link...","Edit menu" ), this);
    a->setEnabled (false);
    a->setStatusTip ( tr( "Edit link to another vym map" ));
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editVymLink() ) );
    actionListBranches.append(a);
    actionVymLink=a;

    a = new QAction(tr( "Delete vym link","Edit menu" ),this);
    a->setStatusTip ( tr( "Delete link to another vym map" ));
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteVymLink() ) );
    actionDeleteVymLink=a;

    a = new QAction(QPixmap(flagsPath+"flag-hideexport.png"), tr( "Hide in exports","Edit menu" ), this);
    a->setStatusTip ( tr( "Hide object in exports" ) );
    a->setShortcut (Qt::Key_H );
    a->setCheckable(true);
    tb->addAction (a);
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editToggleHideExport() ) );
    actionToggleHideExport=a;

    a = new QAction(tr( "Add timestamp","Edit menu" ), this);
    a->setStatusTip ( tr( "Add timestamp" ));
    a->setEnabled (false);
    actionListBranches.append(a);
    a->setShortcut ( Qt::Key_T );   // Add timestamp
    a->setShortcutContext (Qt::WindowShortcut);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction(a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editAddTimestamp() ) );
    actionAddTimestamp=a;

    a = new QAction(tr( "Edit Map Info...","Edit menu" ),this);
    a->setStatusTip ( tr( "Edit Map Info" ));
    a->setEnabled (true);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editMapInfo() ) );
    actionMapInfo=a;

    // Import at selection (adding to selection)
    a = new QAction( tr( "Add map (insert)","Edit menu" ),this);
    a->setStatusTip (tr( "Add map at selection" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportAdd() ) );
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    actionImportAdd=a;

    // Import at selection (replacing selection)
    a = new QAction( tr( "Add map (replace)","Edit menu" ), this);
    a->setStatusTip (tr( "Replace selection with map" ));
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editImportReplace() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionImportReplace=a;

    // Save selection 
    a = new QAction( tr( "Save selection","Edit menu" ), this);
    a->setStatusTip (tr( "Save selection" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( editSaveBranch() ) );
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    actionListBranches.append(a);
    actionSaveBranch=a;

    // Only remove branch, not its children
    a = new QAction(tr( "Remove only branch ","Edit menu" ), this);
    a->setStatusTip ( tr( "Remove only branch and keep its children" ));
    a->setShortcut (Qt::ALT + Qt::Key_X );
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteKeepChildren() ) );
    a->setEnabled (false);
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    actionListBranches.append(a);
    actionDeleteKeepChildren=a;

    // Only remove children of a branch
    a = new QAction( tr( "Remove children","Edit menu" ), this);
    a->setStatusTip (tr( "Remove children of branch" ));
    a->setShortcut (Qt::SHIFT + Qt::Key_Delete );
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( editDeleteChildren() ) );
    a->setEnabled (false);
    actionListBranches.append(a);
    actionDeleteChildren=a;

    a = new QAction( tr( "Add Image...","Edit menu" ), this);
    a->setStatusTip (tr( "Add Image" ));
    a->setShortcutContext (Qt::WindowShortcut);
    a->setShortcut (Qt::Key_I );    //FIXME-2 not working???
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editLoadImage() ) );
    actionLoadImage=a;

    a = new QAction( tr( "Property window","Dialog to edit properties of selection" )+QString ("..."), this);
    a->setStatusTip (tr( "Toggle property window to set properties for selection" ));
    a->setShortcut ( Qt::Key_P );	    //Property window
    a->setShortcutContext (Qt::WindowShortcut);
    a->setCheckable (true);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( windowToggleProperty() ) );
    actionViewTogglePropertyWindow=a;
}

// Format Actions
void Main::setupFormatActions()
{
    QMenu *formatMenu = menuBar()->addMenu (tr ("F&ormat","Format menu"));

    QToolBar *tb = addToolBar( tr("Format toolbar","Format Toolbar name"));
    tb->setObjectName ("formatTB");
    QAction *a;
    QPixmap pix( 16,16);
    pix.fill (Qt::black);
    a= new QAction(pix, tr( "Set &Color" )+QString("..."), this);
    a->setStatusTip ( tr( "Set Color" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectColor() ) );
    tb->addAction(a);
    formatMenu->addAction (a);
    actionFormatColor=a;
    a= new QAction( QPixmap(iconPath+"formatcolorpicker.png"), tr( "Pic&k color","Edit menu" ), this);
    a->setStatusTip (tr( "Pick color\nHint: You can pick a color from another branch and color using CTRL+Left Button" ) );
    a->setShortcut (Qt::CTRL + Qt::Key_K );
    connect( a, SIGNAL( triggered() ), this, SLOT( formatPickColor() ) );
    a->setEnabled (false);
    tb->addAction(a);
    formatMenu->addAction (a);
    actionListBranches.append(a);
    actionFormatPickColor=a;

    a= new QAction(QPixmap(iconPath+"formatcolorbranch.png"), tr( "Color &branch","Edit menu" ), this);
    a->setStatusTip ( tr( "Color branch" ) );
    //a->setShortcut (Qt::CTRL + Qt::Key_B + Qt::SHIFT);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorBranch() ) );
    a->setEnabled (false);
    tb->addAction(a);
    formatMenu->addAction (a);
    actionListBranches.append(a);
    actionFormatColorSubtree=a;

    a= new QAction(QPixmap(iconPath+"formatcolorsubtree.png"), tr( "Color sub&tree","Edit menu" ), this);
    a->setStatusTip ( tr( "Color Subtree" ));
    //a->setShortcut (Qt::CTRL + Qt::Key_B);	// Color subtree
    connect( a, SIGNAL( triggered() ), this, SLOT( formatColorSubtree() ) );
    a->setEnabled (false);
    formatMenu->addAction (a);
    tb->addAction(a);
    actionListBranches.append(a);
    actionFormatColorSubtree=a;

    formatMenu->addSeparator();
    actionGroupFormatLinkStyles=new QActionGroup ( this);
    actionGroupFormatLinkStyles->setExclusive (true);
    a= new QAction( tr( "Linkstyle Line" ), actionGroupFormatLinkStyles);
    a->setStatusTip (tr( "Line" ));
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleLine() ) );
    formatMenu->addAction (a);
    actionFormatLinkStyleLine=a;
    a= new QAction( tr( "Linkstyle Curve" ), actionGroupFormatLinkStyles);
    a->setStatusTip (tr( "Line" ));
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStyleParabel() ) );
    formatMenu->addAction (a);
    actionFormatLinkStyleParabel=a;
    a= new QAction( tr( "Linkstyle Thick Line" ), actionGroupFormatLinkStyles );
    a->setStatusTip (tr( "PolyLine" ));
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyLine() ) );
    formatMenu->addAction (a);
    actionFormatLinkStylePolyLine=a;
    a= new QAction( tr( "Linkstyle Thick Curve" ), actionGroupFormatLinkStyles);
    a->setStatusTip (tr( "PolyParabel" ) );
    a->setCheckable(true);
    a->setChecked (true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatLinkStylePolyParabel() ) );
    formatMenu->addAction (a);
    actionFormatLinkStylePolyParabel=a;

    a = new QAction( tr( "Hide link if object is not selected","Branch attribute" ), this);
    a->setStatusTip (tr( "Hide link" ));
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatHideLinkUnselected() ) );
    actionFormatHideLinkUnselected=a;

    formatMenu->addSeparator();
    a= new QAction( tr( "&Use color of heading for link","Branch attribute" ),  this);
    a->setStatusTip (tr( "Use same color for links and headings" ));
    a->setCheckable(true);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatToggleLinkColorHint() ) );
    formatMenu->addAction (a);
    actionFormatLinkColorHint=a;

    pix.fill (Qt::white);
    a= new QAction( pix, tr( "Set &Link Color")+"..." , this  );
    a->setStatusTip (tr( "Set Link Color" ));
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectLinkColor() ) );
    actionFormatLinkColor=a;

    a= new QAction( pix, tr( "Set &Selection Color")+"...", this  );
    a->setStatusTip (tr( "Set Selection Color" ));
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectSelectionColor() ) );
    actionFormatSelectionColor=a;

    a= new QAction( pix, tr( "Set &Background Color" )+"...", this );
    a->setStatusTip (tr( "Set Background Color" ));
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackColor() ) );
    actionFormatBackColor=a;

    a= new QAction( pix, tr( "Set &Background image" )+"...", this );
    a->setStatusTip (tr( "Set Background image" ));
    formatMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( formatSelectBackImage() ) );
    actionFormatBackImage=a;
}

// View Actions
void Main::setupViewActions()
{
    QToolBar *tb = addToolBar( tr("Zoom toolbar","View Toolbar name") );
    tb->setObjectName ("viewTB");
    QMenu *viewMenu = menuBar()->addMenu ( tr( "&View" ));


    QAction *a;
    a = new QAction( QPixmap(iconPath+"viewmag+.png"), tr( "Zoom in","View action" ), this);
    a->setStatusTip (tr( "Zoom in" ));
    a->setShortcut(Qt::Key_Plus);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomIn() ) );

    a = new QAction( QPixmap(iconPath+"viewmag-.png"), tr( "Zoom out","View action" ), this);
    a->setStatusTip (tr( "Zoom out" ));
    a->setShortcut(Qt::Key_Minus);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewZoomOut() ) );

    a = new QAction(QPixmap(iconPath+"viewmag-reset.png"), tr( "reset Zoom","View action" ), this);
    a->setStatusTip ( tr( "Zoom reset" ) );
    a->setShortcut (Qt::Key_Comma); // Reset zoom
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(viewZoomReset() ) );

    a = new QAction( QPixmap(iconPath+"viewshowsel.png"), tr( "Center on selection","View action" ), this);
    a->setStatusTip (tr( "Show selection" ));
    a->setShortcut(Qt::Key_Period);
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( viewCenter() ) );

    viewMenu->addSeparator();	

    tb = addToolBar( tr("Editors toolbar","Editor Toolbar name") );
    //tb->setLabel( "Editor Toolbar" );
    tb->setObjectName ("editorTB");

    if (noteEditorDW)
	a=noteEditorDW->toggleViewAction();
    else    
	a = new QAction(QPixmap(), tr( "Show Note Editor","View action" ),this);
    a->setStatusTip ( tr( "Show Note Editor" ));
    a->setShortcut ( Qt::Key_N );   // Toggle Note Editor
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    tb->addAction(a);
    a->setIcon (QPixmap(flagsPath+"flag-note.png"));
    viewMenu->addAction (a);
    actionViewToggleNoteEditor=a;

    if (!noteEditorDW)
	connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleNoteEditor() ) );


    a=headingEditorDW->toggleViewAction();
    a->setStatusTip ( tr( "Toggle Heading editor" ));
    a->setShortcut ( Qt::Key_E );   // Toggle Heading Editor
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    tb->addAction(a);
    a->setIcon (QPixmap(iconPath+"headingeditor.png"));
    viewMenu->addAction (a);
    actionViewToggleHeadingEditor=a;

    // Original icon is "category" from KDE
    a = new QAction(QPixmap(iconPath+"treeeditor.png"), tr( "Toggle Tree editor","View action" ),this);
    a->setStatusTip ( tr( "Toggle tree editor" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_T );	// Toggle Tree Editor 
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleTreeEditor() ) );
    actionViewToggleTreeEditor=a;

    a = new QAction(QPixmap(iconPath+"history.png"),  tr( "History Window","View action" ),this );
    a->setStatusTip ( tr( "Toggle History Window" ));
    a->setShortcut ( Qt::CTRL + Qt::Key_H  );	// Toggle history window
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    a->setCheckable(true);
    tb->addAction(a);
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowToggleHistory() ) );
    actionViewToggleHistoryWindow=a;

    viewMenu->addAction (actionViewTogglePropertyWindow);

    viewMenu->addSeparator();	

    a = new QAction(tr( "Antialiasing","View action" ),this );
    a->setStatusTip ( tr( "Antialiasing" ));
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
    a->setShortcut (Qt::ALT + Qt::Key_N );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowNextEditor() ) );

    a = new QAction (tr( "Previous Map","View action" ), this );
    a->setStatusTip (a->text());
    a->setShortcut (Qt::ALT + Qt::Key_P );
    switchboard.addConnection(a,tr("View shortcuts","Shortcut group"));
    viewMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT(windowPreviousEditor() ) );
}

// Mode Actions
void Main::setupModeActions()
{
    //QPopupMenu *menu = new QPopupMenu( this );
    //menuBar()->insertItem( tr( "&Mode (using modifiers)" ), menu );

    QToolBar *tb = addToolBar( tr ("Modifier modes toolbar","Modifier Toolbar name") );
    tb->setObjectName ("modesTB");
    QAction *a;
    actionGroupModModes=new QActionGroup ( this);
    actionGroupModModes->setExclusive (true);
    a= new QAction( QPixmap(iconPath+"modecolor.png"), tr( "Use modifier to color branches","Mode modifier" ), actionGroupModModes);
    a->setShortcut (Qt::Key_J);
    switchboard.addConnection(a,tr("Modes","Shortcut group"));
    a->setStatusTip ( tr( "Use modifier to color branches" ));
    a->setCheckable(true);
    tb->addAction(a);
    a->setChecked(true);
    actionModModeColor=a;

    a= new QAction( QPixmap(iconPath+"modecopy.png"), tr( "Use modifier to copy","Mode modifier" ), actionGroupModModes );
    a->setShortcut( Qt::Key_K); 
    switchboard.addConnection(a,tr("Modes","Shortcut group"));
    a->setStatusTip( tr( "Use modifier to copy" ));
    a->setCheckable(true);
    tb->addAction(a);
    actionModModeCopy=a;

    a= new QAction(QPixmap(iconPath+"modelink.png"), tr( "Use modifier to draw xLinks","Mode modifier" ), actionGroupModModes );
    a->setShortcut (Qt::Key_L);
    switchboard.addConnection(a,tr("Modes","Shortcut group"));
    a->setStatusTip( tr( "Use modifier to draw xLinks" ));
    a->setCheckable(true);
    tb->addAction(a);
    actionModModeXLink=a;
}

// Flag Actions
void Main::setupFlagActions()
{
    // Create System Flags
    QToolBar *tb=NULL;

    Flag *flag=new Flag(flagsPath+"flag-note.png");
    setupFlag (flag,tb,"system-note",tr("Note","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url.png");
    setupFlag (flag,tb,"system-url",tr("URL to Document ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url-bugzilla-novell.png");
    setupFlag (flag,tb,"system-url-bugzilla-novell",tr("URL to Bugzilla ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-url-bugzilla-novell-closed.png");
    setupFlag (flag,tb,"system-url-bugzilla-novell-closed",tr("URL to Bugzilla ","SystemFlag"));

    flag=new Flag(flagsPath+"flag-vymlink.png");
    setupFlag (flag,tb,"system-vymLink",tr("Link to another vym map","SystemFlag"));

    flag=new Flag(flagsPath+"flag-scrolled-right.png");
    setupFlag (flag,tb,"system-scrolledright",tr("subtree is scrolled","SystemFlag"));

    flag=new Flag(flagsPath+"flag-tmpUnscrolled-right.png");
    setupFlag (flag,tb,"system-tmpUnscrolledRight",tr("subtree is temporary scrolled","SystemFlag"));

    flag=new Flag(flagsPath+"flag-hideexport.png");
    setupFlag (flag,tb,"system-hideInExport",tr("Hide object in exported maps","SystemFlag"));

    addToolBarBreak();

    // Create Standard Flags
    tb=addToolBar (tr ("Standard Flags toolbar","Standard Flag Toolbar"));
    tb->setObjectName ("standardFlagTB");
    standardFlagsMaster->setToolBar (tb);

    flag=new Flag(flagsPath+"flag-exclamationmark.png");
    flag->setGroup("standard-mark");
    setupFlag (flag,tb,"exclamationmark",tr("Take care!","Standardflag"));

    flag=new Flag(flagsPath+"flag-questionmark.png");
    flag->setGroup("standard-mark");
    setupFlag (flag,tb,"questionmark",tr("Really?","Standardflag"));

    flag=new Flag(flagsPath+"flag-hook-green.png");
    flag->setGroup("standard-status");
    setupFlag (flag,tb,"hook-green",tr("Status - ok,done","Standardflag"));

    flag=new Flag(flagsPath+"flag-wip.png");
    flag->setGroup("standard-status");
    setupFlag (flag,tb,"wip",tr("Status - work in progress","Standardflag"));

    flag=new Flag(flagsPath+"flag-cross-red.png");
    flag->setGroup("standard-status");
    setupFlag (flag,tb,"cross-red",tr("Status - missing, not started","Standardflag"));
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-stopsign.png");
    setupFlag (flag,tb,"stopsign",tr("This won't work!","Standardflag"));

    flag=new Flag(flagsPath+"flag-smiley-good.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,tb,"smiley-good",tr("Good","Standardflag"));

    flag=new Flag(flagsPath+"flag-smiley-sad.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,tb,"smiley-sad",tr("Bad","Standardflag"));

    flag=new Flag(flagsPath+"flag-smiley-omg.png");
    flag->setGroup("standard-smiley");
    setupFlag (flag,tb,"smiley-omb",tr("Oh no!","Standardflag"));
    // Original omg.png (in KDE emoticons)
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-kalarm.png");
    setupFlag (flag,tb,"clock",tr("Time critical","Standardflag"));

    flag=new Flag(flagsPath+"flag-phone.png");
    setupFlag (flag,tb,"phone",tr("Call...","Standardflag"));

    flag=new Flag(flagsPath+"flag-lamp.png");
    setupFlag (flag,tb,"lamp",tr("Idea!","Standardflag"));

    flag=new Flag(flagsPath+"flag-arrow-up.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,tb,"arrow-up",tr("Important","Standardflag"));

    flag=new Flag(flagsPath+"flag-arrow-down.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,tb,"arrow-down",tr("Unimportant","Standardflag"));

    flag=new Flag(flagsPath+"flag-arrow-2up.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,tb,"2arrow-up",tr("Very important!","Standardflag"));

    flag=new Flag(flagsPath+"flag-arrow-2down.png");
    flag->setGroup("standard-arrow");
    setupFlag (flag,tb,"2arrow-down",tr("Very unimportant!","Standardflag"));
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-thumb-up.png");
    flag->setGroup("standard-thumb");
    setupFlag (flag,tb,"thumb-up",tr("I like this","Standardflag"));

    flag=new Flag(flagsPath+"flag-thumb-down.png");
    flag->setGroup("standard-thumb");
    setupFlag (flag,tb,"thumb-down",tr("I do not like this","Standardflag"));
    flag->unsetGroup();

    flag=new Flag(flagsPath+"flag-rose.png");
    setupFlag (flag,tb,"rose",tr("Rose","Standardflag"));

    flag=new Flag(flagsPath+"flag-heart.png");
    setupFlag (flag,tb,"heart",tr("I just love...","Standardflag"));

    flag=new Flag(flagsPath+"flag-present.png");
    setupFlag (flag,tb,"present",tr("Surprise!","Standardflag"));

    flag=new Flag(flagsPath+"flag-flash.png");
    setupFlag (flag,tb,"flash",tr("Dangerous","Standardflag"));

    // Original: xsldbg_output.png
    flag=new Flag(flagsPath+"flag-info.png");
    setupFlag (flag,tb,"info",tr("Info","Standardflag"));

    // Original khelpcenter.png
    flag=new Flag(flagsPath+"flag-lifebelt.png");
    setupFlag (flag,tb,"lifebelt",tr("This will help","Standardflag"));

    // Freemind flags
    flag=new Flag(flagsPath+"freemind/warning.png");
    flag->setVisible(false);
    setupFlag (flag,tb,  "freemind-warning",tr("Important","Freemind-Flag"));

    for (int i=1; i<8; i++)
    {
	flag=new Flag(flagsPath+QString("freemind/priority-%1.png").arg(i));
	flag->setVisible(false);
	flag->setGroup ("Freemind-priority");
	setupFlag (flag,tb, QString("freemind-priority-%1").arg(i),tr("Priority","Freemind-Flag"));
    }

    flag=new Flag(flagsPath+"freemind/back.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-back",tr("Back","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/forward.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-forward",tr("forward","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/attach.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-attach",tr("Look here","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/clanbomber.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-clanbomber",tr("Dangerous","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/desktopnew.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-desktopnew",tr("Don't flagrget","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/flag.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-flag",tr("Flag","Freemind-Flag"));


    flag=new Flag(flagsPath+"freemind/gohome.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-gohome",tr("Home","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/kaddressbook.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-kaddressbook",tr("Telephone","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/knotify.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-knotify",tr("Music","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/korn.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-korn",tr("Mailbox","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/mail.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-mail",tr("Maix","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/password.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-password",tr("Password","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/pencil.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-pencil",tr("To be improved","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/stop.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-stop",tr("Stop","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/wizard.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-wizard",tr("Magic","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/xmag.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-xmag",tr("To be discussed","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/bell.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-bell",tr("Reminder","Freemind-Flag"));

    flag=new Flag(flagsPath+"freemind/bookmark.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-bookmark",tr("Excellent","Freemind-Flag"));

    flag= new Flag(flagsPath+"freemind/penguin.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-penguin",tr("Linux","Freemind-Flag"));

    flag=new Flag (flagsPath+"freemind/licq.png");
    flag->setVisible(false);
    setupFlag (flag,tb,"freemind-licq",tr("Sweet","Freemind-Flag"));
}

void Main::setupFlag (Flag *flag, QToolBar *tb, const QString &name, const QString &tooltip)
{
    flag->setName(name);
    flag->setToolTip (tooltip);
    QAction *a;
    if (tb)
    {
	a=new QAction (flag->getPixmap(),name,this);
	// StandardFlag
	tb->addAction (a);
	flag->setAction (a);
	a->setVisible (flag->isVisible());
	a->setCheckable(true);
	a->setObjectName(name);
	a->setToolTip(tooltip);
	connect (a, SIGNAL( triggered() ), this, SLOT( standardFlagChanged() ) );
	standardFlagsMaster->addFlag (flag);	
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
    //a->setStatusTip ( "Set application to open pdf files"));
    //a->setShortcut ( Qt::ALT + Qt::Key_T );	    //New TCP server
    switchboard.addConnection(a,tr("Network shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( networkStartServer() ) );
    netMenu->addAction (a);

    a = new QAction(  "Connect MapEditor to server",this);
    //a->setStatusTip ( "Set application to open pdf files"));
    a->setShortcut ( Qt::ALT + Qt::Key_C );	// Connect to server
    switchboard.addConnection(a,tr("Network shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( networkConnect() ) );
    netMenu->addAction (a);
}

// Settings Actions
void Main::setupSettingsActions()
{
    QMenu *settingsMenu = menuBar()->addMenu( tr( "&Settings" ));

    QAction *a;

    a = new QAction( tr( "Set application to open pdf files","Settings action"), this);
    a->setStatusTip ( tr( "Set application to open pdf files"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsPDF() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set application to open external links","Settings action"), this);
    a->setStatusTip( tr( "Set application to open external links"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsURL() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set path for macros","Settings action")+"...", this);
    a->setStatusTip( tr( "Set path for macros"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsMacroDir() ) );
    settingsMenu->addAction (a);

    a = new QAction( tr( "Set number of undo levels","Settings action")+"...", this);
    a->setStatusTip( tr( "Set number of undo levels"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsUndoLevels() ) );
    settingsMenu->addAction (a);

    settingsMenu->addSeparator();

    a = new QAction( tr( "Autosave","Settings action"), this);
    a->setStatusTip( tr( "Autosave"));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mainwindow/autosave/use",true).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutosaveToggle() ) );
    settingsMenu->addAction (a);
    actionSettingsAutosaveToggle=a;

    a = new QAction( tr( "Autosave time","Settings action")+"...", this);
    a->setStatusTip( tr( "Autosave time"));
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsAutosaveTime() ) );
    settingsMenu->addAction (a);
    actionSettingsAutosaveTime=a;

    a = new QAction( tr( "Write backup file on save","Settings action"), this);
    a->setStatusTip( tr( "Write backup file on save"));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mainwindow/writeBackupFile",false).toBool());
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsWriteBackupFileToggle() ) );
    settingsMenu->addAction (a);
    actionSettingsWriteBackupFile=a;

    settingsMenu->addSeparator();

    a = new QAction( tr( "Edit branch after adding it","Settings action" ), this );
    a->setStatusTip( tr( "Edit branch after adding it" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoEditNewBranch",true).toBool());
    settingsMenu->addAction (a);
    actionSettingsAutoEditNewBranch=a;

    a= new QAction( tr( "Select branch after adding it","Settings action" ), this );
    a->setStatusTip( tr( "Select branch after adding it" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoSelectNewBranch",false).toBool() );
    settingsMenu->addAction (a);
    actionSettingsAutoSelectNewBranch=a;

    a= new QAction(tr( "Select existing heading","Settings action" ), this);
    a->setStatusTip( tr( "Select heading before editing" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/autoSelectText",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsAutoSelectText=a;

    a= new QAction( tr( "Delete key","Settings action" ), this);
    a->setStatusTip( tr( "Delete key for deleting branches" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/useDelKey",true).toBool() );
    settingsMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleDelKey() ) );
    actionSettingsUseDelKey=a;

    a= new QAction( tr( "Exclusive flags","Settings action" ), this);
    a->setStatusTip( tr( "Use exclusive flags in flag toolbars" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/mapeditor/editmode/useFlagGroups",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsUseFlagGroups=a;

    a= new QAction( tr( "Use hide flags","Settings action" ), this);
    a->setStatusTip( tr( "Use hide flag during exports " ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/export/useHideExport",true).toBool() );
    settingsMenu->addAction (a);
    actionSettingsUseHideExport=a;

    a= new QAction( tr( "Note editor is dockable","Settings action" ), this);
    a->setStatusTip( tr( "If Note editor is dockable, it is always on top, also when floating","Explanation for note editor handling" ));
    a->setCheckable(true);
    a->setChecked ( settings.value ("/satellite/noteeditor/isDockWindow",true).toBool() );
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleNoteEditorIsDockWindow() ) );
    actionSettingsNoteEditorIsDockWindow=a;
    settingsMenu->addAction (a);


    settingsMenu->addSeparator();

    a = new QAction( tr( "Animation","Settings action"), this);
    a->setStatusTip( tr( "Animation"));
    a->setCheckable(true);
    a->setChecked (settings.value("/animation/use",true).toBool() );
    connect( a, SIGNAL( triggered() ), this, SLOT( settingsToggleAnimation() ) );
    settingsMenu->addAction (a);
    actionSettingsUseAnimation=a;

    a = new QAction( tr( "Automatic layout","Settings action"), this);
    a->setStatusTip( tr( "Automatic layout"));
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
    a->setStatusTip( "Call test function 1" );
    a->setShortcut (Qt::SHIFT + Qt::Key_T); // Test function 1  
    switchboard.addConnection(a,tr("Test shortcuts","Shortcut group"));
    testMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction1() ) );

    a = new QAction( "Test function 2" , this);
    a->setStatusTip( "Call test function 2" );
    a->setShortcut (Qt::ALT + Qt::Key_T);   // Test function 2
    switchboard.addConnection(a,tr("Test shortcuts","Shortcut group"));
    testMenu->addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( testFunction2() ) );

    a = new QAction( "Command" , this);
    a->setStatusTip( "Enter command to call in editor" );
    switchboard.addConnection(a,tr("Test shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( testCommand() ) );
    testMenu->addAction (a);
}

// Help Actions
void Main::setupHelpActions()
{
    QMenu *helpMenu = menuBar()->addMenu ( tr( "&Help","Help menubar entry" ));

    QAction *a;
    a = new QAction(  tr( "Open VYM Documentation (pdf) ","Help action" ), this );
    a->setStatusTip( tr( "Open VYM Documentation (pdf)" ));
    switchboard.addConnection(a,tr("Help shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDoc() ) );
    helpMenu->addAction (a);

    a = new QAction(  tr( "Open VYM example maps ","Help action" ), this );
    a->setStatusTip( tr( "Open VYM example maps " ));
    switchboard.addConnection(a,tr("Help shortcuts","Shortcut group"));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpDemo() ) );
    helpMenu->addAction (a);

    a = new QAction( tr( "About VYM","Help action" ), this);
    a->setStatusTip( tr( "About VYM")+vymName);
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAbout() ) );
    helpMenu->addAction (a);

    a = new QAction( tr( "About QT","Help action" ), this);
    a->setStatusTip( tr( "Information about QT toolkit" ));
    connect( a, SIGNAL( triggered() ), this, SLOT( helpAboutQT() ) );
    helpMenu->addAction (a);
}

// Context Menus
void Main::setupContextMenus()
{
    // Context Menu for branch or mapcenter
    branchContextMenu =new QMenu (this);
    branchContextMenu->addAction (actionViewTogglePropertyWindow);
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

    // Submenu for Links (URLs, vymLinks)
    branchLinksContextMenu =new QMenu (this);

	branchContextMenu->addSeparator();  
	branchLinksContextMenu=branchContextMenu->addMenu(tr("References (URLs, vymLinks, ...)","Context menu name"));	
	branchLinksContextMenu->addAction ( actionOpenURL );
	branchLinksContextMenu->addAction ( actionOpenURLTab );
	branchLinksContextMenu->addAction ( actionOpenMultipleVisURLTabs );
	branchLinksContextMenu->addAction ( actionOpenMultipleURLTabs );
	branchLinksContextMenu->addAction ( actionURL );
	branchLinksContextMenu->addAction ( actionLocalURL );
	branchLinksContextMenu->addAction ( actionHeading2URL );
	branchLinksContextMenu->addAction ( actionBugzilla2URL );
	if (settings.value( "/mainwindow/showTestMenu",false).toBool() )
	{
	    branchLinksContextMenu->addAction ( actionGetBugzillaData );
	    branchLinksContextMenu->addAction ( actionGetBugzillaDataSubtree );
	    branchLinksContextMenu->addAction ( actionFATE2URL );
	}   
	branchLinksContextMenu->addSeparator();	
	branchLinksContextMenu->addAction ( actionOpenVymLink );
	branchLinksContextMenu->addAction ( actionOpenMultipleVymLinks );
	branchLinksContextMenu->addAction ( actionVymLink );
	branchLinksContextMenu->addAction ( actionDeleteVymLink );
	

    // Context Menu for XLinks in a branch menu
    // This will be populated "on demand" in MapEditor::updateActions
    branchContextMenu->addSeparator();	
    branchXLinksContextMenuEdit =branchContextMenu->addMenu (tr ("Edit XLink","Context menu name"));
    connect( 
	branchXLinksContextMenuEdit, SIGNAL( triggered(QAction *) ), 
	this, SLOT( editEditXLink(QAction * ) ) );
    QAction *a;
    a = new QAction( tr("Follow XLink","Context menu") , this);
    a->setShortcut (Qt::Key_F); 
    switchboard.addConnection(a,tr("Edit","Shortcut group"));
    addAction (a);
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
    canvasContextMenu->addAction (actionMapInfo);
    canvasContextMenu->insertSeparator(actionMapInfo);   
    canvasContextMenu->addActions(actionGroupFormatLinkStyles->actions() );
    canvasContextMenu->insertSeparator(actionGroupFormatLinkStyles->actions().last() );   
    canvasContextMenu->addAction(actionFormatLinkColorHint);
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
	QString text = tr("&%1 %2").arg(i + 1).arg(files[i]);
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

void Main::hideEvent (QHideEvent * )
{
    if (!noteEditor->isMinimized() ) noteEditor->hide();
}

void Main::showEvent (QShowEvent * )
{
    if (actionViewToggleNoteEditor->isChecked()) noteEditor->showNormal();
}


MapEditor* Main::currentMapEditor() const
{
    if ( tabWidget->currentWidget())
	return vymViews.at(tabWidget->currentIndex())->getMapEditor();
    return NULL;    
}

uint  Main::currentModelID() const
{
    if (currentModel())
	return currentModel()->getID();
    return 0;	
}

VymModel* Main::currentModel() const
{
    if ( tabWidget->currentWidget())
	return vymViews.at(tabWidget->currentIndex())->getModel();
    return NULL;    
}

VymModel* Main::getModel(uint id) const	
{
    // Used in BugAgent
    for (int i=0; i<vymViews.count();i++)
	if (vymViews.at(i)->getModel()->getID()==id)
	    return vymViews.at(i)->getModel();
    return NULL;    
}


void Main::editorChanged(QWidget *)
{
    // Unselect all possibly selected objects
    // (Important to update note editor)
    VymModel *m;
    for (int i=0;i<=tabWidget->count() -1;i++)
    {
	m= vymViews.at(i)->getModel();
	if (m) m->unselect();
    }
    m=currentModel();
    if (m) 
    {	
	m->reselect();
	updateQueries (m);
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
	dstModel->select("mc:");
	dstModel->loadMap (clipboardDir+"/"+clipboardFile,ImportReplace);
    }
}

ErrorCode Main::fileLoad(QString fn, const LoadMode &lmode, const FileType &ftype) 
{
    ErrorCode err=success;

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
			return aborted;
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
	    tabWidget->setCurrentIndex (tabIndex);
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
		    return success;
			
		case QMessageBox::Cancel:
		    // don't create new map
		    statusBar()->showMessage( "Loading " + fn + " failed!", statusbarTime );
		    fileCloseMap();
		    return aborted;
	    }
	}   


	//tabWidget->currentPage() won't be NULL here, because of above...
	tabWidget->setCurrentIndex (tabIndex);

	if (err!=aborted)
	{
	    // Save existing filename in case  we import
	    QString fn_org=vm->getFilePath();

	    // Finally load map into mapEditor
	    vm->setFilePath (fn);
	    err=vm->loadMap(fn,lmode,true,ftype);

	    // Restore old (maybe empty) filepath, if this is an import
	    if (lmode!=NewMap)
		vm->setFilePath (fn_org);
	}   

	// Finally check for errors and go home
	if (err==aborted) 
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
	    statusBar()->showMessage( "Loaded " + fn, statusbarTime );
	}   
    }
    return err;
}


void Main::fileLoad(const LoadMode &lmode)
{
    QStringList filters;
    filters <<"VYM map (*.vym *.vyp)"<<"VYM Backups (*.vym~)"<<"XML (*.xml)"<<"All (* *.*)";
    QFileDialog *fd=new QFileDialog( this);
    fd->setDirectory (lastFileDir);
    fd->setFileMode (QFileDialog::ExistingFiles);
    fd->setFilters (filters);
    switch (lmode)
    {
	case NewMap:
	    fd->setWindowTitle(vymName+ " - " +tr("Load vym map"));
	    break;
	case ImportAdd:
	    fd->setWindowTitle(vymName+ " - " +tr("Import: Add vym map to selection"));
	    break;
	case ImportReplace:
	    fd->setWindowTitle(vymName+ " - " +tr("Import: Replace selection with vym map"));
	    break;
    }
    fd->show();

    QString fn;
    if ( fd->exec() == QDialog::Accepted )
    {
	lastFileDir=fd->directory().path();
	QStringList flist = fd->selectedFiles();
	QStringList::Iterator it = flist.begin();
	
	progressCounterTotal=flist.count();
	while( it != flist.end() ) 
	{
	    fn = *it;
	    fileLoad(*it, lmode);		   
	    ++it;
	}
    }
    removeProgressCounter();
    delete (fd);
}

void Main::fileLoad()
{
    fileLoad (NewMap);
}

void Main::fileLoadRecent()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
	progressCounterTotal=1;
        fileLoad (action->data().toString(), NewMap);
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
	//FIXME-3 needed???  tabWidget->setCurrentWidget (m->getMapEditor());
	fileSaveAs(savemode);
    }

    if (m->save (savemode)==success)
    {
	statusBar()->showMessage( 
	    tr("Saved  %1").arg(m->getFilePath()), 
	    statusbarTime );
	addRecentMap (m->getFilePath() );
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
	QStringList filters;
	if (savemode==CompleteMap)
	    filters<<"VYM map (*.vym)";
	else    
	    filters<<"VYM part of map (*vyp)";
	filters<<"All (* *.*)";
	QFileDialog *fd=new QFileDialog( this);
	fd->setDirectory (lastFileDir);
	fd->setFileMode (QFileDialog::ExistingFiles);
	fd->setFilters (filters);
	fd->show();

	if ( fd->exec() == QDialog::Accepted && !fd->selectedFiles().isEmpty())
	{
	    QString fn=fd->selectedFiles().first();
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
	    m->setFilePath(fn);
	    fileSave(m, savemode);

	    // Set name of tab, assuming current tab is the one we just saved
	    if (savemode==CompleteMap)
		tabWidget->setTabText (tabWidget->currentIndex(), m->getFileName() );
	    return; 
	}
    }
}

void Main::fileSaveAs()
{
    fileSaveAs (CompleteMap);
}

void Main::fileImportKDE3Bookmarks()
{
    ImportKDE3Bookmarks im;
    im.transform();
    if (aborted!=fileLoad (im.getTransformedFile(),NewMap) && currentMapEditor() )
	currentMapEditor()->getModel()->setFilePath ("");
}

void Main::fileImportKDE4Bookmarks()
{
    ImportKDE4Bookmarks im;
    im.transform();
    if (aborted!=fileLoad (im.getTransformedFile(),NewMap) && currentMapEditor() )
	currentMapEditor()->getModel()->setFilePath ("");
}

void Main::fileImportFirefoxBookmarks()
{
    QFileDialog *fd=new QFileDialog( this);
    fd->setDirectory (vymBaseDir.homePath()+"/.mozilla/firefox");
    fd->setFileMode (QFileDialog::ExistingFiles);
    QStringList filters;
    filters<<"Firefox "+tr("Bookmarks")+" (*.html)";
    fd->setFilters(filters);
    fd->setWindowTitle(tr("Import")+" "+"Firefox "+tr("Bookmarks"));
    fd->show();

    if ( fd->exec() == QDialog::Accepted )
    {
	ImportFirefoxBookmarks im;
	QStringList flist = fd->selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    im.setFile (*it);
	    if (im.transform() && 
		aborted!=fileLoad (im.getTransformedFile(),NewMap,FreemindMap) && 
		currentMapEditor() )
		currentMapEditor()->getModel()->setFilePath ("");
	    ++it;
	}
    }
    delete (fd);
}

void Main::fileImportFreemind()
{
    QStringList filters;
    filters <<"Freemind map (*.mm)"<<"All files (*)";
    QFileDialog *fd=new QFileDialog( this);
    fd->setDirectory (lastFileDir);
    fd->setFileMode (QFileDialog::ExistingFiles);
    fd->setFilters (filters);
    fd->setWindowTitle(vymName+ " - " +tr("Load Freemind map"));
    fd->show();

    QString fn;
    if ( fd->exec() == QDialog::Accepted )
    {
	lastFileDir=fd->directory().path();
	QStringList flist = fd->selectedFiles();
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
    delete (fd);
}


void Main::fileImportMM()
{
    ImportMM im;

    QFileDialog *fd=new QFileDialog( this);
    fd->setDirectory (lastFileDir);
    fd->setFileMode (QFileDialog::ExistingFiles);
    QStringList filters;
    filters<<"Mind Manager (*.mmap)";
    fd->setFilters (filters);
    fd->setWindowTitle(tr("Import")+" "+"Mind Manager");
    fd->show();

    if ( fd->exec() == QDialog::Accepted )
    {
	lastFileDir=fd->directory();
	QStringList flist = fd->selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    im.setFile (*it);
	    if (im.transform() && 
		success==fileLoad (im.getTransformedFile(),NewMap) && 
		currentMapEditor() )
		currentMapEditor()->getModel()->setFilePath ("");
	    ++it;
	}
    }
    delete (fd);
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
	ex.setDir(lastImageDir);
	ex.setWindowTitle(vymName+ " -" +tr("Export as CSV")+" "+tr("(still experimental)"));
	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }
}

void Main::fileExportLaTeX()	//FIXME-3 not scriptable yet
{
    VymModel *m=currentModel();
    if (m)
    {
	ExportLaTeX ex;
	ex.setModel (m);
	ex.addFilter ("Tex (*.tex)");
	ex.setDir(lastImageDir);
	ex.setWindowTitle(vymName+ " -" +tr("Export as LaTeX")+" "+tr("(still experimental)"));
	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }
}

void Main::fileExportKDE3Bookmarks()	//FIXME-3 not scriptable yet
{
    ExportKDE3Bookmarks ex;
    VymModel *m=currentModel();
    if (m)
    {
	ex.setModel (m);
	ex.doExport();
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
	ex.setDir(lastImageDir);
	ex.addFilter ("Taskjuggler (*.tjp)");
	if (ex.execDialog() ) 
	{
	    m->setExportMode(true);
	    ex.doExport();
	    m->setExportMode(false);
	}
    }	
}

void Main::fileExportOOPresentation()	//FIXME-3 not scriptable yet
{
    ExportOOFileDialog *fd=new ExportOOFileDialog( this,vymName+" - "+tr("Export to")+" Open Office");
    // TODO add preview in dialog
    //ImagePreview *p =new ImagePreview (fd);
    //fd->setContentsPreviewEnabled( TRUE );
    //fd->setContentsPreview( p, p );
    //fd->setPreviewMode( QFileDialog::Contents );
    fd->setWindowTitle(vymName+" - " +tr("Export to")+" Open Office");
    fd->setDirectory (QDir().current());
    if (fd->foundConfig())
    {
	fd->show();

	if ( fd->exec() == QDialog::Accepted )
	{
	    if (!fd->selectedFiles().isEmpty())
	    {
		QString fn=fd->selectedFiles().first();
		if (!fn.contains (".odp")) fn +=".odp";

		//lastImageDir=fn.left(fn.findRev ("/"));
		VymModel *m=currentModel();
		if (m) m->exportOOPresentation(fn,fd->selectedConfig());	
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
	    //FIXME-3 not needed? mb.setActiveWindow();
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
	vymViews.removeAt (tabWidget->currentIndex() );
	tabWidget->removeTab (tabWidget->currentIndex() );

	delete (m); // changing model still will try to update selection in editors, remove model first
	//delete (m->getMapEditor());

	updateActions();
	return false;
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

void Main::editOpenFindResultWidget()  
{
    if (!findResultWidget->parentWidget()->isVisible())
    {
//	findResultWidget->parentWidget()->show();
	findResultWidget->popup();
    } else 
	findResultWidget->parentWidget()->hide();
}

/* FIXME-3 not needed
void Main::editHideFindWidget()
{
    // findWidget hides itself, but we want
    // to have focus back at mapEditor usually
    MapEditor *me=currentMapEditor();
    if (me) me->setFocus();
}
*/

#include "findwidget.h" // FIXME-3 Integrated FRW and FW
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

void Main::updateQueries (VymModel* m) //FIXME-2 disabled for now to avoid selection in FRW
{
 //   qDebug() << "MW::updateQueries m="<<m<<"   cM="<<currentModel();
    return;
    if (m && currentModel()==m)
    {
	QString s=findResultWidget->getFindText();
	if (!s.isEmpty() ) editFindNext (s);
    }	
}

void Main::openTabs(QStringList urls)
{
    if (!urls.isEmpty())
    {	
	bool success=true;
	QStringList args;
	QString browser=settings.value("/mainwindow/readerURL" ).toString();
	//qDebug ()<<"Services: "<<QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
	if (*browserPID==0 ||
	    (browser.contains("konqueror") &&
	     !QDBusConnection::sessionBus().interface()->registeredServiceNames().value().contains (QString("org.kde.konqueror-%1").arg(*browserPID)))
	   )	 
	{
	    // Start a new browser, if there is not one running already or
	    // if a previously started konqueror is gone.
	    if (debug) cout <<"Main::openTabs no konqueror-"<<*browserPID<<" found\n";
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
	    if (debug) cout << "Main::openTabs  Started konqueror-"<<*browserPID<<endl;
#if defined(Q_OS_WIN32)
            // There's no sleep in VCEE, replace it with Qt's QThread::wait().
            this->thread()->wait(3000);
#else
	    sleep (3);	//needed to open first konqueror
#endif
	}

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
		if (debug) cout << "MainWindow::openURLs  args="<<args.join(" ").toStdString()<<endl;
		if (!QProcess::startDetached ("qdbus",args))
		    success=false;
	    }
	    if (!success)
		QMessageBox::warning(0, 
		    tr("Warning"),
		    tr("Couldn't start %1 to open a new tab in %2.").arg("dcop").arg("konqueror"));
	    return;	
	} else if (browser.contains ("firefox") || browser.contains ("mozilla") )
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
}

void Main::editOpenURL()
{
    // Open new browser
    VymModel *m=currentModel();
    if (m)
    {	
	QString url=m->getURL();
	QStringList args;
	if (url=="") return;
	QString browser=settings.value("/mainwindow/readerURL" ).toString();
	args<<url;
	QString workDir=QDir::currentPath();
	if (!QProcess::startDetached(browser,args))
	{
	    // try to set path to browser
	    QMessageBox::warning(0, 
		tr("Warning"),
		tr("Couldn't find a viewer to open %1.\n").arg(url)+
		tr("Please use Settings->")+tr("Set application to open an URL"));
	    settingsURL() ; 
	}   
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


void Main::editURL()
{
    VymModel *m=currentModel();
    if (m) m->editURL();
}

void Main::editLocalURL()
{
    VymModel *m=currentModel();
    if (m) m->editLocalURL();
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
    for (int j=0; j<vl.size(); ++j)
    {
	// compare path with already loaded maps
	int index=-1;
	int i;
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
	    fileLoad (vlmin.at(j), NewMap);
	    tabWidget->setCurrentIndex (tabWidget->count()-1);	
	}
	// Go to tab containing the map
	//tabWidget->setCurrentIndex (index);	
    }	    
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
	m->editVymLink();   
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

    stats+=QString ("%1 xLinks \n").arg (xl/2,6);
    stats+=QString ("%1 notes\n").arg (n,6);
    stats+=QString ("%1 images\n").arg (f,6);
    stats+=QString ("%1 branches\n").arg (m->branchCount(),6);
    dia.setStats (stats);

    // Finally show dialog
    if (dia.exec() == QDialog::Accepted)
    {
	m->setAuthor (dia.getAuthor() );
	m->setComment (dia.getComment() );
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
    if (m) m->select (m->addMapCenter ());
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
    if ( m)
    {
	BranchItem *bi=m->addNewBranch (-1);


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

void Main::editNewBranchBelow()
{
    VymModel *m=currentModel();
    if (m)
    {
	BranchItem *bi=m->addNewBranch (1);

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
    if (m && actionSettingsUseDelKey->isChecked())
	m->deleteSelection();
}

void Main::editLoadImage()
{
    VymModel *m=currentModel();
    if (m) m->loadFloatImage();
}

void Main::editSaveImage()
{
    VymModel *m=currentModel();
    if (m) m->saveFloatImage();
}

void Main::editEditXLink(QAction *a)
{
    VymModel *m=currentModel();
    if (m)
	m->editXLink(branchXLinksContextMenuEdit->actions().indexOf(a));
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


void Main::formatSelectColor()
{
    QColor col = QColorDialog::getColor((currentColor ), this );
    if ( !col.isValid() ) return;
    colorChanged( col );
}

void Main::formatPickColor()
{
    VymModel *m=currentModel();
    if (m)
	colorChanged( m->getCurrentHeadingColor() );
}

void Main::colorChanged(QColor c)
{
    QPixmap pix( 16, 16 );
    pix.fill( c );
    actionFormatColor->setIcon( pix );
    currentColor=c;
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

void Main::formatToggleLinkColorHint()
{
    VymModel *m=currentModel();
    if (m) m->toggleMapLinkColorHint();
}


void Main::formatHideLinkUnselected()	//FIXME-3 get rid of this with imagepropertydialog
{
    VymModel *m=currentModel();
    if (m)
	m->setHideLinkUnselected(actionFormatHideLinkUnselected->isChecked());
}

void Main::viewZoomReset()
{
    MapEditor *me=currentMapEditor();
    if (me) me->setZoomFactorTarget (1);
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
	tr("QInputDialog::getInteger()"),
	tr("Number of undo/redo levels:"), settings.value("/mapeditor/stepsTotal").toInt(), 0, 1000, 1, &ok);
    if (ok)
    {
	settings.setValue ("/mapeditor/stepsTotal",i);
	QMessageBox::information( this, tr( "VYM -Information:" ),
	   tr("Settings have been changed. The next map opened will have \"%1\" undo/redo levels").arg(i)); 
   }	
}

void Main::settingsAutosaveToggle()
{
    settings.setValue ("/mainwindow/autosave/use",actionSettingsAutosaveToggle->isChecked() );
}

void Main::settingsAutosaveTime()
{
    bool ok;
    int i = QInputDialog::getInteger(
	this, 
	tr("QInputDialog::getInteger()"),
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

void Main::settingsToggleNoteEditorIsDockWindow()
{
    QMessageBox::information(0, 
	tr("Information"),
	tr("Changed settings will be applied after restarting vym"));
}

void Main::settingsToggleAnimation()
{
    settings.setValue ("/animation/use",actionSettingsUseAnimation->isChecked() );
}

void Main::settingsToggleDelKey()
{
    if (actionSettingsUseDelKey->isChecked())
    {
	actionDelete->setShortcut (QKeySequence (Qt::Key_Delete));
    } else
    {
	actionDelete->setShortcut (QKeySequence (""));
    }
}

void Main::windowToggleNoteEditor()
{
    if (noteEditor->isVisible() )
	windowHideNoteEditor();
    else
	windowShowNoteEditor();
}

void Main::windowToggleTreeEditor()
{
    if ( tabWidget->currentWidget())
	vymViews.at(tabWidget->currentIndex())->toggleTreeEditor();
}

void Main::windowToggleHistory()
{
    if (historyWindow->isVisible())
	historyWindow->hide();
    else    
	historyWindow->show();

}

void Main::windowToggleProperty()
{
    if (branchPropertyWindow->isVisible())
	branchPropertyWindow->hide();
    else    
	branchPropertyWindow->show();
    branchPropertyWindow->setModel (currentModel() );
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

void Main::updateNoteEditor(QModelIndex index )
{
    TreeItem *ti=((VymModel*) QObject::sender())->getItem(index);
    /*
    cout << "Main::updateNoteEditor model="<<sender();
    cout << "  item="<<ti->getHeadingStd()<<" ("<<ti<<")"<<endl;
    */
    noteEditor->setNote (ti->getNoteObj() );
}

void Main::selectInNoteEditor(QString s,int i)
{
    // TreeItem is already selected at this time, therefor
    // the note is already in the editor
    noteEditor->findText (s,0,i);
}

void Main::changeSelection (VymModel *model, const QItemSelection &newsel, const QItemSelection &oldsel)
{
    branchPropertyWindow->setModel (model ); //FIXME-3 this used to be called from BranchObj::select(). Maybe use signal now...

    if (model && model==currentModel() )
    {
	// NoteEditor
	TreeItem *ti;
	if (!oldsel.indexes().isEmpty() )
	{
	    ti=model->getItem(oldsel.indexes().first());

	    // Don't update note if both treeItem and noteEditor are empty
	    //if (! (ti->hasEmptyNote() && noteEditor->isEmpty() ))
	    //	ti->setNoteObj (noteEditor->getNoteObj(),false );
	} 
	if (!newsel.indexes().isEmpty() )
	{
	    ti=model->getItem(newsel.indexes().first());
	    if (!ti->hasEmptyNote() )
		noteEditor->setNote(ti->getNoteObj() );
	    else
		noteEditor->setNote(NoteObj() );    //FIXME-4 maybe add a clear() to TE
	    // Show URL and link in statusbar	
	    QString status;
	    QString s=ti->getURL();
	    if (!s.isEmpty() ) status+="URL: "+s+"  ";
	    s=ti->getVymLink();
	    if (!s.isEmpty() ) status+="Link: "+s;
	    if (!status.isEmpty() ) statusMessage (status);

	    headingEditor->setText (ti->getHeading() );
	} else
	    noteEditor->setInactive();

	updateActions();
    }
}

void Main::updateActions()
{
    // updateActions is also called when satellites are closed	
    actionViewToggleNoteEditor->setChecked (noteEditor->isVisible());
    actionViewToggleHistoryWindow->setChecked (historyWindow->isVisible());
    actionViewTogglePropertyWindow->setChecked (branchPropertyWindow->isVisible());
    if ( tabWidget->currentWidget())
	actionViewToggleTreeEditor->setChecked (
	    vymViews.at(tabWidget->currentIndex())->getTreeEditor()->isVisible()
	);

    VymModel  *m =currentModel();
    if (m) 
    {
	// Printing
	actionFilePrint->setEnabled (true);

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

	// History window
	historyWindow->setWindowTitle (vymName + " - " +tr("History for %1","Window Caption").arg(m->getFileName()));


	// Expanding/collapsing
	actionExpandAll->setEnabled (true);
	actionExpandOneLevel->setEnabled (true);
	actionCollapseOneLevel->setEnabled (true);
	actionCollapseUnselected->setEnabled (true);

    } else
    {
	// Printing
	actionFilePrint->setEnabled (false);

	// Expanding/collapsing
	actionExpandAll->setEnabled (false);
	actionExpandOneLevel->setEnabled (false);
	actionCollapseOneLevel->setEnabled (false);
	actionCollapseUnselected->setEnabled (false);
    }

    if (m && m->getMapLinkColorHint()==LinkableMapObj::HeadingColor) 
	actionFormatLinkColorHint->setChecked(true);
    else    
	actionFormatLinkColorHint->setChecked(false);


    if (m && m->hasChanged() )
	actionFileSave->setEnabled( true);
    else    
	actionFileSave->setEnabled( false);
    if (m && m->isUndoAvailable())
	actionUndo->setEnabled( true);
    else    
	actionUndo->setEnabled( false);

    if (m && m->isRedoAvailable())
	actionRedo->setEnabled( true);
    else    
	actionRedo->setEnabled( false);

    // Export last
    QString s, t, u;
    if (m && m->exportLastAvailable(s,t,u) )
    {
	actionFileExportLast->setEnabled (true);
	actionFileExportLast->setStatusTip( tr( "Export in last used format (%1)","status tip" ).arg(s));
	actionFileExportLast->setText( tr( "Export in last used format (%1)","status tip" ).arg(s));

    }	
    else
	actionFileExportLast->setEnabled (false);
    actionFileExportLast->setStatusTip( tr( "Export in last used format (%1)","status tip" ).arg(s));
    actionFileExportLast->setText( tr( "Export in last used format (%1)","status tip" ).arg(s));

    if (m)
    {
	TreeItem *selti=m->getSelectedItem();
	BranchItem *selbi=m->getSelectedBranch();
	if (selti)
	{
	    if (selbi || selti->getType()==TreeItem::Image)
	    {
		actionFormatHideLinkUnselected->setChecked (((MapItem*)selti)->getHideLinkUnselected());
		actionFormatHideLinkUnselected->setEnabled (true);
	    }

	    if (selbi)	
	    {
		actionHeading2URL->setEnabled (true);  

		// Take care of xlinks  
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

		if ( selti->getURL().isEmpty() )
		{
		    actionOpenURL->setEnabled (false);
		    actionOpenURLTab->setEnabled (false);
		}   
		else	
		{
		    actionOpenURL->setEnabled (true);
		    actionOpenURLTab->setEnabled (true);
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

		if (selbi->canMoveUp()) 
		    actionMoveUp->setEnabled (true);
		else	
		    actionMoveUp->setEnabled (false);
		if (selbi->canMoveDown()) 
		    actionMoveDown->setEnabled (true);
		else	
		    actionMoveDown->setEnabled (false);

		actionSortChildren->setEnabled (true);
		actionSortBackChildren->setEnabled (true);

		actionToggleHideExport->setEnabled (true);  
		actionToggleHideExport->setChecked (selbi->hideInExport() );	

		actionCopy->setEnabled (true);	
		actionCut->setEnabled (true);	
		if (!clipboardEmpty)
		    actionPaste->setEnabled (true); 
		else	
		    actionPaste->setEnabled (false);	
		for (int i=0; i<actionListBranches.size(); ++i)	
		    actionListBranches.at(i)->setEnabled(true);
		actionDelete->setEnabled (true);
	    }	// Branch
	    if ( selti->getType()==TreeItem::Image)
	    {
		actionOpenURL->setEnabled (false);
		actionOpenVymLink->setEnabled (false);
		actionDeleteVymLink->setEnabled (false);    
		actionToggleHideExport->setEnabled (true);  
		actionToggleHideExport->setChecked (selti->hideInExport() );	


		actionCopy->setEnabled (true);
		actionCut->setEnabled (true);	
		actionPaste->setEnabled (false);    //FIXME-4 why not allowing copy of images?
		for (int i=0; i<actionListBranches.size(); ++i)	
		    actionListBranches.at(i)->setEnabled(false);
		actionDelete->setEnabled (true);
		actionMoveUp->setEnabled (false);
		actionMoveDown->setEnabled (false);
	    }	// Image

	} else
	{   // !selti
	    actionCopy->setEnabled (false); 
	    actionCut->setEnabled (false);  
	    actionPaste->setEnabled (false);	
	    for (int i=0; i<actionListBranches.size(); ++i) 
		actionListBranches.at(i)->setEnabled(false);

	    actionToggleScroll->setEnabled (false);
	    actionOpenURL->setEnabled (false);
	    actionOpenVymLink->setEnabled (false);
	    actionDeleteVymLink->setEnabled (false);	
	    actionHeading2URL->setEnabled (false);  
	    actionDelete->setEnabled (false);
	    actionMoveUp->setEnabled (false);
	    actionMoveDown->setEnabled (false);
	    actionFormatHideLinkUnselected->setEnabled (false);
	    actionSortChildren->setEnabled (false);
	    actionSortBackChildren->setEnabled (false);
	    actionToggleHideExport->setEnabled (false);	
	}   
    } // m
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

void Main::windowShowNoteEditor()
{
    noteEditor->setShowWithMain(true);
    noteEditor->show();
    actionViewToggleNoteEditor->setChecked (true);
}

void Main::windowHideNoteEditor()
{
    noteEditor->setShowWithMain(false);
    noteEditor->hide();
    actionViewToggleNoteEditor->setChecked (false);
}

void Main::setScript (const QString &script)
{
    scriptEditor->setScript (script);
}

void Main::runScript (const QString &script)
{
    VymModel *m=currentModel();
    if (m) m->runScript (script);
}

void Main::runScriptEverywhere (const QString &script)
{
    MapEditor *me;
    for (int i=0;i<=tabWidget->count() -1;i++)
    {
	me=(MapEditor*)tabWidget->widget(i);
	if (me) me->getModel()->runScript (script);
    }	
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

void Main::testCommand()
{
    if (!currentMapEditor()) return;
    scriptEditor->show();
    /*
    bool ok;
    QString com = QInputDialog::getText(
	    vymName, "Enter Command:", QLineEdit::Normal,"command", &ok, this );
    if (ok) currentMapEditor()->parseAtom(com);
    */
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
    searchList << "doc/tex";	// Easy testing working on vym.tex
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
    QFileDialog *fd=new QFileDialog( this);
    #if defined(Q_OS_MACX)
	fd->setDir (QDir("./vym.app/Contents/Resources/demos"));
    #else
	// default path in SUSE LINUX
	fd->setDirectory (QDir(vymBaseDir.path()+"/demos"));
    #endif

    fd->setFileMode (QFileDialog::ExistingFiles);
    fd->setFilters (filters);
    fd->setWindowTitle (vymName+ " - " +tr("Load vym example map"));
    fd->show();

    QString fn;
    if ( fd->exec() == QDialog::Accepted )
    {
	lastFileDir=fd->directory().path();
	QStringList flist = fd->selectedFiles();
	QStringList::Iterator it = flist.begin();
	while( it != flist.end() ) 
	{
	    fn = *it;
	    fileLoad(*it, NewMap);		   
	    ++it;
	}
    }
    delete (fd);
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
	QString mDir (settings.value ("macros/macroDir",vymBaseDir.path()+"/macros").toString() );

	QString fn=mDir + QString("/macro-%1.vys").arg(i+1);
	QFile f (fn);
	if ( !f.open( QIODevice::ReadOnly ) )
	{
	    QMessageBox::warning(0, 
		tr("Warning"),
		tr("Couldn't find a macro at  %1.\n").arg(fn)+
		tr("Please use Settings->")+tr("Set directory for vym macros"));
	    return;
	}   

	QTextStream ts( &f );
	QString macro= ts.readAll();

	if (! macro.isEmpty())
	{
	    VymModel *m=currentModel();
	    if (m) m->runScript(macro);
	}   
    }	
}


