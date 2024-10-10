#ifndef VYMMODEL_H
#define VYMMODEL_H

#include <QtNetwork>

#include <QPointF>
#include <QTextCursor>

#if defined(VYM_DBUS)
#include "adaptormodel.h"
#endif

#include "file.h"
#include "flagrow-master.h"
#include "frame-container.h"
#include "imageitem.h"
#include "jira-issue.h"
#include "linkobj.h"
#include "mapdesign.h"
#include "settings.h"
#include "treemodel.h"
#include "vymlock.h"

class AttributeItem;
class BranchItem;
class FindResultModel;
class MapEditor;
class SlideItem;
class SlideModel;
class Task;
class XLink;
class VymView;
class VymModelWrapper;
class ZipAgent;

class QGraphicsScene;
class QJsonObject;

typedef QMap<uint, QStringList> ItemList;

class VymModel : public TreeModel {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.vym.VymModel-h")

    friend class XLinkItem;

    ////////////////////////////////////////////
    // General housekeeping
    ////////////////////////////////////////////
  private:
    QString mapVersionInt; //!< version string saved in vym file
    QString title;
    QString author;
    QString comment;
    QDate date;

    static uint idLast; //! the last used unique ID
    uint modelIdInt;
    VymModelWrapper *wrapper;

  public:
    VymModel();
    ~VymModel();
    void clear();
    void init();
    void
    makeTmpDirectories(); //!< create temporary directories e.g. for history
    QString tmpDirPath(); //!< Return path to temporary directory
    QString zipDirPath(); //!< Return path to zip directory

    MapEditor *getMapEditor();
    uint modelId(); //! Return unique ID of model
    VymModelWrapper *getWrapper();

    void setView(VymView *); //! Set vymView for resizing editors after load
  private:
    VymView *vymView;

  public:
    bool isRepositionBlocked(); //!< While load or undo there is no need to
                                //!< update graphicsview
    void updateActions();       //!< Update buttons in mainwindow

    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    ////////////////////////////////////////////
    // Load/save
    ////////////////////////////////////////////
  private:
    bool zipped;       // should map be zipped
    static int mapNum; // unique number for model used in save/undo
    File::FileType fileType; // type of file, e.g. vym, freemind...
    QString fileName;  // short name of file (for tab)
                      // example.vym

    QString filePath; // path to file which will be saved
                      // /home/tux/example.vym

    QString fileDir; // dir where file is saved
                     // /home/tux/

    QString destPath; // path to .vym file (needed for vymlinks)
                      // /home/tux/example.vym

    QString mapName; // fileName without ".vym"
                     // example

    QString tmpMapDirPath;  // tmp directory with undo history
    QDir zipDirInt;         // dir used for compressing the map

    QTimer *autosaveTimer;
    QTimer *fileChangedTimer;
    QDateTime fileChangedTime;

  public:
    void resetUsedFlags(); //! Before exports or saving, reset the flags

    /*! This function saves all information of the map to disc.
    saveToDir also calls the functions for all BranchObj and other objects in
    the map. The structure of the map itself is returned as QString and passed
    back to Main, where saveToDir is called initially
    */
    QString saveToDir(const QString &tmpdir, const QString &prefix,
                      FlagRowMaster::WriteMode flagMode, const QPointF &offset,
                      TreeItem *saveSel);

    /*! Save all data in tree*/
    QString
    saveTreeToDir(const QString &, const QString &, const QPointF &,
                  QList<XLink *> &tmpXLinks); // Save data recursivly to tempdir

    /*! \brief Sets filepath, filename and mapname

     If the filepath is "/home/tux/map.xml", then the filename will be set
     to map.xml. The destname is needed for vymLinks, pointing to another map.
     The destname holds the real name of the file, after it has been compressed,
     e.g. "map.vym"
    */

    /*! \brief Set File path

     The destname is needed to construct the references between maps
    */
    void setFilePath(QString filepath, QString destname);
    void setFilePath(QString); //!< Overloaded for convenience
    QString getFilePath();     //!< Full path e.g. "/home/tux/map.xml"
    QString getFileDir();      //!< e.g. "/home/tux"
    QString getFileName();     //!< e.g. "map.xml"
    QString getMapName();      //!< e.g. "map"
    QString getDestPath();     //!< e.g. "/home/tux/map.vym"

    /*! \brief Load map

    The data is read from file. Depending on LoadMode the current
    selection gets replaced by data or the data is appended.
    */
    File::ErrorCode
    loadMap(QString, //!< Path
            const File::LoadMode &lmode =
                File::NewMap, //!< New map, replace or add to selection
            const File::FileType &ftype = File::VymMap, //!< VymMap or FreeMind
            const int &contentFilter =
                0x0000,  //!< For undo types of content can be filterd
            BranchItem *bi = nullptr,   //!< Branch to replace or add to
            int insertPos = -1          //!< Optionally tell position where to add data
    );

  public:
    /*! \brief Save the map to file */
    void saveMap(const File::SaveMode &);
    bool isSaving();

  private slots:
    void zipFinished ();

  private:


    ZipAgent *zipAgent;
    bool isSavingInt;

  public:
    ImageItem* loadImage(
            BranchItem *dst = nullptr, 
            const QStringList &imagePaths = QStringList());
    ImageItem* loadImage(
            BranchItem *dst = nullptr, 
            const QString &imagePath = QString());
    void saveImage(ImageItem *ii = nullptr, QString fn = "");

  private:
    void importDirInt(QDir d, BranchItem *dst);

  public:
    void importDir(const QString &, BranchItem *bi = nullptr);
    void importDir();
    bool addMapInsert(QString filepath, int pos = -1, BranchItem *bi = nullptr);
    bool addMapReplace(QString filepath, BranchItem *bi = nullptr);

  private:
    bool removeVymLock();

  public:
    bool tryVymLock();
    bool renameMap(const QString &newPath); //! Rename map and change lockfile
    void setReadOnly(bool b);
    bool isReadOnly();

  private:
    VymLock vymLock; //! Handle lockfiles and related information
    bool readonly;   //! if map is locked, it can be opened readonly

  private slots:
    void autosave();
    void fileChanged();

    ////////////////////////////////////////////
    // history (undo/redo)
    ////////////////////////////////////////////
  private:
    bool mapDefault; //!< Flag if map is untouched
    bool mapChanged; //!< Flag if undo is possible
    bool mapUnsaved; //!< Flag if map should be saved

    QString histPath;       //!< Path to history file
    SimpleSettings undoSet; //!< undo/redo commands, saved in histPath
    QString undoScriptComment;//!< Comment for a set of undo commands in history
    QString undoScript;      //!< undo commands in saveStateBeginScript, including select statements
    QString redoScript;      //!< redo commands in saveStateBeginScript, including select statements
    bool buildingUndoScript; //!< true, while a set of undo commands is built

    int stepsTotal;         //!< total number of steps (undos+redos)
    int curStep;            //!< Current step in history (ring buffer)
    int curClipboard;       //!< number of history step, which is the current
                            //!< clipboard
    uint dataStep;          //!< Step of saved data
    int redosAvail;         //!< Available number of redo steps
    int undosAvail;         //!< Available number of undo steps
    bool repositionBlocked; //!< block while load or undo
    bool saveStateBlocked;  //!< block saving current state

  public:   // FIXME-3 much of below should be private!
    void blockReposition();   //! Block reposition while bigger changes, e.g. an import
    void unblockReposition(); //! Unblock reposition and do repositon
    bool isDefault();   //!< true, if map is still the empty default map
    void makeDefault(); //!< Reset changelog, declare this as default map
    bool hasChanged();  //!< true, if something has changed and is not saved yet
    void setChanged();  //!< called from TextEditor via LinkableMapObj

    /*! \brief Get name of object

      Returns heading of a branch or name of an object for use in comment
      of undo/redo history
    */
    QString getObjectName(TreeItem *);

    void redo();            //!< Redo last action
    bool isRedoAvailable(); //!< True, if redo is available
    QString lastRedoSelection();
    QString lastRedoCommand();
    QVariant repeatLastCommand(); //!< Repeat last command on current selection

    void undo();               //!< Undo last action
    bool isUndoAvailable();    //!< True, if undo is available
    void gotoHistoryStep(int); //!< Goto a specifig step in history

    QString getHistoryPath(); //!< Path to directory containing the history
    void resetHistory();      //!< Initialize history

    QString setAttributeVar(AttributeItem*, QString varName ="b");  //!< Returns command to set AttributeItem in scripts for undo/redo
    QString setBranchVar(BranchItem*, QString varName ="b");  //!< Returns command to set BranchItem in scripts for undo/redo
    QString setImageVar(ImageItem*, QString varName = "i");   //!< Returns command to set ImageItem in scripts for undo/redo
    QString setXLinkVar(XLink*, QString varName = "x");        //!< Returns command to set XLink in scripts for undo/redo

  private:
    /*! \brief Save the current changes in map

    Two commands and selections are saved:

        - undocommand and undoselection to undo the change
        - redocommand and redoselection to redo the action after an undo

    Additionally a comment is logged.

    */
    void saveStateNew(
                   const QString &undoCommand,
                   const QString &redoCommand,
                   const QString &comment = "",
                   TreeItem *saveUndoItem = nullptr,
                   TreeItem *saveRedoItem = nullptr);

  public:
    /*! Save branch using BranchWrapper.
     *  public: Called from MapEditor::mouseReleased() */
    void saveStateBranch(
            BranchItem *bi,
            const QString &undoCommand,
            const QString &redoCommand,
            const QString &comment);

    /*! Put several states into one Script for a single undo step */
    void saveStateBeginScript(const QString &comment);
    void saveStateEndScript();

    ////////////////////////////////////////////
    // unsorted so far
    ////////////////////////////////////////////
  public:
    QGraphicsScene *getScene();

    TreeItem *findBySelectString(QString s);
    TreeItem *findID(const uint &i);    // find MapObj by unique ID
    TreeItem *findUuid(const QUuid &i); // find MapObj by unique ID
    BranchItem* findBranchByAttribute(const QString &key, const QString &value);

    void test();

    ////////////////////////////////////////////
    // Interface
    ////////////////////////////////////////////
  public:
    void setTitle(const QString &);
    QString getTitle();
    void setAuthor(const QString &);
    QString getAuthor();
    void setComment(const QString &);
    QString getComment();
    void setMapVersion(const QString &);
    QString mapVersion();
    QString getDate();
    int branchCount();
    int centerCount();

    void setSortFilter(const QString &);
    QString getSortFilter();

  protected:
    QString sortFilter;
  signals:
    void sortFilterChanged(QString); //!< Notify editors of new filter

  public:
    void setHeading(const VymText &vt,
                    TreeItem *ti = nullptr); //!< Set heading of item
    void setHeadingPlainText(const QString &s,
                             TreeItem *ti = nullptr); //!< Set heading of item
    Heading getHeading();               //!< Get heading of item
    QString headingText(TreeItem*);     //!< For debugging. Also works for nullptr
    void updateNoteText(const VymText &); //!< Signal emmited in NoteEditor via MainWindow
    void setNote(const VymNote &vn,     //!< Set note text
                         BranchItem *bi = nullptr,
                         bool senderIsNoteEditor = false);
    VymNote getNote();                  //!< Get note text
    bool hasRichTextNote();             //!< Check type of vymText used
    bool loadNote(const QString &fn, BranchItem *bi = nullptr);   //!< Load note from file
    bool saveNote(const QString &fn);   //!< Save note to file

  private:
    BranchItem *findCurrent;  // next object in find process
    BranchItem *findPrevious; // next object in find process
    bool EOFind;              // true, if search failed

  public:
    void findDuplicateURLs();       // find duplicate URLs, testing only so far
    bool findAll(FindResultModel *, // Search all objects at once, also notes
                 QString s, Qt::CaseSensitivity cs = Qt::CaseInsensitive,
                 bool searchNotes = true);

  private:
    QString findString;

  public:
    void setUrl(QString url, bool updateFromCloud = true, BranchItem *bi = nullptr);
    QString getUrl(); // returns URL of selection or ""
    QStringList getUrls(bool ignoreScrolled = true); // returns URLs of subtree
    void setJiraQuery(const QString &, BranchItem *bi = nullptr);

    void setFrameAutoDesign(const bool &useInnerFrame, const bool &);
    void setFrameType(const bool &useInnerFrame, const FrameContainer::FrameType &, BranchItem *bi = nullptr);
    void setFrameType(const bool &useInnerFrame, const QString &, BranchItem *bi = nullptr);
    void setFramePenColor(const bool &useInnerFrame, const QColor &, BranchItem *bi = nullptr);
    void setFrameBrushColor(const bool &useInnerFrame, const QColor &, BranchItem *bi = nullptr);
    void setFramePadding(const bool &useInnerFrame, const int &, BranchItem *bi = nullptr);
    void setFramePenWidth(const bool &useInnerFrame, const int &, BranchItem *bi = nullptr);
    void setBranchesLayout(const QString &, BranchItem *bi = nullptr);
    void setImagesLayout(const QString &, BranchItem *bi = nullptr);
    void setHeadingColumnWidthAutoDesign(const bool &, BranchItem *bi = nullptr);
    void setHeadingColumnWidth(const int &, BranchItem *bi = nullptr);
    void setRotationsAutoDesign(const bool &);
    void setRotationHeading(const int &);
    void setRotationSubtree(const int &);
    void setScaleAutoDesign(const bool &);
    void setScaleHeading(const qreal &, const bool relative = false);
    qreal getScaleHeading();
    void setScaleSubtree(const qreal &);
    qreal getScaleSubtree();
    void setScaleImage(const qreal &, const bool relative = false, ImageItem *ii = nullptr);
    void setScale(const qreal &f, const bool relative);
    void growSelectionSize();
    void shrinkSelectionSize();
    void resetSelectionSize();
    void setHideLinkUnselected(bool);

    /*! Should object be hidden in exports (clouded)? */
    void setHideExport(bool, TreeItem *ti = nullptr);

    /*! Should object be hidden in exports (clouded)? */
    void toggleHideExport();

    /*! Toggle task for branch */
    void toggleTask(BranchItem *bi = nullptr);

    /*! Cycle through task states */
    bool cycleTaskStatus(BranchItem *bi = nullptr, bool reverse = false);

    /*! Set task to sleep for number of days or until a given date*/
    bool setTaskSleep(const QString &s, BranchItem *bi = nullptr);

    /*! Set manual delta for priority of task */
    void setTaskPriorityDelta(const int &n, BranchItem *bi = nullptr);

    /*! Get manual delta for priority of task */
    int getTaskPriorityDelta(BranchItem *bi = nullptr);

    /*! count tasks in this model */
    int taskCount();

    /*! Update task priorities */
  private slots:
    void updateTasksAlarm(bool force = false);

  private:
    /*! Timer to check if tasks need to be awoken */
    QTimer *taskAlarmTimer;

  public:
    BranchItem *addTimestamp();

    void copy();  //!< Copy to clipboard
    void paste(); //!< Paste clipboard to branch and backup
    void cut();   //!< Cut to clipboard (and copy)

    bool canMoveUp(TreeItem *bi);
    bool canMoveDown(TreeItem *bi);
    void moveUp(TreeItem *ti = nullptr); //!< Move branch or image up
    void moveDown(TreeItem *ti = nullptr);               //!< Move branch or image down
    void moveUpDiagonally();       //!< Move branch up diagonally: Branchs becomes child of branch above
    void moveDownDiagonally();     //!< Move branch down diagonally: Branchs becomes sibling of parent
    void detach(BranchItem* bi = nullptr);   //!< Detach branch and use as new mapcenter
  private:
    QList <BranchItem*> sortBranchesByNum(QList <BranchItem*>, bool inverse = false);
    QList <BranchItem*> sortBranchesByHeading(QList <BranchItem*>, bool inverse = false);
  public:
    void sortChildren(bool inverse = false, BranchItem *bi = nullptr); //!< Sort children lexically
    QList <ImageItem*> sortImagesByNum(QList <ImageItem*>, bool inverse = false);

    // The create methods are used to quickly parse a XML file
    BranchItem *createBranchWhileLoading( //!< Create Branch. Append for  insPos < 0
            BranchItem *dst,
            int insertPos = -1);
    ImageItem *createImage(BranchItem *dst);   //!< Create image

  public:
    bool createXLink(XLink *l); //!< Create XLink, will create MO automatically if needed
    QColor getXLinkColor();
    int getXLinkWidth();
    Qt::PenStyle getXLinkStyle();
    QString getXLinkStyleBegin();
    QString getXLinkStyleEnd();

    AttributeItem* setAttribute(
            BranchItem *dst,
            const QString &k,
            const QVariant &v,
            bool removeIfEmpty = true);
    void deleteAttribute(BranchItem *dst, const QString &k);
    AttributeItem* getAttributeByKey(const QString &key, TreeItem *ti = nullptr);

    //! \brief Add new mapcenter
    BranchItem *addMapCenter(bool interactive = false);
    BranchItem *addMapCenterAtPos(QPointF absPos, bool interactive = false);

    /*! \brief Add new branch

    Depending on num the new branch is created

    -3 above selection as child of selections parent
    -2 as child of selection
    -1 below selection as child of selections parent
    0..n	insert at a specific position in selections parent
    (needed for free relinking)
    */

  private:
    BranchItem *addNewBranchInt(BranchItem *dst, int pos);
        // pos allows to add above/below
        // selection, or as child  at pos
  public:
    /*! \Add new branch

    // Depending on pos:
    // -3	insert in children of parent  above selection
    // -2	add branch to selection
    // -1	insert in children of parent below selection
    // 0..n	insert in children of parent at pos
    */
    BranchItem *addNewBranch(BranchItem *bi = nullptr, int pos = -2, bool interactive = false);
    BranchItem *
    addNewBranchBefore(BranchItem *bi = nullptr, bool interactive = false); //!< Insert branch between selection and its parent

    /*! \brief Relink a branch to a new destination dst
        Relinks branch to dst at branch position pos. There is no saveState
        here, as for example moveUp or moving in MapEditor have
        different needs to call saveState
        Returns true if relinking was successful.
    */
    bool relinkBranch(BranchItem *branch, BranchItem *dst, int num_new = -1);
    bool relinkBranches(QList <BranchItem*> branches, BranchItem *dst, int num_new = -1);
    bool relinkImage(ImageItem *image, TreeItem *dst, int num_new = -1);
    bool relinkImages(QList <ImageItem*> images, TreeItem *dst, int num_new = -1);

    bool relinkTo(const QString &dest, int num);

  public:
    void deleteSelection(ulong selID = 0); //!< Delete selection
    void deleteKeepChildren(BranchItem *bi = nullptr); //!< remove branch, but keep children
  public:
    void deleteChildren(BranchItem *bi = nullptr);  //!< Delete all children, including attributes
    void deleteChildrenBranches(BranchItem *bi = nullptr);  //!< Delete only branches

    TreeItem *deleteItem(
        TreeItem *); //!< Delete item and return parent (if parent!= rootItem)

    void deleteXLink(XLink *); //!< Delete XLink and saveState

  private:
    void deleteXLinkInt(XLink *); //!< Delete XLink internal

  public:
    bool scrollBranch(BranchItem *);
    bool unscrollBranch(BranchItem *);
    void toggleScroll(BranchItem *bi = nullptr);
    void unscrollSubtree(BranchItem *bi = nullptr);
    void emitExpandAll();
    void emitExpandOneLevel();
    void emitCollapseOneLevel();
    void emitCollapseUnselected();
  signals:
    void expandAll();
    void expandOneLevel();
    void collapseOneLevel();
    void collapseUnselected();

  public:
    void toggleTarget();
    ItemList getLinkedMaps();
    ItemList getTargets();

  private:
    Flag* findFlagByName(const QString &name);
  public:
    void setFlagByName(const QString &name, BranchItem *bi = nullptr, bool useGroups = true);
    void unsetFlagByName(const QString &name, BranchItem *bi = nullptr);
    void toggleFlagByName(const QString &name, BranchItem *bi = nullptr, bool useGroups = true);
    void toggleFlagByUid(const QUuid &uid, BranchItem *bi = nullptr, bool useGroups = true);
    void clearFlags(BranchItem *bi = nullptr);

    void colorBranch(QColor, BranchItem *bi = nullptr);
    void colorSubtree(QColor, BranchItem *bi = nullptr);
    QColor getCurrentHeadingColor();

    void note2URLs();                    // get URLs from note
    void editHeading2URL();              // copy heading to URL
    void getJiraData(bool subtree = true, BranchItem *bi = nullptr);

  private:
    void initAttributesFromJiraIssue(BranchItem *bi, const JiraIssue &);
    void updateJiraFlag(TreeItem*);

  public slots:
    void processJiraTicket(QJsonObject);
    void processJiraJqlQuery(QJsonObject);

  public:
    void setHeadingConfluencePageName(); // get page details from Confluence
    void setVymLink(const QString &, BranchItem *bi = nullptr);    // Set vymLink for selection
    void deleteVymLink();                // delete link to another map
    QString getVymLink();                // return path to map
    QStringList getVymLinks();           // return paths in subtree
    void followXLink(int);
    void editXLink();
    void setXLinkColor(const QString &, XLink *xl = nullptr);
    void setXLinkStyle(const QString &, XLink *xl = nullptr);
    void setXLinkStyleBegin(const QString &, XLink *xl = nullptr);
    void setXLinkStyleEnd(const QString &, XLink *xl = nullptr);
    void setXLinkWidth(int, XLink *xl = nullptr);

    ////////////////////////////////////////////
    // Scripting
    ////////////////////////////////////////////
  public:
    /* \brief Runs the script */
    QVariant execute(const QString &script);

    ////////////////////////////////////////////
    // Exports
    ////////////////////////////////////////////
  private:
    TreeItem::HideTmpMode hidemode; // true while exporting to hide some stuff
    bool exportBoundingBoxes = false;

  public:
    /*! Set or unset temporary hiding of objects during export  */
    void setExportMode(bool);

    /*! Save as image. Returns offset to upper left corner of image */
    QPointF exportImage(QString fname = "", bool askForName = true,
                        QString format = "PNG");

    /*! Save as PDF  . Returns offset to upper left corner of image */
    void exportPDF(QString fname = "", bool askForName = true);

    /*! Save as SVG  . Returns offset to upper left corner of image */
    QPointF exportSVG(QString fname = "", bool askForName = true);

    /*! Export as XML to directory */
    void exportXML(QString fname = "", QString dir = "", bool useDialog = true);

    /*! Export as A&O report text to file */
    void exportAO(QString fname = "", bool askForName = true);

    /*! Export as ASCII text to file */
    void exportASCII(const QString &fname = "", bool listTasks = false,
                     bool askForName = true);

    /*! Export as CSV text to file */
    void exportCSV(const QString &fname = "", bool askForName = true);

    /*! Export as Firefox bookmarks to JSON file */
    void exportFirefoxBookmarks(const QString &fname = "", bool askForName = true);

    /*! Export as HTML to directory */
    void exportHTML(const QString &fname = "", const QString &dir = "", 
                    bool useDialog = true);

    /*! Export as HTML to Confluence*/
    void exportConfluence(bool createPage = true, const QString &pageURL = "", 
                    const QString &pageName = "", 
                    bool useDialog = true);

    /*! Export as OpenOfficeOrg presentation */
    void exportImpress(const QString &, const QString &);

    /*! Export agent might set export last command AFTER export
     *  e.g. CreateConfluencePage might turn into UpdateConfluencePage */
    void setExportLastCommand(const QString &cmd);
    void setExportLastDescription(const QString &desc);
    void setExportLastDestination(const QString &displayedDest);

    /*! Returns if Export in recently used format is possible*/
    bool exportLastAvailable(QString &description, QString &command,
                             QString &dest);

    /*! Export in recently used format (saved in map)*/
    void exportLast();

    /*! Export as LaTeX */
    void exportLaTeX(const QString &fname = "", bool useDialog = true);

    /*! Export as Markdown */
    void exportMarkdown(const QString &fname = "", bool useDialog = true);

    /*! Export as OrgMode input for emacs*/
    void exportOrgMode(const QString &fname = "", bool useDialog = true);

    ////////////////////////////////////////////
    // View related
    ////////////////////////////////////////////
  public:
    void registerMapEditor(QWidget *);

    void setMapZoomFactor(const double &);
    void setMapRotation(const double &);
    void setMapAnimDuration(const int &d);
    void setMapAnimCurve(const QEasingCurve &c);
    bool centerOnID(const QString &id);

  private:
    double zoomFactor;
    double mapRotationInt;
    int animDuration;
    QEasingCurve animCurve;

    bool hasContextPos; //!< True, if we have a context pos
    QPointF contextPos; //!< local position during context menu
  public:
    void setContextPos(QPointF); //!< local position during context menu
    void unsetContextPos();      //!< forget local position after context menu

    void reposition(bool force = false);           //!< Call reposition for all MCs and update XLinks
    void repositionXLinks();     //!< Call updateXLink for all xlinks
    void setHideTmpMode(TreeItem::HideTmpMode mode);

    void emitNoteChanged(TreeItem *ti);
    void emitDataChanged(TreeItem *ti);
    void emitUpdateQueries(); //!< tell MainWindow to update find results...
    void emitUpdateLayout();

  signals:
    void updateQueries(VymModel *m);
    void updateLayout();
    void noteChanged(QModelIndex ix);
    void newChildObject(QModelIndex ix);

  private:
    MapEditor *mapEditor;

    ////////////////////////////////////////////
    // MapDesign
    ////////////////////////////////////////////
  public:
    MapDesign* mapDesign();

  private:
    MapDesign* mapDesignInt;
    void applyDesign(
            MapDesign::UpdateMode,
            BranchItem *bi = nullptr);
    void applyDesignRecursively(
            MapDesign::UpdateMode,
            BranchItem *bi = nullptr);

  public:
    void setDefaultFont(const QFont &);
    bool setLinkStyle(const QString &m, int depth = -1);   // Set style of link
    void setDefaultLinkColor(const QColor&);         // default color of links
    void setLinkColorHint(const LinkObj::ColorHint &);
    void toggleLinkColorHint(); // after changing linkStyles

    void setDefXLinkPen(const QPen &p);
    void setDefXLinkStyleBegin(const QString &s);
    void setDefXLinkStyleEnd(const QString &s);

    void setBackgroundColor(QColor);
    bool setBackgroundImage(const QString &);
    void setBackgroundImageName(const QString &);
    void unsetBackgroundImage();
    bool hasBackgroundImage();
    QString backgroundImageName();


    /*!  Set position as QGraphicsItem. Only without parentItem absolute position is used */
    void setPos(const QPointF &p, TreeItem *ti = nullptr);

    ////////////////////////////////////////////
    // Network related
    ////////////////////////////////////////////
  public:
    /*! \brief Networking states

    In Network modus we want to switch of saveState, autosave, ...
    */
    enum NetState {
        Offline, //!< Offline
        Client,  //!< I am the client and connected to server
        Server   //!< I am the server
    };

  private:
    // Network connections **Experimental**
    NetState netstate;     // offline, client, server
    QTcpServer *tcpServer; // Act as server in conference mode (experimental)
    QList<QTcpSocket *> clientList; // List of connected clients
    quint16 sendCounter;            // Increased with every sent command

    QTcpSocket *clientSocket; // socket of this client
    QString server;           // server address of this client
    int port;                 // server port of this client

  protected:
    void sendSelection();

  public:
    void newServer();
    void connectToServer();

  private slots:
    void newClient();
    void sendData(const QString &s);
    void readData();
    void displayNetworkError(QAbstractSocket::SocketError);

  public:
    void downloadImage(const QUrl &url, BranchItem *bi = nullptr);


    ////////////////////////////////////////////
    // Selection related
    ////////////////////////////////////////////
  private:
    QUuid latestAddedItemUuid; // latest added object, reset on setChanged()
    QUuid lastToggledUuid;     // Latest toggled object 
    QList<uint> selectionHistory;
    int currentSelection;
    bool keepSelectionHistory; // If set, selection doesn't change history

  public slots:
    void updateSelection(QItemSelection, QItemSelection); // update selection

  public:
    void setSelectionModel(QItemSelectionModel *); // Set common selectionModel
    QItemSelectionModel *getSelectionModel();

    void setSelectionBlocked(bool);
    bool isSelectionBlocked();

    bool select(const QString &);           //! Select by string
    bool selectUids(QStringList uids);      //! select by unique ID (QUuid)
    bool selectToggle(TreeItem *ti);        //! Toggle select state
    bool selectToggle(const uint &id);      //! Overloaded function to toggle select state
    bool selectToggle(const QUuid &uid);    //! Overloaded function to toggle select state
    bool selectToggle(const QString &selectString); //! Overloaded function to toggle select state
    bool select(TreeItem *ti);              //! Select by pointer to TreeItem
    bool select(const QUuid&);              //! Select by Uuid
    bool select(const QModelIndex &index);  //! Select by ModelIndex
    void select(QList <BranchItem*> selbis);//! Used to restore selections
    void select(QList <TreeItem*> tis);     //! Select list of pointers e.g. when relinking
    void unselectAll();
    void unselect(QItemSelection desel);
    bool reselect();
    bool canSelectPrevious();
    bool selectPrevious();
    bool canSelectNext();
    bool selectNext();
    void resetSelectionHistory();
    void appendSelectionToHistory();
    void emitShowSelection(bool scaled = false, bool rotated = false); //!< Show selection in all views

  signals:
    void showSelection(bool scaled, bool rotated);

  public:
    TreeItem *lastToggledItem();
    bool selectFirstBranch(BranchItem *bi = nullptr);
    bool selectFirstChildBranch(BranchItem *bi = nullptr);
    bool selectLastBranch(BranchItem *bi = nullptr);
    bool selectLastChildBranch(BranchItem *bi = nullptr);
    bool selectLastSelectedBranch();
    bool selectLatestAdded();
    bool selectParent(TreeItem *ti = nullptr);

  public:
    TreeItem::Type selectionType();
    BranchItem *getSelectedBranch(BranchItem *bi = nullptr);
    QList<BranchItem *> getSelectedBranches(TreeItem *ti = nullptr);
    ImageItem *getSelectedImage();
    QList <ImageItem *> getSelectedImages(TreeItem *ti = nullptr);
    Task *getSelectedTask();
    XLinkItem *getSelectedXLinkItem();
    XLink *getSelectedXLink(XLink *xl = nullptr);
    AttributeItem *getSelectedAttribute();
    TreeItem *getSelectedItem(TreeItem *ti = nullptr);
    QList<TreeItem *> getSelectedItems(TreeItem *ti = nullptr);
    QList<TreeItem *> getSelectedItemsReduced();
    QModelIndex getSelectedIndex();
    QList<ulong> getSelectedIDs();
    QStringList getSelectedUUIDs();
    bool isSelected(TreeItem *);
    QString getSelectString();
    QString getSelectString(TreeItem *item);
    QString getSelectString(BranchItem *item);
    QString getSelectString(const uint &i);

  signals:
    void selectionChanged(const QItemSelection &newsel,
                          const QItemSelection &oldsel);

  public:
    void selectMapLinkColor();

  private:
    QItemSelectionModel *selModel;
    QString lastSelectString;
    bool selectionBlocked; //! Used to block changes of selection while editing
                           //! a heading

  public:
    void setSelectionPenColor(QColor);
    QColor getSelectionPenColor();
    void setSelectionPenWidth(qreal);
    qreal getSelectionPenWidth();
    void setSelectionBrushColor(QColor);
    QColor getSelectionBrushColor();

    ////////////////////////////////////////////
    // Iterating and selecting branches
    ////////////////////////////////////////////
  public:
    void newBranchIterator(const QString &itname,
                      BranchItem *bi = nullptr,             //! All branches or only subtree
                      bool deepLevelsFirst = false);        //! Named iterator
    BranchItem* nextBranchIterator(const QString &itname);  //! next branch via iterator
  private:
    QHash <QString, QList <QUuid>> branchIterators;
    QHash <QString, int> branchIteratorsCurrentIndex;

    ////////////////////////////////////////////
    // Slide related
    ////////////////////////////////////////////
  public:
    SlideModel *getSlideModel();
    int slideCount();
    SlideItem *addSlide();
    void deleteSlide(SlideItem *si);
    void deleteSlide(int n);
    void relinkSlide(SlideItem *si, int pos);
    bool moveSlideDown(int n = -1);
    bool moveSlideUp(int n = -1);
    SlideItem *findSlideID(uint id);

  public slots:
    void updateSlideSelection(QItemSelection, QItemSelection);

  private:
    SlideModel *slideModel;
    bool blockSlideSelection;

    ////////////////////////////////////////////
    // Logfile related
    ////////////////////////////////////////////
    void logInfo(const QString &comment, const QString &caller = "");
    void logCommand(const QString &command, const QString &comment, const QString &caller = QString());
};

#endif
