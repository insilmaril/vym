#ifndef VYMMODEL_H
#define VYMMODEL_H

#include <QtNetwork>

#include <QPointF>
#include <QTextCursor>

#if defined(VYM_DBUS)
#include "adaptormodel.h"
#endif

#include "branchitem.h"
#include "file.h"
#include "imageitem.h"
#include "link-container.h"
#include "mapeditor.h"
#include "treeitem.h"
#include "treemodel.h"
#include "vymlock.h"
#include "vymmodelwrapper.h"

class AttributeItem;
class BranchItem;
class FindResultModel;
class Link;
class MapDesign;
class MapEditor;
class SlideItem;
class SlideModel;
class Task;
class XLinkItem;
class VymView;

class QGraphicsScene;
class QJsonObject;

typedef QMap<uint, QStringList> ItemList;

class VymModel : public TreeModel {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.insilmaril.vym.VymModel-h")

    ////////////////////////////////////////////
    // General housekeeping
    ////////////////////////////////////////////
  private:
    QString version; //!< version string saved in vym file
    QString title;
    QString author;
    QString comment;
    QDate date;

    static uint idLast; //! the last used unique ID
    uint modelID;
    VymModelWrapper *wrapper;

  public:
    VymModel();
    ~VymModel();
    void clear();
    void init();
    void
    makeTmpDirectories(); //!< create temporary directories e.g. for history
    QString tmpDirPath(); //!< Return path to temporary directory

    MapEditor *getMapEditor();
    uint getModelID(); //! Return unique ID of model
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
    FileType fileType; // type of file, e.g. vym, freemind...
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

    QString tmpMapDirPath; // tmp directory with undo history

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
                  QList<Link *> &tmpLinks); // Save data recursivly to tempdir

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

    bool parseVymText(const QString &s);

    /*! \brief Load map

    The data is read from file. Depending on LoadMode the current
    selection gets replaced by data or the data is appended.
    */
    File::ErrorCode
    loadMap(QString, //!< Path
            const LoadMode &lmode =
                NewMap, //!< New map, replace or add to selection
            const FileType &ftype = VymMap, //!< VymMap or FreeMind
            const int &contentFilter =
                0x0000,  //!< For undo types of content can be filterd
            int pos = -1 //!< Optionally tell position where to add data
    );

  public:
    /*! \brief Save the map to file */
    File::ErrorCode save(const SaveMode &);

  public:
    void loadImage(BranchItem *dst = NULL, const QString &fn = "");
    void saveImage(ImageItem *ii = NULL, QString fn = "");

  private:
    void importDirInt(BranchItem *, QDir);

  public:
    void importDir(const QString &);
    void importDir();

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
    QString undoBlockComment;//!< Comment for a set of undo commands in history
    QString undoBlock;      //!< undo commands in saveStateBeginBlock, including select statements
    QString redoBlock;      //!< redo commands in saveStateBeginBlock, including select statements
    bool buildingUndoBlock; //!< true, while a set of undo commands is built

    int stepsTotal;         //!< total number of steps (undos+redos)
    int curStep;            //!< Current step in history (ring buffer)
    int curClipboard;       //!< number of history step, which is the current
                            //!< clipboard
    int redosAvail;         //!< Available number of redo steps
    int undosAvail;         //!< Available number of undo steps
    bool repositionBlocked; //!< block while load or undo
    bool saveStateBlocked;  //!< block saving current state
  public:
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

    /*! \brief Save the current changes in map

    Two commands and selections are saved:

        - undocommand and undoselection to undo the change
        - redocommand and redoselection to redo the action after an undo

    Additionally a comment is logged.

    */
    void saveState(const SaveMode &savemode, const QString &undoSelection,
                   const QString &undoCommand, const QString &redoSelection,
                   const QString &redoCommand, const QString &comment = "",
                   TreeItem *saveSelection = nullptr, QString dataXML = "");

    /*! Overloaded for convenience */
    void saveStateChangingPart(TreeItem *undoSelection, TreeItem *redoSelection,
                               const QString &redoCommand,
                               const QString &comment);

    /*! Overloaded for convenience */
    void saveStateRemovingPart(TreeItem *redoSelection, const QString &comment);

    /*! Overloaded for convenience */
    void saveState(TreeItem *undoSelection, const QString &undoCommand,
                   TreeItem *redoSelection, const QString &redoCommand,
                   const QString &comment = "");

    /*! Overloaded for convenience */
    void saveState(const QString &undoSelection, const QString &undoCommand,
                   const QString &redoSelection, const QString &redoCommand,
                   const QString &comment = "");

    /*! Overloaded for convenience */
    void saveState(const QString &undoCommand, const QString &redoCommand,
                   const QString &comment = "");

    /*! Save a change in string and merge
    minor sequential  changes  */
    void saveStateMinimal(TreeItem *undoSelection, const QString &undoCommand,
                          TreeItem *redoSelection, const QString &redoCommand,
                          const QString &comment);

    /*! Save state before loading a map */
    void saveStateBeforeLoad(LoadMode lmode, const QString &fname);

    /*! Put several states into one block for a single undo step */
    void saveStateBeginBlock(const QString &comment);
    void saveStateEndBlock();
    
    ////////////////////////////////////////////
    // unsorted so far
    ////////////////////////////////////////////
  public:
    QGraphicsScene *getScene();

    TreeItem *findBySelectString(QString s);
    TreeItem *findID(const uint &i);    // find MapObj by unique ID
    TreeItem *findUuid(const QUuid &i); // find MapObj by unique ID

    ////////////////////////////////////////////
    // Interface
    ////////////////////////////////////////////
  public:
    void setVersion(const QString &);
    QString getVersion();
    void setTitle(const QString &);
    QString getTitle();
    void setAuthor(const QString &);
    QString getAuthor();
    void setComment(const QString &);
    QString getComment();
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
                    BranchItem *bi = NULL); //!< Set heading of item
    void setHeadingPlainText(const QString &s,
                             BranchItem *bi = NULL); //!< Set heading of item
    Heading getHeading();                            //!< Get heading of item
    void updateNoteText(
        const VymText &); //!< Signal emmited in NoteEditor via MainWindow
    void setNote(const VymNote &vn);  //!< Set note text
    VymNote getNote();                //!< Get note text
    bool hasRichTextNote();           //!< Check type of vymText used
    void loadNote(const QString &fn); //!< Load note from file
    void saveNote(const QString &fn); //!< Save note to file

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
    void setURL(QString url, bool updateFromCloud = true, BranchItem *bi = nullptr);
    QString getURL(); // returns URL of selection or ""
    QStringList getURLs(bool ignoreScrolled = true); // returns URLs of subtree

    void setFrameType(const FrameContainer::FrameType &, BranchItem *bi = nullptr);
    void setFrameType(const QString &);
    void toggleFrameIncludeChildren(BranchItem *bi = nullptr);
    void setFrameIncludeChildren(bool, BranchItem *bi = nullptr);
    void setFramePenColor(const QColor &, BranchItem *bi = nullptr);
    void setFrameBrushColor(const QColor &, BranchItem *bi = nullptr);
    void setFramePadding(const int &, BranchItem *bi = nullptr);
    void setFramePenWidth(const int &, BranchItem *bi = nullptr);
    void setBranchesLayout(const QString &, BranchItem *bi = nullptr);
    void setImagesLayout(const QString &, BranchItem *bi = nullptr);
    void setRotationHeading(const int &);
    void setRotationContent(const int &);
    void setHideLinkUnselected(bool);

    /*! Should object be hidden in exports (clouded)? */
    void setHideExport(bool, TreeItem *ti = NULL);

    /*! Should object be hidden in exports (clouded)? */
    void toggleHideExport();

    /*! Toggle task for branch */
    void toggleTask();

    /*! Cycle through task states */
    bool cycleTaskStatus(bool reverse = false);

    /*! Set task to sleep for number of days or until a given date*/
    bool setTaskSleep(const QString &s);

    /*! Set manual delta for priority of task */
    void setTaskPriorityDelta(const int &n, BranchItem *bi = nullptr);

    /*! Get manual delta for priority of task */
    int getTaskPriorityDelta();

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

    bool moveUp(BranchItem *bi);   //!< Move branch up without saving state
    void moveUp();                 //!< Move branch up with saving state
    bool moveDown(BranchItem *bi); //!< Move branch down without saving state
    void moveDown();               //!< Move branch down
    void moveUpDiagonally();       //!< Move branch up diagonally: Branchs becomes child of branch above
    void moveDownDiagonally();     //!< Move branch down diagonally: Branchs becomes sibling of parent
    void detach();                 //!< Detach branch and use as new mapcenter
    void sortChildren(bool inverse = false); //!< Sort children lexically

    // The create methods are used to quickly parse a XML file
    BranchItem *createMapCenter();             //!< Create MapCenter
    BranchItem *createBranch(BranchItem *dst); //!< Create Branch
    ImageItem *createImage(BranchItem *dst);   //!< Create image

  public:
    bool createLink(
        Link *l); //!< Create XLink, will create MO automatically if needed
    QColor getXLinkColor();
    int getXLinkWidth();
    Qt::PenStyle getXLinkStyle();
    QString getXLinkStyleBegin();
    QString getXLinkStyleEnd();

    AttributeItem *setAttribute();
    AttributeItem *setAttribute(BranchItem *dst, AttributeItem *);

    /*! \brief Add new mapcenter

    Disclaimer: Still experimental, not fully supported yet.
    */
    BranchItem *addMapCenter(bool saveStateFlag = true);
    BranchItem *addMapCenter(QPointF absPos);

    /*! \brief Add new branch

    Depending on num the new branch is created

    -3 above selection as child of selections parent
    -2 as child of selection
    -1 below selection as child of selections parent
    0..n	insert at a specific position in selections parent
    (needed for free relinking)
    */

  private:
    BranchItem *addNewBranchInt(BranchItem *dst,
                                int pos); // pos allows to add above/below
                                          // selection, or as child  at pos
  public:
    /*! \Add new branch

    // Depending on pos:
    // -3	insert in children of parent  above selection
    // -2	add branch to selection
    // -1	insert in children of parent below selection
    // 0..n	insert in children of parent at pos
    */
    BranchItem *addNewBranch(BranchItem *bi = NULL, int pos = -2);
    BranchItem *
    addNewBranchBefore(); //!< Insert branch between selection and its parent

    /*! \brief Relink a branch to a new destination dst
        Relinks branch to dst at branch position pos. There is no saveState
        here, as for example moveUp or moving in MapEditor have
        different needs to call saveState
        Returns true if relinking was successful.
    */
    bool relinkBranch(BranchItem *branch, BranchItem *dst, int num_new = -1,
                      bool updateSelection = false);
    bool relinkImage(ImageItem *image, BranchItem *dst);

    bool relinkTo(const QString &dest, int num);

  private:
    bool cleaningUpLinks; //!< True while cleaning up to avoid recursion
  public:
    void cleanupItems();    //!< Delete orphaned Items
    void deleteLater(uint); //!< Delete later with new beginRemoveRow
    void deleteSelection(); //!< Delete selection
    void deleteKeepChildren(
        bool saveStateFlag = true); //!< remove branch, but keep children
  public:
    void deleteChildren(); //!< keep branch, but remove children

    TreeItem *deleteItem(
        TreeItem *); //!< Delete item and return parent (if parent!= rootItem)
    void deleteLink(Link *); //!< Remove Link and related LinkItems in TreeModel
    void clearItem(TreeItem *ti); //!< Remove all children of TreeItem ti
    bool scrollBranch(BranchItem *);
    bool unscrollBranch(BranchItem *);
    void toggleScroll();
    void unscrollChildren();
    void setScaleFactor(qreal);
    void growSelectionSize();
    void shrinkSelectionSize();
    void resetSelectionSize();
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
    void setFlagByName(const QString &name, bool useGroups = true);
    void unsetFlagByName(const QString &name);
    void toggleFlagByName(const QString &name, bool useGroups = true);
    void toggleFlagByUid(const QUuid &uid, bool useGroups = true);
    void clearFlags();

    void colorBranch(QColor);
    void colorSubtree(QColor, BranchItem *bi = NULL);
    QColor getCurrentHeadingColor();

    void note2URLs();                    // get URLs from note
    void editHeading2URL();              // copy heading to URL
    void getJiraData(bool subtree = true);      // get data from Jira

  public slots:
    void updateJiraData(QJsonObject);

  public:
    void setHeadingConfluencePageName(); // get page details from Confluence
    void setVymLink(const QString &);    // Set vymLink for selection
    void deleteVymLink();                // delete link to another map
    QString getVymLink();                // return path to map
    QStringList getVymLinks();           // return paths in subtree
    void followXLink(int);
    void editXLink();
    void setXLinkColor(const QString &);
    void setXLinkStyle(const QString &);
    void setXLinkStyleBegin(const QString &);
    void setXLinkStyleEnd(const QString &);
    void setXLinkWidth(int);

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
    void registerEditor(QWidget *);
    void unregisterEditor(QWidget *);

    void setMapZoomFactor(const double &);
    void setMapRotationAngle(const double &);
    void setMapAnimDuration(const int &d);
    void setMapAnimCurve(const QEasingCurve &c);
    bool centerOnID(const QString &id);

  private:
    double zoomFactor;
    double rotationAngle;
    int animDuration;
    QEasingCurve animCurve;

    bool hasContextPos; //!< True, if we have a context pos
    QPointF contextPos; //!< local position during context menu
  public:
    void setContextPos(QPointF); //!< local position during context menu
    void unsetContextPos();      //!< forget local position after context menu

    void reposition(); //!< Call reposition for all MCOs
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
    MapDesign* getMapDesign();
  private:
    MapDesign* mapDesign;

    QColor defLinkColor;        // default color for links
    QPen defXLinkPen;           // default pen for xlinks
    QString defXLinkStyleBegin; // default style begin
    QString defXLinkStyleEnd;
                                             // default style end
    LinkContainer::ColorHint linkColorHint; // use heading color or own color
    LinkContainer::Style linkstyle;         // default style for links
    QFont defaultFont;

  public:
    bool setMapLinkStyle(const QString &);   // Set style of link
    LinkContainer::Style getMapLinkStyle();
    void setMapDefLinkColor(QColor);         // default color of links
    void setMapLinkColorHintInt();           // color of links
    void setMapLinkColorHint(LinkContainer::ColorHint); // color of links
    void toggleMapLinkColorHint(); // after changing linkStyles
    void selectMapBackgroundImage();
    void setMapBackgroundImage(const QString &);
    void selectMapBackgroundColor();
    void setMapBackgroundColor(QColor);
    QColor getMapBackgroundColor();

    QFont getMapDefaultFont();
    void setMapDefaultFont(const QFont &);

    LinkContainer::ColorHint getMapLinkColorHint();
    QColor getMapDefLinkColor();
    void setMapDefXLinkPen(const QPen &p);
    QPen getMapDefXLinkPen();

    void setMapDefXLinkStyleBegin(const QString &s);
    QString getMapDefXLinkStyleBegin();
    void setMapDefXLinkStyleEnd(const QString &s);
    QString getMapDefXLinkStyleEnd();

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
    void downloadImage(const QUrl &url, BranchItem *bi = NULL);

    ////////////////////////////////////////////
    // Selection related
    ////////////////////////////////////////////
  private:
    TreeItem *latestAddedItem; // latest added object, reset on setChanged()
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

    bool select(const QString &);          //! Select by string
    bool selectID(const QString &);        //! select by unique ID (QUuid)
    bool selectToggle(TreeItem *ti);       //! Toggle select state
    bool selectToggle(const QString &selectString); //! Overloaded function to toggle select state
    bool select(TreeItem *ti);             //! Select by pointer to TreeItem
    bool select(const QModelIndex &index); //! Select by ModelIndex
    void unselectAll();
    void unselect(QItemSelection desel);
    bool reselect();
    bool canSelectPrevious();
    bool selectPrevious();
    bool canSelectNext();
    bool selectNext();
    void resetSelectionHistory();
    void appendSelectionToHistory();
    void emitShowSelection(); //!< Show selection in all views

  signals:
    void showSelection();

  public:
    TreeItem *lastToggledItem();
    bool selectFirstBranch();
    bool selectFirstChildBranch();
    bool selectLastBranch();
    bool selectLastChildBranch();
    bool selectLastSelectedBranch();
    bool selectLastImage();
    bool selectLatestAdded();
    bool selectParent();

  public:
    TreeItem::Type selectionType();
    BranchItem *getSelectedBranch();
    QList<BranchItem *> getSelectedBranches(BranchItem *bi = nullptr);
    ImageItem *getSelectedImage();
    Task *getSelectedTask();
    XLinkItem *getSelectedXLinkItem();
    Link *getSelectedXLink();
    AttributeItem *getSelectedAttribute();
    TreeItem *getSelectedItem();
    QList<TreeItem *> getSelectedItems();
    QModelIndex getSelectedIndex();
    QList<uint> getSelectedIDs();
    QStringList getSelectedUUIDs();
    bool isSelected(TreeItem *);
    QString getSelectString();
    QString getSelectString(TreeItem *item);
    QString getSelectString(BranchItem *item);
    QString getSelectString(const uint &i);
    void setLatestAddedItem(TreeItem *ti);
    TreeItem *getLatestAddedItem();

  signals:
    void selectionChanged(const QItemSelection &newsel,
                          const QItemSelection &oldsel);

  public:
    void emitSelectionChanged(const QItemSelection &oldsel);
    void emitSelectionChanged();
    void selectMapLinkColor();
    void selectMapSelectionColor();

  private:
    void setSelectionColorInt(QColor);
    QItemSelectionModel *selModel;
    QString lastSelectString;
    bool selectionBlocked; //! Used to block changes of selection while editing
                           //! a heading

  public:
    void setSelectionColor(QColor);
    QColor getSelectionColor();

    ////////////////////////////////////////////
    // Iterating and selecting branches
    ////////////////////////////////////////////
  public:
    bool initIterator(const QString &iname,
                      bool deepLevelsFirst = false); //! Named iterator
    bool nextIterator(const QString &iname);         //! Select next iterator
  private:
    QHash<QString, QUuid> selIterCur;
    QHash<QString, QUuid> selIterPrev;
    QHash<QString, QUuid> selIterStart;
    QHash<QString, bool> selIterActive;

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
};

#endif
