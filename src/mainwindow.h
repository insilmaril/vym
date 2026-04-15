#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QItemSelection>
#include <QJSValue>
#include <QMainWindow>
#include <QProgressDialog>
#include <QTextStream>

#include "file.h"
#include "flag.h"

#include "settings.h"

class QPrinter;
class QJSEngine;

class HistoryWindow;
class MapEditor;
class TreeItem;
class VymText;
class VymModel;
class VymView;
class VymWrapper;

class Main : public QMainWindow {
    Q_OBJECT

  public:
    /*! Modifier modes are used when SHIFT together with a mouse button is
     * pressed */
    enum ModMode {
        ModModeUndefined,  //!< Unused
        ModModePoint,      //!< Regular mode: Point and relink items
        ModModeColor,      //!< Pick color from object
        ModModeXLink,      //!< Create a XLink (XLinkObj) from selected object
        ModModeMoveObject, //!< Move object without linking
        ModModeMoveView    //!< Move view without changing
    };

    Main(QWidget *parent = 0);
    ~Main();
    void loadCmdLine();

  private:
    QProgressDialog progressDialog;
    int progressMax;
    int progressCounter;
    int progressCounterTotal;

  public:
    void logInfo(const QString &comment, const QString &caller = "");
    void statusMessage(const QString &, int timeout = 10000);
    void setProgressMaximum(int max);
    void addProgressValue(float v);
    void initProgressCounter(uint n = 1);
    void removeProgressCounter();

  public slots:
    void fileNew();
    void fileNewCopy();

    void satelliteVisibilityChanged();

  protected:
    void closeEvent(QCloseEvent *);

  public:
    QPrinter *setupPrinter();

  private:
    void setupAPI();

    /*! Helper method to clone actions later in MapEditor */
    void cloneActionMapEditor(QAction *a);

    void setupFileActions();
    void setupEditActions();
    void setupEditMenu();
    void setupSelectActions();
    void setupFormatActions();
    void setupViewActions();
    void setupConnectActions();
    void setupModeActions();
    void setupWindowActions();
    void setupFlagActions();

  public slots:
    void addUserFlag();

  public:
    Flag *setupFlag(const QString &path, Flag::FlagType type,
                    const QString &name, const QString &tooltip,
                    const QUuid &uid = QUuid(), const QKeySequence &ks = 0);

  private:
    void setupNetworkActions();
    void setupSettingsActions();
    void setupTestActions();
    void setupHelpActions();
    void setupContextMenus();
    void setupRecentMapsMenu();
    void setupMacros();
    void setupToolbars();
    VymView *currentView() const;
    VymView *view(const int i);

  public:
    MapEditor *currentMapEditor() const;
    VymModel *currentModel() const;
    uint currentMapId() const;
    int currentMapIndex() const;
    VymModel *modelWithId(uint);
    bool gotoModel(VymModel *m);
    bool gotoModelWithId(uint id);

  public slots:
    bool closeModelWithId(uint id);
    void closeSavedModels();

  public:
    int modelCount();
    void updateTabName(VymModel *vm);

  private slots:
    void editorChanged();

  private:
    bool exitAfterLastMapClosed;

  public slots:
    bool fileLoad(QString, const File::LoadMode &, const File::FileType &ftype);
    void fileLoad(const File::LoadMode &);
  private slots:
    void fileLoad();
    void fileSaveSession();
  public slots:
    void fileRestoreSession();
  private slots:
    void fileLoadRecent();
    void fileClearRecent();
    void addRecentMap(const QString &);
    void fileSave(VymModel *, const File::SaveMode &);
    void fileSave();
  public slots:
    void fileSave(VymModel *); // autosave from MapEditor
  private slots:
    bool fileSaveAs(const File::SaveMode &, QString fileName);
    void fileSaveAs();
    void fileSaveAsDefault();
    void fileImportFirefoxBookmarks();
    void fileImportFreemind();
    void fileImportIThoughts();
    void fileImportMM();
    void fileImportDir();
    void fileExportAO();
    void fileExportASCII();
    void fileExportASCIITasks();
    void fileExportCSV();
    void fileExportConfluence();
    void fileExportFirefoxBookmarks();
    void fileExportHTML();
    void fileExportImage();
    void fileExportImpress();
    void fileExportLaTeX();
    void fileExportMarkdown();
    void fileExportOrgMode();
    void fileExportPDF();
    void fileExportSVG();
    void fileExportTaskJuggler();
    void fileExportXML();
    void fileExportLast();
    void fileCloseTab(int i);           // Index of tab
    void fileCloseCurrentMap();         // Calls fileCloseModelWithId(-1);
    void fileCloseMapWithId(uint i);    // id = -1 uses current model
    void filePrint();

  public:
    bool exitAfterScript();
    void setExitAfterScript(bool b);

  private:
    bool exitAfterScriptInt;
    QString repeatActionInt;

  public:
    void setRepeatAction(const QString &script);

  public slots:
    void fileExitVym();
    void editUndo();
    void editRedo();
    void gotoHistoryStep(int);

  private slots:
    void editCopy();
    void editPaste();
    void editCut();

  public slots:
    void updateQueries(VymModel *);
    bool openUrl(const QString &url = "", bool privateMode = false);
    void openTabs(QStringList, bool privateMode = false);

  private slots:
    void editOpenMultipleVisUrls(bool ignoreScrolled = true, bool privateMode = false);
    void editOpenMultipleUrls();
    void editOpenMultipleUrlsPrivate();
    void editNote2URLs();
    void editURL();
    void editLocalURL();
    void editHeading2URL();
    void setJiraQuery();
    void getJiraDataSubtree();
    void getConfluencePageDetails();
    void getConfluencePageDetailsRecursively();
    void getConfluenceUser();
    void openVymLinks(const QStringList &, bool background = false);
    void editVymLink();
    void editOpenMultipleVymLinks();
  public slots:
    void editHeading();
    void editHeadingFinished(VymModel *m);
    void editOpenVymLink(bool background = false);
    void editOpenVymLinkBackground();
  private slots:
    void editDeleteVymLink();
    void editToggleHideExport();
    void editToggleTask();
    void editCycleTaskStatus();
    void editTaskResetDeltaPrio();
    void editTaskSleepN();
    void editAddTimestamp();
    void editMapProperties();
    void editMoveUp();
    void editMoveDown();
    void editMoveUpDiagonally();
    void editMoveDownDiagonally();
    void editDetach();
    void editSortChildren();
    void editSortBackChildren();
    void editToggleScroll();
    void editExpandAll();
    void editExpandOneLevel();
    void editCollapseOneLevel();
    void editCollapseUnselected();
    void editUnscrollSubtree();
    void editGrowSelectionSize();
    void editShrinkSelectionSize();
    void editRotateSubtreeCW();
    void editRotateSubtreeCCW();
    void editResetSelectionSize();
    void editRepeatLastAction();
    void editAddMapCenter();
    void editAddClone();
    void editAddBranch();
    void editAddBranchBefore();
    void editAddBranchAbove();
    void editAddBranchBelow();
    void editImportAdd();
    void editImportReplace();
    void editSaveSelection();
    void editDeleteKeepChildren();
    void editDeleteChildren();
    void editDeleteSelection();
    void editLoadImage();
    void editSaveImage();
    void editEditXLink(QAction *);
    void popupFollowReference();
    void followReference(QAction *);

  private slots:
    bool initLinkedMapsMenu(VymModel *model, QMenu *menu);

  public slots:
    void editGoToLinkedMap();

  private slots:
    void editToggleTarget();
    bool initTargetsMenu(VymModel *model, QMenu *menu);
    void editGoToTarget();
    void editMoveToTarget();
    void editSelectFirstSibling();
    void editSelectLastSibling();
    void editSelectPrevious();
    void editSelectNext();
    void editSelectNothing();
    void editOpenFindResultWidget();
    void editFindNext(QString s, bool searchNotesFlag);
    void editFindDuplicateURLs();

  public slots:
    void selectQuickColor(int n);
    void setQuickColor(QColor col);
    void quickColorPressed();
    void formatPickColor();
    QColor getCurrentColor();
    int getCurrentColorIndex();
    void setCurrentColor(QColor);

  private slots:
    void formatColorBranch();
    void formatColorSubtree();
    void formatLinkStyleLine();
    void formatLinkStyleParabel();
    void formatLinkStylePolyLine();
    void formatLinkStylePolyParabel();
    void formatBackground();
    void formatSelectLinkColor();
    void formatSelectSelectionColor();
    void formatSelectFont();
    void formatToggleLinkColorHint();
    void formatHideLinkUnselected();

  public slots:
    void viewZoomReset();
    void viewZoomIn();
    void viewZoomOut();
    void viewRotateCounterClockwise();
    void viewRotateClockwise();
    void viewCenter();
    void viewCenterScaled();
    void viewCenterRotated();

  public slots:
    void networkStartServer();
    void networkConnect();
    void downloadFinished();
    void settingsPDF();
    void settingsURL();
    void settingsActionLog();
    void settingsMacroPath();
    void settingsUndoLevels();
    void settingsDefaultMapPath();

  public:
    QString defaultMapPath();   // Default path, used with "auto" to define newMapPath
    QString newMapPath();       // Depends on settings and dark theme
    bool useAutosave();
    void setAutosave(bool b);

  public slots:
    void settingsAutosaveTime();
    void settingsDefaultMapAuthor();
    void settingsDarkTheme();
    void settingsShowParentsLevelTasks();
    void settingsShowParentsLevelFindResults();
    void settingsToggleAutoLayout();
    void settingsToggleWriteBackupFile();
    void settingsToggleAnimation();
    void settingsToggleDownloads();
    bool settingsConfluence();
    bool settingsJIRA();

    void focusMapEditor();
    void focusNoteEditor();
    void toggleNoteEditor();
    void toggleTreeEditors();
    void switchEditors();
    void setTreeEditorsVisibility(bool);
    void focusTaskEditor();
    void toggleTaskEditor();
    void toggleSlideEditors();
    void setSlideEditorsVisibility(bool);
    void focusScriptEditor();
    void toggleScriptEditor();
    void focusScriptOutput();
    void toggleScriptOutput();
    void focusHistory();
    void toggleHistory();
    void focusProperty();
    void toggleProperty();
    void focusHeadingEditor();
    void toggleHeadingEditor();
    void updateHistory(SimpleSettings &);
    void toggleAntiAlias();
    bool isAliased();
    bool hasSmoothPixmapTransform();
    void toggleSmoothPixmap();
    void clearScriptOutput();
    void updateHeading(const VymText &vt);
    void updateNoteText(const VymText &vt);
    void updateNoteEditor(TreeItem *ti);
    void updateHeadingEditor(TreeItem *ti = nullptr);
    void selectInNoteEditor(QString s, int i);
    void setFocusMapEditor();
    void changeSelection(VymModel *model, const QItemSelection &newSel,
                         const QItemSelection &delSel);
    void updateDockWidgetTitles(VymModel *model);

    void updateActions();
    ModMode getModMode();
    bool autoSelectNewBranch();

    void scriptPrint(const QString &, const QString &color = "");
    QVariant runScript(const QString &);
    QVariant runScriptWithMacros(const QString &);
    void abortScript(const QJSValue::ErrorType &err, const QString &msg);
    void abortScript(const QString &msg);
    QVariant setScriptResult(const QVariant &r);

  private:
    QJSEngine *scriptEngine;
    QVariant scriptResult;

  public slots:
    QObject *getCurrentModelWrapper();
    bool gotoWindow(const int &n);

  private slots:
    void nextEditor();
    void previousEditor();
    void nextSlide();
    void previousSlide();

    void flagChanged();

    void testFunction1();
    void testFunction2();
    void toggleWinter();
    void toggleHideExport();
    void testCommand();

    void helpDoc();
    void helpDemo();
    void helpShortcuts();
    void helpMacros();
  public:
    QString scriptingCommands();
  private slots:
    void helpScriptingCommands();
    void helpDebugInfo();
    void helpAbout();
    void helpAboutQT();
    void callMacro();
    void downloadReleaseNotesFinished();

  private:
    QUrl serverUrl(const QString &scriptName);
    bool checkUpdatesAfterReleaseNotes;

  public:
    void checkReleaseNotesAndUpdates();

  public slots:
    void checkReleaseNotes();
    bool downloadsEnabled(bool userTriggered = false);
    void downloadUpdatesFinished(bool userTriggered = false);
    void downloadUpdatesFinishedInt();
    void downloadUpdates(bool userTriggered);
    void checkUpdates();
    void escapePressed();
    void togglePresentationMode();
    void toggleHideTmpMode();

  private:
    QString shortcutScope; //! For listing shortcuts
    QTabWidget *tabWidget;

    QStringList imageTypes;

    QUuid prevSelection;

    HistoryWindow *historyWindow;

    QDockWidget *headingEditorDW;
    QDockWidget *noteEditorDW;
    QDockWidget *scriptEditorDW;
    QDockWidget *branchPropertyEditorDW;

  public:
    QList<QAction *>
        mapEditorActions;   //! allows mapEditor to clone actions and shortcuts
    QList<QAction *>
        taskEditorActions;  //! allows taskEditor to clone actions and shortcuts
    QList<QAction *>
        vimActions;         //! Actions with vim inspired shortcuts
  private:
    QList<QAction *>
        restrictedMapActions; //! Actions reqire map and write access
    QList<QAction *>
        unrestrictedMapActions;       //! Actions require map, but work also in
                                      //! readonly, e.g. print, copy
    QList<QAction *> actionListFiles; //! File related actions, e.g. load, save,
                                      //! restore session
    QList<QAction *> actionListBranches;
    QList<QAction *> actionListImages;
    QList<QAction *> actionListItems;

    int xLinkMenuWidth;

    QMenu *recentFilesMenu;
    enum { MaxRecentFiles = 20 };
    QAction *recentFileActions[MaxRecentFiles];
    QAction *actionRecentFilesClear;

    QAction *macroActions[48];
    QStringList macro;

    QList <QColor> quickColors;

    QMenu *toolbarsMenu;
    QMenu *toggleWindowsMenu;
    QMenu *focusWindowsMenu;

    QMenu *branchAddContextMenu;
    QMenu *branchGeometryContextMenu;
    QMenu *branchHierarchyContextMenu;
    QMenu *branchLinksContextMenu;
    QMenu *branchRemoveContextMenu;
    QMenu *branchXLinksContextMenuEdit;
    QMenu *branchXLinksContextMenuFollow;   // Can also have Urls and VymLinks since 2.9.592
    QMenu *targetsContextMenu;
    QMenu *fileLastMapsMenu;
    QMenu *fileImportMenu;
    QMenu *fileExportMenu;

    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *selectMenu;
    QMenu *formatMenu;
    QMenu *viewMenu;
    QMenu *connectMenu;

    QToolBar *fileToolbar;
    QToolBar *clipboardToolbar;
    QToolBar *editActionsToolbar;
    QToolBar *selectionToolbar;
    QToolBar *editorsToolbar;
    QToolBar *colorsToolbar;
    QToolBar *viewTransformationsToolbar;
    QToolBar *limitedViewToolbar;
    QToolBar *modModesToolbar;
    QToolBar *referencesToolbar;
    QToolBar *standardFlagsToolbar;
    QToolBar *userFlagsToolbar;

    bool presentationMode;
    QMap<QToolBar *, bool>
        toolbarStates; // Save visibilty of toolbars during presentation mode

    QAction *actionFileNew;
    QAction *actionFileNewCopy;
    QAction *actionFileOpen;
    QAction *actionFileClose;
    QAction *actionFileRestoreSession;
    QAction *actionFileSave;
    QAction *actionFilePrint;
    QAction *actionFileExitVym;
    QAction *actionClearRecent;
    QAction *actionMapProperties;
    QAction *actionFileExportLast;
    QAction *actionFileExportConfluence;
    QAction *actionUndo;
    QAction *actionUndoVim;
    QAction *actionRedo;
    QAction *actionRepeatCommand;
    QAction *actionCopy;
    QAction *actionCopyVim;
    QAction *actionCut;
    QAction *actionPaste;
    QAction *actionPasteVim;
    QAction *actionMoveUp;
    QAction *actionMoveDown;
    QAction *actionMoveDownDiagonally;
    QAction *actionMoveUpDiagonally;
    QAction *actionDetach;
    QAction *actionSortChildren;
    QAction *actionSortBackChildren;
    QAction *actionToggleScroll;
    QAction *actionUnscrollSubtree;
    QAction *actionExpandAll;
    QAction *actionExpandOneLevel;
    QAction *actionCollapseOneLevel;
    QAction *actionCollapseUnselected;
    QAction *actionOpenUrl;
    QAction *actionOpenMultipleVisUrls;
    QAction *actionOpenMultipleUrls;
    QAction *actionOpenMultipleUrlsPrivate;
    QAction *actionGetURLsFromNote;
    QAction *actionURLNew;
    QAction *actionLocalURL;
    QAction *actionHeading2URL;
    QAction *actionGetJiraDataSubtree;
    QAction *actionSetJiraQuery;
    QAction *actionGetConfluencePageDetails;
    QAction *actionGetConfluencePageDetailsRecursively;
    QAction *actionOpenVymLink;
    QAction *actionOpenVymLinkBackground;
    QAction *actionOpenMultipleVymLinks;
    QAction *actionEditVymLink;
    QAction *actionDeleteVymLink;
    QAction *actionAddTimestamp;
    QAction *actionToggleTask;
    QAction *actionTogglePresentationMode;
    QAction *actionToggleHideTmpMode;
    QAction *actionCycleTaskStatus;
    QAction *actionTaskResetDeltaPrio;
    QAction *actionTaskSleep0;
    QAction *actionTaskSleepN;
    QAction *actionTaskSleep1;
    QAction *actionTaskSleep2;
    QAction *actionTaskSleep3;
    QAction *actionTaskSleep4;
    QAction *actionTaskSleep5;
    QAction *actionTaskSleep7;
    QAction *actionTaskSleep14;
    QAction *actionTaskSleep28;
    QAction *actionToggleHideExport;
    QAction *actionMapInfo;
    QAction *actionHeading;
    QAction *actionDelete;
    QAction *actionCutVim;

  public:
    QAction *actionAddMapCenter;
    QAction *actionAddClone;

  private:
    QAction *actionAddBranch;
    QAction *actionAddBranchBefore;
    QAction *actionAddBranchAbove;
    QAction *actionAddBranchBelow;
    QAction *actionDeleteKeepChildren;
    QAction *actionDeleteChildren;
    QAction *actionImportAdd;
    QAction *actionImportReplace;
    QAction *actionSaveSelection;
    QAction *actionLoadImage;

    QAction *actionGrowSelectionSize;
    QAction *actionShrinkSelectionSize;
    QAction *actionResetSelectionSize;

    QAction *actionRotateSubtreeCW;
    QAction *actionRotateSubtreeCCW;

    QAction *actionToggleTarget;
    QAction *actionGoToTargetLinkedMap;
    QAction *actionGoToTarget;
    QAction *actionMoveToTarget;
    QAction *actionSelectFirstSibling;
    QAction *actionSelectFirstSiblingVim;
    QAction *actionSelectLastSibling;
    QAction *actionSelectLastSiblingVim;
    QAction *actionSelectPrevious;
    QAction *actionSelectNext;
    QAction *actionSelectNothing;
    QAction *actionFind;
    QAction *actionFindVim;
    QAction *actionFollowReference;

    QActionGroup *actionGroupQuickColors;
    QAction *actionFormatQuickColor;
    QAction *actionFormatPickColor;
    QAction *actionFormatColorBranch;
    QAction *actionFormatColorSubtree;
    QAction *actionFormatLinkColorHint;
    QAction *actionFormatBackground;
    QAction *actionFormatLinkColor;
    QAction *actionFormatSelectionColor;
    QAction *actionFormatFont;

    QAction *actionZoomIn;
    QAction *actionZoomOut;
    QAction *actionZoomReset;
    QAction *actionRotateCounterClockwise;
    QAction *actionRotateClockwise;
    QAction *actionCenterOn;
    QAction *actionCenterOnScaled;
    QAction *actionCenterOnRotated;

    QActionGroup *actionGroupModModes;
    QAction *actionModModePoint;
    QAction *actionModModeColor;
    QAction *actionModModeCopy;
    QAction *actionModModeXLink;
    QAction *actionModModeMoveObject;
    QAction *actionModModeMoveView;

    QAction *actionToggleHideMode;

    QAction *actionToggleWinter;

    QActionGroup *actionGroupFormatFrameTypes;

    QActionGroup *actionGroupFormatLinkStyles;
    QAction *actionFormatLinkStyleLine;
    QAction *actionFormatLinkStyleParabel;
    QAction *actionFormatLinkStylePolyLine;
    QAction *actionFormatLinkStylePolyParabel;
    QAction *actionFormatHideLinkUnselected;

    QAction *actionViewFocusNoteEditor;
    QAction *actionViewToggleNoteEditor;

    QAction *actionViewFocusHeadingEditor;
    QAction *actionViewToggleHeadingEditor;

    QAction *actionViewFocusTaskEditor;
    QAction *actionViewToggleTaskEditor;

    QAction *actionViewSwitchEditors;
    QAction *actionViewToggleTreeEditors;
    QAction *actionViewToggleSlideEditors;

    QAction *actionViewFocusScriptEditor;
    QAction *actionViewToggleScriptEditor;

    QAction *actionViewFocusScriptOutput;
    QAction *actionViewToggleScriptOutput;

    QAction *actionViewFocusHistoryWindow;
    QAction *actionViewToggleHistoryWindow;

    QAction *actionViewFocusPropertyEditor;
    QAction *actionViewTogglePropertyEditor;

    QAction *actionViewToggleAntiAlias;
    QAction *actionViewToggleSmoothPixmapTransform;
    QAction *actionViewCenter;

    QAction *actionConnectGetConfluenceUser;
    QAction *actionSettingsAutoSelectNewBranch;
    QAction *actionSettingsUseFlagGroups;
    QAction *actionSettingsUseHideExport;
    QAction *actionSettingsToggleAutosave;
    QAction *actionSettingsAutosaveTime;
    QAction *actionSettingsDarkTheme;
    QAction *actionSettingsShowParentsLevelTasks;
    QAction *actionSettingsShowParentsLevelFindResults;
    QAction *actionSettingsToggleAutoLayout;
    QAction *actionSettingsWriteBackupFile;
    QAction *actionSettingsToggleDownloads;
    QAction *actionSettingsUseAnimation;
    QAction *actionSettingsJIRA;
    QAction *actionSettingsConfluence;
};

#endif
