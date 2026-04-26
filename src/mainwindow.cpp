#include "mainwindow.h"

#include <iostream>

#if defined(VYM_DBUS)
#include "adaptorvym.h"
#endif

#include <QColorDialog>
#include <QDockWidget>
#include <QFileDialog>
#include <QFontDialog>
#include <QInputDialog>
#include <QMenuBar>
#include <QPrinter>
#include <QJSEngine>
#include <QSslSocket>
#include <QStatusBar>
#include <QTextStream>

#include "aboutdialog.h"
#include "actionlog-dialog.h"
#include "attributeitem.h"
#include "background-dialog.h"
#include "branch-container.h"
#include "branchitem.h"
#include "branchpropeditor.h"
#include "command.h"
#include "confluence-agent.h"
#include "confluence-user.h"
#include "confluence-userdialog.h"
#include "confluence-settings-dialog.h"
#include "darktheme-settings-dialog.h"
#include "debuginfo.h"
#include "default-map-settings-dialog.h"
#include "download-agent.h"
#include "extrainfodialog.h"
#include "file.h"
#include "findresultmodel.h"
#include "findresultwidget.h"
#include "headingeditor.h"
#include "historywindow.h"
#include "image-container.h"
#include "imports.h"
#include "jira-agent.h"
#include "jira-settings-dialog.h"
#include "lineeditdialog.h"
#include "macros.h"
#include "mapeditor.h"
#include "misc.h"
#include "noteeditor.h"
#include "options.h"
#include "scripteditor.h"
#include "vym-wrapper.h"
#include "scriptoutput.h"
#include "settings.h"
#include "shortcuts.h"
#include "showtextdialog.h"
#include "task.h"
#include "taskeditor.h"
#include "taskmodel.h"
#include "treeeditor.h"
#include "vymmodelwrapper.h"
#include "vymprocess.h"
#include "vymview.h"
#include "warningdialog.h"
#include "xlink.h"
#include "xlinkitem.h"

QPrinter *printer = nullptr;

//#include <modeltest.h>

#if defined(VYM_DBUS)
#include <QDBusConnection>
#endif

extern NoteEditor *noteEditor;
extern HeadingEditor *headingEditor;
extern BranchPropertyEditor *branchPropertyEditor;
extern QJSEngine *scriptEngine;
extern ScriptEditor *scriptEditor;
extern ScriptOutput *scriptOutput;
extern Main *mainWindow;
extern FindResultWidget *findResultWidget;
extern TaskEditor *taskEditor;
extern TaskModel *taskModel;
extern Macros macros;
extern FlagRowMaster *standardFlagsMaster;
extern FlagRowMaster *userFlagsMaster;
extern FlagRowMaster *systemFlagsMaster;
extern QString vymName;
extern QString vymVersion;
extern QString vymPlatform;
extern QString vymCodeQuality;
extern QString vymCodeName;
extern QString vymBuildDate;
extern QString localeName;
extern bool debug;
extern bool testmode;
extern QTextStream vout;
extern Switchboard switchboard;

extern bool restoreMode;
extern QStringList ignoredLockedFiles;
extern QStringList lastSessionFiles;

extern QList<Command *> vymCommands;
extern QList<Command *> modelCommands;
extern QList<Command *> branchCommands;
extern QList<Command *> imageCommands;
extern QList<Command *> itemListCommands;
extern QList<Command *> xlinkCommands;

extern bool usingDarkTheme;
extern QString iconTheme;

extern bool useActionLog;
extern QString actionLogPath;

QMenu *branchContextMenu;
QMenu *canvasContextMenu;
QMenu *floatimageContextMenu;
QMenu *taskContextMenu;

extern Settings settings;
extern Options options;
extern ImageIO imageIO;

extern QDir vymBaseDir;
extern QDir vymTranslationsDir;
extern QDir lastExportDir;
extern QDir lastImageDir;
extern QDir lastMapDir;
#if defined(Q_OS_WIN32)
extern QDir vymInstallDir;
#endif

extern QColor vymBlueColor;

extern QString toolBarStyle;

Main::Main(QWidget *parent) : QMainWindow(parent)
{
    // qDebug() << "Constr. MainWindow";
    mainWindow = this;

    setWindowTitle(vymName + " - View Your Mind");

    shortcutScope = tr("Main window", "Shortcut scope");
    switchboard.addScope("MainWindow", shortcutScope);

    // Sometimes we may need to remember old selections
    prevSelection = QUuid();

    // Define commands in API (used globally)
    setupAPI();

    // Initialize some settings, which are platform dependant
    QString p, s;

    // application to open URLs
    p = "/system/readerURL";
#if defined(Q_OS_WIN)
    // Assume that system has been set up so that
    // Explorer automagically opens up the URL
    // in the user's preferred browser.
    s = settings.value(p, "C:\\Program Files (x86)\\Microsoft\\Edge\\Application\\msedge.exe").toString();
#elif defined(Q_OS_MACOS)
    s = settings.value(p, "/usr/bin/open").toString();
#else
    s = settings.value(p, "xdg-open").toString();
#endif
    settings.setValue(p, s);

    // application to open PDFs
    p = "/system/readerPDF";
#if defined(Q_OS_WIN)
    s = settings.value(p, "explorer").toString();
#elif defined(Q_OS_MACOS)
    s = settings.value(p, "/usr/bin/open").toString();
#else
    s = settings.value(p, "xdg-open").toString();
#endif
    settings.setValue(p, s);

    // width of xLinksMenu
    xLinkMenuWidth = 60;

    // Create Layout
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    setCentralWidget(centralWidget);

    // Create tab widget which holds the maps
    tabWidget = new QTabWidget(centralWidget);
    connect(tabWidget, SIGNAL(currentChanged(int)), this,
            SLOT(editorChanged()));

    // Allow closing of tabs (introduced in Qt 4.5)
    tabWidget->setTabsClosable(true);
    connect(tabWidget, SIGNAL(tabCloseRequested(int)), this,
            SLOT(fileCloseTab(int)));

    tabWidget->setMovable(true);

    layout->addWidget(tabWidget);

    switchboard.addScope("TextEditor", tr("Text Editors", "Shortcut group"));

    // Create main menus

    fileMenu = menuBar()->addMenu(tr("&Map", "Map menu"));
    editMenu = menuBar()->addMenu(tr("E&dit", "Edit menu"));
    selectMenu = menuBar()->addMenu(tr("Select", "Select menu"));
    formatMenu = menuBar()->addMenu(tr("F&ormat", "Format menu"));
    viewMenu = menuBar()->addMenu(tr("&View"));
    toolbarsMenu =
        viewMenu->addMenu(tr("Toolbars", "Toolbars overview in view menu"));
    toggleWindowsMenu =
        viewMenu->addMenu(tr("Toggle window", "Toggle visibility of editor windows overview in view menu"));
    focusWindowsMenu =
        viewMenu->addMenu(tr("Focus window", "Toggle visibility of editor windows overview in view menu"));

    viewMenu->addSeparator();

    connectMenu = menuBar()->addMenu(tr("&Connect"));

    // Setup actions
    setupFileActions();
    setupEditActions();
    setupSelectActions();
    setupFormatActions();
    setupViewActions();
    setupModeActions();
    setupNetworkActions();
    setupSettingsActions();
    setupConnectActions();
    setupContextMenus();
    setupMacros();
    setupToolbars();
    setupFlagActions();

    // Populate menus
    setupEditMenu();

    // Dock widgets ///////////////////////////////////////////////
    QDockWidget *dw;
    dw = new QDockWidget();
    dw->setWidget(noteEditor);
    dw->setObjectName("NoteEditor");
    dw->hide();
    noteEditorDW = dw;
    addDockWidget(Qt::LeftDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    dw = new QDockWidget();
    dw->setWidget(headingEditor);
    dw->setObjectName("HeadingEditor");
    dw->hide();
    headingEditorDW = dw;
    addDockWidget(Qt::BottomDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    findResultWidget = new FindResultWidget();
    dw = new QDockWidget(tr("Search results list", "FindResultWidget"));
    dw->setWidget(findResultWidget);
    dw->setObjectName("FindResultWidget");
    dw->hide();
    addDockWidget(Qt::RightDockWidgetArea, dw);
    connect(findResultWidget, SIGNAL(noteSelected(QString, int)), this,
            SLOT(selectInNoteEditor(QString, int)));
    connect(findResultWidget, SIGNAL(findPressed(QString, bool)), this,
            SLOT(editFindNext(QString, bool)));

    scriptEditor = new ScriptEditor(this);
    dw = new QDockWidget(tr("Script Editor", "ScriptEditor"));
    dw->setWidget(scriptEditor);
    dw->setObjectName("ScriptEditor");
    dw->hide();
    addDockWidget(Qt::LeftDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    scriptOutput = new ScriptOutput(this);
    dw = new QDockWidget(tr("Script output window"));
    dw->setWidget(scriptOutput);
    dw->setObjectName("ScriptOutput");
    dw->hide();
    addDockWidget(Qt::BottomDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    dw = new QDockWidget(tr("Property Editor", "PropertyEditor"));
    dw->setWidget(branchPropertyEditor);
    dw->setObjectName("PropertyEditor");
    dw->hide();
    addDockWidget(Qt::LeftDockWidgetArea, dw);
    branchPropertyEditorDW = dw;
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    historyWindow = new HistoryWindow();
    dw = new QDockWidget(tr("History window", "HistoryWidget"));
    dw->setWidget(historyWindow);
    dw->setObjectName("HistoryWidget");
    dw->hide();
    addDockWidget(Qt::RightDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    // Connect NoteEditor, so that we can update flags if text changes
    connect(noteEditor, SIGNAL(textHasChanged(VymText)), this,
            SLOT(updateNoteText(VymText)));

    // Connect heading editor
    connect(headingEditor, SIGNAL(textHasChanged(const VymText &)), this,
            SLOT(updateHeading(const VymText &)));

    connect(scriptEditor, SIGNAL(runScript(QString)), this,
            SLOT(runScript(QString)));

    // Create TaskEditor after setting up above actions, allow cloning
    taskEditor = new TaskEditor();
    dw = new QDockWidget(tr("Task list", "TaskEditor"));
    dw->setWidget(taskEditor);
    dw->setObjectName("TaskEditor");
    dw->hide();
    addDockWidget(Qt::TopDockWidgetArea, dw);
    connect(dw, SIGNAL(visibilityChanged(bool)), this, SLOT(satelliteVisibilityChanged()));

    if (options.isActive("shortcutsLaTeX"))
        switchboard.printLaTeX();

    if (settings.value("/mainwindow/showTestMenu", false).toBool())
        setupTestActions();
    setupHelpActions();

    // Status bar and progress bar there
    statusBar();
    progressMax = 0;
    progressCounter = 0;
    progressCounterTotal = 0;

    progressDialog.setAutoReset(false);
    progressDialog.setAutoClose(false);
    progressDialog.setMinimumWidth(600);
    // progressDialog.setWindowModality (Qt::WindowModal);   // That forces
    // mainwindo to update and slows down
    progressDialog.setCancelButton(nullptr);

    // Load window settings
    restoreState(settings.value("/mainwindow/state").toByteArray());
    restoreGeometry(settings.value("/mainwindow/geometry").toByteArray());

    updateGeometry();

    setTreeEditorsVisibility(settings.value("/mainwindow/view/showTreeEditors", true).toBool());
    setSlideEditorsVisibility(settings.value("/mainwindow/view/showSlideEditors", false).toBool());

    // After startup, schedule looking for updates AFTER
    // release notes have been downloaded
    // (avoid race condition with simultanously receiving cookies)
    checkUpdatesAfterReleaseNotes = true;

#if defined(VYM_DBUS)
    // Announce myself on DBUS
    new AdaptorVym(this); // Created and not deleted as documented in Qt
    if (!QDBusConnection::sessionBus().registerObject("/vym", this))
        qWarning("MainWindow: Couldn't register DBUS object!");
#endif

    // Allows a (test-)script to make vym quit after script execution
    exitAfterScriptInt = false;

    exitAfterLastMapClosed = false;
}

Main::~Main()
{
    // qDebug() << "Destr Mainwindow begin   testmode=" << testmode;

    // Make sure there is no focus elsewhere, e.g. in BranchPropertyEditor
    // which could cause a crash.  (Qt bug?)
    setFocus();

    // Save Settings

    //  FIXME-2 Save settings in destructor.  Maybe move to separate function and call more often
    if (!testmode) {
        settings.setValue("/mainwindow/geometry", saveGeometry());
        settings.setValue("/mainwindow/state", saveState()); // FIXME-3 use restoreDockWidget
                                                             // https://doc.qt.io/qt-6/qmainwindow.html#saveState

        settings.setValue("/mainwindow/view/AntiAlias",
                          actionViewToggleAntiAlias->isChecked());
        settings.setValue("/mainwindow/view/SmoothPixmapTransformations",
                          actionViewToggleSmoothPixmapTransform->isChecked());
        settings.setValue("/system/autosave/use",
                          actionSettingsToggleAutosave->isChecked());
        settings.setValue("/system/autosave/ms",
                          settings.value("/system/autosave/ms", 60000));
        settings.setValue("/mainwindow/autoLayout/use",
                          actionSettingsToggleAutoLayout->isChecked());
        settings.setValue("/mapeditor/editmode/autoSelectNewBranch",
                          actionSettingsAutoSelectNewBranch->isChecked());
        settings.setValue("/system/writeBackupFile",
                          actionSettingsWriteBackupFile->isChecked());

        settings.setValue("/logfile/enabled", useActionLog);
        settings.setValue("/logfile/path", actionLogPath);

        if (printer) {
            settings.setValue("/system/printerName", printer->printerName());
            settings.setValue("/system/printerFormat", printer->outputFormat());
            settings.setValue("/system/printerFileName",
                              printer->outputFileName());
        }
        settings.setValue("/mapeditor/editmode/useFlagGroups",
                          actionSettingsUseFlagGroups->isChecked());
        settings.setValue("/export/useHideExport",
                          actionSettingsUseHideExport->isChecked());
        settings.setValue("/system/version", vymVersion);
        settings.setValue("/system/builddate", vymBuildDate);
    }

    // call the destructors
    delete noteEditorDW;
    delete historyWindow;
    delete branchPropertyEditorDW;

    delete standardFlagsMaster;
    delete userFlagsMaster;
    delete systemFlagsMaster;

    // qDebug() << "Destr Mainwindow end";
}

void Main::loadCmdLine()
{
    QStringList flist = options.getFileList();
    QStringList::Iterator it = flist.begin();

    initProgressCounter(flist.count());
    while (it != flist.end()) {
        File::FileType type = getMapType(*it);
        fileLoad(*it, File::NewMap, type);
        *it++;
    }
    removeProgressCounter();
}

void Main::logInfo(const QString &comment, const QString &caller)   // FIXME-2 Replace everywhere with log.cpp ...
{
    if (!useActionLog) return;

    QString c;
    if (!caller.isEmpty())
        c = "::" + caller;

    QString log = QString("\n// %1 [Info MainWindow%2] %3\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs), c, comment);

    // std::cout << log.toStdString() << std::endl << std::flush;

    appendStringToFile(actionLogPath, log);
}

void Main::statusMessage(const QString &s, int timeout)
{
    // Surpress messages while progressdialog during
    // load is active
    statusBar()->showMessage(s, timeout);
    statusBar()->update();
    qApp->processEvents();
}

void Main::setProgressMaximum(int max)
{
    if (progressCounter == 0) {
        // Init range only on first time, when progressCounter still 0
        // Normalize range to 1000
        progressDialog.setRange(0, 1000);
        progressDialog.setValue(1);
    }
    progressCounter++; // Another map is loaded

    progressMax = max * 1000;
    QApplication::processEvents();
}

void Main::addProgressValue(float v)

{
    int progress_value =
        (v + progressCounter - 1) * 1000 / progressCounterTotal;
    /*
        qDebug() << "addVal v="<<v
         <<"  cur="<<progressDialog.value()
         <<"  pCounter="<<progressCounter
         <<"  pCounterTotal="<<progressCounterTotal
             <<"  newv="<< progress_value
         ;
         */

    // Make sure the progress dialog shows, even if value == 0
    if (progress_value < 1)
        progress_value = 1;
    progressDialog.setValue(progress_value);
    if (progress_value == 1)
        QApplication::processEvents();
}

void Main::initProgressCounter(uint n) { progressCounterTotal = n; }

void Main::removeProgressCounter()
{
    // Hide dialog again
    progressCounter = 0;
    progressCounterTotal = 0;
    progressDialog.reset();
    progressDialog.hide();
}

void Main::satelliteVisibilityChanged()
{
    updateActions();
}

void Main::closeEvent(QCloseEvent *event)
{
    fileExitVym();
    event->ignore();
}

QPrinter *Main::setupPrinter()
{
    // Global Printer
    printer = new QPrinter(QPrinter::HighResolution);
    return printer;
}

// Define commands for models
void Main::setupAPI()
{
    //
    // Below are the commands for vym itself
    //

    Command *c = new Command("callMacro", Command::AnySel);
    c->addParameter(Command::StringPar, false, "Include macros and run script");
    vymCommands.append(c);

    c = new Command("clearConsole", Command::AnySel);
    vymCommands.append(c);

    c = new Command("closeMapWithId", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Id of map (unsigned int)");
    vymCommands.append(c);

    c = new Command("currentColor", Command::AnySel);
    vymCommands.append(c);

    c = new Command("currentMap", Command::AnySel, Command::VymModelPar);
    vymCommands.append(c);

    c = new Command("currentMapIndex", Command::AnySel);
    vymCommands.append(c);

    c = new Command("editHeading", Command::BranchSel);
    vymCommands.append(c);

    c = new Command("exit", Command::AnySel);
    c->setComment("Exit vym after script execution");
    vymCommands.append(c);

    c = new Command("gotoMap", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Index of map");
    vymCommands.append(c);

    c = new Command("loadMap", Command::AnySel);
    c->addParameter(Command::StringPar, false, "Path to map");
    vymCommands.append(c);

    c = new Command("mapCount", Command::AnySel);
    vymCommands.append(c);

    c = new Command("mapWithId", Command::AnySel, Command::VymModelPar);
    c->addParameter(Command::IntPar, false, "unique id of map");
    vymCommands.append(c);

    c = new Command("print", Command::AnySel);
    c->setComment("Print string to console");
    c->addParameter(Command::StringPar, false, "String to print");
    vymCommands.append(c);

    c = new Command("printCol", Command::AnySel);
    c->setComment("Print string to console with ANSI color");
    c->addParameter(Command::StringPar, false, "String to print");
    c->addParameter(Command::StringPar, false, "Color [red|green|yellow|blue|magenta|cyan|white]");
    vymCommands.append(c);

    c = new Command("selectQuickColor", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Index of quick color [0..6]");
    vymCommands.append(c);

    c = new Command("usesDarkTheme", Command::AnySel, Command::BoolPar);
    vymCommands.append(c);

    c = new Command("version", Command::AnySel);
    vymCommands.append(c);

    //
    // Below are the commands for a map
    //

    c = new Command("addMapCenterAtPos", Command::AnySel);
    c->addParameter(Command::DoublePar, false, "Position x");
    c->addParameter(Command::DoublePar, false, "Position y");
    c->setComment("Add MapCenter at position (x, y)");
    modelCommands.append(c);

    c = new Command("addSlide", Command::BranchSel);
    modelCommands.append(c);

    c = new Command("addXLink", Command::BranchLikeSel);
    c->addParameter(Command::StringPar, false, "End of XLink");
    c->addParameter(Command::IntPar, true, "Width of XLink");
    c->addParameter(Command::ColorPar, true, "Color of XLink");
    c->addParameter(Command::StringPar, true, "Penstyle of XLink");
    c->setComment("Add xlink from this branch to another branch");
    branchCommands.append(c);

    c = new Command("centerCount", Command::BranchLikeSel, Command::IntPar);
    modelCommands.append(c);

    c = new Command("centerOnID", Command::AnySel);
    c->addParameter(Command::StringPar, false, "UUID of object to center on");
    modelCommands.append(c);

    c = new Command("copy", Command::BranchOrImageSel);
    modelCommands.append(c);

    c = new Command("cut", Command::BranchOrImageSel);
    modelCommands.append(c);

    c = new Command("deleteAttribute", Command::BranchSel, Command::IntPar);
    c->setComment("Delete attribute with given key");
    c->addParameter(Command::StringPar, false, "Key of attribute to delete");
    modelCommands.append(c);

    c = new Command("deleteConfluencePageLabel", Command::BranchSel, Command::IntPar);
    c->setComment("Delete label from Confluence page given in branch attributes");
    c->addParameter(Command::StringPar, false, "Label to be removed from Confluence page");
    modelCommands.append(c);

    c = new Command("exportMap", Command::AnySel, Command::BoolPar);
    c->addParameter(Command::ArrayPar, false,
              "Format (AO, ASCII, CONFLUENCE, CSV, HTML, Image, Impress, Last, "
              "LaTeX, Markdown, OrgMode, PDF, SVG, XML)");
    c->setComment("Export map in various formats. Format, filename, destination can be passed as array");
    modelCommands.append(c);

    c = new Command("findBranchByAttribute", Command::AnySel, Command::BranchPar);
    c->setComment("Find branch with given key/value pair. "
            "Returns first hit or null.");
    c->addParameter(Command::StringPar, false, "Key of attribute");
    c->addParameter(Command::StringPar, false, "Value of attribute");
    modelCommands.append(c);

    c = new Command("findAttributeById", Command::AnySel, Command::AttributePar);
    c->setComment("Find Attribute with given unique Uuid. ");
    c->addParameter(Command::StringPar, false, "Uuid of attribute");
    modelCommands.append(c);

    c = new Command("findBranchById", Command::AnySel, Command::BranchPar);
    c->setComment("Find branch with given unique Uuid. ");
    c->addParameter(Command::StringPar, false, "Uuid of branch");
    modelCommands.append(c);

    c = new Command("findBranchBySelection", Command::AnySel, Command::BranchPar);
    c->setComment("Find branch with given selection string. ");
    c->addParameter(Command::StringPar, false, "Selection string");
    modelCommands.append(c);

    c = new Command("findImageById", Command::AnySel, Command::ImagePar);
    c->setComment("Find image with given unique Uuid. ");
    c->addParameter(Command::StringPar, false, "Uuid of image");
    modelCommands.append(c);

    c = new Command("findXLinkById", Command::AnySel, Command::XLinkPar);
    c->setComment("Find xlink given unique Uuid. ");
    c->addParameter(Command::StringPar, false, "Uuid of xlink");
    modelCommands.append(c);

    c = new Command("getBackgroundColor", Command::AnySel, Command::StringPar);
    c->setComment("Get color of map background");
    modelCommands.append(c);

    c = new Command("getDestPath", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getFileDir", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getFileName", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getIntAttribute", Command::BranchSel, Command::IntPar);
    c->addParameter(Command::StringPar, false, "Key of string attribute");
    modelCommands.append(c);

    c = new Command("getAuthor", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getComment", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getTitle", Command::AnySel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getLinkColorHint", Command::AnySel);
    c->setComment("Returns 'DefaultColor' for links or 'HeadingColor'");
    modelCommands.append(c);

    c = new Command("getSelectionString", Command::TreeItemSel, Command::StringPar);
    modelCommands.append(c);

    c = new Command("getZoom", Command::AnySel, Command::DoublePar);
    modelCommands.append(c);

    c = new Command("hasBackgroundImage", Command::AnySel, Command::BoolPar);
    c->setComment("Returns true, if map uses an image as background");
    modelCommands.append(c);

    c = new Command("isBusy", Command::AnySel);
    c->setComment("Returns true while map is saving or loading");
    modelCommands.append(c);

    c = new Command("itemList", Command::AnySel, Command::BoolPar);
    c->addParameter(Command::BoolPar, true, "Flag to go deep levels first (currently unused)");
    c->setComment("Create new itemList to iterate over branches");
    modelCommands.append(c);

    c = new Command("loadBackgroundImage", Command::AnySel);
    c->setComment("Load background image");
    c->addParameter(Command::StringPar, false, "Path to background iamge");
    modelCommands.append(c);

    c = new Command("loadBranchReplace", Command::AnySel, Command::BoolPar);
    c->addParameter(Command::StringPar, false, "Filename of map to load");
    c->addParameter(Command::BranchPar, false, "Branch to be replaced by map");
    c->setComment("Replace branch with data from given path");
    modelCommands.append(c);

    c = new Command("moveSlideDown", Command::AnySel);
    modelCommands.append(c);

    c = new Command("moveSlideUp", Command::AnySel);
    modelCommands.append(c);

    c = new Command("move", Command::BranchOrImageSel);
    c->addParameter(Command::DoublePar, false, "Position x");
    c->addParameter(Command::DoublePar, false, "Position y");
    modelCommands.append(c);

    c = new Command("moveRel", Command::BranchOrImageSel);
    c->addParameter(Command::DoublePar, false, "Position x");
    c->addParameter(Command::DoublePar, false, "Position y");
    modelCommands.append(c);

    c = new Command("nextBranch", Command::BranchSel, Command::BranchPar);
    c->addParameter(Command::StringPar, false, "Name of iterator");
    modelCommands.append(c);

    c = new Command("paste", Command::BranchSel);
    modelCommands.append(c);

    c = new Command("redo", Command::AnySel);
    modelCommands.append(c);

    c = new Command("remove", Command::TreeItemSel);
    modelCommands.append(c);

    c = new Command("removeAttribute", Command::AnySel);
    c->addParameter(Command::AttributePar, false, "Attribute to be removed");
    c->setComment("Remove attribute");
    modelCommands.append(c);

    c = new Command("removeBranch", Command::AnySel);
    c->addParameter(Command::BranchPar, false, "Branch to be removed");
    c->setComment("Remove branch");
    modelCommands.append(c);

    c = new Command("removeImage", Command::AnySel);
    c->addParameter(Command::ImagePar, false, "Branch to be removed");
    c->setComment("Remove image");
    modelCommands.append(c);

    c = new Command("removeKeepChildren", Command::BranchSel);
    c->setComment("Remove branch but keep its children");
    modelCommands.append(c);

    c = new Command("removeSlide", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Index of slide to remove");
    modelCommands.append(c);

    c = new Command("repeatLastAction", Command::AnySel);
    modelCommands.append(c);

    c = new Command("resetBranchIterator", Command::BranchSel, Command::BoolPar);
    c->addParameter(Command::StringPar, false, "Name of iterator");
    c->setComment("Move iterator to first MapCenter in map");
    modelCommands.append(c);

    c = new Command("saveSelection", Command::BranchOrImageSel);
    c->addParameter(Command::StringPar, false, "Filename to save branch or image");
    modelCommands.append(c);

    c = new Command("select", Command::AnySel, Command::BoolPar);
    c->addParameter(Command::StringPar, false, "Selection string");
    modelCommands.append(c);

    c = new Command("selectedBranch", Command::AnySel, Command::BranchPar);
    modelCommands.append(c);

    c = new Command("selectedBranches", Command::AnySel, Command::BranchListPar);
    modelCommands.append(c);

    c = new Command("selectLatestAdded", Command::AnySel, Command::BoolPar);
    modelCommands.append(c);

    c = new Command("setAnimCurve", Command::AnySel);
    c->addParameter(Command::IntPar, false,
              "EasingCurve used in animation in MapEditor");
    modelCommands.append(c);

    c = new Command("setAuthor", Command::AnySel);
    c->addParameter(Command::StringPar, false, "");
    modelCommands.append(c);

    c = new Command("setAnimDuration", Command::AnySel);
    c->addParameter(Command::IntPar, false,
              "Duration of animation in MapEditor in milliseconds");
    modelCommands.append(c);

    c = new Command("setBackgroundColor", Command::AnySel);
    c->setComment("Set color of map background and use it instead of an image");
    c->addParameter(Command::ColorPar, false, "Color of map background");
    modelCommands.append(c);

    c = new Command("setComment", Command::AnySel);
    c->addParameter(Command::StringPar, false, "");
    modelCommands.append(c);

    c = new Command("setTitle", Command::AnySel);
    c->addParameter(Command::StringPar, false, "");
    modelCommands.append(c);

    c = new Command("setDefaultLinkColor", Command::AnySel);
    c->addParameter(Command::ColorPar, false, "Default color of links");
    modelCommands.append(c);

    c = new Command("setLinkColorHint", Command::AnySel);
    c->addParameter(Command::StringPar, false, "Name of color hint to use");
    c->setComment("Use 'DefaultColor' for links or 'HeadingColor'");
    modelCommands.append(c);

    c = new Command("setLinkStyle", Command::AnySel);
    c->addParameter(Command::StringPar, false, "Link style in map");
    c->addParameter(Command::IntPar, true, "Depth of branch with this style");
    modelCommands.append(c);

    c = new Command("setRotationView", Command::AnySel);
    c->addParameter(Command::DoublePar, false, "Rotation of view of map");
    modelCommands.append(c);

    c = new Command("setSelectionBrushColor", Command::AnySel);
    c->addParameter(Command::ColorPar, false, "Color of selection box background");
    modelCommands.append(c);

    c = new Command("setSelectionColor", Command::AnySel);
    c->addParameter(Command::ColorPar, false, "Color of selection box");
    modelCommands.append(c);

    c = new Command("setSelectionPenColor", Command::AnySel);
    c->addParameter(Command::ColorPar, false, "Color of selection box border");
    modelCommands.append(c);

    c = new Command("setSelectionPenWidth", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Selection box border width ");
    modelCommands.append(c);

    c = new Command("setTitle", Command::AnySel);
    c->addParameter(Command::StringPar, false, "");
    modelCommands.append(c);

    c = new Command("setSaveAsBackgroundProcess", Command::AnySel);
    c->addParameter(Command::BoolPar, false, "Enable (default) or disable background saving");
    c->setComment("Use background progress to save maps or selections");
    modelCommands.append(c);

    c = new Command("setZoom", Command::AnySel);
    c->addParameter(Command::DoublePar, false, "Zoomfactor of map");
    modelCommands.append(c);

    modelCommands.append(c);

    c = new Command("sleep", Command::AnySel);
    c->addParameter(Command::IntPar, false, "Sleep (seconds)");
    modelCommands.append(c);

    c = new Command("slideCount", Command::AnySel, Command::IntPar);
    modelCommands.append(c);

    c = new Command("toggleFlagByUid", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Uid of flag to toggle");
    modelCommands.append(c);

    c = new Command("undo", Command::AnySel);
    modelCommands.append(c);

    c = new Command("unselectAll", Command::AnySel);
    modelCommands.append(c);

    c = new Command("unsetBackgroundImage", Command::AnySel);
    c->setComment("Use background color instead of background image");
    c->addParameter(Command::ColorPar, false, "Background color");
    modelCommands.append(c);


    //
    // Below are the commands for a branch
    //

    c = new Command("addBranch", Command::BranchSel);
    c->setComment("Add branch as child branch to current branch");
    branchCommands.append(c);

    c = new Command("addBranchAt", Command::BranchSel);
    c->setComment("Add branch at position to current branch");
    c->addParameter(Command::IntPar, true, "Index of new branch");
    branchCommands.append(c);

    c = new Command("addBranchBefore", Command::BranchSel);
    c->setComment("Add branch as parent before current branch");
    branchCommands.append(c);

    c = new Command("attributeAsInt", Command::BranchSel, Command::IntPar);
    c->setComment("Get integer value of attribute with given key");
    c->addParameter(Command::StringPar, false, "Key of integer attribute");
    branchCommands.append(c);

    c = new Command("attributeAsString", Command::BranchSel, Command::StringPar);
    c->setComment("Get string value of attribute with given key");
    c->addParameter(Command::StringPar, false, "Key of string attribute");
    branchCommands.append(c);

    c = new Command("branchCount", Command::BranchSel, Command::IntPar);
    c->setComment("Return number of child branches");
    branchCommands.append(c);

    c = new Command("clearFlags", Command::BranchSel);
    c->setComment("Clear all flags of branch");
    branchCommands.append(c);

    c = new Command("colorBranch", Command::BranchSel);
    c->addParameter(Command::ColorPar, true, "New color");
    c->setComment("Set color of heading of branch");
    branchCommands.append(c);

    c = new Command("colorSubtree", Command::BranchSel);
    c->addParameter(Command::ColorPar, true, "New color");
    c->setComment("Set color of headings of all child branches and all their children");
    branchCommands.append(c);

    c = new Command("cycleTask", Command::BranchOrImageSel, Command::BoolPar);
    c->addParameter(Command::BoolPar, true, "Flag to cycle in reverse order");
    c->setComment("Cycle states of task in branch. Returns false, if branch has no task");
    branchCommands.append(c);

    c = new Command("deleteConfluencePageLabel", Command::BranchSel);
    c->addParameter(Command::StringPar, true, "Label name");
    c->setComment("Remove label from the a branch with Confluence page details");
    branchCommands.append(c);

    c = new Command("depth", Command::BranchOrImageSel, Command::IntPar);
    c->setComment("Return depth of branch");
    branchCommands.append(c);

    c = new Command("detach", Command::BranchSel);
    c->setComment("Detach branch and turn it into a map center");
    branchCommands.append(c);

    // Same parameter for all frame commands
    QString useInnerFrameDesc = "Use setting for heading if true, for subtree if false";

    c = new Command("getFrameAutoDesign", Command::BranchSel, Command::BoolPar);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->setComment("Flag if automatic design is used for frame of heading or subtree");
    branchCommands.append(c);

    c = new Command("getFrameBrushColor", Command::BranchSel, Command::StringPar);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->setComment("Get color of frame background");
    branchCommands.append(c);

    c = new Command("getFramePadding", Command::BranchSel, Command::IntPar);
    c->setComment("Get padding between frame border and heading");
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    branchCommands.append(c);

    c = new Command("getFramePenColor", Command::BranchSel, Command::StringPar);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->setComment("Get color of frame border");
    branchCommands.append(c);

    c = new Command("getFramePenWidth", Command::BranchSel, Command::IntPar);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->setComment("Get width of frame pen");
    branchCommands.append(c);

    c = new Command("getFrameType", Command::BranchSel, Command::StringPar);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->setComment("Get frame type");
    branchCommands.append(c);

    c = new Command("getHeading", Command::BranchSel, Command::StringPar);
    c->setComment("Get heading of branch as text");
    c->addParameter(Command::BoolPar, false, "Use setting for heading if true, for subtree if false");
    branchCommands.append(c);

    c = new Command("getHeadingColor", Command::BranchSel, Command::StringPar);
    c->setComment("Get color of heading as text");
    branchCommands.append(c);

    c = new Command("getHeadingXML", Command::BranchSel, Command::StringPar);
    c->setComment("Get heading of branch as XML");
    branchCommands.append(c);

    c = new Command("getUid", Command::BranchSel, Command::StringPar);
    c->setComment("Get Uuid of branch as string");
    branchCommands.append(c);

    c = new Command("getJiraData", Command::BranchSel, Command::StringPar);
    c->setComment("Get data from Jira server, either ticket or run defined query");
    c->addParameter(Command::BoolPar, false, "Update every branch in subtree");
    branchCommands.append(c);

    c = new Command("getNum", Command::BranchSel, Command::IntPar);
    c->setComment("Return position of branch in subtree");
    branchCommands.append(c);

    c = new Command("getPosX", Command::TreeItemSel);
    c->setComment("get x position of branch relative to parent");
    branchCommands.append(c);

    c = new Command("getPosY", Command::TreeItemSel);
    c->setComment("get y position of branch relative to parent");
    branchCommands.append(c);

    c = new Command("getRotationHeading", Command::BranchSel);
    c->setComment("get rotation of heading");
    branchCommands.append(c);

    c = new Command("getRotationSubtree", Command::BranchSel);
    c->setComment("get rotation of subttree");
    branchCommands.append(c);

    c = new Command("getScenePos", Command::BranchSel);
    c->setComment("get position of branch in scene coordinates");
    branchCommands.append(c);

    c = new Command("getScenePosX", Command::BranchSel);
    c->setComment("get x position of branch in scene coordinates");
    branchCommands.append(c);

    c = new Command("getScenePosY", Command::BranchSel);
    c->setComment("get y position of branch in scene coordinates");
    branchCommands.append(c);

    c = new Command("getTaskPriorityDelta", Command::BranchSel, Command::IntPar);
    c->setComment("Return delta of priority of task");
    branchCommands.append(c);

    c = new Command("getTaskSleep", Command::BranchSel, Command::StringPar);
    c->setComment("Return sleep time of task");
    branchCommands.append(c);

    c = new Command("getTaskSleepDays", Command::BranchSel, Command::IntPar);
    c->setComment("Return sleep time of task in days");
    branchCommands.append(c);

    c = new Command("getTaskStatus", Command::BranchSel, Command::StringPar);
    c->setComment("Return status of task");
    branchCommands.append(c);

    c = new Command("getUrl", Command::BranchSel, Command::StringPar);
    c->setComment("Return  url of branch");
    branchCommands.append(c);

    c = new Command("getVymLink", Command::BranchSel, Command::StringPar);
    c->setComment("Get vymLink of branch");
    branchCommands.append(c);

    c = new Command("hasActiveFlag", Command::TreeItemSel, Command::BoolPar);
    c->addParameter(Command::StringPar, false, "Name of flag");
    c->setComment("Check if branch has an active flag with given name");
    branchCommands.append(c);

    c = new Command("hasAttributeWithKey", Command::TreeItemSel, Command::BoolPar);
    c->addParameter(Command::StringPar, false, "Key");
    c->setComment("Check if branch has an attribute with given key");
    branchCommands.append(c);

    c = new Command("hasNote", Command::BranchSel, Command::BoolPar);
    c->setComment("Check if branch has a note");
    branchCommands.append(c);

    c = new Command("hasRichTextHeading", Command::BranchSel, Command::BoolPar);
    c->setComment("Check if branch has a RichText heading or just plain text");
    branchCommands.append(c);

    c = new Command("hasTask", Command::BranchSel, Command::BoolPar);
    branchCommands.append(c);

    c = new Command("headingText", Command::BranchSel, Command::StringPar);
    c->setComment("Set heading of branch from plaintext string");
    branchCommands.append(c);

    c = new Command("imageCount", Command::BranchSel, Command::IntPar);
    c->setComment("Return number of child images");
    branchCommands.append(c);

    c = new Command("importDir", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Directory name to import");
    c->setComment("Add directory structure to branch (experimental)");
    branchCommands.append(c);

    c = new Command("isScrolled", Command::BranchSel, Command::BoolPar);
    c->setComment("Check if branch is scrolled");
    branchCommands.append(c);

    c = new Command("loadBranchInsert", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Filename of map to load");
    c->addParameter(Command::IntPar, true, "Index where map is inserted");
    c->setComment("Insert branch with given path to branch at index");
    branchCommands.append(c);

    c = new Command("loadImage", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Filename of image");
    c->setComment("Load an image with given path and attach to branch");
    branchCommands.append(c);

    c = new Command("loadNote", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Filename of note");
    c->setComment("Load a note with given path and attach to branch");
    branchCommands.append(c);

    c = new Command("moveDown", Command::BranchSel);
    c->setComment("Move branch down");
    branchCommands.append(c);

    c = new Command("moveUp", Command::BranchSel);
    c->setComment("Move branch up");
    branchCommands.append(c);

    c = new Command("note2URLs", Command::BranchSel);
    c->setComment("Extract URLs from note of branch");
    branchCommands.append(c);

    c = new Command("relinkToBranch", Command::BranchSel);
    c->setComment("Relink branch to destination branch");
    c->addParameter(Command::BranchPar, false, "Destination branch");
    branchCommands.append(c);

    c = new Command("relinkToBranchAt", Command::BranchSel);
    c->setComment("Relink branch to destination branch at position");
    c->addParameter(Command::BranchPar, false, "Destination branch");
    c->addParameter(Command::IntPar, false, "Position (0 is first)");
    branchCommands.append(c);

    c = new Command("removeChildren", Command::BranchSel);
    c->setComment("Remove all children of branch");
    branchCommands.append(c);

    c = new Command("removeChildrenBranches", Command::BranchSel);
    c->setComment("Remove all children branches of branch");
    branchCommands.append(c);

    c = new Command("saveNote", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Filename of note to save");
    branchCommands.append(c);

    c = new Command("scroll", Command::BranchSel);
    c->setComment("Scroll branch");
    branchCommands.append(c);

    c = new Command("select", Command::BranchSel);
    c->setComment("Select (only) this branch");
    branchCommands.append(c);

    c = new Command("selectFirstBranch", Command::BranchSel, Command::BoolPar);
    c->setComment("Select the first of all sibling branches");
    branchCommands.append(c);

    c = new Command("selectFirstChildBranch", Command::BranchSel, Command::BoolPar);
    c->setComment("Select the first of all child branches");
    branchCommands.append(c);

    c = new Command("selectLastChildBranch", Command::BranchSel, Command::BoolPar);
    branchCommands.append(c);

    c = new Command("selectLastBranch", Command::BranchSel, Command::BoolPar);
    c->setComment("Select the last of all sibling branches");
    branchCommands.append(c);

    c = new Command("selectParent", Command::BranchSel, Command::BoolPar);
    c->setComment("Select parent of branch");
    branchCommands.append(c);

    c = new Command("selectXLink", Command::BranchSel, Command::BoolPar);
    c->addParameter(Command::IntPar, false, "Number of xlink");
    c->setComment("Select the xlink with given index attached to this branch");
    branchCommands.append(c);

    c = new Command("selectXLinkOtherEnd", Command::BranchSel, Command::BoolPar);
    c->addParameter(Command::IntPar, false, "Number of xlink");
    c->setComment("Select the branch, which is the the other end of xlink with given index attached to this branch");
    branchCommands.append(c);

    c = new Command("setAttribute", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Key of attribute as string");
    c->addParameter(Command::StringPar, false, "String Value of attribute");
    branchCommands.append(c);

    c = new Command("setFlagByName", Command::TreeItemSel);
    c->setComment("Set flag of branch by string with name of flag");
    c->addParameter(Command::StringPar, false, "Name of flag");
    branchCommands.append(c);

    c = new Command("setBranchesLayout", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Layout of branches in subtree");
    branchCommands.append(c);

    c = new Command("setFrameAutoDesign", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->addParameter(Command::BoolPar, false, "Flag for using automatic frame design");
    c->setComment("Toggle automatic frame design");
    branchCommands.append(c);

    c = new Command("setFrameType", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, useInnerFrameDesc);
    c->addParameter(Command::StringPar, false, "Type of frame");
    c->setComment("Set type of frame");
    branchCommands.append(c);

    c = new Command("setFramePenColor", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Use setting for heading if true, for subtree if false");
    c->addParameter(Command::ColorPar, false, "Color of frame border line");
    c->setComment("Set color of frame border");
    branchCommands.append(c);

    c = new Command("setFrameBrushColor", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Use setting for heading if true, for subtree if false");
    c->addParameter(Command::ColorPar, false, "Color of frame background");
    c->setComment("Set color of frame background");
    branchCommands.append(c);

    c = new Command("setFramePadding", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Use setting for heading if true, for subtree if false");
    c->addParameter(Command::IntPar, false, "Padding around frame");
    c->setComment("Set padding of frame");
    branchCommands.append(c);

    c = new Command("setFramePenWidth", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Use setting for heading if true, for subtree if false");
    c->addParameter(Command::IntPar, false, "Width of frame pen");
    c->setComment("Set width of frame border");
    branchCommands.append(c);

    c = new Command("setHeadingConfluencePageName", Command::BranchSel);    // FIXME-3 adapt name and add recursie command
    c->setComment("Get page name and details for URL in branch from Confluence");
    branchCommands.append(c);

    c = new Command("setHeadingRichText", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "New heading");
    c->setComment("Set heading of branch as HTML-like string");
    branchCommands.append(c);

    c = new Command("setHeadingText", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "New heading");
    c->setComment("Set heading of branch as plain text string");
    branchCommands.append(c);

    c = new Command("setHideExport", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Set if branch should be visible in export");
    branchCommands.append(c);

    c = new Command("setHideLinksUnselected", Command::BranchSel);
    c->addParameter(Command::BoolPar, false,
              "Set if links of items should be visible for unselected items");
    branchCommands.append(c);

    c = new Command("setImagesLayout", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Layout of images");
    branchCommands.append(c);

    c = new Command("setNoteRichText", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Note of branch");
    branchCommands.append(c);

    c = new Command("setNoteText", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Note of branch");
    c->setComment("Set note of branch to plain text");
    branchCommands.append(c);

    c = new Command("setOnlyFlags", Command::BranchSel);
    c->addParameter(Command::ArrayPar, false, "Array with Uuids of flags to be set");
    c->setComment("Set only given flags in branch (used for undo)");
    branchCommands.append(c);

    c = new Command("setPos", Command::BranchSel);
    c->addParameter(Command::DoublePar, false, "Position x");
    c->addParameter(Command::DoublePar, false, "Position y");
    c->setComment("Set position of branch to (x,y) in local coordinates");
    branchCommands.append(c);

    c = new Command("setRotationAutoDesign", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Rotate automatically");
    branchCommands.append(c);

    c = new Command("setRotationHeading", Command::BranchSel);
    c->addParameter(Command::IntPar, false, "Rotation angle of heading and flags");
    branchCommands.append(c);

    c = new Command("setRotationSubtree", Command::BranchSel);
    c->addParameter(Command::IntPar, false, "Rotation angle of heading and subtree");
    branchCommands.append(c);

    c = new Command("setScaleAutoDesign", Command::BranchSel);
    c->addParameter(Command::BoolPar, false, "Scale automatically");
    branchCommands.append(c);

    c = new Command("setScaleHeading", Command::BranchOrImageSel);
    c->addParameter(Command::DoublePar, false, "Scale heading of branch by factor f");
    c->setComment("Scale heading of a branch");
    branchCommands.append(c);

    c = new Command("setScaleSubtree", Command::BranchSel);
    c->addParameter(Command::DoublePar, false, "Scale subtree by factor f");
    c->setComment("Scale branch and its children");
    branchCommands.append(c);

    c = new Command("setTaskPriorityDelta", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Manually add value to priority of task");
    c->setComment("Set value to be added on priority of task");
    branchCommands.append(c);

    c = new Command("setTaskSleep", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Days to sleep");
    c->setComment("Set how long task should sleep");
    branchCommands.append(c);

    c = new Command("setUrl", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Url of TreeItem");
    c->setComment("Set Url of branch");
    branchCommands.append(c);

    c = new Command("setVymLink", Command::BranchSel);
    c->addParameter(Command::StringPar, false, "Vymlink of branch");
    c->setComment("Set VymLink of branch");
    branchCommands.append(c);

    c = new Command("setXLinkColor", Command::XLinkSel);
    c->addParameter(Command::StringPar, false, "Color of xlink");
    c->setComment("Set color of xlink");
    branchCommands.append(c);

    c = new Command("setXLinkStyle", Command::XLinkSel);
    c->addParameter(Command::StringPar, false, "Style of xlink");
    c->setComment("Set style of xlink");
    branchCommands.append(c);

    c = new Command("setXLinkStyleBegin", Command::XLinkSel);
    c->addParameter(Command::StringPar, false, "Style of xlink begin");
    c->setComment("Set begin style of xlink");
    branchCommands.append(c);

    c = new Command("setXLinkStyleEnd", Command::XLinkSel);
    c->addParameter(Command::StringPar, false, "Style of xlink end");
    c->setComment("Set end style of xlink");
    branchCommands.append(c);

    c = new Command("setXLinkWidth", Command::XLinkSel);
    c->addParameter(Command::IntPar, false, "Width of xlink");
    c->setComment("Set width of xlink");
    branchCommands.append(c);

    c = new Command("sortChildren", Command::BranchSel);
    c->addParameter(Command::BoolPar, true,
              "Sort children of branch in revers order if set");
    c->setComment("Sort children of branch");
    branchCommands.append(c);

    c = new Command("toggleFlagByName", Command::BranchSel);
    c->setComment("Toggle flag of branch by string with name of flag");
    c->addParameter(Command::StringPar, false, "Name of flag to toggle");
    branchCommands.append(c);

    c = new Command("toggleScroll", Command::BranchSel);
    c->setComment("Toggle scroll state of branch");
    branchCommands.append(c);

    c = new Command("toggleTarget", Command::BranchSel);
    c->setComment("Toggle target flag of branch");
    branchCommands.append(c);

    c = new Command("toggleTask", Command::BranchSel);
    c->setComment("Set if branch should or should not have a task");
    branchCommands.append(c);

    c = new Command("unscroll", Command::BranchSel);
    c->setComment("Unscroll branch");
    branchCommands.append(c);

    c = new Command("unscrollSubtree", Command::BranchSel);
    c->setComment("Unscroll branch and all children in its subtree");
    branchCommands.append(c);

    c = new Command("unsetFlagByName", Command::BranchSel);
    c->setComment("Unset flag of branch by string with name of flag");
    c->addParameter(Command::StringPar, false, "Name of flag to unset");
    branchCommands.append(c);

    c = new Command("xlinkCount", Command::BranchSel, Command::IntPar);
    c->setComment("Return number of xlinks connected to branch");
    branchCommands.append(c);

    //
    // Below are the commands for an image
    //
    c = new Command("hasRichTextHeading", Command::ImageSel, Command::BoolPar);
    c->setComment("Check if image has a RichText heading or just plain text");
    imageCommands.append(c);

    c = new Command("getPosX", Command::ImageSel);
    c->setComment("Get x position of image relative to parent");
    imageCommands.append(c);

    c = new Command("getPosY", Command::ImageSel);
    c->setComment("Get y position of image relative to parent");
    imageCommands.append(c);

    c = new Command("getScale", Command::ImageSel);
    c->setComment("Get x scale factor of image");
    imageCommands.append(c);

    c = new Command("getScenePosX", Command::ImageSel);
    c->setComment("Get x position of image in scene coordinates");
    imageCommands.append(c);

    c = new Command("getScenePosY", Command::ImageSel);
    c->setComment("Get y position of image in scene coordinates");
    imageCommands.append(c);

    c = new Command("headingText", Command::ImageSel, Command::StringPar);
    c->setComment("Set heading of image from plaintext string");
    imageCommands.append(c);

    c = new Command("saveImage", Command::ImageSel);
    c->addParameter(Command::StringPar, false, "Filename of image to save");
    c->addParameter(Command::StringPar, false, "Format of image to save");
    imageCommands.append(c);

    c = new Command("relinkToBranch", Command::ImageSel);
    c->setComment("Relink image to destination branch");
    c->addParameter(Command::BranchPar, false, "Destination branch");
    imageCommands.append(c);

    c = new Command("relinkToBranchAt", Command::ImageSel);
    c->setComment("Relink image to destination branch at position");
    c->addParameter(Command::BranchPar, false, "Destination branch");
    c->addParameter(Command::IntPar, false, "Position (0 is first)");
    imageCommands.append(c);

    c = new Command("selectParent", Command::ImageSel, Command::BoolPar);
    c->setComment("Select parent of image");
    imageCommands.append(c);

    c = new Command("setHeadingRichText", Command::ImageSel);
    c->addParameter(Command::StringPar, false, "New heading");
    c->setComment("Set heading of image as HTML-like string");
    imageCommands.append(c);

    c = new Command("setHeadingText", Command::ImageSel);
    c->addParameter(Command::StringPar, false, "New heading");
    c->setComment("Set heading of image as plain text string");
    imageCommands.append(c);

    c = new Command("setHideLinksUnselected", Command::ImageSel);
    c->addParameter(Command::BoolPar, false,
              "Set if links of items should be visible for unselected items");
    imageCommands.append(c);

    c = new Command("setScale", Command::ImageSel);
    c->addParameter(Command::DoublePar, false, "Scale image of branch by factor f");
    c->setComment("Scale image");
    imageCommands.append(c);

    //
    // Below are the commands for an xlink
    //
    
    c = new Command("getColor", Command::XLinkSel, Command::StringPar);
    c->setComment("Get color of xlink");
    xlinkCommands.append(c);

    c = new Command("getWidth", Command::XLinkSel, Command::IntPar);
    c->setComment("Get width of xlink");
    xlinkCommands.append(c);

    c = new Command("getPenStyle", Command::XLinkSel, Command::StringPar);
    c->setComment("Get style of xlink as string (QPenStyle)");
    xlinkCommands.append(c);

    c = new Command("getStyleBegin", Command::XLinkSel, Command::StringPar);
    c->setComment("Get style of xlink start as string");
    xlinkCommands.append(c);

    c = new Command("getStyleEnd", Command::XLinkSel, Command::StringPar);
    c->setComment("Get style of xlink end as string");
    xlinkCommands.append(c);

    c = new Command("setColor");
    c->addParameter(Command::StringPar, true, "Color of xlink as string");
    c->setComment("Set color of xlink");
    xlinkCommands.append(c);

    //
    // Below are the commands for an itemList
    //
    
    c = new Command("count");
    c->setComment("Return number of items in list");
    c->setReturnType(Command::IntPar);
    itemListCommands.append(c);

    c = new Command("setModeBranches");
    c->setComment("Set iteration mode to include all branches in map");
    c->addParameter(Command::BoolPar, true, "Deep levels first");
    itemListCommands.append(c);

    c = new Command("setModeSelectedBranches");
    c->setComment("Set iteration mode to include all selected branches in map");
    itemListCommands.append(c);

    c = new Command("setModeSelectedSubtrees");
    c->setComment("Set iteration mode to include all selected branches and their children in map");
    c->addParameter(Command::BoolPar, true, "Deep levels first");
    itemListCommands.append(c);

    // Finally set objectTypes in all defined commands
    foreach (Command *c, vymCommands)
        c->setObjectType(Command::VymObject);

    foreach (Command *c, branchCommands)
        c->setObjectType(Command::BranchObject);

    foreach (Command *c, modelCommands)
        c->setObjectType(Command::MapObject);

    foreach (Command *c, imageCommands)
        c->setObjectType(Command::ImageObject);

    foreach (Command *c, xlinkCommands)
        c->setObjectType(Command::XLinkObject);

    foreach (Command *c, itemListCommands)
        c->setObjectType(Command::ItemListObject);

}

void Main::cloneActionMapEditor(QAction *a) // Add action to mapEditorActions and set context
{
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
}


// File Actions
void Main::setupFileActions()
{
    QAction *a;
    QString tag = tr("File actions", "MainWindow shortcut groups");
    a = new QAction(QPixmap(QString(":/document-new-%1.svg").arg(iconTheme)), tr("&New map", "File menu"),
                    this);
    switchboard.addAction(a, "fileMapNew", Qt::CTRL | Qt::Key_N, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNew()));
    cloneActionMapEditor(a);
    fileMenu->addAction(a);
    actionFileNew = a;

    a = new QAction(QPixmap(":/filenewcopy.svg"),
                    tr("&Copy to new map", "File menu"), this);
    switchboard.addAction(a, "fileMapNewCopy", Qt::CTRL | Qt::SHIFT | Qt::Key_C, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileNewCopy()));
    cloneActionMapEditor(a);
    fileMenu->addAction(a);
    actionFileNewCopy = a;

    a = new QAction(QPixmap(QString(":/document-open-%1").arg(iconTheme)), tr("&Open...", "File menu"),
                    this);
    switchboard.addAction(a, "fileMapOpen", Qt::CTRL | Qt::Key_L, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileLoad()));
    cloneActionMapEditor(a);
    fileMenu->addAction(a);
    actionFileOpen = a;

    a = new QAction(tr("&Restore last session", "Edit menu"), this);
    switchboard.addAction(a, "fileMapRestore", Qt::CTRL | Qt::Key_S, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileRestoreSession()));
    fileMenu->addAction(a);
    a->setEnabled(true);
    actionFileRestoreSession = a;

    fileLastMapsMenu = fileMenu->addMenu(tr("Open Recent", "File menu"));
    fileMenu->addSeparator();

    a = new QAction(QPixmap(":/edit-clear-list.svg"), tr("&Clear", "Clear recent files menu"), this);
    a->setEnabled(false);
    connect(a, SIGNAL(triggered()), this, SLOT(fileClearRecent()));
    fileLastMapsMenu->addAction(a);
    actionClearRecent = a;

    a = new QAction(QPixmap(QString(":/document-save-%1.svg").arg(iconTheme)), tr("&Save...", "File menu"), this);
    switchboard.addAction(a, "fileMapSave", Qt::CTRL | Qt::Key_S, shortcutScope, tag);
    cloneActionMapEditor(a);
    fileMenu->addAction(a);
    restrictedMapActions.append(a);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSave()));
    actionFileSave = a;

    a = new QAction(QPixmap(QString(":/document-save-as-%1.svg").arg(iconTheme)), tr("Save &As...", "File menu"),
                    this);
    fileMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

    a = new QAction(tr("Save as default map", "File menu"), this);
    fileMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(fileSaveAsDefault()));

    a = new QAction(tr("Save selection", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editSaveSelection()));
    a->setEnabled(false);
    fileMenu->addAction(a);
    actionListBranches.append(a);
    actionSaveSelection = a;

    fileMenu->addSeparator();

    fileImportMenu = fileMenu->addMenu(tr("Import", "File menu"));

    // Import at selection (adding to selection)
    a = new QAction(tr("Add map (insert)", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editImportAdd()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionImportAdd = a;
    fileImportMenu->addAction(a);

    // Import at selection (replacing selection)
    a = new QAction(tr("Add map (replace)", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editImportReplace()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionImportReplace = a;
    fileImportMenu->addAction(a);
    fileImportMenu->addSeparator();

    a = new QAction( tr("Firefox Bookmarks", "Import filters") +
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this,
            SLOT(fileImportFirefoxBookmarks()));
    fileImportMenu->addAction(a);

    a = new QAction("Freemind..." + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileImportFreemind()));
    fileImportMenu->addAction(a);

    a = new QAction("IThoughts..." + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileImportIThoughts()));
    fileImportMenu->addAction(a);

    a = new QAction("Mind Manager..." + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileImportMM()));
    fileImportMenu->addAction(a);

    a = new QAction(tr("Import Dir...", "Import Filters") + " " +
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileImportDir()));
    fileImportMenu->addAction(a);

    fileExportMenu = fileMenu->addMenu(tr("Export", "File menu"));

    tag = tr("Exports", "MainWindow shortcut groups");
    a = new QAction(QPixmap(QString(":/document-export-%1.svg").arg(iconTheme)),
                    tr("Repeat last export"), this);
    switchboard.addAction(a, "fileExportLast", Qt::CTRL | Qt::Key_E, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportLast()));
    cloneActionMapEditor(a);
    fileExportMenu->addAction(a);
    actionFileExportLast = a;
    actionListFiles.append(a);

    a = new QAction(tr("Webpage (HTML)...", "File export menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportHTML()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Confluence (HTML)...", "File export menu") + " " + " " +
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportConfluence()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);
    actionFileExportConfluence = a;

    a = new QAction( tr("Firefox Bookmarks", "File export menu") + 
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this,
            SLOT(fileExportFirefoxBookmarks()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Text (ASCII)...", "File export menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportASCII()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Text (Markdown)...", "File export menu") + " " +
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportMarkdown()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Text with tasks", "File export menu") + " " +
                        tr("(still experimental)"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportASCIITasks()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Text (A&O report)...", "Export format"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportAO()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("Image%1", "File export menu").arg("..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportImage()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("PDF%1", "File export menu").arg("..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportPDF()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("SVG%1", "File export menu").arg("..."), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportSVG()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction("LibreOffice Impress...", this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportImpress()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction("XML...", this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportXML()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction(tr("CSV...") + " " + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportCSV()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction("Taskjuggler... " + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportTaskJuggler()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction("OrgMode... " + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportOrgMode()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    a = new QAction("LaTeX... " + tr("(still experimental)"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExportLaTeX()));
    fileExportMenu->addAction(a);
    actionListFiles.append(a);

    fileMenu->addSeparator();

    a = new QAction(tr("Map properties"), this);
    switchboard.addAction(a, "editMapProperties", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editMapProperties()));
    fileMenu->addAction(a);
    actionListFiles.append(a);
    actionMapProperties = a;

    fileMenu->addSeparator();

    a = new QAction(QPixmap(QString(":/document-print-%1.svg").arg(iconTheme)), tr("&Print") + QString("..."),
                    this);
    switchboard.addAction(a, "fileMapPrint", Qt::CTRL | Qt::Key_P, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(filePrint()));
    fileMenu->addAction(a);
    unrestrictedMapActions.append(a);
    actionFilePrint = a;

    a = new QAction(QPixmap(QString(":/document-close-%1.svg").arg(iconTheme)), tr("&Close Map", "File menu"),
                    this);
    switchboard.addAction(a, "fileMapClose", Qt::CTRL | Qt::Key_W, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileCloseCurrentMap()));
    fileMenu->addAction(a);
    actionFileClose = a;

    tag = tr("Exit", "MainWindow shortcut groups");
    a = new QAction(QPixmap(QString(":/application-exit-%1.svg").arg(iconTheme)), tr("E&xit", "File menu"), this);
    switchboard.addAction(a, "fileExit", Qt::CTRL | Qt::Key_Q, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(fileExitVym()));
    fileMenu->addAction(a);
    actionFileExitVym = a;

    tag = tr("Miscellaneous", "MainWindow shortcut groups");
    a = new QAction("Toggle winter mode", this);
    a->setShortcutContext(Qt::WidgetShortcut);

    if (settings.value("/mainwindow/showTestMenu", false).toBool()) {
        addAction(a);
        mapEditorActions.append(a);
        switchboard.addAction(a, "mapWinterMode", Qt::CTRL | Qt::Key_Asterisk, shortcutScope, tag);
    }
    connect(a, SIGNAL(triggered()), this, SLOT(toggleWinter()));
    actionToggleWinter = a;
}

// Edit Actions
void Main::setupEditActions()
{
    QAction *a;
    QString tag = tr("Undo/Redo", "MainWindow shortcut groups");
    a = new QAction(QPixmap(":/undo.png"), tr("&Undo", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    mapEditorActions.append(a);
    restrictedMapActions.append(a);
    switchboard.addAction(a, "mapUndo",Qt::CTRL | Qt::Key_Z, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editUndo()));
    actionUndo = a;

    a = new QAction(QPixmap(":/undo.png"), tr("&Undo", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    mapEditorActions.append(a);
    vimActions.append(a);
    restrictedMapActions.append(a);
    switchboard.addAction(a, "mapUndoVimAlt", Qt::Key_U, shortcutScope, tag); // Vim Alternative
    connect(a, SIGNAL(triggered()), this, SLOT(editUndo()));
    actionUndoVim = a;

    a = new QAction(QPixmap(":/redo.png"), tr("&Redo", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    restrictedMapActions.append(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapRedo", Qt::CTRL | Qt::Key_Y, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editRedo()));
    actionRedo = a;

    a = new QAction(tr("Repeat last action", "Edit menu") + " (experimental)", this);
    switchboard.addAction(a, "repeatLastActionVim", Qt::Key_Period, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editRepeatLastAction()));
    //actionListBranches.append(a);
    actionRepeatCommand = a;

    editMenu->addSeparator();

    a = new QAction(QPixmap(QString(":/edit-copy-%1.svg").arg(iconTheme)), tr("&Copy", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    unrestrictedMapActions.append(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapCopy", Qt::CTRL | Qt::Key_C, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCopy()));
    actionCopy = a;

    a = new QAction(QPixmap(QString(":/edit-copy-%1.svg").arg(iconTheme)), tr("&Copy", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    unrestrictedMapActions.append(a);
    mapEditorActions.append(a);
    vimActions.append(a);
    switchboard.addAction(a, "mapCopyVimAlt", Qt::Key_Y, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCopy()));
    actionCopyVim = a;

    a = new QAction(QPixmap(QString(":/edit-cut-%1.svg").arg(iconTheme)), tr("Cu&t", "Edit menu"), this);
    // Multi key shortcuts https://bugreports.qt.io/browse/QTBUG-39127
    a->setEnabled(false);
    a->setShortcutContext(Qt::WidgetShortcut);
    restrictedMapActions.append(a);
    mapEditorActions.append(a);
    restrictedMapActions.append(a);
    switchboard.addAction(a, "mapCut", Qt::CTRL | Qt::Key_X, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCut()));
    addAction(a);
    actionCut = a;

    a = new QAction(QPixmap(QString(":/edit-cut-%1.svg").arg(iconTheme)), tr("Cu&t", "Edit menu"), this);
    switchboard.addAction(a, "mapCutVimAlt", Qt::Key_D, shortcutScope, tag);
    addAction(a);
    vimActions.append(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editDeleteSelection()));
    actionListItems.append(a);
    actionCutVim = a;

    a = new QAction(QPixmap(QString(":/edit-paste-%1.svg").arg(iconTheme)), tr("&Paste", "Edit menu"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(editPaste()));
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    restrictedMapActions.append(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapPaste", Qt::CTRL | Qt::Key_V, shortcutScope, tag);
    actionPaste = a;

    a = new QAction(QPixmap(QString(":/edit-paste-%1.svg").arg(iconTheme)), tr("&Paste", "Edit menu"),
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(editPaste()));
    a->setShortcutContext(Qt::WidgetShortcut);
    a->setEnabled(false);
    restrictedMapActions.append(a);
    mapEditorActions.append(a);
    vimActions.append(a);
    switchboard.addAction(a, "mapPasteVimAlt", Qt::Key_P, shortcutScope, tag);
    actionPasteVim = a;

    // Shortcut to delete selection
    a = new QAction(tr("Delete Selection", "Edit menu"), this);
#if defined(Q_OS_MACOS)
    switchboard.addAction(a, "mapDelete", Qt::Key_Backspace, shortcutScope, tag);
#else
    switchboard.addAction(a, "mapDelete", Qt::Key_Delete, shortcutScope, tag);
#endif
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editDeleteSelection()));
    actionListItems.append(a);
    actionDelete = a;

    tag = tr("Add", "MainWindow shortcut groups");
    // Shortcut to add mapcenter
    a = new QAction(QPixmap(":/newmapcenter.png"),
                    tr("Add mapcenter", "Canvas context menu"), this);
    switchboard.addAction(a, "mapAddCenter", Qt::Key_C, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddMapCenter()));
    actionListFiles.append(a);
    actionAddMapCenter = a;

    // Shortcut to add branch
    a = new QAction(QPixmap(":/newbranch.png"),
                    tr("Add branch as child", "Edit menu"), this);
    switchboard.addAction(a, "mapeditAddBranch", Qt::Key_A, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranch()));
    cloneActionMapEditor(a);
    taskEditorActions.append(a);
    actionListBranches.append(a);
    actionAddBranch = a;

    // Add branch by inserting it at selection
    a = new QAction(tr("Add branch (insert)", "Edit menu"), this);
    switchboard.addAction(a, "mapEditAddBranchBefore", Qt::SHIFT | Qt::CTRL | Qt::Key_A, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranchBefore()));
    actionListBranches.append(a);
    actionAddBranchBefore = a;

    // Add branch above
    a = new QAction(tr("Add branch above", "Edit menu"), this);
    switchboard.addAction(a, "mapEditAddBranchAbove", Qt::SHIFT | Qt::Key_A, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranchAbove()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionAddBranchAbove = a;

    a = new QAction(tr("Add branch above", "Edit menu"), this);
    switchboard.addAction(a, "mapEditAddBranchAboveAlt", Qt::SHIFT | Qt::Key_Insert, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranchAbove()));
    actionListBranches.append(a);

    // Add branch below
    a = new QAction(tr("Add branch below", "Edit menu"), this);
    switchboard.addAction(a, "mapEditAddBranchBelow", Qt::CTRL | Qt::Key_Insert, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranchBelow()));
    a->setEnabled(false);
    actionListBranches.append(a);

    a = new QAction(tr("Add branch below", "Edit menu"), this);
    switchboard.addAction(a, "mapEditAddBranchBelowAlt", Qt::CTRL | Qt::Key_A, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddBranchBelow()));
    actionListBranches.append(a);
    actionAddBranchBelow = a;

    tag = tr("Move", "MainWindow shortcut groups");
    a = new QAction(QPixmap(":/up.png"), tr("Move branch up", "Edit menu"),
                    this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    taskEditorActions.append(a);
    restrictedMapActions.append(a);
    actionListBranches.append(a);
    actionListImages.append(a);
    switchboard.addAction(a, "mapEditMoveBranchUp", Qt::Key_PageUp, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editMoveUp()));
    actionMoveUp = a;

    a = new QAction(QPixmap(":/down.png"), tr("Move branch down", "Edit menu"),
                    this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    taskEditorActions.append(a);
    restrictedMapActions.append(a);
    actionListBranches.append(a);
    actionListImages.append(a);
    switchboard.addAction(a, "mapEditMoveBranchDown", Qt::Key_PageDown, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editMoveDown()));
    actionMoveDown = a;

    a = new QAction(QPixmap(":up-diagonal-right.png"), tr("Move branch diagonally up", "Edit menu"),
                    this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    taskEditorActions.append(a);
    restrictedMapActions.append(a);
    actionListBranches.append(a);
#if defined(Q_OS_MACOS)
    switchboard.addAction(a, "mapEditMoveBranchUpDiagonally", Qt::SHIFT | Qt::Key_PageUp, shortcutScope, tag);
#else
    switchboard.addAction(a, "mapEditMoveBranchUpDiagonally", Qt::CTRL | Qt::Key_PageUp, shortcutScope, tag);
#endif
    connect(a, SIGNAL(triggered()), this, SLOT(editMoveUpDiagonally()));
    actionMoveUpDiagonally = a;

    a = new QAction(QPixmap(":down-diagonal-left.png"), tr("Move branch diagonally down", "Edit menu"),
                    this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    taskEditorActions.append(a);
    restrictedMapActions.append(a);
    actionListBranches.append(a);
#if defined(Q_OS_MACOS)
    switchboard.addAction(a, "mapEditMoveBranchDownDiagonally", Qt::SHIFT | Qt::Key_PageDown, shortcutScope, tag);
#else
    switchboard.addAction(a, "mapEditMoveBranchDownDiagonally", Qt::CTRL | Qt::Key_PageDown, shortcutScope, tag);
#endif 
    connect(a, SIGNAL(triggered()), this, SLOT(editMoveDownDiagonally()));
    actionMoveDownDiagonally = a;

    a = new QAction(QPixmap(), tr("&Detach", "Context menu"), this);
    a->setStatusTip(tr("Detach branch and use as mapcenter", "Context menu"));
    switchboard.addAction(a, "mapDetachBranch", Qt::Key_D | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editDetach()));
    actionListBranches.append(a);
    actionDetach = a;

    QString sortDisplayTag = tr("Sort and display", "MainWindow shortcut groups");
    a = new QAction(QPixmap(QString(":/view-sort-ascending-name-%1.svg").arg(iconTheme)), tr("Sort children", "Edit menu"), this);
    a->setEnabled(true);
    switchboard.addAction(a, "mapSortBranches", Qt::Key_O, shortcutScope, sortDisplayTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editSortChildren()));
    actionListBranches.append(a);
    actionSortChildren = a;

    a = new QAction(QPixmap(QString(":/view-sort-descending-name-%1.svg").arg(iconTheme)), tr("Sort children backwards", "Edit menu"), this);
    a->setEnabled(true);
    switchboard.addAction(a, "mapSortBranchesReverse", Qt::SHIFT | Qt::Key_O, shortcutScope, sortDisplayTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editSortBackChildren()));
    actionListBranches.append(a);
    actionSortBackChildren = a;

    a = new QAction(QPixmap(":/flag-scrolled-right.png"),
                    tr("Scroll branch", "Edit menu"), this);
    switchboard.addAction(a, "mapToggleScroll", Qt::Key_S, shortcutScope, sortDisplayTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editToggleScroll()));
    actionListBranches.append(a);
    a->setEnabled(false);
    a->setCheckable(true);
    addAction(a);
    actionListBranches.append(a);
    actionToggleScroll = a;

    a = new QAction(tr("Unscroll branch and subtree", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editUnscrollSubtree()));
    actionListBranches.append(a);
    actionUnscrollSubtree = a;

    QString geometryTag = tr("Geometry of items", "MainWindow shortcut groups");
    a = new QAction(tr("Grow selection", "Edit menu"), this);
    switchboard.addAction(a, "mapGrowSelection", Qt::CTRL | Qt::Key_Plus, shortcutScope, geometryTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editGrowSelectionSize()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionGrowSelectionSize = a;

    a = new QAction(tr("Shrink selection", "Edit menu"), this);
    switchboard.addAction(a, "mapShrinkSelection", Qt::CTRL | Qt::Key_Minus, shortcutScope, geometryTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editShrinkSelectionSize()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionShrinkSelectionSize = a;

    a = new QAction(tr("Reset selection size", "Edit menu"), this);
    switchboard.addAction(a, "mapResetSelectionSize", Qt::CTRL | Qt::Key_0, shortcutScope, geometryTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editResetSelectionSize()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionResetSelectionSize = a;

    a = new QAction(tr("Rotate subtree clockwise", "Edit menu"), this);
    switchboard.addAction(a, "mapRotateSubtreeCW", Qt::CTRL | Qt::Key_R, shortcutScope, geometryTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editRotateSubtreeCW()));
    actionListBranches.append(a);
    actionRotateSubtreeCW = a;

    a = new QAction(tr("Rotate subtree counter-clockwise", "Edit menu"), this);
    switchboard.addAction(a, "mapRotateSubtreeCCW", Qt::CTRL | Qt::SHIFT | Qt::Key_R, shortcutScope, geometryTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editRotateSubtreeCCW()));
    actionListBranches.append(a);
    actionRotateSubtreeCCW = a;

    editMenu->addSeparator();

    tag = tr("URLs", "Shortcuts in references context menu");
    a = new QAction(QPixmap(":/flag-url.svg"), tr("Open URL", "Edit menu"),
                    this);
    switchboard.addAction(a, "mapOpenUrl", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(openUrl()));
    actionListBranches.append(a);
    actionOpenUrl = a;

    a = new QAction(tr("Open all visible URLs in subtree", "Edit menu"), this);
    switchboard.addAction(a, "mapOpenUrlsSubTree", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenMultipleVisUrls()));
    actionListBranches.append(a);
    actionOpenMultipleVisUrls = a;

    a = new QAction(tr("Open all URLs in subtree", "Edit menu"), this);
    switchboard.addAction(a, "mapOpenMultipleUrls", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenMultipleUrls()));
    actionListBranches.append(a);
    actionOpenMultipleUrls = a;

    a = new QAction(tr("Open all URLs in subtree in private mode", "Edit menu"), this);
    if (settings.value("/mainwindow/showTestMenu", false).toBool()) {
        switchboard.addAction(a, "mapOpenMultipleUrls", shortcutScope, tag);
        addAction(a);
        connect(a, SIGNAL(triggered()), this, SLOT(editOpenMultipleUrlsPrivate()));
        actionListBranches.append(a);
    }
    actionOpenMultipleUrlsPrivate = a;

    a = new QAction(QPixmap(), tr("Extract URLs from note", "Edit menu"), this);
    switchboard.addAction(a, "mapUrlsFromNote", Qt::SHIFT | Qt::Key_N, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editNote2URLs()));
    actionListBranches.append(a);
    actionGetURLsFromNote = a;

    a = new QAction(QPixmap(":/flag-urlnew.svg"),
                    tr("Edit URL...", "Edit menu"), this);
    switchboard.addAction(a, "mapEditURL", Qt::SHIFT | Qt::Key_U, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editURL()));
    actionListBranches.append(a);
    actionURLNew = a;

    a = new QAction(QPixmap(), tr("Edit local URL...", "Edit menu"), this);
    switchboard.addAction(a, "mapEditLocalURL", Qt::CTRL | Qt::Key_U, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editLocalURL()));
    actionListBranches.append(a);
    actionLocalURL = a;

    a = new QAction(tr("Use heading for URL", "Edit menu"), this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapHeading2URL", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editHeading2URL()));
    actionListBranches.append(a);
    actionHeading2URL = a;

    tag = tr("Connect", "Connection shortcuts in MainWindow");
    a = new QAction(
            QPixmap(":/flag-jira.svg"), 
            tr("Get data from Jira for subtree", "Edit menu"),
            this);
    switchboard.addAction(a, "mapUpdateSubTreeFromJira", Qt::Key_J | Qt::SHIFT, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(getJiraDataSubtree()));
    actionGetJiraDataSubtree = a;

    a = new QAction(
            QPixmap(":/flag-jira.svg"),
            tr("Set Jira query", "Edit menu"),
            this);
    switchboard.addAction(a, "mapSetJiraQuery", Qt::Key_J | Qt::CTRL, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(setJiraQuery()));
    actionSetJiraQuery = a;

    a = new QAction(tr("Get page name and details from Confluence", "Edit menu"),
                    this);
    switchboard.addAction(a, "mapGetConfluencePageDetails", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(getConfluencePageDetails()));
    actionListBranches.append(a);
    actionGetConfluencePageDetails = a;

    a = new QAction(tr("Get page name and details from Confluence for child pages", "Edit menu"),
                    this);
    switchboard.addAction(a, "mapGetConfluencePagesDetails", shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(getConfluencePageDetailsRecursively()));
    actionListBranches.append(a);
    actionGetConfluencePageDetailsRecursively = a;

    tag = tr("vymlinks - linking maps", "Shortcuts for vymLinks in MainWindow");
    a = new QAction(QPixmap(":/flag-vymlink.png"),
                    tr("Open linked map", "Edit menu"), this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapOpenVymLink", Qt::SHIFT | Qt::Key_V, shortcutScope, tag);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenVymLink()));
    actionListBranches.append(a);
    actionOpenVymLink = a;

    a = new QAction(QPixmap(":/flag-vymlink.png"),
                    tr("Open linked map in background tab", "Edit menu"), this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapOpenVymLink", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenVymLinkBackground()));
    actionListBranches.append(a);
    actionOpenVymLinkBackground = a;

    a = new QAction(QPixmap(), tr("Open all vym links in subtree", "Edit menu"),
                    this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapOpenMultipleVymLinks", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenMultipleVymLinks()));
    actionListBranches.append(a);
    actionOpenMultipleVymLinks = a;

    a = new QAction(QPixmap(":/flag-vymlinknew.png"),
                    tr("Edit vym link...", "Edit menu"), this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapEditVymLink", Qt::Key_V, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editVymLink()));
    actionListBranches.append(a);
    actionEditVymLink = a;

    a = new QAction(tr("Delete vym link", "Edit menu"), this);
    a->setEnabled(false);
    switchboard.addAction(a, "mapDeleteVymLink", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editDeleteVymLink()));
    actionListBranches.append(a);
    actionDeleteVymLink = a;

    a = new QAction(QPixmap(":/flag-hideexport.png"),
                    tr("Hide in exports", "Edit menu"), this);
    a->setCheckable(true);
    a->setEnabled(false);
    addAction(a);
    switchboard.addAction(a, "mapToggleHideExport", Qt::Key_H, shortcutScope, sortDisplayTag);
    connect(a, SIGNAL(triggered()), this, SLOT(editToggleHideExport()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionToggleHideExport = a;

    tag = tr("Tasks", "Shortcuts for tasks in MainWindow");
    a = new QAction(QPixmap(":/taskeditor.png"), tr("Toggle task", "Edit menu"),
                    this);
    a->setCheckable(true);
    a->setEnabled(false);
    addAction(a);
    switchboard.addAction(a, "mapToggleTask", Qt::Key_W | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editToggleTask()));
    actionListBranches.append(a);
    actionToggleTask = a;

    a = new QAction(QPixmap(), tr("Cycle task status", "Edit menu"), this);
    a->setCheckable(false);
    a->setEnabled(false);
    addAction(a);
    switchboard.addAction(a, "mapCycleTaskStatus", Qt::Key_W, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCycleTaskStatus()));
    actionListBranches.append(a);
    actionCycleTaskStatus = a;

    a = new QAction(QPixmap(), tr("Reset delta priority for visible tasks", "Reset delta"), this);
    a->setCheckable(false);
    a->setEnabled(false);
    addAction(a);
    switchboard.addAction(a, "mapResetTaskDeltaPrio", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskResetDeltaPrio()));
    actionListBranches.append(a);
    actionTaskResetDeltaPrio = a;

    a = new QAction(QPixmap(), tr("Reset sleep", "Task sleep"), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(0);
    addAction(a);
    switchboard.addAction(a, "mapResetSleep", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep0 = a;

    a = new QAction(QPixmap(),
                    tr("Sleep %1 days", "Task sleep").arg("n") + "...", this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(-1);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleepN", Qt::Key_Q | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleepN = a;

    a = new QAction(QPixmap(), tr("Sleep %1 day", "Task sleep").arg(1), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(1);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep1", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep1 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 days", "Task sleep").arg(2), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(2);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep2", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep2 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 days", "Task sleep").arg(3), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(3);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep3", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep3 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 days", "Task sleep").arg(4), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(4);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep4", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep4 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 days", "Task sleep").arg(5), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(5);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep5", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep5 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 days", "Task sleep").arg(7), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(7);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep7", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep7 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 weeks", "Task sleep").arg(2), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(14);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep14", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep14 = a;

    a = new QAction(QPixmap(), tr("Sleep %1 weeks", "Task sleep").arg(4), this);
    a->setCheckable(false);
    a->setEnabled(false);
    a->setData(28);
    addAction(a);
    switchboard.addAction(a, "mapTaskSleep28", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editTaskSleepN()));
    actionListBranches.append(a);
    actionTaskSleep28 = a;

    tag = tr("Removing parts of a map", "Shortcuts");

    // Only remove branch, not its children
    a = new QAction(
        tr("Remove only branch and keep its children ", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editDeleteKeepChildren()));
    a->setEnabled(false);
    addAction(a);
    switchboard.addAction(a, "mapDeleteKeepChildren", Qt::CTRL | Qt::SHIFT | Qt::Key_X, shortcutScope, tag);
    actionListBranches.append(a);
    actionDeleteKeepChildren = a;

    // Only remove children of a branch
    a = new QAction(tr("Remove children", "Edit menu"), this);
    addAction(a);
    switchboard.addAction(a, "mapDeleteChildren", Qt::SHIFT | Qt::Key_X, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editDeleteChildren()));
    a->setEnabled(false);
    addAction(a);
    actionListBranches.append(a);
    actionDeleteChildren = a;

    tag = tr("Various", "Shortcuts");
    a = new QAction(tr("Add timestamp", "Edit menu"), this);
    a->setEnabled(false);
    actionListBranches.append(a);
    addAction(a);
    switchboard.addAction(a, "mapAddTimestamp", Qt::Key_T, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editAddTimestamp()));
    actionListBranches.append(a);
    actionAddTimestamp = a;

    a = new QAction(tr("Map properties...", "Edit menu"), this);
    a->setEnabled(true);
    connect(a, SIGNAL(triggered()), this, SLOT(editMapProperties()));
    actionListFiles.append(a);
    actionMapInfo = a;

    a = new QAction(QPixmap(QString(":/insert-image-%1.svg").arg(iconTheme)), tr("Add image", "Edit and context menus") + "...", this);
    addAction(a);
    switchboard.addAction(a, "mapLoadImage", Qt::Key_I | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editLoadImage()));
    actionListBranches.append(a);
    actionLoadImage = a;

    QString n = tr("Item properties", "Dialog to edit properties of selected item");
    a = new QAction(n, this) ;
    addAction(a);
    switchboard.addAction(a, "mapFocusPropertyEditor", Qt::Key_B, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(focusProperty()));
    actionViewFocusPropertyEditor = a;

    a = new QAction(n, this) ;
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleProperty()));
    actionViewTogglePropertyEditor = a;
}

void Main::setupEditMenu()
{
    editMenu->addAction(actionUndo);
    editMenu->addAction(actionUndoVim);
    editMenu->addAction(actionRedo);
    editMenu->addAction(actionRepeatCommand);

    editMenu->addSeparator();

    editMenu->addAction(actionCopy);
    editMenu->addAction(actionCopyVim);
    editMenu->addAction(actionCut);
    editMenu->addAction(actionPaste);
    editMenu->addAction(actionPasteVim);
    editMenu->addAction(actionDelete);

    editMenu->addSeparator();

    editMenu->addMenu(branchAddContextMenu);
    editMenu->addMenu(branchRemoveContextMenu);
    editMenu->addMenu(branchHierarchyContextMenu);
    editMenu->addMenu(branchGeometryContextMenu);

    editMenu->addSeparator();

    editMenu->addAction(actionToggleScroll);
    editMenu->addAction(actionUnscrollSubtree);

}

// Select Actions
void Main::setupSelectActions()
{
    QString tag = tr("Selections", "Shortcuts");
    QAction *a;

    tag = tr("Search functions", "Shortcuts");
    a = new QAction(QPixmap(QString(":/edit-find-%1.svg").arg(iconTheme)), tr("Find...", "Edit menu"), this);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapFind", Qt::CTRL | Qt::Key_F, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenFindResultWidget()));
    actionListFiles.append(a);
    actionFind = a;

    a = new QAction(QPixmap(QString(":/edit-find-%1.svg").arg(iconTheme)), tr("Find...", "Edit menu"), this);
    selectMenu->addAction(a);
    vimActions.append(a);
    switchboard.addAction(a, "mapFindVimAlt", Qt::Key_Slash, shortcutScope, tag); // Alternative: VIM Find
    connect(a, SIGNAL(triggered()), this, SLOT(editOpenFindResultWidget()));
    actionListFiles.append(a);
    actionFindVim = a;

    a = new QAction(tr("Follow reference", "Context menu"), this);
    addAction(a);
    actionListBranches.append(a);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapFollowXLink", Qt::Key_F, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(popupFollowReference()));
    actionFollowReference = a;

    a = new QAction("Select first branch in siblings", this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    selectMenu->addAction(a);
    switchboard.addAction(a, "Select first branch in siblings", Qt::Key_Home, shortcutScope, tag);
    actionListBranches.append(a);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectFirstSibling()));

    a = new QAction("Select first branch in siblings", this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    vimActions.append(a);
    selectMenu->addAction(a);
    switchboard.addAction(a, "selectFirstSiblingVim", Qt::Key_0, shortcutScope, tag);              // Alternative: VIM Select first
    actionListBranches.append(a);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectFirstSibling()));

    a = new QAction("Select last branch in siblings", this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    selectMenu->addAction(a);
    switchboard.addAction(a, "Select last branch in siblings", Qt::Key_End, shortcutScope, tag);
    actionListBranches.append(a);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectLastSibling()));

    a = new QAction("Select last branch in siblings", this);
    a->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    vimActions.append(a);
    selectMenu->addAction(a);
    switchboard.addAction(a, "selectLastSiblingVim", Qt::Key_Dollar, shortcutScope, tag);
    actionListBranches.append(a);
    addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectLastSibling()));

    a = new QAction(QPixmap(":/flag-target.svg"),
                    tr("Toggle target...", "Edit menu"), this);
    a->setCheckable(true);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapToggleTarget", Qt::SHIFT | Qt::Key_T, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editToggleTarget()));
    actionListBranches.append(a);
    actionToggleTarget = a;

    a = new QAction(QPixmap(":/flag-target.svg"),
                    tr("Goto target...", "Edit menu"), this);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapGotoTarget", Qt::Key_G, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editGoToTarget()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionGoToTarget = a;

    a = new QAction(QPixmap(":/flag-target.svg"),
                    tr("Move to target...", "Edit menu"), this);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapMoveToTarget", Qt::Key_M, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editMoveToTarget()));
    actionListBranches.append(a);
    actionMoveToTarget = a;

    a = new QAction(QPixmap(":/flag-vymlink.png"),
                    tr("Goto linked map...", "Edit menu"), this);
    selectMenu->addAction(a);
    switchboard.addAction(a, "gotoLinkedMap", Qt::Key_G | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editGoToLinkedMap()));
    actionListBranches.append(a);
    actionGoToTargetLinkedMap = a;

    a = new QAction(tr("Find duplicate URLs", "Edit menu") + " (test)", this);
    switchboard.addAction(a, "mapFindDuplicates", shortcutScope, tag);
    if (settings.value("/mainwindow/showTestMenu", false).toBool())
        selectMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(editFindDuplicateURLs()));

    a = new QAction(QPixmap(QString(":/go-previous-%1.svg").arg(iconTheme)),
                    tr("Select previous", "Edit menu"), this);
    a->setShortcutContext(Qt::WidgetShortcut);
    selectMenu->addAction(a);
    actionListFiles.append(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapSelectPrevious", Qt::CTRL | Qt::Key_O, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectPrevious()));
    actionSelectPrevious = a;

    a = new QAction(QPixmap(QString(":/go-next-%1.svg").arg(iconTheme)), tr("Select next", "Edit menu"),
                    this);
    a->setShortcutContext(Qt::WidgetShortcut);
    selectMenu->addAction(a);
    actionListFiles.append(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapSelectNext", Qt::CTRL | Qt::Key_I, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectNext()));
    actionSelectNext = a;

    a = new QAction(tr("Unselect all", "Edit menu"), this);
    selectMenu->addAction(a);
    switchboard.addAction(a, "mapSelectNothing", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editSelectNothing()));
    actionListFiles.append(a);
    actionSelectNothing = a;

}

// Format Actions
void Main::setupFormatActions()
{
    QString tag = tr("Formatting", "Shortcuts");

    QAction* a;

    a = new QAction(QPixmap(":/formatcolorpicker.png"),
                    tr("Pic&k color", "Edit menu"), this);
    formatMenu->addAction(a);
    switchboard.addAction(a, "mapFormatColorPicker", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(formatPickColor()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionFormatPickColor = a;

    a = new QAction(QPixmap(":/formatcolorbranch.png"),
                    tr("Color &branch", "Edit menu"), this);
    formatMenu->addAction(a);
    switchboard.addAction(a, "mapFormatColorBranch", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(formatColorBranch()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionFormatColorBranch = a;

    a = new QAction(QPixmap(":/formatcolorsubtree.png"),
                    tr("Color sub&tree", "Edit menu"), this);
    formatMenu->addAction(a);
    switchboard.addAction(a, "mapFormatColorSubtree", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(formatColorSubtree()));
    a->setEnabled(false);
    actionListBranches.append(a);
    actionFormatColorSubtree = a;

    formatMenu->addSeparator();

    a = new QAction(tr("Select default font", "Branch attribute") + "...",
                    this);
    a->setCheckable(false);
    connect(a, SIGNAL(triggered()), this, SLOT(formatSelectFont()));
    formatMenu->addAction(a);
    actionFormatFont = a;

    formatMenu->addSeparator();

    actionGroupFormatLinkStyles = new QActionGroup(this);
    actionGroupFormatLinkStyles->setExclusive(true);
    a = new QAction(tr("Linkstyle Line"), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    restrictedMapActions.append(a);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatLinkStyleLine()));
    actionFormatLinkStyleLine = a;

    a = new QAction(tr("Linkstyle Curve"), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    restrictedMapActions.append(a);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatLinkStyleParabel()));
    actionFormatLinkStyleParabel = a;

    a = new QAction(tr("Linkstyle Thick Line"), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    restrictedMapActions.append(a);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatLinkStylePolyLine()));
    actionFormatLinkStylePolyLine = a;

    a = new QAction(tr("Linkstyle Thick Curve"), actionGroupFormatLinkStyles);
    a->setCheckable(true);
    a->setChecked(true);
    restrictedMapActions.append(a);
    formatMenu->addAction(a);
    formatMenu->addSeparator();
    connect(a, SIGNAL(triggered()), this, SLOT(formatLinkStylePolyParabel()));
    actionFormatLinkStylePolyParabel = a;

    a = new QAction(
        tr("Hide link if object is not selected", "Branch attribute"), this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(formatHideLinkUnselected()));
    actionListBranches.append(a);
    actionListImages.append(a);
    actionFormatHideLinkUnselected = a;

    a = new QAction(tr("&Use color of heading for link", "Branch attribute"),
                    this);
    a->setCheckable(true);
    connect(a, SIGNAL(triggered()), this, SLOT(formatToggleLinkColorHint()));
    formatMenu->addAction(a);
    actionFormatLinkColorHint = a;

    QPixmap pix(16, 16);
    pix.fill(Qt::white);
    a = new QAction(pix, tr("Set &Link Color") + "...", this);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatSelectLinkColor()));
    actionFormatLinkColor = a;

    a = new QAction(pix, tr("Set &Selection Color") + "...", this);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatSelectSelectionColor()));
    actionFormatSelectionColor = a;

    a = new QAction(pix, tr("Set &Background color and image") + "...", this);
    formatMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(formatBackground()));
    actionFormatBackground = a;
}

// View Actions
void Main::setupViewActions()
{
    QAction *a;

    QString tag = tr("Views", "Mainwindow view shortcut groups");

    a = new QAction(QPixmap(QString(":/folder-cloud-%1.svg").arg(iconTheme)),
            tr("Toggle mode to temporary hide parts", "View action"),
            this);
    viewMenu->addAction(a);
    switchboard.addAction (a, "tmpHideMode", Qt::Key_H | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleHideTmpMode()));
    actionToggleHideTmpMode = a;

    a = new QAction(QPixmap(QString(":/zoom-in-%1.svg").arg(iconTheme)), tr("Zoom in", "View action"),
                    this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapZoomIn", Qt::Key_Plus, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewZoomIn()));
    actionZoomIn = a;

    a = new QAction(QPixmap(QString(":/zoom-out-%1.svg").arg(iconTheme)), tr("Zoom out", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapZoomOut", Qt::Key_Minus, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewZoomOut()));
    actionZoomOut = a;

    a = new QAction(QPixmap(QString(":/transform-rotate-ccw-%1.svg").arg(iconTheme)),
                    tr("Rotate counterclockwise", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapRotateCounterClockwise", Qt::SHIFT | Qt::Key_R, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewRotateCounterClockwise()));
    actionRotateCounterClockwise = a;

    a = new QAction(QPixmap(QString(":/transform-rotate-cw-%1.svg").arg(iconTheme)),
                    tr("Rotate clockwise", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapRotateClockwise", Qt::Key_R, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewRotateClockwise()));
    actionRotateClockwise = a;

    a = new QAction(QPixmap(QString(":/zoom-original-%1").arg(iconTheme)),
                    tr("reset Zoom", "View action"), this);
    switchboard.addAction(a, "mapZoomReset", Qt::Key_Comma, shortcutScope, tag);
    viewMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(viewZoomReset()));
    actionZoomReset = a;

    a = new QAction(QPixmap(QString(":/zoom-fit-best-%1.svg").arg(iconTheme)),
                    tr("Center on selection", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapCenterOn", Qt::Key_NumberSign, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewCenter()));
    actionCenterOn = a;

    a = new QAction(tr("Fit view to selection", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapCenterAndFitView", Qt::Key_Semicolon, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewCenterScaled()));
    actionCenterOnScaled = a;

    a = new QAction( tr("Rotate view to selection", "View action"), this);
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapCenterAndRotateView", Qt::Key_NumberSign | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(viewCenterRotated()));
    actionCenterOnRotated = a;

    tag = tr("Tree editor expand/collapse", "Mainwindow view shortcut groups");

    a = new QAction(QPixmap(), tr("Expand all branches", "Edit menu"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editExpandAll()));
    actionExpandAll = a;
    actionExpandAll->setEnabled(false);
    actionExpandAll->setCheckable(false);
    actionListBranches.append(actionExpandAll);
    addAction(a);

    a = new QAction(tr("Expand one level", "Edit menu"), this);
    switchboard.addAction(a, "mapExpandOneLevel", Qt::Key_Greater, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editExpandOneLevel()));
    a->setEnabled(false);
    a->setCheckable(false);
    addAction(a);
    actionListBranches.append(a);
    actionExpandOneLevel = a;

    a = new QAction("TE: " + tr("Collapse one level", "Edit menu"), this);
    switchboard.addAction(a, "mapCollapseOneLevel", Qt::Key_Less | Qt::CTRL, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCollapseOneLevel()));
    viewMenu->addAction(a);
    a->setEnabled(false);
    a->setCheckable(false);
    actionListBranches.append(a);
    addAction(a);
    actionCollapseOneLevel = a;

    a = new QAction("TE: " + tr("Collapse unselected levels", "Edit menu"), this);
    switchboard.addAction(a, "mapCollapseUnselectedLevels", Qt::Key_Less, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(editCollapseUnselected()));
    viewMenu->addAction(a);
    a->setEnabled(false);
    a->setCheckable(false);
    actionListBranches.append(a);
    addAction(a);
    actionCollapseUnselected = a;

    viewMenu->addSeparator();

    // Editor and other windows

    tag = tr("Windows", "Mainwindow view shortcut groups");
    QString n = tr("Note editor", "View action");
    a = new QAction(QPixmap(":/flag-note.svg"), n, this);
    a->setShortcutContext(Qt::WidgetShortcut);
    focusWindowsMenu->addAction(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "mapFocusNoteEditor", Qt::Key_N, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(focusNoteEditor()));
    actionViewFocusNoteEditor = a;

    a = new QAction(QPixmap(":/flag-note.svg"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleNoteEditor()));
    actionViewToggleNoteEditor = a;
                                    //
    n = tr("Heading editor", "View action");
    a = new QAction(QPixmap(":/headingeditor.png"), n, this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    focusWindowsMenu->addAction(a);
    switchboard.addAction(a, "mapFocusHeadingEditor", Qt::Key_E, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(focusHeadingEditor()));
    actionViewFocusHeadingEditor = a;

    a = new QAction(QPixmap(":/headingeditor.png"), n, this);
    a->setCheckable(true);
    mapEditorActions.append(a);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleHeadingEditor()));
    actionViewToggleHeadingEditor = a;

    n = tr("Tree editor", "View action");
    // Original icon is "category" from KDE
    a = new QAction(QPixmap(":/treeeditor.png"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleTreeEditors()));
    actionViewToggleTreeEditors = a;

    a = new QAction(QPixmap(":/treeeditor.png"), n, this);
    a->setShortcutContext(Qt::WidgetShortcut);
    focusWindowsMenu->addAction(a);
    mapEditorActions.append(a);
    switchboard.addAction(a, "switchTreeEditorAndMapEditor", Qt::Key_Tab, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(switchEditors()));
    actionViewSwitchEditors = a;

    n = tr("Task editor", "View action");
    a = new QAction(QPixmap(":/taskeditor.png"), n, this);
    a->setShortcutContext(Qt::WidgetShortcut);
    mapEditorActions.append(a);
    focusWindowsMenu->addAction(a);
    switchboard.addAction(a, "mapFocusTaskEditor", Qt::Key_Q, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(focusTaskEditor()));
    actionViewFocusTaskEditor = a;

    a = new QAction(QPixmap(":/taskeditor.png"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleTaskEditor()));
    actionViewToggleTaskEditor = a;

    n = tr("Slide editor", "View action");
    a = new QAction(QPixmap(":/slideeditor.png"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    switchboard.addAction(a, "mapShowSlideEditor", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleSlideEditors()));
    actionViewToggleSlideEditors = a;

    n = tr("Script editor", "View action");
    a = new QAction(QPixmap(":/scripteditor.png"), n, this);
    focusWindowsMenu->addAction(a);
    switchboard.addAction(a, "mapFocusScriptEditor", Qt::SHIFT | Qt::Key_S, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(focusScriptEditor()));
    actionViewFocusScriptEditor = a;

    a = new QAction(QPixmap(":/scripteditor.png"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleScriptEditor()));
    actionViewToggleScriptEditor = a;

    a = new QAction(QPixmap(), tr("Script output", "View action"), this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    switchboard.addAction(a, "mapToggleScriptOutput", Qt::CTRL | Qt::SHIFT | Qt::Key_S, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleScriptOutput()));
    actionViewToggleScriptOutput = a; // FIXME-3 show

    n = tr("History window", "View action");
    a = new QAction(QPixmap(":/history.png"), n, this);
    a->setShortcutContext(Qt::WidgetShortcut);
    focusWindowsMenu->addAction(a);
    mapEditorActions.append(a);
#if defined(Q_OS_MACOS)
    switchboard.addAction(a, "mapToggleHistoryWindow", Qt::SHIFT | Qt::CTRL | Qt::Key_H, shortcutScope, tag);   // Cmd-H not available on MacOS
#else
    switchboard.addAction(a, "mapToggleHistoryWindow", Qt::CTRL | Qt::Key_H, shortcutScope, tag);
#endif
    connect(a, SIGNAL(triggered()), this, SLOT(focusHistory()));
    actionViewFocusHistoryWindow = a;

    a = new QAction(QPixmap(":/history.png"), n, this);
    a->setCheckable(true);
    toggleWindowsMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleHistory()));
    actionViewToggleHistoryWindow = a;

    focusWindowsMenu->addAction(actionViewFocusPropertyEditor);
    toggleWindowsMenu->addAction(actionViewTogglePropertyEditor);

    viewMenu->addSeparator();

    // Switches 
    a = new QAction(tr("Antialiasing", "View action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/mainwindow/view/AntiAlias", true).toBool());
    viewMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleAntiAlias()));
    actionViewToggleAntiAlias = a;

    a = new QAction(tr("Smooth pixmap transformations", "View action"), this);
    a->setStatusTip(a->text());
    a->setCheckable(true);
    a->setChecked(
        settings.value("/mainwindow/view/SmoothPixmapTransformations", true)
            .toBool());
    viewMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleSmoothPixmap()));
    actionViewToggleSmoothPixmapTransform = a;

    viewMenu->addSeparator();

    // Slides and presentation
    tag = tr("Presentation mode", "Mainwindow presentation shortcut groups");
    a = new QAction(QPixmap(":view-video-projector.png"), 
            tr("Toggle Presentation mode", "View action") + " " +
            tr("(still experimental)"),
            this);
    viewMenu->addAction(a);
    switchboard.addAction (a, "presentationMode", Qt::Key_Asterisk | Qt::SHIFT, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(togglePresentationMode()));
    actionTogglePresentationMode = a;

    a = new QAction(tr("Next slide", "View action"), this);
    a->setStatusTip(a->text());
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapNextSlide", Qt::Key_Space, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(nextSlide()));

    a = new QAction(tr("Previous slide", "View action"), this); //FIXME-3 no shortcut yet
    a->setStatusTip(a->text());
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapPreviousSlide", shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(previousSlide()));

    // Map navigation
    tag = tr("Navigation between maps", "Mainwindow view shortcut groups");
    a = new QAction(tr("Next Map", "View action"), this);
    a->setStatusTip(a->text());
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapPrevious", Qt::SHIFT | Qt::Key_Right, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(nextEditor()));

    a = new QAction(tr("Previous Map", "View action"), this);
    a->setStatusTip(a->text());
    viewMenu->addAction(a);
    switchboard.addAction(a, "mapNext", Qt::SHIFT | Qt::Key_Left, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(previousEditor()));
}

// Connect Actions
void Main::setupConnectActions()
{
    QString tag = tr("Connect", "Shortcuts");

    QAction *a;

    a = new QAction( tr("Get Confluence user data", "Connect action"), this);
    connectMenu->addAction(a);
    switchboard.addAction(a, "confluenceUser", Qt::SHIFT | Qt::Key_C, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(getConfluenceUser()));
    actionConnectGetConfluenceUser = a;

    connectMenu->addAction(actionGetConfluencePageDetails);
    connectMenu->addAction(actionGetConfluencePageDetailsRecursively);
    connectMenu->addAction(actionGetJiraDataSubtree);
    connectMenu->addAction(actionSetJiraQuery);

    connectMenu->addSeparator();

    connectMenu->addAction(actionSettingsJIRA);
    connectMenu->addAction(actionSettingsConfluence);
}

// Mode Actions
void Main::setupModeActions()
{
    // QPopupMenu *menu = new QPopupMenu( this );
    // menuBar()->insertItem( tr( "&Mode (using modifiers)" ), menu );

    QString tag = tr("Modifier modes", "Shortcuts");
    QAction *a;
    actionGroupModModes = new QActionGroup(this);
    actionGroupModModes->setExclusive(true);

    a = new QAction(
        QIcon(":/mode-select.svg"),
        tr("Use modifier to select and reorder objects", "Mode modifier"),
        actionGroupModModes);
    addAction(a);
    switchboard.addAction(a, "mapModModePoint", Qt::Key_J, shortcutScope, tag);
    a->setCheckable(true);
    a->setChecked(true);
    actionListFiles.append(a);
    actionModModePoint = a;

    a = new QAction(
        QPixmap(":/mode-color.png"),
        tr("Use modifier to pick color from another branch",
           "Mode modifier"),
        actionGroupModModes);
    addAction(a);
    switchboard.addAction(a, "mapModModeColor", Qt::Key_K, shortcutScope, tag);
    a->setCheckable(true);
    actionListFiles.append(a);
    actionModModeColor = a;

    a = new QAction(QPixmap(":/mode-xlink.png"),
                    tr("Use modifier to draw xLinks", "Mode modifier"),
                    actionGroupModModes);
    addAction(a);
    switchboard.addAction(a, "mapModModeXLink", Qt::Key_L, shortcutScope, tag);
    a->setCheckable(true);
    actionListFiles.append(a);
    actionModModeXLink = a;

    a = new QAction(
        QPixmap(":/mode-move-object.svg"),
        tr("Use modifier to move branches without linking", "Mode modifier"),
        actionGroupModModes);
    addAction(a);
    switchboard.addAction(a, "mapModModeMoveObject", Qt::Key_Odiaeresis, shortcutScope, tag);
    a->setCheckable(true);
    actionListFiles.append(a);
    actionModModeMoveObject = a;

    a = new QAction(
        QPixmap(":/mode-move-view.png"),
        tr("Use modifier to move view without selecting", "Mode modifier"),
        actionGroupModModes);
    addAction(a);
    switchboard.addAction(a, "mapModModeMoveView", Qt::Key_Adiaeresis, shortcutScope, tag);
    a->setCheckable(true);
    actionListFiles.append(a);
    actionModModeMoveView = a;
}

void Main::addUserFlag()
{
    VymModel *m = currentModel();

    if (m) {
        QStringList imagePaths = openImageDialog(tr("Load user flag"));

        if (!imagePaths.isEmpty()) {
            QStringList::Iterator it = imagePaths.begin();
            while (it != imagePaths.end()) {
                setupFlag(*it, Flag::UserFlag, *it, "");
                ++it;
            }
        }
    }
}

void Main::setupFlagActions()
{
    Flag *flag;

    // Create System Flags

    // Tasks
    // Origin: ./share/icons/oxygen/48x48/status/task-reject.png
    flag = setupFlag(":/flag-task-new.svg", Flag::SystemFlag, "system-task-new",
                     tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    flag = setupFlag(":/flag-task-new-morning.svg", Flag::SystemFlag,
                     "system-task-new-morning", tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    flag = setupFlag(":/flag-task-new-sleeping.svg", Flag::SystemFlag,
                     "system-task-new-sleeping", tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    // Origin: ./share/icons/oxygen/48x48/status/task-reject.png
    flag = setupFlag(":/flag-task-wip.svg", Flag::SystemFlag, "system-task-wip",
                     tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    flag = setupFlag(":/flag-task-wip-morning.svg", Flag::SystemFlag,
                     "system-task-wip-morning", tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    flag = setupFlag(":/flag-task-wip-sleeping.svg", Flag::SystemFlag,
                     "system-task-wip-sleeping", tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    // Origin: ./share/icons/oxygen/48x48/status/task-complete.png
    flag = setupFlag(":/flag-task-finished.svg", Flag::SystemFlag,
                     "system-task-finished", tr("Note", "SystemFlag"));
    flag->setGroup("system-tasks");

    setupFlag(":/flag-note.svg", Flag::SystemFlag, "system-note",
              tr("Note", "SystemFlag"));

    setupFlag(":/flag-url.svg", Flag::SystemFlag, "system-url",
              tr("URL", "SystemFlag"));

    setupFlag(":/flag-jira.svg", Flag::SystemFlag, "system-jira",
              tr("Jira", "SystemFlag"));

    setupFlag(":/flag-target.svg", Flag::SystemFlag, "system-target",
              tr("Map target", "SystemFlag"));

    setupFlag(":/flag-vymlink.png", Flag::SystemFlag, "system-vymLink",
              tr("Link to another vym map", "SystemFlag"));

    setupFlag(":/flag-scrolled-right.png", Flag::SystemFlag,
              "system-scrolledright", tr("subtree is scrolled", "SystemFlag"));

    setupFlag(":/flag-tmpUnscrolled-right.png", Flag::SystemFlag,
              "system-tmpUnscrolledRight",
              tr("subtree is temporary scrolled", "SystemFlag"));

    setupFlag(":/flag-hideexport", Flag::SystemFlag, "system-hideInExport",
              tr("Hide object in exported maps", "SystemFlag"));

    addToolBarBreak();

    // Add entry now, to avoid chicken and egg problem and position toolbar
    // after all others:
    setupFlag(":/flag-stopsign.svg", Flag::StandardFlag, "stopsign",
                  tr("This won't work!", "Standardflag"), QUuid(), Qt::Key_1);

    flag = setupFlag(":/flag-hook-green.svg",
                  // flag = setupFlag ( "flags/standard/dialog-ok-apply.svg",
                  Flag::StandardFlag, "hook-green",
                  tr("Status - ok,done", "Standardflag"), QUuid(), Qt::Key_2);
    flag->setGroup("standard-status");

    flag = setupFlag(":/flag-wip.svg", Flag::StandardFlag, "wip",
                     tr("Status - work in progress", "Standardflag"), QUuid(),
                     Qt::Key_3);
    flag->setGroup("standard-status");

    flag = setupFlag(":/flag-cross-red.svg", Flag::StandardFlag, "cross-red",
                     tr("Status - missing, not started", "Standardflag"),
                     QUuid(), Qt::Key_4);
    flag->setGroup("standard-status");

    flag = setupFlag(":/flag-exclamation-mark.svg", Flag::StandardFlag,
                     "exclamationmark", tr("Take care!", "Standardflag"),
                     QUuid(), Qt::Key_Exclam);
    flag->setGroup("standard-mark");

    flag = setupFlag(":/flag-question-mark.svg", Flag::StandardFlag,
                     "questionmark", tr("Really?", "Standardflag"), QUuid(),
                     Qt::Key_Question);
    flag->setGroup("standard-mark");

    setupFlag(":/flag-info.svg", Flag::StandardFlag, "info",
                     tr("Info", "Standardflag"), QUuid(), Qt::Key_I);

    setupFlag(":/flag-lamp.svg", Flag::StandardFlag, "lamp",
                     tr("Idea!", "Standardflag"), QUuid(), Qt::Key_Asterisk);

    setupFlag(":/flag-heart.svg", Flag::StandardFlag, "heart",
                     tr("I just love...", "Standardflag"));

    flag = setupFlag(":/flag-face-smile.svg", Flag::StandardFlag, "smiley-good",
                     tr("Good", "Standardflag"), QUuid(), Qt::Key_ParenRight);
    flag->setGroup("standard-faces");

    flag = setupFlag(":/flag-face-sad.svg", Flag::StandardFlag, "smiley-sad",
                     tr("Bad", "Standardflag"), QUuid(), Qt::Key_ParenLeft);
    flag->setGroup("standard-faces");

    flag = setupFlag(":/flag-face-plain.svg", Flag::StandardFlag,
                     "smiley-plain", tr("Hm...", "Standardflag"), QUuid());
    flag->setGroup("standard-faces");

    flag = setupFlag(":/flag-face-surprise.svg", Flag::StandardFlag,
                     "smiley-omb", tr("Oh no!", "Standardflag"), QUuid());
    flag->setGroup("standard-faces");

    setupFlag(":/flag-flash.svg", Flag::StandardFlag, "flash",
                     tr("Dangerous", "Standardflag"));

    flag = setupFlag(":/flag-arrow-up.svg", Flag::StandardFlag, "arrow-up",
                     tr("Important", "Standardflag"), QUuid(),
                     Qt::SHIFT | Qt::Key_PageUp);
    flag->setGroup("standard-arrow");

    flag = setupFlag(":/flag-arrow-down.svg", Flag::StandardFlag, "arrow-down",
                     tr("Unimportant", "Standardflag"), QUuid(),
                     Qt::SHIFT | Qt::Key_PageDown);
    flag->setGroup("standard-arrow");

    flag = setupFlag(":/flag-arrow-2up.svg", Flag::StandardFlag, "2arrow-up",
                     tr("Very important!", "Standardflag"), QUuid(),
                     Qt::SHIFT | Qt::CTRL | Qt::Key_PageUp);
    flag->setGroup("standard-arrow");

    flag = setupFlag(":/flag-arrow-2down.svg", Flag::StandardFlag,
                     "2arrow-down", tr("Very unimportant!", "Standardflag"),
                     QUuid(), Qt::SHIFT | Qt::CTRL | Qt::Key_PageDown);
    flag->setGroup("standard-arrow");

    setupFlag(":/flag-thumb-up.png", Flag::StandardFlag, "thumb-up",
                     tr("I like this", "Standardflag"));

    setupFlag(":/flag-thumb-down.png", Flag::StandardFlag, "thumb-down",
                     tr("I do not like this", "Standardflag"));

    // Original khelpcenter.png
    setupFlag(":/flag-lifebelt.svg", Flag::StandardFlag, "lifebelt",
                     tr("This will help", "Standardflag"));
    setupFlag(":/flag-phone.svg", Flag::StandardFlag, "phone",
                     tr("Call...", "Standardflag"));

    setupFlag(":/flag-clock.svg", Flag::StandardFlag, "clock",
                     tr("Time critical", "Standardflag"));

    setupFlag(":/flag-present.png", Flag::StandardFlag, "present",
                     tr("Surprise!", "Standardflag"));

    setupFlag(":/flag-rose.png", Flag::StandardFlag, "rose",
                     tr("Rose", "Standardflag"));

    // Freemind flags
    setupFlag(":/freemind/warning.png", Flag::FreemindFlag,
                     "freemind-warning", tr("Important", "Freemind flag"));

    for (int i = 1; i < 8; i++) {
        flag = setupFlag(QString(":/freemind/priority-%1.png").arg(i),
                         Flag::FreemindFlag,
                         QString("freemind-priority-%1").arg(i),
                         tr("Important", "Freemind flag"));
        flag->setGroup("freemind-priority");
    }

    setupFlag(":/freemind/back.png", Flag::FreemindFlag, "freemind-back",
                     tr("Back", "Freemind flag"));

    setupFlag(":/freemind/forward.png", Flag::FreemindFlag,
                     "freemind-forward", tr("Forward", "Freemind flag"));

    setupFlag(":/freemind/attach.png", Flag::FreemindFlag,
                     "freemind-attach", tr("Look here", "Freemind flag"));

    setupFlag(":/freemind/clanbomber.png", Flag::FreemindFlag,
                     "freemind-clanbomber", tr("Dangerous", "Freemind flag"));

    setupFlag(":/freemind/desktopnew.png", Flag::FreemindFlag,
                  "freemind-desktopnew", tr("Don't forget", "Freemind flag"));

    setupFlag(":/freemind/flag.png", Flag::FreemindFlag, "freemind-flag",
                     tr("Flag", "Freemind flag"));

    setupFlag(":/freemind/gohome.png", Flag::FreemindFlag,
                     "freemind-gohome", tr("Home", "Freemind flag"));

    setupFlag(":/freemind/kaddressbook.png", Flag::FreemindFlag,
                     "freemind-kaddressbook", tr("Telephone", "Freemind flag"));

    setupFlag(":/freemind/knotify.png", Flag::FreemindFlag,
                     "freemind-knotify", tr("Music", "Freemind flag"));

    setupFlag(":/freemind/korn.png", Flag::FreemindFlag, "freemind-korn",
                     tr("Mailbox", "Freemind flag"));

    setupFlag(":/freemind/mail.png", Flag::FreemindFlag, "freemind-mail",
                     tr("Mail", "Freemind flag"));

    setupFlag(":/freemind/password.png", Flag::FreemindFlag,
                     "freemind-password", tr("Password", "Freemind flag"));

    setupFlag(":/freemind/pencil.png", Flag::FreemindFlag,
                     "freemind-pencil", tr("To be improved", "Freemind flag"));

    setupFlag(":/freemind/stop.png", Flag::FreemindFlag, "freemind-stop",
                     tr("Stop", "Freemind flag"));

    setupFlag(":/freemind/wizard.png", Flag::FreemindFlag,
                     "freemind-wizard", tr("Magic", "Freemind flag"));

    setupFlag(":/freemind/xmag.png", Flag::FreemindFlag, "freemind-xmag",
                     tr("To be discussed", "Freemind flag"));

    setupFlag(":/freemind/bell.png", Flag::FreemindFlag, "freemind-bell",
                     tr("Reminder", "Freemind flag"));

    setupFlag(":/freemind/bookmark.png", Flag::FreemindFlag,
                     "freemind-bookmark", tr("Excellent", "Freemind flag"));

    setupFlag(":/freemind/penguin.png", Flag::FreemindFlag,
                     "freemind-penguin", tr("Linux", "Freemind flag"));

    setupFlag(":/freemind/licq.png", Flag::FreemindFlag, "freemind-licq",
                     tr("Sweet", "Freemind flag"));
}

Flag *Main::setupFlag(const QString &path, Flag::FlagType type,
                      const QString &name, const QString &tooltip,
                      const QUuid &uid, const QKeySequence &keyseq)
{
    Flag *flag = nullptr;

    // Create flag in toolbar
    switch (type) {
    case Flag::FreemindFlag:
        // Maybe introduce dedicated toolbar later,
        // so for now switch to standard flag
        flag = standardFlagsMaster->createFlag(path);
        break;

    case Flag::StandardFlag:
        flag = standardFlagsMaster->createFlag(path);
        break;

    case Flag::UserFlag:
        flag = userFlagsMaster->createFlag(path);

        if (flag &&!uid.isNull())
            // User flags read from file already have a Uuid - use it
            flag->setUuid(uid);
        break;

    case Flag::SystemFlag:
        flag = systemFlagsMaster->createFlag(path);
        break;

    default:
        qWarning() << "Unknown flag type in MainWindow::setupFlag";
        break;
    }

    if (!flag)
        return nullptr;

    flag->setName(name);
    flag->setToolTip(tooltip);
    flag->setType(type);

    if (type == Flag::SystemFlag)
        return flag;

    // StandardFlag or user flag

    QAction *a;

    // Set icon for action
    ImageContainer *ic = flag->getImageContainer();
    a = new QAction(ic->getIcon(), flag->getUuid().toString(), this);

    flag->setAction(a);
    a->setCheckable(true);
    a->setObjectName(flag->getUuid().toString());
    if (tooltip.isEmpty())
        a->setToolTip(flag->getName()); // Stripped name
    else
        a->setToolTip(tooltip);

    if (keyseq != 0) {
        a->setShortcut(keyseq);
        a->setShortcutContext(Qt::WidgetShortcut);

        // Allow mapEditors to actually trigger this action
        mapEditorActions.append(a);
        taskEditorActions.append(a);
    }

    switch (type) {
    case Flag::FreemindFlag:
        // Hide freemind flags per default
        // Maybe introduce dedicate toolbar later,
        // so for now switch to standard flag
        flag->setVisible(false);
        standardFlagsMaster->addActionToToolbar(a);

        connect(a, SIGNAL(triggered()), this, SLOT(flagChanged()));
    case Flag::StandardFlag:
        // Hide some old flags, if not used
        if (name == "present" || name == "rose" || name == "phone" ||
            name == "clock")
            flag->setVisible(false);
        standardFlagsMaster->addActionToToolbar(a);
        connect(a, SIGNAL(triggered()), this, SLOT(flagChanged()));
        break;
    case Flag::UserFlag:
        userFlagsMaster->addActionToToolbar(a);
        connect(a, SIGNAL(triggered()), this, SLOT(flagChanged()));
        break;
    default:
        qWarning() << "Unknown flag type in MainWindow::setupFlag";
    }

    a->setVisible(flag->isVisible());

    return flag;
}

// Network Actions
void Main::setupNetworkActions()
{
    if (!settings.value("/mainwindow/showTestMenu", false).toBool())
        return;

    QAction *a;

    a = new QAction("Start TCPserver for MapEditor", this);
    connect(a, SIGNAL(triggered()), this, SLOT(networkStartServer()));

    a = new QAction("Connect MapEditor to server", this);
    connect(a, SIGNAL(triggered()), this, SLOT(networkConnect()));
}

// Settings Actions
void Main::setupSettingsActions()
{
    QMenu *settingsMenu = menuBar()->addMenu(tr("Settings"));

    QAction *a;

    a = new QAction(
        tr("Check for release notes and updates", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/downloads/enabled", true).toBool());
    connect(a, SIGNAL(triggered()), this, SLOT(settingsToggleDownloads()));
    settingsMenu->addAction(a);
    actionSettingsToggleDownloads = a;

    a = new QAction(tr("Set author for new maps", "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsDefaultMapAuthor()));
    settingsMenu->addAction(a);

    settingsMenu->addSeparator();

    a = new QAction(tr("Set application to open pdf files", "Settings action") +
                        "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsPDF()));
    // FIXME-3 No longer needed settingsMenu->addAction(a);

    a = new QAction(
        tr("Set application to open external links", "Settings action") + "...",
        this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsURL()));
    // FIXME-3 No longer needed settingsMenu->addAction(a);

    a = new QAction(tr("Confluence Credentials", "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsConfluence()));
    settingsMenu->addAction(a);
    actionSettingsConfluence = a;

    a = new QAction(tr("JIRA Credentials", "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsJIRA()));
    settingsMenu->addAction(a);
    actionSettingsJIRA = a;

    settingsMenu->addSeparator();

    a = new QAction(tr("Set path for new maps", "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsDefaultMapPath()));
    settingsMenu->addAction(a);

    a = new QAction(tr("Set path for macros", "Settings action") + "...", this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsMacroPath()));
    settingsMenu->addAction(a);

    a = new QAction(tr("Set number of undo levels", "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsUndoLevels()));
    settingsMenu->addAction(a);

    settingsMenu->addSeparator();

    a = new QAction(tr("Autosave", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/system/autosave/use", true).toBool());
    settingsMenu->addAction(a);
    actionSettingsToggleAutosave = a;

    a = new QAction(tr("Autosave time", "Settings action") + "...", this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsAutosaveTime()));
    settingsMenu->addAction(a);
    actionSettingsAutosaveTime = a;

    // Disable certain actions during testing
    if (testmode) {
        actionSettingsToggleAutosave->setChecked(false);
        actionSettingsToggleAutosave->setEnabled(false);
        actionSettingsAutosaveTime->setEnabled(false);
    }

    a = new QAction(tr("Write backup file on save", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/system/writeBackupFile", false).toBool());
    connect(a, SIGNAL(triggered()), this,
            SLOT(settingsToggleWriteBackupFile()));
    settingsMenu->addAction(a);
    actionSettingsWriteBackupFile = a;

    a = new QAction(tr("Logfile settings", "Settings action") + "...", this);
    connect(a, SIGNAL(triggered()), this, SLOT(settingsActionLog()));
    settingsMenu->addAction(a);

    settingsMenu->addSeparator();

    a = new QAction(tr("Select branch after adding it", "Settings action"),
                    this);
    a->setCheckable(true);
    a->setChecked(
        settings.value("/mapeditor/editmode/autoSelectNewBranch", false)
            .toBool());
    settingsMenu->addAction(a);
    actionSettingsAutoSelectNewBranch = a;

    a = new QAction(tr("Exclusive flags", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(
        settings.value("/mapeditor/editmode/useFlagGroups", true).toBool());
    settingsMenu->addAction(a);
    actionSettingsUseFlagGroups = a;

    a = new QAction(tr("Use hide flags", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/export/useHideExport", true).toBool());
    settingsMenu->addAction(a);
    actionSettingsUseHideExport = a;

    settingsMenu->addSeparator();

    a = new QAction(
        tr("Dark theme", "Settings action") + "...",
        this);
    connect(a, SIGNAL(triggered()), this,
            SLOT(settingsDarkTheme()));
    settingsMenu->addAction(a);
    actionSettingsDarkTheme= a;

    a = new QAction(
        tr("Number of visible parents in task editor", "Settings action") + "...",
        this);
    connect(a, SIGNAL(triggered()), this,
            SLOT(settingsShowParentsLevelTasks()));
    settingsMenu->addAction(a);
    actionSettingsShowParentsLevelTasks = a;

    a = new QAction(tr("Number of visible parents in find results window",
                       "Settings action") + "...",
                    this);
    connect(a, SIGNAL(triggered()), this,
            SLOT(settingsShowParentsLevelFindResults()));
    settingsMenu->addAction(a);
    actionSettingsShowParentsLevelFindResults = a;

    a = new QAction(tr("Animation", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/animation/use", true).toBool());
    connect(a, SIGNAL(triggered()), this, SLOT(settingsToggleAnimation()));
    settingsMenu->addAction(a);
    actionSettingsUseAnimation = a;

    a = new QAction(tr("Automatic layout", "Settings action"), this);
    a->setCheckable(true);
    a->setChecked(settings.value("/mainwindow/autoLayout/use", true).toBool());
    connect(a, SIGNAL(triggered()), this, SLOT(settingsToggleAutoLayout()));
    settingsMenu->addAction(a);
    actionSettingsToggleAutoLayout = a;
}

// Test Actions
void Main::setupTestActions()
{
    QMenu *testMenu = menuBar()->addMenu(tr("Test"));

    QString tag = "Testing";
    QAction *a;
    a = new QAction("Test function 1", this);
    testMenu->addAction(a);
    switchboard.addAction(a, "mapTest1", Qt::ALT | Qt::Key_T, shortcutScope, tag);
    connect(a, SIGNAL(triggered()), this, SLOT(testFunction1()));

    a = new QAction("Test function 2", this);
    testMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(testFunction2()));

    a = new QAction("Toggle hide export mode", this);
    a->setCheckable(true);
    a->setChecked(false);
    testMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(toggleHideExport()));
    actionToggleHideMode = a;

    testMenu->addAction(actionToggleWinter);
}

// Help Actions
void Main::setupHelpActions()
{
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help", "Help menubar entry"));

    QAction *a;
    a = new QAction(tr("Open VYM Documentation (pdf) ", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpDoc()));

    a = new QAction(tr("Open VYM example maps ", "Help action") + "...", this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpDemo()));
    helpMenu->addSeparator();

    a = new QAction(tr("Download and show release notes", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(checkReleaseNotes()));

    a = new QAction(tr("Check, if updates are available", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(checkUpdates()));
    helpMenu->addSeparator();

    a = new QAction(tr("Show keyboard shortcuts", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpShortcuts()));

    a = new QAction(tr("Show keyboard macros", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpMacros()));

    a = new QAction(tr("Show scripting commands", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpScriptingCommands()));

    a = new QAction(tr("Debug info", "Option to show debugging info"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpDebugInfo()));

    helpMenu->addSeparator();
    a = new QAction(tr("Help VYM development", "Help action"), this);
    helpMenu->addAction(a);
    connect(a, SIGNAL(triggered()), this, SLOT(helpVymDevelopment()));

    a = new QAction(tr("About QT", "Help action"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(helpAboutQT()));
    helpMenu->addAction(a);

    a = new QAction(tr("About VYM", "Help action"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(helpAbout()));
    helpMenu->addAction(a);
}

// Context Menus
void Main::setupContextMenus()
{
    // Context menu for goto/move targets  (populated on demand)
    targetsContextMenu = new QMenu(this);

    // Context Menu for branch or mapcenter
    branchContextMenu = new QMenu(this);

    // Submenu "Add"
    branchAddContextMenu = branchContextMenu->addMenu(tr("Add"));
    branchAddContextMenu->addAction(actionPaste);
    branchAddContextMenu->addAction(actionLoadImage);
    branchAddContextMenu->addAction(actionAddMapCenter);
    branchAddContextMenu->addSeparator();
    branchAddContextMenu->addAction(actionAddBranch);
    branchAddContextMenu->addAction(actionAddBranchBefore);
    branchAddContextMenu->addAction(actionAddBranchAbove);
    branchAddContextMenu->addAction(actionAddBranchBelow);
    branchAddContextMenu->addSeparator();
    branchAddContextMenu->addAction(actionImportAdd);
    branchAddContextMenu->addAction(actionImportReplace);
    foreach (auto a, branchAddContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Submenu "Remove"
    branchRemoveContextMenu =
        branchContextMenu->addMenu(tr("Remove", "Context menu name"));
    branchRemoveContextMenu->addAction(actionCut);
    branchRemoveContextMenu->addAction(actionDelete);
    branchRemoveContextMenu->addAction(actionDeleteKeepChildren);
    branchRemoveContextMenu->addAction(actionDeleteChildren);
    foreach (auto a, branchRemoveContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Submenu "Hierarchy"
    branchHierarchyContextMenu =
        branchContextMenu->addMenu(tr("Hierarchy", "Context menu name"));
    branchHierarchyContextMenu->addAction(actionMoveUp);
    branchHierarchyContextMenu->addAction(actionMoveDown);
    branchHierarchyContextMenu->addAction(actionMoveDownDiagonally);
    branchHierarchyContextMenu->addAction(actionMoveUpDiagonally);
    branchHierarchyContextMenu->addAction(actionDetach);
    branchHierarchyContextMenu->addAction(actionSortChildren);
    branchHierarchyContextMenu->addAction(actionSortBackChildren);
    //foreach (auto a, branchRemoveContextMenu->actions())
    //   a->setShortcutVisibleInContextMenu(true);

    // Submenu "Geometry"
    branchGeometryContextMenu =
        branchContextMenu->addMenu(tr("Geometry", "Context menu name"));
    branchGeometryContextMenu->addAction(actionGrowSelectionSize);
    branchGeometryContextMenu->addAction(actionShrinkSelectionSize);
    branchGeometryContextMenu->addAction(actionResetSelectionSize);
    branchGeometryContextMenu->addAction(actionRotateSubtreeCW);
    branchGeometryContextMenu->addAction(actionRotateSubtreeCCW);

    branchContextMenu->addSeparator();
    branchContextMenu->addAction(actionLoadImage);

    branchContextMenu->addSeparator();

    // Context menu for tasks
    taskContextMenu = branchContextMenu->addMenu(tr("Tasks", "Context menu"));
    taskContextMenu->addAction(actionToggleTask);
    taskContextMenu->addAction(actionCycleTaskStatus);
    taskContextMenu->addAction(actionTaskResetDeltaPrio);
    taskContextMenu->addSeparator();
    taskContextMenu->addAction(actionTaskSleep0);
    taskContextMenu->addAction(actionTaskSleepN);
    taskContextMenu->addAction(actionTaskSleep1);
    taskContextMenu->addAction(actionTaskSleep2);
    taskContextMenu->addAction(actionTaskSleep3);
    taskContextMenu->addAction(actionTaskSleep4);
    taskContextMenu->addAction(actionTaskSleep5);
    taskContextMenu->addAction(actionTaskSleep7);
    taskContextMenu->addAction(actionTaskSleep14);
    taskContextMenu->addAction(actionTaskSleep28);
    foreach (auto a, taskContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Submenu for Links (URLs, vymLinks)
    branchLinksContextMenu = new QMenu(this);

    branchLinksContextMenu = branchContextMenu->addMenu(
        tr("References (URLs, vymLinks, ...)", "Context menu name"));
    branchLinksContextMenu->addAction(actionOpenUrl);
    branchLinksContextMenu->addAction(actionOpenMultipleVisUrls);
    branchLinksContextMenu->addAction(actionOpenMultipleUrls);
    branchLinksContextMenu->addAction(actionOpenMultipleUrlsPrivate);
    branchLinksContextMenu->addAction(actionURLNew);
    branchLinksContextMenu->addAction(actionLocalURL);
    branchLinksContextMenu->addAction(actionGetURLsFromNote);
    branchLinksContextMenu->addAction(actionHeading2URL);
    branchLinksContextMenu->addAction(actionGetJiraDataSubtree);
    branchLinksContextMenu->addAction(actionGetConfluencePageDetails);
    branchLinksContextMenu->addAction(actionGetConfluencePageDetailsRecursively);
    branchLinksContextMenu->addSeparator();
    branchLinksContextMenu->addAction(actionOpenVymLink);
    branchLinksContextMenu->addAction(actionOpenVymLinkBackground);
    branchLinksContextMenu->addAction(actionOpenMultipleVymLinks);
    branchLinksContextMenu->addAction(actionEditVymLink);
    branchLinksContextMenu->addAction(actionDeleteVymLink);
    foreach (auto a, branchLinksContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Context Menu for XLinks in a branch menu
    // This will be populated "on demand" in updateActions
    QString tag = tr("XLinks", "Menu for file actions");
    branchContextMenu->addSeparator();
    branchXLinksContextMenuEdit =
        branchContextMenu->addMenu(tr("Edit XLink", "Context menu name"));
    connect(branchXLinksContextMenuEdit, SIGNAL(triggered(QAction *)), this,
            SLOT(editEditXLink(QAction *)));
    QAction *a;
    branchXLinksContextMenuFollow =
        branchContextMenu->addMenu(tr("Follow XLink", "Context menu name"));
    connect(branchXLinksContextMenuFollow, SIGNAL(triggered(QAction *)), this,
            SLOT(followReference(QAction *)));

    branchContextMenu->addSeparator();
    branchContextMenu->addAction(actionViewTogglePropertyEditor);

    // Context menu for floatimage
    floatimageContextMenu = new QMenu(this);
    a = new QAction(tr("Save image", "Context action"), this);
    connect(a, SIGNAL(triggered()), this, SLOT(editSaveImage()));
    floatimageContextMenu->addAction(a);

    floatimageContextMenu->addSeparator();
    floatimageContextMenu->addAction(actionCopy);
    floatimageContextMenu->addAction(actionCut);

    floatimageContextMenu->addSeparator();
    floatimageContextMenu->addAction(actionGrowSelectionSize);
    floatimageContextMenu->addAction(actionShrinkSelectionSize);
    floatimageContextMenu->addAction(actionFormatHideLinkUnselected);

    foreach (auto a, floatimageContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Context menu for canvas
    canvasContextMenu = new QMenu(this);

    canvasContextMenu->addAction(actionAddMapCenter);

    canvasContextMenu->addSeparator();

    canvasContextMenu->addAction(actionFormatFont);

    canvasContextMenu->addSeparator();

    canvasContextMenu->addActions(actionGroupFormatLinkStyles->actions());

    canvasContextMenu->addSeparator();

    canvasContextMenu->addAction(actionFormatLinkColorHint);

    canvasContextMenu->addSeparator();

    canvasContextMenu->addAction(actionFormatLinkColor);
    canvasContextMenu->addAction(actionFormatSelectionColor);
    canvasContextMenu->addAction(actionFormatBackground);

    canvasContextMenu->addSeparator();
    canvasContextMenu->addAction(actionMapProperties);
    foreach (auto a, canvasContextMenu->actions())
        a->setShortcutVisibleInContextMenu(true);

    // Menu for last opened files
    // Create actions
    for (int i = 0; i < MaxRecentFiles; ++i) {
        recentFileActions[i] = new QAction(this);
        recentFileActions[i]->setVisible(false);
        fileLastMapsMenu->addAction(recentFileActions[i]);
        connect(recentFileActions[i], SIGNAL(triggered()), this,
                SLOT(fileLoadRecent()));
    }
    fileLastMapsMenu->addSeparator();
    fileLastMapsMenu->addAction(actionClearRecent); // See Main::fileClearRecent()

    setupRecentMapsMenu();
}

void Main::setupRecentMapsMenu()
{
    QStringList files =
        settings.value("/mainwindow/recentFileList").toStringList();

    int numRecentFiles = qMin(files.size(), (int)MaxRecentFiles);

    for (int i = 0; i < numRecentFiles; ++i) {
        QString text = QString("&%1 %2").arg(i + 1).arg(files[i]);
        recentFileActions[i]->setText(text);
        recentFileActions[i]->setData(files[i]);
        recentFileActions[i]->setVisible(true);
    }
    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFileActions[j]->setVisible(false);

    actionClearRecent->setEnabled(!files.empty());
}

void Main::setupMacros()
{
    for (int i = 0; i <= 47; i++) {
        macroActions[i] = new QAction(this);
        macroActions[i]->setData(i);
        addAction(macroActions[i]);
        connect(macroActions[i], SIGNAL(triggered()), this, SLOT(callMacro()));
    }
    macroActions[0]->setShortcut(Qt::Key_F1);
    macroActions[1]->setShortcut(Qt::Key_F2);
    macroActions[2]->setShortcut(Qt::Key_F3);
    macroActions[3]->setShortcut(Qt::Key_F4);
    macroActions[4]->setShortcut(Qt::Key_F5);
    macroActions[5]->setShortcut(Qt::Key_F6);
    macroActions[6]->setShortcut(Qt::Key_F7);
    macroActions[7]->setShortcut(Qt::Key_F8);
    macroActions[8]->setShortcut(Qt::Key_F9);
    macroActions[9]->setShortcut(Qt::Key_F10);
    macroActions[10]->setShortcut(Qt::Key_F11);
    macroActions[11]->setShortcut(Qt::Key_F12);

    // Shift Modifier
    macroActions[12]->setShortcut(Qt::Key_F1 | Qt::SHIFT);
    macroActions[13]->setShortcut(Qt::Key_F2 | Qt::SHIFT);
    macroActions[14]->setShortcut(Qt::Key_F3 | Qt::SHIFT);
    macroActions[15]->setShortcut(Qt::Key_F4 | Qt::SHIFT);
    macroActions[16]->setShortcut(Qt::Key_F5 | Qt::SHIFT);
    macroActions[17]->setShortcut(Qt::Key_F6 | Qt::SHIFT);
    macroActions[18]->setShortcut(Qt::Key_F7 | Qt::SHIFT);
    macroActions[19]->setShortcut(Qt::Key_F8 | Qt::SHIFT);
    macroActions[20]->setShortcut(Qt::Key_F9 | Qt::SHIFT);
    macroActions[21]->setShortcut(Qt::Key_F10 | Qt::SHIFT);
    macroActions[22]->setShortcut(Qt::Key_F11 | Qt::SHIFT);
    macroActions[23]->setShortcut(Qt::Key_F12 | Qt::SHIFT);

    // Ctrl Modifier
    macroActions[24]->setShortcut(Qt::Key_F1 | Qt::CTRL);
    macroActions[25]->setShortcut(Qt::Key_F2 | Qt::CTRL);
    macroActions[26]->setShortcut(Qt::Key_F3 | Qt::CTRL);
    macroActions[27]->setShortcut(Qt::Key_F4 | Qt::CTRL);
    macroActions[28]->setShortcut(Qt::Key_F5 | Qt::CTRL);
    macroActions[29]->setShortcut(Qt::Key_F6 | Qt::CTRL);
    macroActions[30]->setShortcut(Qt::Key_F7 | Qt::CTRL);
    macroActions[31]->setShortcut(Qt::Key_F8 | Qt::CTRL);
    macroActions[32]->setShortcut(Qt::Key_F9 | Qt::CTRL);
    macroActions[33]->setShortcut(Qt::Key_F10 | Qt::CTRL);
    macroActions[34]->setShortcut(Qt::Key_F11 | Qt::CTRL);
    macroActions[35]->setShortcut(Qt::Key_F12 | Qt::CTRL);

    // Shift + Ctrl Modifier
    macroActions[36]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F1);
    macroActions[37]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F2);
    macroActions[38]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F3);
    macroActions[39]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F4);
    macroActions[40]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F5);
    macroActions[41]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F6);
    macroActions[42]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F7);
    macroActions[43]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F8);
    macroActions[44]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F9);
    macroActions[45]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F10);
    macroActions[46]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F11);
    macroActions[47]->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_F12);
}

void Main::setupToolbars()
{
    QList <QToolBar*> toolbars;
    // File actions
    fileToolbar =
        addToolBar(tr("File actions toolbar", "Toolbar for file actions"));
    toolbars << fileToolbar;

    fileToolbar->setObjectName("fileTB");
    fileToolbar->addAction(actionFileNew);
    fileToolbar->addAction(actionFileOpen);
    fileToolbar->addAction(actionFileSave);
    fileToolbar->addAction(actionFileExportLast);
    fileToolbar->addAction(actionFilePrint);

    // Undo/Redo and clipboard
    clipboardToolbar = addToolBar(tr("Undo and clipboard toolbar",
                                     "Toolbar for redo/undo and clipboard"));
    toolbars << clipboardToolbar;
    clipboardToolbar->setObjectName("clipboard toolbar");
    clipboardToolbar->addAction(actionUndo);
    clipboardToolbar->addAction(actionRedo);
    clipboardToolbar->addAction(actionCopy);
    clipboardToolbar->addAction(actionCut);
    clipboardToolbar->addAction(actionPaste);

    // Basic edits
    editActionsToolbar = addToolBar(tr("Edit actions toolbar", "Toolbar name"));
    toolbars << editActionsToolbar;
    editActionsToolbar->setObjectName("basic edit actions TB");
    editActionsToolbar->addAction(actionAddMapCenter);
    editActionsToolbar->addAction(actionAddBranch);
    editActionsToolbar->addAction(actionMoveUp);
    editActionsToolbar->addAction(actionMoveDown);
    editActionsToolbar->addAction(actionMoveDownDiagonally);
    editActionsToolbar->addAction(actionMoveUpDiagonally);
    editActionsToolbar->addAction(actionSortChildren);
    editActionsToolbar->addAction(actionSortBackChildren);
    editActionsToolbar->addAction(actionToggleScroll);
    editActionsToolbar->addAction(actionToggleHideExport);
    editActionsToolbar->addAction(actionToggleTask);
    editActionsToolbar->addAction(actionToggleTarget);
    // editActionsToolbar->addAction (actionExpandAll);
    // editActionsToolbar->addAction (actionExpandOneLevel);
    // editActionsToolbar->addAction (actionCollapseOneLevel);
    // editActionsToolbar->addAction (actionCollapseUnselected);

    // Selections
    selectionToolbar = addToolBar(tr("Selection toolbar", "Toolbar name"));
    toolbars << selectionToolbar;
    selectionToolbar->setObjectName("toolbar for selecting items");
    selectionToolbar->addAction(actionFind);
    selectionToolbar->addAction(actionSelectPrevious);
    selectionToolbar->addAction(actionSelectNext);

    // URLs and vymLinks
    referencesToolbar = addToolBar(
        tr("URLs and vymLinks toolbar", "Toolbar for URLs and vymlinks"));
    toolbars << referencesToolbar;
    referencesToolbar->setObjectName("URLs and vymlinks toolbar");
    referencesToolbar->addAction(actionURLNew);
    referencesToolbar->addAction(actionEditVymLink);

    // Format and colors
    colorsToolbar = new QToolBar(tr("Colors toolbar", "Colors toolbar name"));
    toolbars << colorsToolbar;
    colorsToolbar->setObjectName("colorsTB");

    actionGroupQuickColors = new QActionGroup(this);
    actionGroupQuickColors->setExclusive(true);

    // Define quickColors
    QColor c;
    c = QColor::fromString("#ff0000"); quickColors << c;  // Red
    c = QColor::fromString("#d95100"); quickColors << c;  // Orange
    c = QColor::fromString("#009900"); quickColors << c;  // Green
    c = QColor::fromString("#aa00ff"); quickColors << c;  // Purple
    c = QColor::fromString("#0000ff"); quickColors << c;  // Blue
    c = QColor::fromString("#00aaff"); quickColors << c;  // LightBlue
    usingDarkTheme ? vymBlueColor = c : vymBlueColor = quickColors.count() - 2;
    c = QColor::fromString("#000000"); quickColors << c;  // Black
    c = QColor::fromString("#444444"); quickColors << c;  // Dark gray
    c = QColor::fromString("#aaaaaa"); quickColors << c;  // Light gray
    c = QColor::fromString("#ffffff"); quickColors << c;  // White
    //c.setNamedColor ("#00aa7f"); quickColors << c;  // Light green
    //c.setNamedColor ("#c466ff"); quickColors << c;  // Light purple

    QPixmap pix(16, 16);
    QAction *a;
    int n = 0;
    foreach (c, quickColors) {
        pix.fill(c);
        a = new QAction(pix, tr("Select color (Press Shift for more options)") + QString("..."), actionGroupQuickColors);
        a->setCheckable(true);
        a->setData(n);
        //formatMenu->addAction(a);
        // switchboard.addAction(a, "mapFormatColor", shortcutScope, tag);
        connect(a, SIGNAL(triggered()), this, SLOT(quickColorPressed()));
        colorsToolbar->addAction(a);
        n++;
    }
    actionGroupQuickColors->actions().first()->setChecked(true);

    colorsToolbar->addAction(actionFormatPickColor);
    colorsToolbar->addAction(actionFormatColorBranch);
    colorsToolbar->addAction(actionFormatColorSubtree);
    // Only place toolbar on very first startup
    if (settings.value("/mainwindow/recentFileList").toStringList().isEmpty())
        addToolBar (Qt::RightToolBarArea, colorsToolbar);
    else
        addToolBar (colorsToolbar);

    // View transformations (shrink/grow/rotate)    // FIXME-3 add shortcut to rotate selected subtree/item   Fn-key maybe and macro?
    viewTransformationsToolbar = addToolBar(tr("View toolbar", "View Toolbar name"));
    toolbars << viewTransformationsToolbar;
    viewTransformationsToolbar->setObjectName("viewTB");
    viewTransformationsToolbar->addAction(actionZoomIn);
    viewTransformationsToolbar->addAction(actionZoomOut);
    viewTransformationsToolbar->addAction(actionZoomReset);
    viewTransformationsToolbar->addAction(actionCenterOn);
    viewTransformationsToolbar->addAction(actionRotateCounterClockwise);
    viewTransformationsToolbar->addAction(actionRotateClockwise);

    // Modified special view, e.g. presentation mode or temporary hiding of branches
    limitedViewToolbar = addToolBar(tr("Limited view toolbar", "View Toolbar name"));
    toolbars << limitedViewToolbar ;
    limitedViewToolbar->setObjectName("limitedViewTB");
    limitedViewToolbar->addAction(actionTogglePresentationMode);
    limitedViewToolbar->addAction(actionToggleHideTmpMode);

    // Editors
    editorsToolbar = addToolBar(tr("Editors toolbar", "Editor Toolbar name"));
    toolbars << editorsToolbar ;
    editorsToolbar->setObjectName("editorsTB");
    editorsToolbar->addAction(actionViewToggleNoteEditor);
    editorsToolbar->addAction(actionViewToggleHeadingEditor);
    editorsToolbar->addAction(actionViewToggleTreeEditors);
    editorsToolbar->addAction(actionViewToggleTaskEditor);
    editorsToolbar->addAction(actionViewToggleSlideEditors);
    editorsToolbar->addAction(actionViewToggleScriptEditor);
    editorsToolbar->addAction(actionViewToggleHistoryWindow);

    // Modifier modes
    modModesToolbar =
        addToolBar(tr("Modifier modes toolbar", "Modifier Toolbar name"));
    toolbars << modModesToolbar ;
    modModesToolbar->setObjectName("modesTB");
    modModesToolbar->addAction(actionModModePoint);
    modModesToolbar->addAction(actionModModeColor);
    modModesToolbar->addAction(actionModModeXLink);
    modModesToolbar->addAction(actionModModeMoveObject);
    modModesToolbar->addAction(actionModModeMoveView);

    // Create flag toolbars (initialized later in setupFlagActions() )
    addToolBarBreak();
    standardFlagsToolbar =
        addToolBar(tr("Standard Flags toolbar", "Standard Flag Toolbar"));
    toolbars << standardFlagsToolbar ;
    standardFlagsToolbar->setObjectName("standardFlagTB");
    standardFlagsMaster->setToolBar(standardFlagsToolbar);

    userFlagsToolbar =
        addToolBar(tr("User Flags toolbar", "user Flags Toolbar"));
    toolbars << userFlagsToolbar ;
    userFlagsToolbar->setObjectName("userFlagsTB");

    userFlagsMaster->setToolBar(userFlagsToolbar);
    userFlagsMaster->createConfigureAction();

    foreach (auto tb, toolbars) {
        // Add View menu
        toolbarsMenu->addAction(tb->toggleViewAction());

        // Set backgrounds
        tb->setStyleSheet(toolBarStyle);
    }

    // Initialize toolbarStates for presentation mode
    toolbarStates[fileToolbar] = true;
    toolbarStates[clipboardToolbar] = true;
    toolbarStates[editActionsToolbar] = true;
    toolbarStates[selectionToolbar] = false;
    toolbarStates[colorsToolbar] = true;
    toolbarStates[viewTransformationsToolbar] = true;
    toolbarStates[limitedViewToolbar] = false;
    toolbarStates[modModesToolbar] = false;
    toolbarStates[referencesToolbar] = false;
    toolbarStates[editorsToolbar] = false;
    toolbarStates[standardFlagsToolbar] = true;
    toolbarStates[userFlagsToolbar] = false;

    // Initialize toolbar visibilities and switch off presentation mode
    presentationMode = true;
    togglePresentationMode();
}

VymView *Main::currentView() const
{
    if (tabWidget->currentWidget())
        return (VymView *)tabWidget->currentWidget();
    else
        return nullptr;
}

VymView *Main::view(const int i) { return (VymView *)tabWidget->widget(i); }

MapEditor *Main::currentMapEditor() const
{
    if (tabWidget->currentWidget())
        return currentView()->getMapEditor();
    return nullptr;
}

uint Main::currentMapId() const
{
    VymModel *m = currentModel();
    if (m)
        return m->modelId();
    else
        return 0;
}

int Main::currentMapIndex() const { return tabWidget->currentIndex(); }

VymModel *Main::currentModel() const
{
    VymView *vv = currentView();
    if (vv)
        return vv->getModel();
    else
        return nullptr;
}

VymModel *Main::modelWithId(uint id) // Used in BugAgent
{
    if (id <= 0)
        return nullptr;

    for (int i = 0; i < tabWidget->count(); i++) {
        if (view(i)->getModel()->modelId() == id)
            return view(i)->getModel();
    }
    return nullptr;
}

bool Main::gotoModel(VymModel *m)
{
    for (int i = 0; i < tabWidget->count(); i++)
        if (view(i)->getModel() == m) {
            tabWidget->setCurrentIndex(i);
            return true;
        }
    return false;
}

bool Main::gotoModelWithId(uint id)
{
    VymModel *vm;
    for (int i = 0; i < tabWidget->count(); i++) {
        vm = view(i)->getModel();
        if (vm && vm->modelId() == id) {
            tabWidget->setCurrentIndex(i);
            return true;
        }
    }
    return false;
}

bool Main::closeModelWithId(uint id)
{
    VymModel *vm;
    for (int i = 0; i < tabWidget->count(); i++) {
        vm = view(i)->getModel();
        if (vm && vm->modelId() == id) {
            if (!vm->isBusy()) {
                VymView *vv = view(i);
                tabWidget->removeTab(i);

                // Destroy stuff, order is important
                branchPropertyEditor->setModel(nullptr);
                delete (vm->getMapEditor());
                delete (vv);
                delete (vm);

                updateActions();
                if (tabWidget->count() == 0 && exitAfterLastMapClosed)
                    fileExitVym();

                return true;    // Found Id, Closing scheduled successful (used in script)
            }
        }
    }

    return false;
}

void Main::closeSavedModels()
{
    // Called from VymModel::zipFinished via QTimer::singleShot
    // to avoid race conditions
    for (int i = 0; i < tabWidget->count(); i++) {
        VymModel *vm = view(i)->getModel();
        if (vm && vm->readyToClose())
            closeModelWithId(vm->modelId());
    }
}

int Main::modelCount() { return tabWidget->count(); }

void Main::updateTabName(VymModel *vm)
{
    if (!vm) {
        qWarning() << "Main::updateTabName   vm == nullptr";
        return;
    }

    for (int i = 0; i < tabWidget->count(); i++)
        if (view(i)->getModel() == vm) {
            if (vm->isReadOnly())
                tabWidget->setTabText(i, vm->getFileName() + " " +
                                             tr("(readonly)"));
            else
                tabWidget->setTabText(i, vm->getFileName());
            return;
        }
}

void Main::editorChanged()
{
    VymModel *vm = currentModel();
    if (vm) {
        BranchItem *bi = vm->getSelectedBranch();
        updateNoteEditor(bi);
        updateHeadingEditor(bi);
        updateQueries(vm);
        taskEditor->setMapName(vm->getMapName());
        updateDockWidgetTitles(vm);
    }

    // Update actions to in menus and toolbars according to editor
    updateActions();
}

void Main::fileNew()
{
    VymModel *vm;

    // Don't show counter while loading default map
    removeProgressCounter();

    if (!fileLoad(newMapPath(), File::DefaultMap, File::VymMap)) {
        QMessageBox::critical(0, tr("Critical Error"),
                              tr("Couldn't load default map:\n\n%1\n\nvym will "
                                 "create an empty map now.",
                                 "Mainwindow: Failed to load default map")
                                  .arg(newMapPath()));

        vm = currentModel();

        // Create MapCenter for empty map  
        vm->addMapCenter(false);
        vm->makeDefault();

        // For the very first map we do not have flagrows yet...
        vm->select("mc:");

        // Set name to "unnamed"
        updateTabName(vm);
    }
    else
        update();

    // Switch to new tab    
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void Main::fileNewCopy()
{
    VymModel *srcModel = currentModel();
    if (srcModel) {
        srcModel->copy();
        fileNew();
        VymModel *dstModel = view(tabWidget->count() - 1)->getModel();
        if (dstModel && dstModel->select("mc:0"))
            dstModel->paste();
        else
            qWarning() << "Main::fileNewCopy couldn't select mapcenter";
    }
}

bool Main::fileLoad(QString fn, const File::LoadMode &lmode,
                               const File::FileType &ftype)
{
    bool noError = true;

    // fn is usually the archive, mapfile the file after uncompressing

    // Make fn absolute (needed for unzip)
    fn = QDir(fn).absolutePath();

    VymModel *vm;

    if (lmode == File::NewMap) {
        // Check, if map is already loaded
        int i = 0;
        while (i <= tabWidget->count() - 1) {
            if (view(i)->getModel()->getFilePath() == fn) {
                // Already there, ask for confirmation
                QMessageBox mb(
                    QMessageBox::Warning,
                    vymName,
                    tr("The map %1\nis already opened."
                       "Opening the same map in multiple editors may lead \n"
                       "to confusion when finishing working with vym."
                       "Do you want to").arg(fn));
                QPushButton *openButton = mb.addButton(tr("Open anyway"), QMessageBox::AcceptRole);
                mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
                mb.exec();
                if (mb.clickedButton() != openButton)
                    return false;

                i = tabWidget->count();
            }
            i++;
        }
    }

    bool createModel;

    // Try to load map
    if (!fn.isEmpty()) {
        // Find out, if we need to create a new map model

        vm = currentModel();

        if (lmode == File::NewMap) {
            if (vm && vm->isDefault()) {
                // There is a map model already and it still is the default map,
                // no need to create a new model.
                createModel = false;
            }
            else
                createModel = true;
        }
        else if (lmode == File::DefaultMap) {
            createModel = true;
        }
        else if (lmode == File::ImportAdd || lmode == File::ImportReplace) {
            if (!vm) {
                QMessageBox::warning(0, "Warning",
                                     "Trying to import into non existing map");
                return false;
            }
            else
                createModel = false;
        }
        else
            createModel = true;

        if (createModel) {
            vm = new VymModel;
            VymView *vv = new VymView(vm);

            tabWidget->addTab(vv, fn);
            tabWidget->setCurrentIndex(tabWidget->count() - 1);
            vv->initFocus();
        }

        // Check, if file exists (important for creating new files
        // from command line
        if (!QFile(fn).exists()) {
            if (lmode == File::DefaultMap) {
                return false;
            }

            if (lmode == File::NewMap) {
                QMessageBox mb(
                       QMessageBox::Warning,
                       vymName,
                       tr("This map does not exist:\n  %1\nDo you want "
                          "to create a new one?").arg(fn));

                QPushButton *createButton = mb.addButton(tr("Create"), QMessageBox::AcceptRole);
                mb.addButton(tr("Cancel"), QMessageBox::RejectRole);

                mb.exec();
                if (mb.clickedButton() == createButton) {
                    // Create new map
                    vm = currentMapEditor()->getModel();
                    vm->setFilePath(fn);
                    updateTabName(vm);
                    statusBar()->showMessage("Created " + fn);
                    return true;
                }

                // don't create new map
                statusBar()->showMessage("Loading " + fn + " failed!");
                int cur = tabWidget->currentIndex();
                tabWidget->setCurrentIndex(tabWidget->count() - 1);
                fileCloseCurrentMap();
                tabWidget->setCurrentIndex(cur);
                return false;
            }
        } // File does not exist

        // Save existing filename in case  we import
        QString fn_org = vm->getFilePath();

        if (lmode != File::DefaultMap) {

            vm->setFilePath(fn);
            // In case of importing better call the related (new) functions
            // in VymModel, instead of loading directly

            progressDialog.setLabelText(
                tr("Loading: %1", "Progress dialog while loading maps")
                    .arg(fn));
        }

        // Finally load map into mapEditor
        if (lmode == File::ImportReplace)
            noError = vm->addMapReplace(fn);
        else if (lmode == File::ImportAdd)
            noError = vm->addMapInsert(fn);
        else
            noError = vm->loadMap(fn, lmode, ftype);

        // Restore old (maybe empty) filepath, if this is an import
        if (lmode == File::ImportAdd || lmode == File::ImportReplace)
            vm->setFilePath(fn_org);

        // Finally check for errors and go home
        if (!noError) {
            if (lmode == File::NewMap)
                fileCloseCurrentMap();
            statusBar()->showMessage("Could not load " + fn);
        }
        else {
            if (lmode == File::NewMap) {
                vm->setFilePath(fn);
                updateTabName(vm);
                actionFilePrint->setEnabled(true);
                addRecentMap(fn);

                lastMapDir.setPath(vm->getFileDir());
            }
            else if (lmode == File::DefaultMap) {
                vm->makeDefault();
                updateTabName(vm);
            }

            editorChanged();
            if (vm->hasViewCenterTarget())
                // Maps since version 2.9.606 save center of view
                vm->getMapEditor()->setViewCenterTarget(vm->viewCenterTarget());
            else
                vm->emitShowSelection(false, false);

            statusBar()->showMessage(tr("Loaded %1").arg(fn));
        }
    }

    fileSaveSession();

    return noError;
}

void Main::fileLoad(const File::LoadMode &lmode)
{
    QString caption;
    switch (lmode) {
    case File::NewMap:
        caption = vymName + " - " + tr("Load vym map");
        break;
    case File::DefaultMap:
        // Not used directly
        return;
    case File::ImportAdd:
        caption = vymName + " - " + tr("Import: Add vym map to selection");
        break;
    case File::ImportReplace:
        caption =
            vymName + " - " + tr("Import: Replace selection with vym map");
        break;
    }

    QString filter;
    filter += "VYM map " + tr("or", "File Dialog") + " Freemind map" +
              " (*.xml *.vym *.vyp *.mm);;";
    filter += "VYM map (*.vym *.vyp);;";
    filter += "VYM Backups (*.vym~);;";
    filter += "Freemind map (*.mm);;";
    filter += "XML (*.xml);;";
    filter += "All (* *.*)";
    QStringList fns =
        QFileDialog::getOpenFileNames(this, caption, lastMapDir.path(), filter);

    if (!fns.isEmpty()) {
        initProgressCounter(fns.count());
        foreach (QString fn, fns)
            fileLoad(fn, lmode, getMapType(fn));    // FIXME-4 getMapType could move to VymModel::loadMap
    }
    removeProgressCounter();
}

void Main::fileLoad()
{
    fileLoad(File::NewMap);
    tabWidget->setCurrentIndex(tabWidget->count() - 1);
}

void Main::fileSaveSession()
{
    QStringList flist;
    for (int i = 0; i < tabWidget->count(); i++)
        flist.append(view(i)->getModel()->getFilePath());

    settings.setValue("/mainwindow/sessionFileList", flist);
    // qDebug() << __func__ << "Current session list: " + flist.join(",");

    // Also called by event loop regulary, but apparently not often enough
    settings.sync();
}

void Main::fileRestoreSession()
{
    restoreMode = true;

    QStringList::Iterator it = lastSessionFiles.begin();

    initProgressCounter(lastSessionFiles.count());
    while (it != lastSessionFiles.end()) {
        File::FileType type = getMapType(*it);
        fileLoad(*it, File::NewMap, type);
        *it++;
    }
    removeProgressCounter();

    // By now all files should have been loaded
    // Reset the restore flag and display message if needed
    if (ignoredLockedFiles.count() > 0) {
        QString msg(
            QObject::tr("Existing lockfiles have been ignored for the maps "
                        "listed below. Please check, if the maps might be "
                        "openend in another instance of vym:\n\n"));
        WarningDialog warn;
        warn.setMinimumWidth(800);
        warn.setMinimumHeight(350);
        warn.showCancelButton(false);
        warn.setCaption("Existing lockfiles ignored");
        warn.setText(msg + ignoredLockedFiles.join("\n"));
        warn.exec();
    }

    restoreMode = false;
    ignoredLockedFiles.clear();
}

void Main::fileLoadRecent()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action) {
        initProgressCounter();
        QString fn = action->data().toString();
        File::FileType type = getMapType(fn);
        if(fileLoad(fn, File::NewMap, type) )
            lastMapDir.setPath(fn.left(fn.lastIndexOf("/")));
        removeProgressCounter();
        tabWidget->setCurrentIndex(tabWidget->count() - 1);
    }
}

void Main::fileClearRecent()
{
    settings.setValue("/mainwindow/recentFileList", QStringList());
    setupRecentMapsMenu();
}

void Main::addRecentMap(const QString &fileName)
{

    QStringList files =
        settings.value("/mainwindow/recentFileList").toStringList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setValue("/mainwindow/recentFileList", files);

    setupRecentMapsMenu();
}

void Main::fileSave(VymModel *m, const File::SaveMode &savemode)
{
    if (!m)
        return;

    if (m->isReadOnly())
        return;

    if (m->getFilePath().isEmpty()) {
        // We have  no filepath yet,
        // call fileSaveAs() now, this will call fileSave()
        // again.  First switch to editor
        fileSaveAs();
        return; // avoid saving twice...
    }

    m->saveMap(savemode);
}

void Main::fileSave() { fileSave(currentModel(), File::CompleteMap); }

void Main::fileSave(VymModel *m) { fileSave(m, File::CompleteMap); }

bool Main::fileSaveAs(const File::SaveMode &saveMode, QString fileName)
{
    VymModel *m = currentModel();
    if (!m) return false;

    QString fileName_org = m->getFilePath(); // Restore fileName later

    // Check for existing file
    if (QFile(fileName).exists()) {
        // Check if the existing file is writable
        if (!QFileInfo(fileName).isWritable()) {
            QMessageBox::critical(0, tr("Critical Error"),
                                  tr("Couldn't save %1,\nbecause file "
                                     "exists and cannot be changed.")
                                      .arg(fileName));
            return false;
        }

        // Ask if existing file can be overwritten
        QMessageBox mb(
            QMessageBox::Warning,
            vymName,
            tr("The file %1\nexists already. Do you want to").arg(fileName));
        QPushButton *overwriteButton = mb.addButton(tr("Overwrite"), QMessageBox::AcceptRole);
        mb.addButton(tr("Cancel"), QMessageBox::RejectRole);
        mb.exec();
        if (mb.clickedButton() != overwriteButton) return false;
    }
    else {
        // New file, add extension to filename, if missing
        // This is always .vym or .vyp, depending on saveMode
        if (saveMode == File::CompleteMap) {
            if (!fileName.contains(".vym") && !fileName.contains(".xml"))
                fileName += ".vym";
        }
        else {
            if (!fileName.contains(".vyp") && !fileName.contains(".xml"))
                fileName += ".vyp";
        }
    }

    m->setFilePath(fileName);

    // Check for existing lockfile
    QFile lockFile(fileName + ".lock");
    if (lockFile.exists()) {
        QMessageBox::critical(0, tr("Critical Error"),
                              tr("Couldn't save %1,\nbecause of "
                                 "existing lockfile:\n\n%2")
                                  .arg(fileName, lockFile.fileName()));
        m->setFilePath(fileName_org);
        return false;
    }

    // Rename also current lockfile, if saving complete map
    if (saveMode == File::CompleteMap && !m->changeLock(fileName)) {
        QMessageBox::critical(0, tr("Critical Error"),
                              tr("Saving the map failed:\nCouldn't rename map to %1").arg(fileName));
        m->setFilePath(fileName_org);
        return false; // FIXME-3 Check: If saved part of map and this error occurs?
    }

    fileSave(m, saveMode);

    // Set name of tab
    if (saveMode == File::CompleteMap)
    {
        addRecentMap(m->getFileName());
        updateTabName(m);
    } else if (saveMode == File::PartOfMap) {
        m->setFilePath(fileName_org);
    }

    lastMapDir.setPath(m->getFileDir());
    return true;
}

void Main::fileSaveAs()
{
    VymModel *m = currentModel();
    if (!m) return;

    QString filter = "VYM map (*.vym)";
    filter += ";;All (* *.*)";

    // Get destination path
    QString fileName = QFileDialog::getSaveFileName(
        this,
        tr("Save map as"),
        lastMapDir.path() + "/" + tr("Untitled", "Default name in FileSaveAs dialog") + ".vym",
        filter, nullptr, QFileDialog::DontConfirmOverwrite);

    fileSaveAs(File::CompleteMap, fileName);
}

void Main::fileSaveAsDefault()
{
    VymModel *m = currentModel();
    if (!m) return;

    QString filter = "VYM map (*.vym)";
    filter += ";;All (* *.*)";

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save map as new default map"), newMapPath(),
        filter, nullptr, QFileDialog::DontConfirmOverwrite);

    if (fileSaveAs(File::CompleteMap, fileName)) {
        // Set new default path
        settings.setValue("/system/defaultMap/auto", false);    // Don't autoselect based on theme
        settings.setValue("/system/defaultMap/path", fileName);
    }
}

void Main::fileImportFirefoxBookmarks()
{
    VymModel *m = currentModel();
    if (!m) {
        fileNew();
        m = currentModel();
        if (!m) return;
    } else {
        if (!m->isDefault())
            // Import into new map
            fileNew();
    }

    if (m) {
        // Try to select first mapcenter of default map
        if (!m->select("mc:0")) return;

        m->setHeadingPlainText("Firefox");

        // Try to add one branch and select it
        /*
        if (!m->addNewBranch()) return;

        m->selectLatestAdded();
        m->setHeadingPlainText("Bookmarks");
        */

        // Open file dialog
        QFileDialog fd;
        fd.setDirectory(vymBaseDir.homePath());
        fd.setFileMode(QFileDialog::ExistingFiles);
        QStringList filters;
        filters << tr("Firefox Bookmarks") + " (*.json)";
        fd.setNameFilters(filters);
        fd.setAcceptMode(QFileDialog::AcceptOpen);
        fd.setWindowTitle(tr("Import Firefox Bookmarks into new map"));
        fd.setLabelText( QFileDialog::Accept, tr("Import"));

        if (fd.exec() == QDialog::Accepted) {
            qApp->processEvents(); // close QFileDialog
            ImportFirefoxBookmarks im(m);
            QStringList flist = fd.selectedFiles();
            QStringList::Iterator it = flist.begin();
            while (it != flist.end()) {
                im.setFile(*it);
                im.transform(); 
                ++it;
            }
        }
    }
}

void Main::fileImportFreemind()
{
    QStringList filters;
    filters << "Freemind map (*.mm)"
            << "All files (*)";
    QFileDialog fd;
    fd.setDirectory(lastMapDir);
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " + tr("Open %1 map").arg("Freeplane"));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    QString fn;
    if (fd.exec() == QDialog::Accepted) {
        lastMapDir = fd.directory();
        QStringList flist = fd.selectedFiles();
        QStringList::Iterator it = flist.begin();
        while (it != flist.end()) {
            fn = *it;
            if (fileLoad(fn, File::NewMap, File::FreemindMap)) {
                currentMapEditor()->getModel()->setFilePath("");
            }
            ++it;
        }
    }
}

void Main::fileImportIThoughts()
{
    QStringList filters;
    filters << "IThoughts map (*.itmz)"
            << "All files (*)";

    QFileDialog fd;
    fd.setDirectory(lastMapDir);
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " + tr("Open %1 map").arg("IThoughts"));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    QString fn;
    if (fd.exec() == QDialog::Accepted) {
        lastMapDir = fd.directory();
        QStringList flist = fd.selectedFiles();
        QStringList::Iterator it = flist.begin();
        while (it != flist.end()) {
            fn = *it;
            if (fileLoad(fn, File::NewMap, File::IThoughtsMap)) {
                currentMapEditor()->getModel()->setFilePath("");
            }
            ++it;
        }
    }
}

void Main::fileImportMM()
{
    ImportMM im;

    QFileDialog fd;
    fd.setDirectory(lastMapDir);
    fd.setFileMode(QFileDialog::ExistingFiles);
    QStringList filters;
    filters << "Mind Manager (*.mmap)";
    fd.setNameFilters(filters);
    fd.setAcceptMode(QFileDialog::AcceptOpen);
    fd.setWindowTitle(tr("Import") + " " + "Mind Manager");
    fd.setLabelText( QFileDialog::Accept, tr("Import"));

    if (fd.exec() == QDialog::Accepted) {
        lastMapDir = fd.directory();
        QStringList flist = fd.selectedFiles();
        QStringList::Iterator it = flist.begin();
        while (it != flist.end()) {
            im.setFile(*it);
            if (im.transform() &&
                fileLoad(im.getTransformedFile(), File::NewMap, File::VymMap) &&
                currentMapEditor())
                currentMapEditor()->getModel()->setFilePath("");
            ++it;
        }
    }
}

void Main::fileImportDir()
{
    VymModel *m = currentModel();
    if (m)
        m->importDir();
}

void Main::fileExportAO()
{
    VymModel *m = currentModel();
    if (m)
        m->exportAO();
}

void Main::fileExportASCII()
{
    VymModel *m = currentModel();
    if (m)
        m->exportASCII();
}

void Main::fileExportASCIITasks()
{
    VymModel *m = currentModel();
    if (m)
        m->exportASCII("", true);
}

void Main::fileExportConfluence()
{
    VymModel *m = currentModel();
    if (m)
        m->exportConfluence();
}

void Main::fileExportCSV()
{
    VymModel *m = currentModel();
    if (m)
        m->exportCSV();
}

void Main::fileExportFirefoxBookmarks()
{
    VymModel *m = currentModel();
    if (m)
        m->exportFirefoxBookmarks();
}

void Main::fileExportHTML()
{
    VymModel *m = currentModel();
    if (m)
        m->exportHTML();
}

void Main::fileExportImage()
{
    VymModel *m = currentModel();
    if (m)
        m->exportImage();
}

void Main::fileExportImpress()
{
    VymModel *m = currentModel();
    if (m)
        m->exportImpress();
}

void Main::fileExportLaTeX()
{
    VymModel *m = currentModel();
    if (m)
        m->exportLaTeX();
}

void Main::fileExportMarkdown()
{
    VymModel *m = currentModel();
    if (m)
        m->exportMarkdown();
}

void Main::fileExportOrgMode()
{
    VymModel *m = currentModel();
    if (m)
        m->exportOrgMode();
}

void Main::fileExportPDF()
{
    VymModel *m = currentModel();
    if (m)
        m->exportPDF();
}

void Main::fileExportSVG()
{
    VymModel *m = currentModel();
    if (m)
        m->exportSVG();
}

void Main::fileExportTaskJuggler()
{
    VymModel *m = currentModel();
    if (m) 
        m->exportTaskJuggler();
}

void Main::fileExportXML()
{
    VymModel *m = currentModel();
    if (m)
        m->exportXML();
}

void Main::fileExportLast()
{
    VymModel *m = currentModel();
    if (m)
        m->exportLast();
}

void Main::fileCloseTab(int i)
{
    if (i < tabWidget->count()) 
    {
        VymView *vv = view(i);
        if (vv) {
            VymModel *vm = vv->getModel();
            if (vm) 
                fileCloseMapWithId(vm->modelId());
        }
        fileSaveSession();
    }
}

void Main::fileCloseCurrentMap()
{
    fileCloseMapWithId(currentMapId());
}

void Main::fileCloseMapWithId(uint id)
{
    VymModel *vm = nullptr;
    VymView *vv;
    for (int i = 0; i < tabWidget->count(); i++) {
        vm = view(i)->getModel();
        if (vm && vm->modelId() == id)
            break;
    }

    if (vm) {
        if (vm->hasChanged()) {
            QMessageBox mb(
                QMessageBox::Warning,
                vymName,
                tr("The map %1 has been modified but not saved yet. Do you "
                   "want to").arg(vm->getFileName()));
            mb.setStandardButtons(QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
            mb.setDefaultButton(QMessageBox::Save);
            mb.setModal(true);
            switch (mb.exec()) {
                case QMessageBox::Save:
                    // save and close
                    vm->closeAfterSaving();
                    fileSave(vm, File::CompleteMap);
                    return;
                case QMessageBox::Discard:
                    // close  without saving
                    break;
                case QMessageBox::Cancel:
                    // do nothing
                    return;
            }
        }

        closeModelWithId(id);
    }
}

void Main::filePrint()
{
    if (currentMapEditor())
        currentMapEditor()->print();
}

bool Main::exitAfterScript()
{
    return exitAfterScriptInt;
}

void Main::setRepeatAction(const QString &script)
{
    repeatActionInt = script;
}

void Main::setExitAfterScript(bool b)
{
    exitAfterScriptInt = b;
}

void Main::fileExitVym()
{
    if (tabWidget->count() == 0)
        qApp->exit();

    exitAfterLastMapClosed = true;

    if (tabWidget->count() == 0)
        qApp->exit(0);

    // Only save session if there still are tabs open
    if (tabWidget->count() > 0)
        fileSaveSession();

    // Get list of open maps and trigger closing, save if necessary
    QList <uint> modelIds;
    for (int i = 0; i < tabWidget->count(); i++) {
        VymModel *vm = view(i)->getModel();
        if (vm)
            modelIds << vm->modelId();
    }

    foreach (auto id, modelIds)
        fileCloseMapWithId(id);
}

void Main::editUndo()
{
    VymModel *m = currentModel();
    if (m)
        m->undo();
}

void Main::editRedo()
{
    VymModel *m = currentModel();
    if (m)
        m->redo();
}

void Main::gotoHistoryStep(int i)
{
    VymModel *m = currentModel();
    if (m)
        m->gotoHistoryStep(i);
}

void Main::editCopy()
{
    VymModel *m = currentModel();
    if (m)
        m->copy();
}

void Main::editPaste()
{
    VymModel *m = currentModel();
    if (m)
        m->paste();
}

void Main::editCut()
{
    VymModel *m = currentModel();
    if (m)
        m->cut();
}

bool Main::openUrl(const QString &url, bool privateMode)  // FIXME-3 settings for URL and PDF are not really any longer used, only fallback below
{
    if (url.isEmpty()) {
        VymModel *m = currentModel();
        if (m) {
            QString url = m->getUrl();
            if (url == "")
                return false;
        } else
            return false;
    }

    if (privateMode) {  // FIXME-3 Currently only Firefox is supported to open private Urls
        QString browser = settings.value(
                "/system/readerUrlPrivate",
                "/Applications/Firefox.app/Contents/MacOS/firefox").toString();
        QStringList args;
        args << "--private-window";
        args << url;
        if (!QProcess::startDetached(browser, args, QDir::currentPath())) {
            // try to set path to browser
            QMessageBox::warning(
                0, tr("Warning"),
                tr("Couldn't find a viewer to open %1.\n").arg(url) +
                    tr("Please use Settings->") +
                    tr("Set application to open an URL"));
            settingsURL();
            return false;
        }
        return true;
    }

    // Use system settings to open file
    bool b = QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
    if (b) return true;

    // Fallback to old vym method to open Url
    QString browser = settings.value("/system/readerUrl").toString();
    QStringList args;
    args << url;
    if (!QProcess::startDetached(browser, args, QDir::currentPath())) {
        // try to set path to browser
        QMessageBox::warning(
            0, tr("Warning"),
            tr("Couldn't find a viewer to open %1.\n").arg(url) +
                tr("Please use Settings->") +
                tr("Set application to open an URL"));
        settingsURL();
        return false;
    }
    return true;
}

void Main::openTabs(QStringList urls, bool privateMode)
{
    if (urls.isEmpty())
        return;

    // Other browser, e.g. xdg-open
    // Just open all urls and leave it to the system to cope with it
    foreach (QString u, urls)
        openUrl(u, privateMode);
}

void Main::editOpenMultipleVisUrls(bool ignoreScrolled, bool privateMode)
{
    VymModel *m = currentModel();
    if (m) {
        QStringList urls;
        urls = m->getUrls(ignoreScrolled);
        openTabs(urls, privateMode);
    }
}

void Main::editOpenMultipleUrls() { editOpenMultipleVisUrls(false); }

void Main::editOpenMultipleUrlsPrivate() { editOpenMultipleVisUrls(false, true); }

void Main::editNote2URLs()
{
    VymModel *m = currentModel();
    if (m)
        m->note2URLs();
}

void Main::editURL()
{
    VymModel *m = currentModel();
    if (m) {
        QInputDialog *dia = new QInputDialog(this);
        dia->setLabelText(tr("Enter Url:"));
        dia->setWindowTitle(vymName);
        dia->setInputMode(QInputDialog::TextInput);
        TreeItem *selti = m->getSelectedItem();
        if (selti)
            dia->setTextValue(selti->url());
        dia->resize(width() * 0.6, 80);
        centerDialog(dia);

        if (dia->exec())
            m->setUrl(dia->textValue());
        delete dia;
    }
}

void Main::editLocalURL()   // FIXME-3 add editLocalDir to also accept directories, not only files
{
    VymModel *m = currentModel();
    if (m) {
        TreeItem *selti = m->getSelectedItem();
        if (selti) {
            QString filter;
            filter += "All files (*);;";
            filter += tr("HTML", "Filedialog") + " (*.html,*.htm);;";
            filter += tr("Text", "Filedialog") + " (*.txt);;";
            filter += tr("Spreadsheet", "Filedialog") + " (*.odp,*.sxc);;";
            filter += tr("Textdocument", "Filedialog") + " (*.odw,*.sxw);;";
            filter += tr("Images", "Filedialog") +
                      " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";

            QString fn = QFileDialog::getOpenFileName(
                this, vymName + " - " + tr("Set URL to a local file"),
                lastMapDir.path(), filter);

            if (!fn.isEmpty()) {
                lastMapDir.setPath(fn.left(fn.lastIndexOf("/")));
                if (!fn.startsWith("file://"))
                    fn = "file:///" + fn;
                m->setUrl(fn);
            }
        }
    }
}

void Main::editHeading2URL()
{
    VymModel *m = currentModel();
    if (m)
        m->editHeading2URL();
}

void Main::setJiraQuery()
{
    VymModel *m = currentModel();
    if (m) {
        QInputDialog dia;
        dia.setLabelText(tr("Enter Jira query:"));
        dia.setWindowTitle(vymName);
        dia.setInputMode(QInputDialog::TextInput);
        BranchItem *selbi = m->getSelectedBranch();
        if (selbi)  {
            AttributeItem *ai = selbi->getAttributeByKey("Jira.query");
            if (ai)
                dia.setTextValue(ai->value().toString());
            dia.resize(width() * 0.6, 80);
            centerDialog(&dia);

            if (dia.exec())
                m->setJiraQuery(dia.textValue());
        }
    }
}

void Main::getJiraDataSubtree()
{
    VymModel *m = currentModel();
    if (m)
        m->getJiraData(true);
}

void Main::getConfluencePageDetails()
{
    VymModel *m = currentModel();
    if (m)
        m->setConfluencePageDetails(false);
}

void Main::getConfluencePageDetailsRecursively()
{
    VymModel *m = currentModel();
    if (m)
        m->setConfluencePageDetails(true);
}

void Main::getConfluenceUser()
{
    VymModel *m = currentModel();
    if (m) {
        BranchItem *selbi = m->getSelectedBranch();
        if (selbi) {
            ConfluenceUserDialog *dia = new ConfluenceUserDialog;
            centerDialog(dia);
            if (dia->exec() > 0) {
                BranchItem *bi = m->addNewBranch();
                if (!bi) return;
                if (!m->select(bi)) return;
                selbi = m->getSelectedBranch();

                ConfluenceUser user = dia->getSelectedUser();

                m->setAttribute(selbi, "ConfluenceUser.displayName", user.getDisplayName());
                m->setAttribute(selbi, "ConfluenceUser.userKey", user.getUserKey());
                m->setAttribute(selbi, "ConfluenceUser.userName", user.getUserName());
                m->setAttribute(selbi, "ConfluenceUser.url", user.getUrl());
                m->setUrl(user.getUrl(), false);
                m->setHeading(user.getDisplayName());
                m->selectParent();
            }
            dia->clearFocus();
            delete dia;
            m->getMapEditor()->activateWindow();
            m->getMapEditor()->setFocus();
        }
    }
}

void Main::editHeading()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->editHeading();
}

void Main::editHeadingFinished(VymModel *m)
{
    if (m) {
        if (!actionSettingsAutoSelectNewBranch->isChecked() &&
            !prevSelection.isNull())
            m->select(prevSelection);
        prevSelection = QUuid();
    }
}

void Main::openVymLinks(const QStringList &vl, bool background)
{
    QStringList vlmin;
    int index = -1;
    for (int j = 0; j < vl.size(); ++j) {
        // compare path with already loaded maps
        QString absPath = QFileInfo(vl.at(j)).absoluteFilePath();
        index = -1;
        for (int i = 0; i <= tabWidget->count() - 1; i++) {
            if (absPath == view(i)->getModel()->getFilePath()) {
                index = i;
                break;
            }
        }
        if (index < 0)
            vlmin.append(absPath);
    }

    progressCounterTotal = vlmin.size();
    for (int j = 0; j < vlmin.size(); j++) {
        // Load map
        if (!QFile(vlmin.at(j)).exists())
            QMessageBox::critical(0, tr("Critical Error"),
                                  tr("Couldn't open map %1").arg(vlmin.at(j)));
        else {
            fileLoad(vlmin.at(j), File::NewMap, File::VymMap);
            if (!background)
                tabWidget->setCurrentIndex(tabWidget->count() - 1);
        }
    }
    // Go to tab containing the map
    if (index >= 0)
        tabWidget->setCurrentIndex(index);
    removeProgressCounter();
}

void Main::editOpenVymLink(bool background)
{
    VymModel *m = currentModel();
    if (m) {
        QStringList vl;
        vl.append(m->getVymLink());
        openVymLinks(vl, background);
    }
}

void Main::editOpenVymLinkBackground() { editOpenVymLink(true); }

void Main::editOpenMultipleVymLinks()
{
    VymModel *m = currentModel();
    if (m) {
        QStringList vl = m->getVymLinks();
        openVymLinks(vl, true);
    }
}

void Main::editVymLink()
{
    VymModel *m = currentModel();
    if (m) {
        BranchItem *bi = m->getSelectedBranch();
        if (bi) {
            QStringList filters;
            filters << "VYM map (*.vym)";
            QFileDialog fd;
            fd.setWindowTitle(vymName + " - " + tr("Link to another vym map"));
            fd.setNameFilters(filters);
            fd.setLabelText( QFileDialog::Accept, tr("Set as link to vym map"));
            fd.setDirectory(lastMapDir);
            fd.setAcceptMode(QFileDialog::AcceptOpen);
            if (!bi->vymLink().isEmpty())
                fd.selectFile(bi->vymLink());
            fd.show();

            if (fd.exec() == QDialog::Accepted &&
                !fd.selectedFiles().isEmpty()) {
                QString fn = fd.selectedFiles().first();
                lastMapDir = QDir(fd.directory().path());
                m->setVymLink(fn);
            }
        }
    }
}

void Main::editDeleteVymLink()
{
    VymModel *m = currentModel();
    if (m)
        m->deleteVymLink();
}

void Main::editToggleHideExport()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleHideExport();
}

void Main::editToggleTask()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleTask();
}

void Main::editCycleTaskStatus()
{
    VymModel *m = currentModel();
    if (m)
        m->cycleTaskStatus();
}

void Main::editTaskResetDeltaPrio()
{
    VymModel *m = currentModel();
    if (m) {
        QList <BranchItem*> taskBranches;
        Task *task;
        for (int i = 0; i < taskModel->count(); i++)
        {
            task = taskModel->getTask(i);
            if (taskEditor->taskVisible(task) && task->getPriorityDelta() != 0)
                taskBranches << task->getBranch();
        }

        foreach (BranchItem *bi, taskBranches)
            bi->getModel()->setTaskPriorityDelta(0, bi);
    }
}

void Main::editTaskSleepN()
{
    VymModel *m = currentModel();
    if (m) {
        qint64 n = ((QAction *)sender())->data().toInt();
        Task *task = m->getSelectedTask();
        if (task) {
            bool ok = true;
            QString s;
            if (n < 0) {
                QString currentSleep;
                QDateTime d = task->alarmTime();
                n = task->getSecsSleep();
                if (n <= 0)
                    currentSleep = "0";
                else if (n < 60)
                    currentSleep = QString("%1s").arg(n);
                else if (n < 24 * 3600) {
                    currentSleep = d.time().toString("hh:mm");
                }
                else if (d.time().hour() == 0 && d.time().minute() == 0) {
                    currentSleep = d.date().toString("dd.MM.yyyy");
                }
                else
                    currentSleep = d.toString(Qt::ISODate);

                LineEditDialog *dia = new LineEditDialog(this);
                dia->setLabel(tr("Enter sleep time (number of days, hours with "
                                 "'h' or date YYYY-MM-DD or DD.MM[.YYYY]",
                                 "task sleep time dialog"));
                dia->setText(currentSleep);
                centerDialog(dia);
                if (dia->exec() == QDialog::Accepted) {
                    ok = true;
                    s = dia->getText();
                }
                else
                    ok = false;

                delete dia;
            }
            else
                s = QString("%1").arg(n);

            if (ok && !m->setTaskSleep(s))
                QMessageBox::warning(
                    0, tr("Warning"),
                    tr("Couldn't set sleep time to %1.\n").arg(s));
        }
    }
}

void Main::editAddTimestamp()
{
    VymModel *m = currentModel();
    if (m)
        m->addTimestamp();
}

void Main::editMapProperties()
{
    VymModel *m = currentModel();
    if (!m)
        return;

    ExtraInfoDialog dia;
    dia.setMapName(m->getFileName());
    dia.setFileLocation(m->getFilePath());
    dia.setMapTitle(m->mapTitle());
    dia.setAuthor(m->mapAuthor());
    dia.setComment(m->mapComment());
    dia.setMapVersion(m->mapVersion());
    dia.setReadOnly(m->isReadOnly());

    // Calc some stats
    QString stats;
    stats += tr("%1 items on map\n", "Info about map")
                 .arg(m->getScene()->items().size(), 6);

    uint branchesCount = 0;
    uint imagesCount = 0;
    uint notesCount = 0;
    uint xlinksCount = 0;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    m->nextBranch(cur, prev);
    while (cur) {
        if (!cur->getNote().isEmpty())
            notesCount++;
        imagesCount += cur->imageCount();
        branchesCount++;
        xlinksCount += cur->xlinkCount();
        m->nextBranch(cur, prev);
    }

    stats += QString("%1 %2\n")
                 .arg(branchesCount, 6)
                 .arg(tr("branches", "Info about map"));
    stats += QString("%1 %2\n")
                 .arg(taskModel->count(), 6)
                 .arg(tr("tasks total", "Info about map"));
    stats += QString("%1 %2\n")
                 .arg(taskModel->count(m), 6)
                 .arg(tr("tasks in map", "Info about map"));
    stats += QString("%1 %2\n").arg(notesCount, 6).arg(tr("notes", "Info about map"));
    stats += QString("%1 %2\n").arg(imagesCount, 6).arg(tr("images", "Info about map"));
    stats += QString("%1 %2\n")
                 .arg(m->slideCount(), 6)
                 .arg(tr("slides", "Info about map"));
    stats +=
        QString("%1 %2\n").arg(xlinksCount / 2, 6).arg(tr("xLinks", "Info about map"));
    dia.setStats(stats);

    // Finally show dialog
    if (dia.exec() == QDialog::Accepted) {
        m->setMapAuthor(dia.getAuthor());
        m->setMapComment(dia.getComment());
        m->setMapTitle(dia.getMapTitle());
    }
}

void Main::editMoveUp()
{
    MapEditor *me = currentMapEditor();
    VymModel *m = currentModel();
    if (me && m && me->state() != MapEditor::EditingHeading)
        m->moveUp();
}

void Main::editMoveDown()
{
    MapEditor *me = currentMapEditor();
    VymModel *m = currentModel();
    if (me && m && me->state() != MapEditor::EditingHeading)
        m->moveDown();
}

void Main::editMoveDownDiagonally()
{
    MapEditor *me = currentMapEditor();
    VymModel *m = currentModel();
    if (me && m && me->state() != MapEditor::EditingHeading)
        m->moveDownDiagonally();
}

void Main::editMoveUpDiagonally()
{
    MapEditor *me = currentMapEditor();
    VymModel *m = currentModel();
    if (me && m && me->state() != MapEditor::EditingHeading)
        m->moveUpDiagonally();
}

void Main::editDetach()
{
    VymModel *m = currentModel();
    if (m)
        m->detach();
}

void Main::editSortChildren()
{
    VymModel *m = currentModel();
    if (m)
        m->sortChildren(false);
}

void Main::editSortBackChildren()
{
    VymModel *m = currentModel();
    if (m)
        m->sortChildren(true);
}

void Main::editToggleScroll()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleScroll();
}

void Main::editExpandAll()
{
    VymModel *m = currentModel();
    if (m)
        m->emitExpandAll();
}

void Main::editExpandOneLevel()
{
    VymModel *m = currentModel();
    if (m)
        m->emitExpandOneLevel();
}

void Main::editCollapseOneLevel()
{
    VymModel *m = currentModel();
    if (m)
        m->emitCollapseOneLevel();
}

void Main::editCollapseUnselected()
{
    VymModel *m = currentModel();
    if (m)
        m->emitCollapseUnselected();
}

void Main::editUnscrollSubtree()
{
    VymModel *m = currentModel();
    if (m)
        m->unscrollSubtree();
}

void Main::editGrowSelectionSize()
{
    VymModel *m = currentModel();
    if (m)
        m->growSelectionSize();
}

void Main::editShrinkSelectionSize()
{
    VymModel *m = currentModel();
    if (m)
        m->shrinkSelectionSize();
}

void Main::editResetSelectionSize()
{
    VymModel *m = currentModel();
    if (m)
        m->resetSelectionSize();
}

void Main::editRotateSubtreeCW()
{
    VymModel *m = currentModel();
    if (m)
        m->rotateSubtree(5);
}

void Main::editRotateSubtreeCCW()
{
    VymModel *m = currentModel();
    if (m)
        m->rotateSubtree(-5);
}

void Main::editRepeatLastAction()
{
    runScriptWithMacros(repeatActionInt);
}

void Main::editAddMapCenter()
{
    VymModel *m = currentModel();
    if (m) {
        // Set interactive=true to edit new heading
        m->select(m->addMapCenter(true));
    }
}

void Main::editAddBranch()
{
    VymModel *m = currentModel();
    if (m) {
        if (!actionSettingsAutoSelectNewBranch->isChecked()) {
            BranchItem *bi = m->getSelectedBranch();
            if (bi)
                prevSelection = bi->getUuid();
        }
        m->addNewBranch(nullptr, -2, true);
    }
}

void Main::editAddBranchBefore()
{
    VymModel *m = currentModel();
    if (m) {
        if (!actionSettingsAutoSelectNewBranch->isChecked()) {
            BranchItem *bi = m->getSelectedBranch();
            if (bi)
                prevSelection = bi->getUuid();
        }

        m->addNewBranchBefore(nullptr, true);
    }
}

void Main::editAddBranchAbove()
{
    VymModel *m = currentModel();
    if (m) {
        if (!actionSettingsAutoSelectNewBranch->isChecked()) {
            BranchItem *bi = m->getSelectedBranch();
            if (bi)
                prevSelection = bi->getUuid();
        }

        m->addNewBranch(nullptr, -3, true);
    }
}

void Main::editAddBranchBelow()
{
    VymModel *m = currentModel();
    if (m) {
        if (!actionSettingsAutoSelectNewBranch->isChecked()) {
            BranchItem *bi = m->getSelectedBranch();
            if (bi)
                prevSelection = bi->getUuid();
        }
        m->addNewBranch(nullptr, -1, true);
    }
}

void Main::editImportAdd() { fileLoad(File::ImportAdd); }

void Main::editImportReplace() { fileLoad(File::ImportReplace); }

void Main::editSaveSelection()
{
    VymModel *m = currentModel();
    if (!m) return;

    QString filter = "Part of VYM map (*.vyp)";

    QString fileName = QFileDialog::getSaveFileName(
        this, tr("Save part of map"), m->getFileDir() + "/" + m->getMapName() + ".vyp",
        filter, nullptr, QFileDialog::DontConfirmOverwrite);

    fileSaveAs(File::PartOfMap, fileName);
}

void Main::editDeleteKeepChildren()
{
    VymModel *m = currentModel();
    if (m)
        m->deleteKeepChildren();
}

void Main::editDeleteChildren()
{
    VymModel *m = currentModel();
    if (m)
        m->deleteChildrenBranches();
}

void Main::editDeleteSelection()
{
    VymModel *m = currentModel();
    if (m)
        m->cut();
}

void Main::editLoadImage()
{
    VymModel *m = currentModel();
    if (m) {
        QStringList imagePaths = openImageDialog(tr("Load images"));

        if (!imagePaths.isEmpty())
            m->loadImage(nullptr, imagePaths);
    }
}

void Main::editSaveImage()
{
    VymModel *m = currentModel();
    if (m)
        m->saveImage();
}

void Main::editEditXLink(QAction *a)
{
    VymModel *m = currentModel();
    if (m) {
        BranchItem *selbi = m->getSelectedBranch();
        if (selbi) {
            XLink *xl = selbi
                          ->getXLinkItemNum(
                              branchXLinksContextMenuEdit->actions().indexOf(a))
                          ->getXLink();
            if (xl && m->select(xl->beginXLinkItem()))
                m->editXLink();
        }
    }
}

void Main::popupFollowReference()
{
    if (branchXLinksContextMenuFollow->actions().count() == 1)
        // If only one reference (XLink, Url, VymLink) is available,
        // just follow it
        followReference(branchXLinksContextMenuFollow->actions().at(0));
    else
        // Popup menu
        branchXLinksContextMenuFollow->exec(QCursor::pos());
}

void Main::followReference(QAction *a)
{
    VymModel *m = currentModel();

    if (m) {
        QString d = a->data().toString();
        if (d.startsWith("XLink:"))
            m->select(QUuid(d.section(':', 1)));
        else if (d.startsWith("Url:"))
            openUrl(d.section(':', 1));
        else if (d.startsWith("VymLink:")) {
            QStringList vymLinks;
            vymLinks << d.section(':', 1);
            openVymLinks(vymLinks);
        } else
            qWarning() << __func__ << "Unknown reference in d=" << d;
    }
}

bool Main::initLinkedMapsMenu(VymModel *model, QMenu *menu)
{
    if (model) {
        ItemList targets = model->getLinkedMaps();

        menu->clear();

        QStringList targetNames;
        QList<uint> targetIDs;

        // Build QStringList with all names of targets
        QMap<uint, QStringList>::const_iterator i;
        i = targets.constBegin();
        while (i != targets.constEnd()) {
            targetNames.append(i.value().first());
            targetIDs.append(i.key());
            ++i;
        }

        // Sort list of names
        targetNames.sort(Qt::CaseInsensitive);

        // Build menu based on sorted names
        while (!targetNames.isEmpty()) {
            // Find target by value
            i = targets.constBegin();
            while (i != targets.constEnd()) {
                if (i.value().first() == targetNames.first())
                    break;
                ++i;
            }

            menu->addAction(targetNames.first())->setData(i.value().last());
            targetNames.removeFirst();
            targets.remove(i.key());
        }
        return true;
    }
    return false;
}

void Main::editGoToLinkedMap()
{
    VymModel *model = currentModel();
    if (initLinkedMapsMenu(model, targetsContextMenu)) {
        QAction *a = targetsContextMenu->exec(QCursor::pos());
        if (a) {
            QStringList sl;
            sl << a->data().toString();
            openVymLinks(sl);
        }
    }
}

void Main::editToggleTarget()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleTarget();
}

bool Main::initTargetsMenu(VymModel *model, QMenu *menu)
{
    if (model) {
        ItemList targets = model->getTargets();

        menu->clear();

        QStringList targetNames;
        QList<uint> targetIDs;

        // Build QStringList with all names of targets
        QMap<uint, QStringList>::const_iterator i;
        i = targets.constBegin();
        while (i != targets.constEnd()) {
            targetNames.append(i.value().first());
            targetIDs.append(i.key());
            ++i;
        }

        // Sort list of names
        targetNames.sort(Qt::CaseInsensitive);

        // Build menu based on sorted names
        while (!targetNames.isEmpty()) {
            // Find target by value
            i = targets.constBegin();
            while (i != targets.constEnd()) {
                if (i.value().first() == targetNames.first())
                    break;
                ++i;
            }

            menu->addAction(targetNames.first())->setData(i.key());
            targetNames.removeFirst();
            targets.remove(i.key());
        }
        return true;
    }
    return false;
}

void Main::editGoToTarget()
{
    VymModel *model = currentModel();
    if (initTargetsMenu(model, targetsContextMenu)) {
        QAction *a = targetsContextMenu->exec(QCursor::pos());
        if (a)
            model->select(model->findID(a->data().toUInt()));
    }
}

void Main::editMoveToTarget()
{
    VymModel *model = currentModel();
    if (initTargetsMenu(model, targetsContextMenu)) {
        QAction *a = targetsContextMenu->exec(QCursor::pos());
        if (a) {
            TreeItem *dsti = model->findID(a->data().toUInt());
            /*
            BranchItem *selbi = model->getSelectedBranch();
            if (!selbi)
                return;
            */

            QList<TreeItem *> itemList = model->getSelectedItems();
            if (itemList.count() < 1) return;

            if (dsti && dsti->hasTypeBranch() ) {
                BranchItem *selbi;
                BranchItem *pi;
                foreach (TreeItem *ti, itemList) {
                    if (ti->hasTypeBranch() )
                    {
                        selbi = (BranchItem*)ti;
                        pi = selbi->parentBranch();
                        
                        // If branch below exists, select that one
                        // Makes it easier to quickly resort using the MoveTo function
                        BranchItem *below = pi->getBranchNum(selbi->num() + 1);
                        if (below)
                            model->select(below);
                        else {
                            BranchItem *above = pi->getBranchNum(selbi->num() - 1);
                            if (above)
                                model->select(above);
                            else if (pi)
                                model->select(pi);
                        }

                        model->relinkBranch(selbi, (BranchItem *)dsti, -1);
                    }
                }
            }
        }
    }
}

void Main::editSelectFirstSibling()
{
    VymModel *m = currentModel();
    if (m)
        m->selectFirstBranch();
}

void Main::editSelectLastSibling()
{
    VymModel *m = currentModel();
    if (m)
        m->selectLastBranch();
}

void Main::editSelectPrevious()
{
    VymModel *m = currentModel();
    if (m)
        m->selectPrevious();
}

void Main::editSelectNext()
{
    VymModel *m = currentModel();
    if (m)
        m->selectNext();
}

void Main::editSelectNothing()
{
    VymModel *m = currentModel();
    if (m)
        m->unselectAll();
}

void Main::editOpenFindResultWidget()
{
    findResultWidget->popup();
    findResultWidget->setFocus();
}

void Main::editFindNext(QString s, bool searchNotesFlag)
{
    Qt::CaseSensitivity cs = Qt::CaseInsensitive;
    VymModel *m = currentModel();
    if (m) {
        if (m->findAll(findResultWidget->getResultModel(), s, cs,
                       searchNotesFlag))
            findResultWidget->setStatus(FindControlsWidget::Success);
        else
            findResultWidget->setStatus(FindControlsWidget::Failed);
    }
}

void Main::editFindDuplicateURLs() // FIXME-4 feature: use FindResultWidget for
                                   // display
{
    VymModel *m = currentModel();
    if (m)
        m->findDuplicateURLs();
}

void Main::updateQueries( VymModel *) // FIXME-4 disabled for now to avoid selection in FRW
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

void Main::selectQuickColor(int n)
{
    if (n < 0 || n > quickColors.count() - 1) return;

    actionGroupQuickColors->actions().at(n)->setChecked(true);
    setCurrentColor(quickColors.at(n));
}

void Main::setQuickColor(QColor col)
{
    int i = getCurrentColorIndex();
    if (i < 0) return;

    QPixmap pix(16, 16);
    pix.fill(col);
    actionGroupQuickColors->checkedAction()->setIcon(pix);
    quickColors.replace(i, col);
}

void Main::quickColorPressed()
{
    int i = getCurrentColorIndex();

    if (i < 0) return;

    if (QApplication::keyboardModifiers() == Qt::ShiftModifier) {
        QColor col = getCurrentColor();
        col = QColorDialog::getColor((col), this);
        if (!col.isValid()) return;

        setQuickColor(col);
    } else
        selectQuickColor(i);
}

void Main::formatPickColor()
{
    VymModel *m = currentModel();
    if (m)
        setQuickColor( m->getCurrentHeadingColor());
}

QColor Main::getCurrentColor() 
{ 
    int i = getCurrentColorIndex();

    if (i < 0) return QColor();

    return quickColors.at(i);
}

int Main::getCurrentColorIndex()
{
    QAction* a = actionGroupQuickColors->checkedAction();

    if (a == nullptr) return -1;

    return actionGroupQuickColors->actions().indexOf(a);
}

void Main::setCurrentColor(QColor c)
{
    int i = getCurrentColorIndex();

    if (i < 0) return;

    QPixmap pix(16, 16);
    pix.fill(c);

    actionGroupQuickColors->actions().at(i)->setIcon(pix);
}

void Main::formatColorBranch()
{
    VymModel *m = currentModel();
    if (m)
        m->colorBranch(getCurrentColor());
}

void Main::formatColorSubtree()
{
    VymModel *m = currentModel();
    if (m)
        m->colorSubtree(getCurrentColor());
}

void Main::formatLinkStyleLine()
{
    VymModel *m = currentModel();
    if (m) {
        m->setLinkStyle("StyleLine");
        actionFormatLinkStyleLine->setChecked(true);
    }
}

void Main::formatLinkStyleParabel()
{
    VymModel *m = currentModel();
    if (m) {
        m->setLinkStyle("StyleParabel");
        actionFormatLinkStyleParabel->setChecked(true);
    }
}

void Main::formatLinkStylePolyLine()
{
    VymModel *m = currentModel();
    if (m) {
        m->setLinkStyle("StylePolyLine");
        actionFormatLinkStylePolyLine->setChecked(true);
    }
}

void Main::formatLinkStylePolyParabel()
{
    VymModel *m = currentModel();
    if (m) {
        m->setLinkStyle("StylePolyParabel");
        actionFormatLinkStylePolyParabel->setChecked(true);
    }
}

void Main::formatBackground() {
    VymModel *m = currentModel();
    if (m) {
        BackgroundDialog dia(m);
        dia.exec();
    }
}

void Main::formatSelectLinkColor()
{
    VymModel *m = currentModel();
    if (m) {
        QColor col = QColorDialog::getColor(m->mapDesign()->defaultLinkColor(), this);
        m->setDefaultLinkColor(col);
    }
}

void Main::formatSelectSelectionColor() // FEATURE #157  no Pen/Brush support yet
{
    VymModel *m = currentModel();
    if (m) {
        QColor col = QColorDialog::getColor(
                m->getSelectionBrushColor(),
                this,
                tr("Color of selection box","Mainwindow"),
                QColorDialog::ShowAlphaChannel);
        m->setSelectionPenColor(col);
        m->setSelectionBrushColor(col);
    }
}

void Main::formatSelectFont()
{
    VymModel *m = currentModel();
    if (m) {
        bool ok;
        QFont font = QFontDialog::getFont(&ok, m->mapDesign()->font(), this);
        if (ok)
            m->setDefaultFont(font);
    }
}

void Main::formatToggleLinkColorHint()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleLinkColorHint();
}

void Main::formatHideLinkUnselected()
{
    VymModel *m = currentModel();
    if (m)
        m->setHideLinkUnselected(actionFormatHideLinkUnselected->isChecked());
}

void Main::viewZoomReset()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->setViewCenterSelection();
}

void Main::viewZoomIn()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->zoomIn();
}

void Main::viewZoomOut()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->zoomOut();
}

void Main::viewRotateCounterClockwise()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->setRotationTarget(me->rotationTarget() - 5);
}

void Main::viewRotateClockwise()
{
    MapEditor *me = currentMapEditor();
    if (me)
        me->setRotationTarget(me->rotationTarget() + 5);
}

void Main::viewCenter()
{
    VymModel *m = currentModel();
    if (m)
        m->emitShowSelection(false);
}

void Main::viewCenterScaled()
{
    VymModel *m = currentModel();
    if (m)
        m->emitShowSelection(true);
}

void Main::viewCenterRotated()
{
    VymModel *m = currentModel();
    if (m)
        m->emitShowSelection(false, true);
}

void Main::networkStartServer()
{
    VymModel *m = currentModel();
    if (m)
        m->newServer();
}

void Main::networkConnect()
{
    VymModel *m = currentModel();
    if (m)
        m->connectToServer();
}

void Main::downloadFinished() // only used for drop events in mapeditor and
                              // VM::downloadImage
{
    QString s;
    DownloadAgent *agent = static_cast<DownloadAgent *>(sender());
    agent->isSuccess() ? s = "Success" : s = "Error  ";

    /*
    qDebug()<<"Main::downloadFinished ";
    qDebug()<<"  result" <<  s;
    qDebug()<<"     msg" << agent->getResultMessage();
    */

    QString script = agent->getFinishedScript();
    VymModel *model = modelWithId(agent->getFinishedScriptModelID());
    if (!script.isEmpty() && model) {
        script.replace("$TMPFILE", agent->getDestination());
        runScript(script);
    }
    agent->deleteLater();
}

void Main::settingsPDF()
{
    // Default browser is set in constructor
    QString s = QFileDialog::getOpenFileName(
        this,
        tr("Set application to open PDF files"),
        settings.value("/system/readerPDF").toString());

    if (!s.isEmpty())
        settings.setValue("/system/readerPDF", s);
}

void Main::settingsURL()
{
    // Default browser is set in constructor
    QString s = QFileDialog::getOpenFileName(
        this,
        tr("Set application to open external links"),
        settings.value("/system/readerURL").toString());

    if (!s.isEmpty())
        settings.setValue("/system/readerURL", s);
}

void Main::settingsActionLog()
{
    ActionLogDialog dia;
    dia.exec();
}

void Main::settingsMacroPath()
{
    QString macroPath = macros.getPath();

    QStringList filters;
    filters << "VYM script files (*.vys)";
    QFileDialog fd;
    fd.setDirectory(dirname(macroPath));
    fd.selectFile(basename(macroPath));
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " + tr("Load vym script"));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    if (fd.exec() == QDialog::Accepted) {
        if (macros.setPath( fd.selectedFiles().first()))
            settings.setValue("/macros/path", macros.getPath());
    }
}

void Main::settingsUndoLevels()
{
    bool ok;
    int i = QInputDialog::getInt(
        this, "QInputDialog::getInt()", tr("Number of undo/redo levels:"),
        settings.value("/history/stepsTotal", 1000).toInt(), 0, 100000, 1, &ok);
    if (ok) {
        settings.setValue("/history/stepsTotal", i);
        QMessageBox::information(this, tr("VYM -Information:"),
                                 tr("Settings have been changed. The next map "
                                    "opened will have \"%1\" undo/redo levels")
                                     .arg(i));
    }
}

void Main::settingsDefaultMapPath()
{
    DefaultMapSettingsDialog dia;
    dia.exec();
}

QString Main::defaultMapPath()
{
    // Define default automatical path (also as fallback)
    QString ext_dark;
    if (usingDarkTheme)
        ext_dark = "-dark";

    return vymBaseDir.path() + QString("/demos/default%1.vym").arg(ext_dark);
}

QString Main::newMapPath()
{
    if (settings.value("/system/defaultMap/auto", true).toBool())
        return defaultMapPath();
    else
        return settings
           .value("/system/defaultMap/path", defaultMapPath())
           .toString();
}

bool Main::useAutosave() { return actionSettingsToggleAutosave->isChecked(); }

void Main::setAutosave(bool b) { actionSettingsToggleAutosave->setChecked(b); }

void Main::settingsAutosaveTime()
{
    bool ok;
    int i = QInputDialog::getInt(
        this, vymName, tr("Number of seconds before autosave:"),
        settings.value("/system/autosave/ms").toInt() / 1000, 10, 60000, 1,
        &ok);
    if (ok)
        settings.setValue("/system/autosave/ms", i * 1000);
}

void Main::settingsDefaultMapAuthor()
{
    bool ok;
    QString s = QInputDialog::getText(
        this, vymName, tr("Set author for new maps (used in lockfile)") + ":",
        QLineEdit::Normal,
        settings
            .value("/user/name", tr("unknown user",
                                    "default name for map author in settings"))
            .toString(),
        &ok);
    if (ok)
        settings.setValue("/user/name", s);
}

void Main::settingsDarkTheme()
{
    DarkThemeSettingsDialog dia;
    QString settingDarkTheme = settings.value("/system/darkTheme", "system").toString();
    if (settingDarkTheme == "always")
        dia.ui.alwaysUseDarkThemeButton->setChecked(true);
    else 
        if (settingDarkTheme == "never")
            dia.ui.neverUseDarkThemeButton->setChecked(true);
        else
            dia.ui.systemUseDarkThemeButton->setChecked(true);
    dia.exec();

    QString newSetting;
    if (dia.ui.alwaysUseDarkThemeButton->isChecked())
            newSetting = "always";
    else
        if (dia.ui.neverUseDarkThemeButton->isChecked())
            newSetting = "never";
        else
            newSetting = "system";

    if (settingDarkTheme != newSetting) {
        settings.setValue("/system/darkTheme", newSetting);
        QMessageBox::information(
            0, tr("Information"),
            tr("Restart vym to apply the changed dark theme setting"));
    }
}

void Main::settingsShowParentsLevelFindResults()
{
    bool ok;
    int i = QInputDialog::getInt(
        this, vymName, tr("Number of parents shown in find results:"),
        findResultWidget->getResultModel()->getShowParentsLevel(), 0, 10, 0,
        &ok);
    if (ok)
        findResultWidget->getResultModel()->setShowParentsLevel(i);
}

void Main::settingsShowParentsLevelTasks()
{
    bool ok;
    int i = QInputDialog::getInt(
        this, vymName, tr("Number of parents shown for a task:"),
        taskModel->getShowParentsLevel(), 0, 10, 0, &ok);
    if (ok)
        taskModel->setShowParentsLevel(i);
}

void Main::settingsToggleAutoLayout()
{
    settings.setValue("/mainwindow/autoLayout/use",
                      actionSettingsToggleAutoLayout->isChecked());
}

void Main::settingsToggleWriteBackupFile()
{
    settings.setValue("/system/writeBackupFile",
                      actionSettingsWriteBackupFile->isChecked());
}

void Main::settingsToggleAnimation()
{
    settings.setValue("/animation/use",
                      actionSettingsUseAnimation->isChecked());
}

void Main::settingsToggleDownloads() { downloadsEnabled(true); }

bool Main::settingsConfluence()
{
    if (!QSslSocket::supportsSsl())
    {
        QMessageBox::warning(
            0, tr("Warning"),
            tr("No SSL support available for this build of vym"));
        helpDebugInfo();
        return false;
    }

    ConfluenceSettingsDialog dia;
    dia.exec();

    if (dia.result() > 0)
        return true;
    else
        return false;
}

bool Main::settingsJIRA()
{
    if (!QSslSocket::supportsSsl())
    {
        QMessageBox::warning(
            0, tr("Warning"),
            tr("No SSL support available for this build of vym"));
        helpDebugInfo();
        return false;
    }

    JiraSettingsDialog dia;
    dia.exec();

    if (dia.result() > 0)
        return true;
    else
        return false;
}

void Main::focusMapEditor()
{
    VymView *vv = currentView();
    if (vv) 
        vv->setFocusMapEditor();
}

void Main::focusNoteEditor()
{
    noteEditor->parentWidget()->show();
    noteEditor->setFocus();
    actionViewToggleNoteEditor->setChecked(true);
}

void Main::toggleNoteEditor()
{
    if (noteEditor->parentWidget()->isVisible()) {
        noteEditor->parentWidget()->hide();
        focusMapEditor();
        actionViewToggleNoteEditor->setChecked(false);
    } else
        focusNoteEditor();
}

void Main::switchEditors()
{
    VymView *vv = currentView();
    if (vv) {
        MapEditor *me = vv->getMapEditor();
        if (me) {
            if (me->hasFocus()) {
                setTreeEditorsVisibility(true);
                vv->setFocusTreeEditor();
            } else
                vv->setFocusMapEditor();
        }
    }
}

void Main::toggleTreeEditors()
{
    bool b = !settings.value("/mainwindow/view/showTreeEditors", true).toBool();
    setTreeEditorsVisibility(b);
}

void Main::setTreeEditorsVisibility(bool b)
{
    // Close *all* TreeEditors in each VymView and update vym settings
    settings.setValue("/mainwindow/view/showTreeEditors", b);
    for (int i = 0; i < tabWidget->count(); i++) {
        if (((VymView*)tabWidget->widget(i)))
            ((VymView*)tabWidget->widget(i))->setTreeEditorVisibility(b);
    }
    updateActions();
}

void Main::focusTaskEditor()
{
    taskEditor->parentWidget()->show();
    actionViewToggleTaskEditor->setChecked(true);
    taskEditor->setFocus();
}

void Main::toggleTaskEditor()
{
    if (taskEditor->parentWidget()->isVisible()) {
        taskEditor->parentWidget()->hide();
        actionViewToggleTaskEditor->setChecked(false);
    } else
        focusTaskEditor();
}

void Main::toggleSlideEditors()
{
    bool b = !settings.value("/mainwindow/view/showSlideEditors", false).toBool();
    setSlideEditorsVisibility(b);

    if (b) {
        VymView *vv = currentView();
        if (vv)
            vv->setFocusSlideEditor();
    } else
        setFocusMapEditor();
}

void Main::setSlideEditorsVisibility(bool b)
{
    settings.setValue("/mainwindow/view/showSlideEditors", b);
    for (int i = 0; i < tabWidget->count(); i++)
        ((VymView*)tabWidget->widget(i))->setSlideEditorVisibility(b);
    updateActions();
}

void Main::focusScriptEditor()
{
    scriptEditor->parentWidget()->show();
    actionViewToggleScriptEditor->setChecked(true);
    scriptEditor->setFocus();
}

void Main::toggleScriptEditor()
{
    if (scriptEditor->parentWidget()->isVisible()) {
        scriptEditor->parentWidget()->hide();
        actionViewToggleScriptEditor->setChecked(false);
    } else
        focusScriptEditor();
}

void Main::focusScriptOutput()
{
    scriptOutput->parentWidget()->show();
    actionViewToggleScriptOutput->setChecked(true);
    // Currently ScriptEditor gets focus, when output is toggled
    // scriptOutput->setFocus();
    focusScriptEditor();
}
void Main::toggleScriptOutput()
{
    if (scriptOutput->parentWidget()->isVisible()) {
        scriptOutput->parentWidget()->hide();
        actionViewToggleScriptOutput->setChecked(false);
    } else
        focusScriptOutput();
}

void Main::focusHistory()
{
    historyWindow->parentWidget()->show();
    historyWindow->setFocus();
}

void Main::toggleHistory()
{
    if (historyWindow->parentWidget()->isVisible())
        historyWindow->parentWidget()->hide();
    else
        focusHistory();
}

void Main::focusProperty()
{
    branchPropertyEditor->parentWidget()->show();
    branchPropertyEditor->setFocus();
    branchPropertyEditor->setModel(currentModel());
}
void Main::toggleProperty()
{
    if (branchPropertyEditor->parentWidget()->isVisible())
        branchPropertyEditor->parentWidget()->hide();
    else
        focusProperty();
}

void Main::focusHeadingEditor()
{
    headingEditor->parentWidget()->show();
    headingEditor->setFocus();
    actionViewToggleHeadingEditor->setChecked(true);
}

void Main::toggleHeadingEditor()
{
    if (headingEditor->parentWidget()->isVisible()) {
        headingEditor->parentWidget()->hide();
        actionViewToggleHeadingEditor->setChecked(false);
    } else
        focusHeadingEditor();
}

void Main::toggleAntiAlias()
{
    bool b = actionViewToggleAntiAlias->isChecked();
    MapEditor *me;
    for (int i = 0; i < tabWidget->count(); i++) {
        me = view(i)->getMapEditor();
        if (me)
            me->setAntiAlias(b);
    }
}

bool Main::isAliased() { return actionViewToggleAntiAlias->isChecked(); }

bool Main::hasSmoothPixmapTransform()
{
    return actionViewToggleSmoothPixmapTransform->isChecked();
}

void Main::toggleSmoothPixmap()
{
    bool b = actionViewToggleSmoothPixmapTransform->isChecked();
    MapEditor *me;
    for (int i = 0; i < tabWidget->count(); i++) {

        me = view(i)->getMapEditor();
        if (me)
            me->setSmoothPixmap(b);
    }
}

void Main::clearScriptOutput() { scriptOutput->clear(); }

void Main::updateHistory(SimpleSettings &undoSet)
{
    historyWindow->update(undoSet);
}

void Main::updateHeading(const VymText &vt)
{
    VymModel *m = currentModel();
    if (m)
        m->setHeading(vt);
}

void Main::updateNoteText(const VymText &vt)
{
    // this slot is connected to noteEditor::textHasChanged()
    VymModel *m = currentModel();
    if (m)
        m->updateNoteText(vt);
}

void Main::updateNoteEditor(TreeItem *ti)
{
    if (ti) {
        VymNote note = ti->getNote();
        if (!note.isEmpty()) {
            noteEditor->setNote(note);  // fileName is set implicitly from note
        } else {
            noteEditor->clear();        // Also sets empty state
        }
        noteEditor->setFileNameHint(ti->headingText());
    } else
        noteEditor->setInactive();
    noteEditor->setTitle();
}

void Main::updateHeadingEditor(TreeItem *ti)
{
    VymModel *m = currentModel();
    if (!m) return;

    TreeItem *selti = m->getSelectedItem(ti);

    if (ti && ti->hasTypeBranchOrImage()) {
        // Color settings, also to prepare switching to RichText later
        if (ti->hasTypeBranch()) {
            BranchItem *bi = (BranchItem*)ti;
            headingEditor->setMapBackgroundColor(bi->getBackgroundColor(bi));
            headingEditor->setRichTextForegroundColor(bi->headingColor());  // FIXME-3 what about background color?? Frame background col?
        }

        headingEditor->setVymText(selti->heading());
        headingEditor->setTitle();
    }
}

void Main::selectInNoteEditor(QString s, int i)
{
    // TreeItem is already selected at this time, therefor
    // the note is already in the editor
    noteEditor->findText(s, QTextDocument::FindFlags(), i);
}

void Main::setFocusMapEditor()
{
    VymView *vv = currentView();
    if (vv)
        vv->setFocusMapEditor();
}

void Main::changeSelection(VymModel *model, const QItemSelection &,
                           const QItemSelection &)
{
    // Setting the model in BPE implicitely
    // also sets treeItem and updates content in BPE
    branchPropertyEditor->setModel(model);

    if (model && model == currentModel()) {
        BranchItem *selbi = model->getSelectedBranch();

        // Update satellites
        if (!selbi || model->getSelectedBranches().size() != 1) {
            noteEditor->setInactive();
            headingEditor->setInactive();
            taskEditor->clearSelection();
        } else {
            // Update note editor
            updateNoteEditor(selbi);

            // Show URL and link in statusbar
            QString status;
            QString s = selbi->url();
            if (!s.isEmpty())
                status += "URL: " + s + "  ";
            s = selbi->vymLink();
            if (!s.isEmpty())
                status += "Link: " + s;
            if (!status.isEmpty())
                statusMessage(status);

            // Update text in HeadingEditor
            updateHeadingEditor(selbi);

            // Select in TaskEditor, if necessary
            Task *t = selbi->getTask();

            if (t)
                taskEditor->select(t);
            else
                taskEditor->clearSelection();
        }
    }

    updateActions();
}

void Main::updateDockWidgetTitles(VymModel *model)
{
    QString s;
    if (model && !model->isRepositionBlocked()) {
        BranchItem *bi = model->getSelectedBranch();
        if (bi) {
            s = bi->headingPlain();
            noteEditor->setVymText(bi->getNote());
        }

        noteEditor->setTitle(s);
    }
}

void Main::updateActions()
{
    // updateActions is also called when satellites are closed
    actionViewToggleNoteEditor->setChecked(
        noteEditor->parentWidget()->isVisible());
    actionViewToggleHeadingEditor->setChecked(
        headingEditor->parentWidget()->isVisible());
    actionViewToggleTreeEditors->setChecked(
        settings.value("/mainwindow/view/showTreeEditors", true).toBool());
    actionViewToggleTaskEditor->setChecked(
        taskEditor->parentWidget()->isVisible());
    actionViewToggleHistoryWindow->setChecked(
        historyWindow->parentWidget()->isVisible());
    actionViewTogglePropertyEditor->setChecked(
        branchPropertyEditor->parentWidget()->isVisible());
    actionViewToggleScriptEditor->setChecked(
        scriptEditor->parentWidget()->isVisible());
    actionViewToggleSlideEditors->setChecked(
        settings.value("/mainwindow/view/showSlideEditors", true).toBool());

    if (JiraAgent::available())
        actionGetJiraDataSubtree->setEnabled(true);
    else
        actionGetJiraDataSubtree->setEnabled(false);

    if (ConfluenceAgent::available())
    {
        actionGetConfluencePageDetails->setEnabled(true);
        actionGetConfluencePageDetailsRecursively->setEnabled(true);
        actionConnectGetConfluenceUser->setEnabled(true);
        actionFileExportConfluence->setEnabled(true);
    } else
    {
        actionGetConfluencePageDetails->setEnabled(false);
        actionGetConfluencePageDetailsRecursively->setEnabled(false);
        actionConnectGetConfluenceUser->setEnabled(false);
        actionFileExportConfluence->setEnabled(false);
    }

    VymModel *m = currentModel();
    if (m) {
        QList<TreeItem *> seltis = m->getSelectedItems();
        QList<BranchItem *> selbis = m->getSelectedBranches();
        TreeItem *selti;
        selti = (seltis.count() >= 1) ? seltis.first() : nullptr;

        BranchItem *selbi;
        selbi = (selbis.count() >= 1) ? selbis.first() : nullptr;

        // readonly mode
        if (m->isReadOnly()) {
            // Disable toolbars
            standardFlagsMaster->setEnabled(false);
            userFlagsMaster->setEnabled(false);
            clipboardToolbar->setEnabled(false);
            editActionsToolbar->setEnabled(false);
            selectionToolbar->setEnabled(false);
            editorsToolbar->setEnabled(false);
            colorsToolbar->setEnabled(false);
            viewTransformationsToolbar->setEnabled(false);
            limitedViewToolbar->setEnabled(false);
            modModesToolbar->setEnabled(false);
            referencesToolbar->setEnabled(false);
            standardFlagsToolbar->setEnabled(false);
            userFlagsToolbar->setEnabled(false);

            // Disable map related actions in readonly mode // FIXME-3 not all actions disabled 
            foreach (QAction *a, restrictedMapActions)
                a->setEnabled(false);

        }
        else { // not readonly     

            // Enable toolbars
            standardFlagsMaster->setEnabled(true);
            userFlagsMaster->setEnabled(true);
            clipboardToolbar->setEnabled(true);
            editActionsToolbar->setEnabled(true);
            selectionToolbar->setEnabled(true);
            editorsToolbar->setEnabled(true);
            colorsToolbar->setEnabled(true);
            viewTransformationsToolbar->setEnabled(true);
            limitedViewToolbar->setEnabled(true);
            modModesToolbar->setEnabled(true);
            referencesToolbar->setEnabled(true);
            standardFlagsToolbar->setEnabled(true);
            userFlagsToolbar->setEnabled(true);

            // Enable map related actions
            foreach (QAction *a, restrictedMapActions)
                a->setEnabled(true);
        }
        // Enable all files actions first
        for (int i = 0; i < actionListFiles.size(); ++i)
            actionListFiles.at(i)->setEnabled(true);

        foreach (QAction *a, unrestrictedMapActions)
            a->setEnabled(true);

        // Disable other actions for now
        foreach (QAction *a, actionListBranches)
            a->setEnabled(false);
        foreach (QAction *a, actionListImages)
            a->setEnabled(false);
        foreach (QAction *a, actionListItems)
            a->setEnabled(false);

        // Ideally we would get targets from model first, but
        // avoid iterating whole tree, so just enable
        // bool b = m->getTargets().isEmpty();
        actionGoToTarget->setEnabled(true);
        actionGoToTargetLinkedMap->setEnabled(true);

        // Link style in context menu
        switch (m->mapDesign()->linkStyle(1)) { // FIXME-4 Currently global for map, all depths
            case LinkObj::Line:
                actionFormatLinkStyleLine->setChecked(true);
                break;
            case LinkObj::Parabel:
                actionFormatLinkStyleParabel->setChecked(true);
                break;
            case LinkObj::PolyLine:
                actionFormatLinkStylePolyLine->setChecked(true);
                break;
            case LinkObj::PolyParabel:
                actionFormatLinkStylePolyParabel->setChecked(true);
                break;
            default:
                break;
        }

        // Update colors
        QPixmap pix(16, 16);
        pix.fill(m->mapDesign()->backgroundColor());
        actionFormatBackground->setIcon(pix);
        pix.fill(m->getSelectionBrushColor());
        actionFormatSelectionColor->setIcon(pix);
        pix.fill(m->mapDesign()->defaultLinkColor());
        actionFormatLinkColor->setIcon(pix);

        // Selection history
        if (!m->canSelectPrevious())
            actionSelectPrevious->setEnabled(false);

        if (!m->canSelectNext())
            actionSelectNext->setEnabled(false);

        if (!m->getSelectedItem())
            actionSelectNothing->setEnabled(false);

        // Save and exit
        if (m->isSaving()) {
            actionFileSave->setEnabled(false);
            actionFileClose->setEnabled(false);
        } else {
            if (!m->hasChanged())
                actionFileSave->setEnabled(false);
            else {
                actionFileSave->setEnabled(true);
                actionFileRestoreSession->setEnabled(false);
                // qDebug() << __func__ << "disabling restoreSession";
            }
            actionFileClose->setEnabled(true);
        }
        

        // Undo/Redo
        QWidget *w = clipboardToolbar->widgetForAction(actionUndo);
        if (!m->isUndoAvailable()) {
            actionUndo->setEnabled(false);
            actionUndoVim->setEnabled(false);
            w->setToolTip(tr("Undo (%1)").arg(actionUndo->shortcut().toString()));
        } else {
            actionUndo->setToolTip(m->lastUndoComment());
            w->setToolTip(tr("Undo: %1 (%2)").arg(m->lastUndoComment(), actionUndo->shortcut().toString()));
        }

        w = clipboardToolbar->widgetForAction(actionRedo);
        if (!m->isRedoAvailable()) {
            actionRedo->setEnabled(false);
            w->setToolTip(tr("Redo (%1)").arg(actionRedo->shortcut().toString()));
        } else
            w->setToolTip(tr("Redo: %1 (%2)").arg(m->lastRedoComment(), actionRedo->shortcut().toString()));

        actionRepeatCommand->setEnabled(!repeatActionInt.isEmpty());

        // History window
        historyWindow->setWindowTitle(
            vymName + " - " +
            tr("History for %1", "Window Caption").arg(m->getFileName()));

        // Expanding/collapsing
        actionExpandAll->setEnabled(true);
        actionExpandOneLevel->setEnabled(true);
        actionCollapseOneLevel->setEnabled(true);
        actionCollapseUnselected->setEnabled(true);

        if (m->mapDesign()->linkColorHint() == LinkObj::HeadingColor)
            actionFormatLinkColorHint->setChecked(true);
        else
            actionFormatLinkColorHint->setChecked(false);

        // Export last
        QString desc, com, dest;
        if (m && m->exportLastAvailable(desc, com, dest))   // FIXME Only update, when currentModel changes?
            actionFileExportLast->setEnabled(true);
        else {
            actionFileExportLast->setEnabled(false);
            com = dest = "";
            desc = " - ";
        }
        QString sc = actionFileExportLast->shortcut().toString();
        if (!sc.isEmpty()) {
#if defined(Q_OS_MACOS)
            sc.replace("Ctrl","Cmd");
#endif
            sc = " (" + sc + ")";
        }

        actionFileExportLast->setToolTip(
            tr("Repeat last Export %1\nFormat: %2 to\n%3", "status tip")
                .arg(sc, desc, dest));

        if (seltis.count() > 0) { // Tree Item selected
            if (selti) 
                actionToggleTarget->setChecked(selti->isTarget());

            foreach (QAction *a, actionListItems)
                a->setEnabled(true);

            foreach (QAction *a, actionListImages)
                a->setEnabled(true);

            if (selti && selti->getType() == TreeItem::Image) {
                actionFormatHideLinkUnselected->setChecked(
                    ((MapItem *)selti)->hideLinkUnselected());
                actionFormatHideLinkUnselected->setEnabled(true);
            }

            if (selbis.count() > 0) { // Branch Item selected
                foreach (QAction *a, actionListBranches)
                    a->setEnabled(true);

                actionHeading2URL->setEnabled(true);

                // Note
                if (selbi) actionGetURLsFromNote->setEnabled(!selbi->getNote().isEmpty());

                // Take care of xlinks
                // FIXME-5 similar code in mapeditor mousePressEvent
                bool b = false;
                if (selbi && selbi->hasReference())
                    b = true;

                branchXLinksContextMenuEdit->setEnabled(b);
                branchXLinksContextMenuFollow->setEnabled(b);
                branchXLinksContextMenuEdit->clear();
                branchXLinksContextMenuFollow->clear();
                actionFollowReference->setEnabled(b);
                if (b) {
                    BranchItem *bi;
                    QString s;

                    // Add XLinks
                    for (int i = 0; i < selbi->xlinkCount(); ++i) {
                        bi = selbi->getXLinkItemNum(i)->getPartnerBranch();
                        if (bi) {
                            QString uid = (bi->getUuid()).toString();
                            s = bi->headingPlain();
                            if (s.length() > xLinkMenuWidth)
                                s = s.left(xLinkMenuWidth) + "...";
                            branchXLinksContextMenuEdit->addAction(s);
                            branchXLinksContextMenuFollow->addAction(
                                    tr("Branch", "Context menu to follow links") + ": " + s);
                            branchXLinksContextMenuFollow->actions().last()->setData(
                                    QString("XLink:%1").arg(uid));
                        }
                    }

                    // Add URL
                    s = selbi->url();
                    if (!s.isEmpty()) {
                        branchXLinksContextMenuFollow->addAction(
                                tr("Url", "Context menu to follow links") + ": " + s);
                        branchXLinksContextMenuFollow->actions().last()->setData(
                                QString("Url:%1").arg(s));
                    }

                    // Add VymLink
                    s = selbi->vymLink();
                    if (!s.isEmpty()) {
                        branchXLinksContextMenuFollow->addAction(
                                tr("Map", "Context menu to follow links") + ": " + s);
                        branchXLinksContextMenuFollow->actions().last()->setData(
                                QString("VymLink:%1").arg(s));
                    }
                }
                // Standard and user flags
                if (selbi)
                {
                    standardFlagsMaster->updateToolBar(selbi->activeFlagUids());
                    userFlagsMaster->updateToolBar(selbi->activeFlagUids());
                }

                // System Flags
                actionToggleScroll->setEnabled(true);
                if (selbi && selbi->isScrolled())
                    actionToggleScroll->setChecked(true);
                else
                    actionToggleScroll->setChecked(false);

                QString url;
                if (selti) url = selti->url();
                if (url.isEmpty()) {
                    actionOpenUrl->setEnabled(false);
                    actionGetConfluencePageDetails->setEnabled(false);
                }
                else {
                    actionOpenUrl->setEnabled(true);
                    if (ConfluenceAgent::available())
                        actionGetConfluencePageDetails->setEnabled(true);
                    else
                        actionGetConfluencePageDetails->setEnabled(false);
                }

                if (selti && selti->vymLink().isEmpty()) {
                    actionOpenVymLink->setEnabled(false);
                    actionOpenVymLinkBackground->setEnabled(false);
                    actionDeleteVymLink->setEnabled(false);
                }
                else {
                    actionOpenVymLink->setEnabled(true);
                    actionOpenVymLinkBackground->setEnabled(true);
                    actionDeleteVymLink->setEnabled(true);
                }

                if (selbi && selbi->getBranchContainer()->getOrientation() == BranchContainer::LeftOfParent)
                {
                    actionMoveDownDiagonally->setIcon(QPixmap(":down-diagonal-right.png"));
                    actionMoveUpDiagonally->setIcon(QPixmap(":up-diagonal-left.png"));
                }
                else
                {
                    actionMoveDownDiagonally->setIcon(QPixmap(":down-diagonal-left.png"));
                    actionMoveUpDiagonally->setIcon(QPixmap(":up-diagonal-right.png"));
                }

                if ((selbi && selbi->branchCount() < 2)  && selbis.count() < 2) { 
                    actionSortChildren->setEnabled(false);
                    actionSortBackChildren->setEnabled(false);
                }

                if (selbi) {
                    actionToggleHideExport->setEnabled(true);
                    actionToggleHideExport->setChecked(selbi->hideTemporary());

                    actionToggleTask->setEnabled(true);
                    if (!selbi->getTask())
                        actionToggleTask->setChecked(false);
                    else
                        actionToggleTask->setChecked(true);
                } else
                {
                    actionToggleHideExport->setEnabled(false);
                    actionToggleTask->setEnabled(false);
                }


                const QClipboard *clipboard = QApplication::clipboard();
                const QMimeData *mimeData = clipboard->mimeData();
                if (mimeData->formats().contains("application/x-vym") ||
                    mimeData->hasImage()) {
                    actionPasteVim->setEnabled(true);
                } else {
                    actionPaste->setEnabled(false);
                    actionPasteVim->setEnabled(false);
                }

                actionToggleTarget->setEnabled(true);
            } // end of BranchItem

            if (selti && selti->getType() == TreeItem::Image) {
                // Image selected
                foreach (QAction *a, actionListImages)
                    a->setEnabled(true);

                standardFlagsMaster->setEnabled(false);
                userFlagsMaster->setEnabled(false);

                actionOpenUrl->setEnabled(false);
                actionOpenVymLink->setEnabled(false);
                actionOpenVymLinkBackground->setEnabled(false);
                actionDeleteVymLink->setEnabled(false);
                actionToggleHideExport->setEnabled(true);
                actionToggleHideExport->setChecked(selti->hideTemporary());

                actionToggleTarget->setEnabled(true);

                // Allow pasting image onto image
                const QClipboard *clipboard = QApplication::clipboard();
                const QMimeData *mimeData = clipboard->mimeData();
                if (mimeData->hasImage())
                    actionPaste->setEnabled(true);
                else
                    actionPaste->setEnabled(false);

                actionGrowSelectionSize->setEnabled(true);
                actionShrinkSelectionSize->setEnabled(true);
                actionResetSelectionSize->setEnabled(true);
            } // Image selected

            if (selti && selti->hasTypeBranchOrImage()) {
                bool b = m->canMoveUp(selti);
                actionMoveUp->setEnabled(b);
                if (selti->hasTypeImage())
                    b = false;
                actionMoveUpDiagonally->setEnabled(b);
                actionMoveDown->setEnabled(m->canMoveDown(selti));
                if ((selti->depth() == 0) || selbis.count() > 1 || selti->hasTypeImage())
                    actionMoveDownDiagonally->setEnabled(false);
            }

        } // TreeItem
        else
        {
            actionToggleHideExport->setEnabled(false);
        }

        // Check (at least for some) multiple selection
        if (selbis.count() > 0)
        {
            actionFormatColorBranch->setEnabled(true);
            actionFormatColorSubtree->setEnabled(true);
        }
    }
    else {
        // No map available
        for (int i = 0; i < actionListFiles.size(); ++i)
            actionListFiles.at(i)->setEnabled(false);

        foreach (QAction *a, unrestrictedMapActions)
            a->setEnabled(false);

        // Disable toolbars
        standardFlagsMaster->setEnabled(false);
        userFlagsMaster->setEnabled(false);
        clipboardToolbar->setEnabled(false);
        editActionsToolbar->setEnabled(false);
        selectionToolbar->setEnabled(false);
        editorsToolbar->setEnabled(false);
        colorsToolbar->setEnabled(false);
        viewTransformationsToolbar->setEnabled(false);
        limitedViewToolbar->setEnabled(false);
        modModesToolbar->setEnabled(false);
        referencesToolbar->setEnabled(false);
        standardFlagsToolbar->setEnabled(false);
        userFlagsToolbar->setEnabled(false);
    }
}

Main::ModMode Main::getModMode()
{
    if (actionModModePoint->isChecked())
        return ModModePoint;
    if (actionModModeColor->isChecked())
        return ModModeColor;
    if (actionModModeXLink->isChecked())
        return ModModeXLink;
    if (actionModModeMoveObject->isChecked())
        return ModModeMoveObject;
    if (actionModModeMoveView->isChecked())
        return ModModeMoveView;
    return ModModeUndefined;
}

bool Main::autoSelectNewBranch()
{
    return actionSettingsAutoSelectNewBranch->isChecked();
}

void Main::scriptPrint(const QString &s, const QString &color)
{
    scriptOutput->append(s);

    std::string prefix;
    std::string postfix = "\033[0m";

    if (color == "red")
        prefix = "\033[1;31m";
    else if (color == "green")
        prefix = "\033[1;32m";
    else if (color == "blue")
        prefix = "\033[1;34m";
    else if (color == "yellow")
        prefix = "\033[1;33m";
    else if (color == "magenta")
        prefix = "\033[1;35m";
    else if (color == "cyan")
        prefix = "\033[1;36m";
    else if (color == "white")
        prefix = "\033[1;37m";

    std::cout << prefix << s.toStdString()  << postfix << endl;
}

QVariant Main::runScript(const QString &script)
{
    // Setup new scriptEngine
    scriptEngine = new QJSEngine();
    VymWrapper *vymWrapper = new VymWrapper;
    //scriptEngine->installExtensions(QJSEngine::ConsoleExtension);
    if (debug) {
        std::cout << "Created new scriptengine " << scriptEngine << endl;
        std::cout << "MainWindow::runScript starting to execute:" << endl;
        std::cout << qPrintable("----------\n" + script + "\n----------") << endl;
    }
    logInfo("Starting to execute: " + script.left(30), __func__);

    // Run script

    scriptResult.clear();

    // Make sure that deleting scriptEngine later does not delete vymWrapper, too
    scriptEngine->setObjectOwnership(vymWrapper, QJSEngine::CppOwnership);

    /*
    if (debug)
        std:cout << "      vymWrapper: " << vymWrapper << "  " << vymWrapper->mapCount() << " maps   version:" << vymWrapper->version().toStdString() << endl;
    */

    QJSValue vwrapper = scriptEngine->newQObject(vymWrapper);

    scriptEngine->globalObject().setProperty("vym", vwrapper);
    QJSEngine *scriptEngineOrg = scriptEngine;
    QJSValue result = scriptEngine->evaluate(script);

    // Nested scripts (undo/redo) would reset global scriptEngine pointer
    // Global pointer is required for Main::abortScript()
    scriptEngine = scriptEngineOrg;

    logInfo("Finished executing: " + script.left(30), __func__);

    if (debug) {
        std::cout << "MainWindow::runScript finished scriptEngine: " << scriptEngine << endl;
        std::cout << "       hasError: " << result.isError() << endl;
        //std::cout << "     lastResult: " << scriptEngine->globalObject().property("lastResult").toVariant().toString() << endl;
    }

    if (result.isError()) {
        // Warnings, in case that output window is not visible...
        statusMessage("Script execution failed");
        int lineNumber = result.property("lineNumber").toInt();
        qWarning() << "Script execution failed"
            << lineNumber
            << ":" << result.toString();
        scriptOutput->append(QString("uncaught exception at line %1: %2")
                                 .arg(lineNumber).arg(result.toString()));
    }

    if (debug) {
        std::cout << "Main::runScript finished  scriptEngine: " << scriptEngine << endl;
        std::cout << "Deleting scriptEngine " << scriptEngine << endl;
    }
    vymWrapper->deleteLater();
    scriptEngine->deleteLater();
    scriptEngine = nullptr;

    if (exitAfterScriptInt)
        fileExitVym();

    return scriptResult;
}

QVariant Main::runScriptWithMacros(const QString &script)
{
    return runScript(macros.get() + script);
}

void Main::abortScript(const QJSValue::ErrorType &err, const QString &msg)
{
    if (!scriptEngine)
        qWarning() << "MainWindow::abortScript  has no scriptEngine!";
    else
        scriptEngine->throwError(err, msg);
}

void Main::abortScript(const QString &msg)
{
    abortScript(QJSValue::GenericError, msg);
}

QVariant Main::setScriptResult(const QVariant &r)
{
    scriptResult = r;
    return r;
}

QObject *Main::getCurrentModelWrapper()
{
    // Called from VymWrapper to find out current model in a script
    VymModel *m = currentModel();
    //std:cout << "Main::getCurrentModelWrapper  mw=" << m->getWrapper() << endl;
    if (m)
        return m->getWrapper();
    else
        return nullptr;
}

bool Main::gotoWindow(const int &n)
{
    if (n < tabWidget->count() && n >= 0) {
        tabWidget->setCurrentIndex(n);
        return true;
    }
    return false;
}

void Main::nextEditor()
{
    if (tabWidget->currentIndex() < tabWidget->count())
        tabWidget->setCurrentIndex(tabWidget->currentIndex() + 1);
}

void Main::previousEditor()
{
    if (tabWidget->currentIndex() > 0)
        tabWidget->setCurrentIndex(tabWidget->currentIndex() - 1);
}

void Main::nextSlide()
{
    VymView *cv = currentView();
    if (cv)
        cv->nextSlide();
}

void Main::previousSlide()
{
    VymView *cv = currentView();
    if (cv)
        cv->previousSlide();
}

void Main::flagChanged()
{
    MapEditor *me = currentMapEditor();
    VymModel *m = currentModel();
    if (me && m && me->state() != MapEditor::EditingHeading) {
        m->toggleFlagByUid(QUuid(sender()->objectName()), nullptr,
                           actionSettingsUseFlagGroups->isChecked());
    }
}

void Main::testFunction1()
{
    // Avail. styles:
    // Linux (KDE): Breeze,bb10dark,bb10bright,cleanlooks,gtk2,cde,motif,plastique,Windows,Fusion
    // Windows: windowsvista,Windows,Fusion
    //#include <QStyleFactory>
    //qApp->setStyle(QStyleFactory::create("windowsvista"));

    VymModel *m = currentModel();
    if (m) {
        m->test();
        //m->getMapEditor()->testFunction1();
    }
}

void Main::testFunction2()
{
    VymModel *m = currentModel();
    if (m) {
        //currentMapEditor()->testFunction2();
    }
}

void Main::toggleWinter()
{
    if (!currentMapEditor())
        return;
    currentMapEditor()->toggleWinter();
}

void Main::toggleHideExport()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleHideExport();
}

void Main::testCommand()
{
    if (!currentMapEditor())
        return;
    scriptEditor->show();
}

void Main::helpDoc()
{
    QString locale = QLocale::system().name();
    QString docname;
    if (locale.left(2) == "es")
        docname = "vym_es.pdf";
    else if (locale.left(2) == "fr")
        docname = "vym_fr.pdf";
    else
        docname = "vym.pdf";

    QStringList searchList;
#if defined(Q_OS_MACOS)
    searchList << vymBaseDir.path() + "/doc";
#elif defined(Q_OS_WIN32)
    searchList << vymInstallDir.path() + "doc/" + docname;
#else
#if defined(VYM_DOCDIR)
    searchList << VYM_DOCDIR;
#endif
    // default path in SUSE LINUX
    searchList << "/usr/share/doc/packages/vym";
#endif

    searchList << "doc"; // relative path for easy testing in tarball
    searchList << "/usr/share/doc/vym";      // Debian
    searchList << "/usr/share/doc/packages"; // Knoppix

    bool found = false;
    QFile docfile;
    for (int i = 0; i < searchList.count(); ++i) {
        docfile.setFileName(searchList.at(i) + "/" + docname);
        if (docfile.exists()) {
            found = true;
            break;
        }
    }

    if (!found) {
        QMessageBox::critical(0, tr("Critcal error"),
                              tr("Couldn't find the documentation %1 in:\n%2")
                                  .arg(docname, searchList.join("\n")));
        return;
    }

    QStringList args;
    VymProcess *pdfProc = new VymProcess();
    args << QDir::toNativeSeparators(docfile.fileName());

    if (!pdfProc->startDetached(settings.value("/system/readerPDF").toString(),
                                args)) {
        // error handling
        QMessageBox::warning(
            0, tr("Warning"),
            tr("Couldn't find a viewer to open %1.\n").arg(docfile.fileName()) +
                tr("Please use Settings->") +
                tr("Set application to open PDF files"));
        settingsPDF();
        return;
    }
}

void Main::helpDemo()
{
    QStringList filters;
    filters << "VYM example map (*.vym)";
    QFileDialog fd;
    fd.setDirectory(vymBaseDir.path() + "/demos");
    fd.setFileMode(QFileDialog::ExistingFiles);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " + tr("Load vym example map"));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    QString fn;
    if (fd.exec() == QDialog::Accepted) {
        lastMapDir = fd.directory();
        QStringList flist = fd.selectedFiles();
        QStringList::Iterator it = flist.begin();
        initProgressCounter(flist.count());
        while (it != flist.end()) {
            fn = *it;
            fileLoad(*it, File::NewMap, File::VymMap);
            ++it;
        }
        removeProgressCounter();
        tabWidget->setCurrentIndex(tabWidget->count() - 1);
    }
}

void Main::helpShortcuts()
{
    ShowTextDialog dia;
    dia.useFixedFont(true);
    dia.setText(switchboard.getASCII());
    dia.exec();
}

void Main::helpMacros()
{
    ShowTextDialog dia;
    dia.useFixedFont(true);
    dia.setText(macros.help());
    dia.exec();
}

QString Main::scriptingCommands()
{
    QString s  = "Available commands in vym\n";
    s += "=========================\n";
    foreach (Command *c, vymCommands) {
        s += c->description();
        s += "\n";
    }
    s += "\n";

    s += "Available commands in map\n";
    s += "=========================\n";
    foreach (Command *c, modelCommands) {
        s += c->description();
        s += "\n";
    }
    s += "\n";

    s += "Available commands of a branch\n";
    s += "==============================\n";
    foreach (Command *c, branchCommands) {
        s += c->description();
        s += "\n";
    }
    s += "\n";

    s += "Available commands of an image\n";
    s += "==============================\n";
    foreach (Command *c, imageCommands) {
        s += c->description();
        s += "\n";
    }

    s += "Available commands of an xlink\n";
    s += "==============================\n";
    foreach (Command *c, xlinkCommands) {
        s += c->description();
        s += "\n";
    }

    return s;
}

void Main::helpScriptingCommands()
{
    ShowTextDialog dia;
    dia.useFixedFont(true);
    QString s = scriptingCommands();
    dia.setText(s);
    dia.exec();
}

void Main::helpDebugInfo()
{
    ShowTextDialog dia;
    dia.useFixedFont(true);
    dia.setText(debugInfo());
    dia.setMinimumWidth(900);
    dia.exec();
}

void Main::helpVymDevelopment()
{
    DownloadAgent *agent =
        new DownloadAgent(QUrl("https://www.insilmaril.de/vym/helpVymDevelopment.html"));
    connect(agent, SIGNAL(downloadFinished()), this,
            SLOT(helpVymDevelopmentFinished()));
    QTimer::singleShot(0, agent, SLOT(execute()));
}

void Main::helpVymDevelopmentFinished()
{
    DownloadAgent *agent = static_cast<DownloadAgent *>(sender());

    if (agent->isSuccess()) {
        QString page;
        if (loadStringFromDisk(agent->getDestination(), page)) {
            ShowTextDialog dia(this);
            dia.setText(page);
            dia.exec();
        }
    }
    else {
        statusMessage("Downloading VYM development page failed.");
	logInfo("Failed to download page: " + agent->getResultMessage(), __func__);
        if (debug) {
            qDebug() << "Main::helpVymDevelopmentFinished ";
            qDebug() << "  result: failed";
            qDebug() << "     msg: " << agent->getResultMessage();
        }
    }
    agent->deleteLater();
}

void Main::helpAbout()
{
    AboutDialog ad;
    ad.setMinimumSize(600, 400);
    ad.resize(QSize(600, 400));
    ad.exec();
}

void Main::helpAboutQT()
{
    QMessageBox::aboutQt(this, "Qt Application Example");
}

void Main::callMacro()
{
    QAction *action = qobject_cast<QAction *>(sender());
    int i = -1;
    if (action) {
        QString modifiers;

        i = action->data().toInt();

        if (i > 11 && i < 24) {
            modifiers = "shift_";
            i = i - 12;
        } else if (i > 23 && i < 36) {
            modifiers = "ctrl_";
            i = i - 24;
        } else if (i > 35) {
            modifiers = "ctrl_shift_";
            i = i - 36;
        }

        // Function keys start at "1", not "0"
        i++;

        QString macro = QString("macro_%1f%2();\n").arg(modifiers).arg(i);


        runScript(macros.get() + macro);
        setRepeatAction(macro);
    }
}

void Main::downloadReleaseNotesFinished()
{
    DownloadAgent *agent = static_cast<DownloadAgent *>(sender());

    if (agent->isSuccess()) {
        QString page;
        if (agent->isSuccess()) {
            if (loadStringFromDisk(agent->getDestination(), page)) {
                page.replace("<!-- VYMVERSION -->", " (" + vymVersion + ")");
                ShowTextDialog dia(this);
                dia.setText(page);
                dia.exec();

                // Don't load the release notes automatically again
                settings.setValue("/downloads/releaseNotes/shownVersion",
                                  vymVersion);
            }
        }
    }
    else {
        statusMessage("Downloading release notes failed.");
        if (debug) {
            qDebug() << "Main::downloadReleaseNotesFinished ";
            qDebug() << "  result: failed";
            qDebug() << "     msg: " << agent->getResultMessage();
        }
    }
    agent->deleteLater();

    if (checkUpdatesAfterReleaseNotes)
    {
        // After startup we want to check also for updates, but only after
        // releasenotes are there (and we have a cookie already)
        checkUpdatesAfterReleaseNotes = false;
        checkUpdates();
    }
}

QUrl Main::serverUrl(const QString &scriptName)
{
    // Local URL for testing only
    // QString("http://localhost/release-notes.php?vymVersion=%1") /
    return QUrl(
            QString("http://www.insilmaril.de/vym/%1?").arg(scriptName) +
            QString("&vymVersion=%1").arg(vymVersion) +
            QString("&darkTheme=%1").arg(usingDarkTheme) +
            QString("&localeName=%1").arg(localeName) +
            QString("&vymBuildDate=%1").arg(vymBuildDate) +
            QString("&vymCodeQuality=%1").arg(vymCodeQuality) +
            QString("&vymCodeName=%1").arg(vymCodeName)
        );
}

void Main::checkReleaseNotesAndUpdates ()
{
    // Called once after startup
    // checkUpdatesAfterReleaseNotes is already true then
    checkReleaseNotes();
}

void Main::checkReleaseNotes ()
{
    bool userTriggered;
    if (qobject_cast<QAction *>(sender()))
        userTriggered = true;
    else
        userTriggered = false;

    if (downloadsEnabled()) {
        if (userTriggered ||
            versionLowerThanVym(
                settings.value("/downloads/releaseNotes/shownVersion", "0.0.1")
                    .toString())) {
            DownloadAgent *agent = new DownloadAgent(serverUrl("release-notes.php"));
            connect(agent, SIGNAL(downloadFinished()), this,
                    SLOT(downloadReleaseNotesFinished()));
            QTimer::singleShot(0, agent, SLOT(execute()));
        }
    }
    else {
        // No downloads enabled
        if (userTriggered) {
            // Notification: vym could not download release notes
            QMessageBox::warning(
                0, tr("Warning"),
                tr("Please allow vym to download release notes!"));
            if (downloadsEnabled(userTriggered))
                checkUpdates();
        }
    }
}

bool Main::downloadsEnabled(bool userTriggered)
{
    bool result;
    if (!userTriggered &&
        settings.value("/downloads/enabled", false).toBool()) {
        result = true;
    }
    else {
        QDate lastAsked =
            settings.value("/downloads/permissionLastAsked", QDate(1970, 1, 1))
                .toDate();
        if (userTriggered ||
            !settings.contains("/downloads/permissionLastAsked") ||
            lastAsked.daysTo(QDate::currentDate()) > 7) {
            QString infotext;
            infotext =
                tr("<html>"
                   "<h3>Do you allow vym to check online for updates or "
                   "release notes?</h3>"
                   "If you allow, vym will "
                   "<ul>"
                   "<li>check once for release notes</li>"
                   "<li>check regulary for updates and notify you in case you "
                   "should update, e.g. if there are "
                   "important bug fixes available</li>"
                   "<li>receive a cookie with a random ID and send some anonymous data, like:"
                   "<ul>"
                   "<li>vym version</li>"
                   "<li>platform name and the ID (e.g. \"Windows\" or \"Linux\")</li>"
                   "<li>if you are using dark theme</li>"
                   "</ul>"
                   "This data is sent to me, Uwe Drechsel."
                   "<p>As vym developer I am motivated to see "
                   "many people using vym. Of course I am curious to see, on "
                   "which system vym is used. Maintaining each "
                   "of the systems requires a lot of my (spare) time.</p> "
                   "<p>No other data than above will be sent, especially no "
                   "private data will be collected or sent."
                   "(Check the source code, if you don't believe.)"
                   "</p>"
                   "</li>"
                   "</ul>"
                   "If you do not allow, "
                   "<ul>"
                   "<li>nothing will be downloaded and especially I will "
                   "<b>not be motivated</b> "
                   "to spend some more thousands of hours on developing a free "
                   "software tool."
                   "</ul>"
                   "Please allow vym to check for updates :-)");
            QMessageBox mb(QMessageBox::Information, vymName, infotext);
            QPushButton *allowButton = mb.addButton(tr("Allow"), QMessageBox::AcceptRole);
            mb.addButton(tr("Do not allow"), QMessageBox::RejectRole);
            mb.setDefaultButton(allowButton);
            mb.exec();
            if (mb.clickedButton() == allowButton) {
                result = true;
                QMessageBox msgBox;
                msgBox.setText(tr("Thank you for enabling downloads!"));
                msgBox.setStandardButtons(QMessageBox::Close);
                msgBox.setIconPixmap(QPixmap(":/flag-face-smile.svg"));
                msgBox.exec();
            } else {
                result = false;
                QMessageBox msgBox;
                msgBox.setText(tr("That's ok, though I would be happy to see many users working with vym and also on which platforms."));
                msgBox.setStandardButtons(QMessageBox::Close);
                msgBox.setIconPixmap(QPixmap(":/flag-face-sad.svg"));
                msgBox.exec();
            }
        } else
            result = false;
        actionSettingsToggleDownloads->setChecked(result);
        settings.setValue("/downloads/enabled", result);
        settings.setValue("/downloads/permissionLastAsked",
                          QDate::currentDate().toString(Qt::ISODate));
    }
    return result;
}

void Main::downloadUpdatesFinished(bool userTriggered)
{
    DownloadAgent *agent = static_cast<DownloadAgent *>(sender());

    if (agent->isSuccess()) {
        ShowTextDialog dia;
        dia.setWindowTitle(vymName + " - " + tr("Update information"));
        QString page;
        if (loadStringFromDisk(agent->getDestination(), page)) {
            page.replace("<!-- VYMVERSION -->", " (" + vymVersion + ")");
            if (page.contains("vymisuptodate")) {
                statusMessage(tr("vym is up to date.", "MainWindow"));
                if (userTriggered) {
                    // Notification: vym is up to date!
                    dia.setHtml(page);
                    dia.exec();
                }
            }
            else if (page.contains("vymneedsupdate")) {
                // Notification: updates available
                dia.setHtml(page);
                dia.exec();
            }
            else {
                // Notification: Unknown version found
                dia.setHtml(page);
                dia.exec();
            }

            // Prepare to check again later
            settings.setValue("/downloads/updates/lastChecked",
                              QDate::currentDate().toString(Qt::ISODate));
        }
        else
            statusMessage("Couldn't load update page from " +
                          agent->getDestination());
    }
    else {
        statusMessage("Check for updates failed.");
        if (debug) {
            qDebug() << "Main::downloadUpdatesFinished ";
            qDebug() << "  result: failed";
            qDebug() << "     msg: " << agent->getResultMessage();
        }
    }
    agent->deleteLater();
}

void Main::downloadUpdatesFinishedInt() { downloadUpdatesFinished(true); }

void Main::downloadUpdates(bool userTriggered)
{
    DownloadAgent *agent = new DownloadAgent(serverUrl("updates.php"));
    if (userTriggered)
        connect(agent, SIGNAL(downloadFinished()), this,
                SLOT(downloadUpdatesFinishedInt()));
    else
        connect(agent, SIGNAL(downloadFinished()), this,
                SLOT(downloadUpdatesFinished()));
    statusMessage(tr("Checking for updates...", "MainWindow"));
    QTimer::singleShot(0, agent, SLOT(execute()));
}

void Main::checkUpdates()
{
    bool userTriggered;
    if (qobject_cast<QAction *>(sender()))
        userTriggered = true;
    else
        userTriggered = false;

    if (downloadsEnabled()) {
        // Too much time passed since last update check?
        QDate lastChecked =
            settings.value("/downloads/updates/lastChecked", QDate(1970, 1, 1))
                .toDate();
        if (!lastChecked.isValid())
            lastChecked = QDate(1970, 1, 1);
        if (lastChecked.daysTo(QDate::currentDate()) >
                settings.value("/downloads/updates/checkInterval", 3).toInt() ||
            userTriggered == true) {
            downloadUpdates(userTriggered);
        }
    }
    else {
        // No downloads enabled
        if (userTriggered) {
            // Notification: vym could not check for updates
            QMessageBox::warning(0, tr("Warning"),
                                 tr("Please allow vym to check for updates!"));
            if (downloadsEnabled(userTriggered))
                checkUpdates();
        }
    }
}

void Main::escapePressed()
{
    if (presentationMode)
        togglePresentationMode();

    setFocusMapEditor();
}

void Main::togglePresentationMode()
{
    QMap<QToolBar *, bool>::const_iterator i = toolbarStates.constBegin();
    if (!presentationMode) {

        presentationMode = true;
        while (i != toolbarStates.constEnd()) {
            toolbarStates[i.key()] = i.key()->isVisible();
            i.key()->hide();
            ++i;
        }
        menuBar()->hide();
    }
    else {
        presentationMode = false;
        while (i != toolbarStates.constEnd()) {
            i.key()->setVisible(i.value());
            ++i;
        }
        menuBar()->show();
    }

}

void Main::toggleHideTmpMode()
{
    VymModel *m = currentModel();
    if (m)
        m->toggleHideTmpMode();
}
