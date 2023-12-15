#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QGraphicsView>
#include <QItemSelectionModel>
#include <QLineEdit>
#include <QPropertyAnimation>

#include "tmp-parent-container.h"
#include "settings.h"
#include "vymmodel.h"
#include "xlink.h"

class XLinkItem;
class Winter;

/*! \brief Main widget in vym to display and edit a map */

class MapEditor : public QGraphicsView {
    Q_OBJECT

  public:
    enum EditorState {
        Neutral,
        EditingHeading,
        DrawingLink,
        DrawingXLink,
        MovingObject,
        MovingObjectTmpLinked,
        MovingObjectWithoutLinking,
        PanningView,
        PickingColor
    };

    MapEditor(VymModel *vm);
    ~MapEditor();
    VymModel *getModel();
    QGraphicsScene *getScene();

    // Animation of scrollbars
    Q_PROPERTY(QPointF scrollBarPos READ getScrollBarPos WRITE setScrollBarPos)

  protected:
    QPointF scrollBarPos;
    QPointF scrollBarPosTarget;
    QPropertyAnimation scrollBarPosAnimation;
    QTimer *panningTimer;
    QPointF vPan;      //! Direction of panning during moving of object

  private slots:
    void panView();

  public:
    void ensureAreaVisibleAnimated(
            const QRectF &area, 
            bool scaled = false, 
            bool rotated = false,
            qreal new_angle = 0) ;
    void ensureSelectionVisibleAnimated(bool scaled = false, bool rotated = false);
    void scrollTo(const QModelIndex &index);
    void setScrollBarPosTarget(QRectF rect); //!  ensureVisible of rect
    QPointF getScrollBarPosTarget();
    void setScrollBarPos(const QPointF &p);
    QPointF getScrollBarPos();
    void animateScrollBars();

    // Animation of containers
  private:
    QTimer *animationTimer;
    bool animationUse;
    uint animationTicks;
    uint animationInterval;
    int timerId;                 // animation timer
    QList<Container*> animatedContainers;

  private slots:
    void animate(); //!< Called by timer to animate stuff
  public:
    void startAnimation(Container *c, const QPointF &v);
    void startAnimation(Container *c, const QPointF &start, const QPointF &dest);
    void stopAnimation(Container *c);
    void stopAllAnimation();

    // Animation of zoom
    Q_PROPERTY(qreal zoomFactor READ getZoomFactor WRITE setZoomFactor)

  protected:
    qreal zoomFactor;
    qreal zoomFactorTarget;
    QPropertyAnimation zoomAnimation;

  public:
    void setZoomFactorTarget(const qreal &zf);
    qreal getZoomFactorTarget();
    void setZoomFactor(const qreal &zf);
    qreal getZoomFactor();

    // Animation of rotation
    Q_PROPERTY(qreal angle READ getAngle WRITE setAngle)

  protected:
    qreal angle;
    qreal angleTarget;
    QPropertyAnimation rotationAnimation;

  public:
    void setAngleTarget(const qreal &a);
    qreal getAngleTarget();
    void setAngle(const qreal &a);
    qreal getAngle();

    // Animation of viewCenter
    Q_PROPERTY(QPointF viewCenter READ getViewCenter WRITE setViewCenter)

  protected:
    QPointF viewCenter;
    QPointF viewCenterTarget;

  public:
    void setViewCenterTarget(
        const QPointF &p, const qreal &zft, const qreal &at,
        const int duration = 2000,
        const QEasingCurve &easingCurve = QEasingCurve::OutQuint);
    void
    setViewCenterTarget(); //! Convenience function, center on selected item
    QPointF getViewCenterTarget();
    void setViewCenter(const QPointF &p);
    QPointF getViewCenter();
    QPropertyAnimation viewCenterAnimation;

    void updateMatrix(); //! Sets transformation matrix with current rotation
                         //! and zoom values
    void minimizeView();

    // xmas egg
  protected:
    Winter *winter;

  public:
    void print();                     //!< Print the map
    QRectF getTotalBBox();            //!< Bounding box of all items in map
    QImage getImage(QPointF &offset); //!< Get a pixmap of the map
    void setAntiAlias(bool);          //!< Set or unset antialiasing
    void setSmoothPixmap(bool);       //!< Set or unset smoothing of pixmaps
  public slots:
    void autoLayout();    //!< Auto layout of map by using collision detection
    void testFunction1(); //! just testing new stuff
    void testFunction2(); //! just testing new stuff

  public:
    TreeItem *findMapItem(
            QPointF p, 
            const QList <TreeItem*> &excludedItems = QList<TreeItem*>(),
            bool findNearCenter = false);  //! find item in map at position
                                           //! p. Ignore item exclude
    BranchItem *findMapBranchItem(
            QPointF p,
            const QList <TreeItem*> &excludedItems = QList<TreeItem*>(),
            bool findNearCenter = false); //! only return BranchItem
    void toggleWinter();

    BranchItem *getBranchDirectAbove(
        BranchItem *bi); //! get branch direct above bi (in TreeView)
    BranchItem *
    getBranchAbove(BranchItem *bi); //! get branch above bi (in TreeView)
    BranchItem *getBranchDirectBelow(
        BranchItem *bi); //! bet branch direct below bi (in TreeView)
    BranchItem *
    getBranchBelow(BranchItem *bi); //! bet branch below bi (in TreeView)
    BranchItem *
    getLeftBranch(TreeItem *ti); //! bet branch left of bi (in TreeView)
    BranchItem *
    getRightBranch(TreeItem *ti); //! bet branch right of bi (in TreeView)

  private:
    // Toggle objects by moving the cursor up/down with shift modifier
    // (needs to consider the current direction of movement)
    enum ToggleDirection {toggleUndefined, toggleUp, toggleDown};
    ToggleDirection lastToggleDirection;

  public slots:
    void cursorUp();
    void cursorUpToggleSelection();
    void cursorDown();
    void cursorDownToggleSelection();
    void cursorLeft();
    void cursorRight();
    void cursorFirst();
    void cursorLast();
    void editHeading();
    void editHeadingFinished();

  private:
    QLineEdit *lineEdit;

  private:
    void contextMenuEvent(QContextMenuEvent *e);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void startPanningView(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void moveObject(QMouseEvent *, const QPointF &p_event);    // Called from mouseMoveEvent
    void mouseReleaseEvent(QMouseEvent *);
    void mouseDoubleClickEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void focusOutEvent(QFocusEvent *);
    void resizeEvent(QResizeEvent *);

    void dragEnterEvent(QDragEnterEvent *);
    void dragMoveEvent(QDragMoveEvent *);
    void dragLeaveEvent(QDragLeaveEvent *);
    void dropEvent(QDropEvent *);

  private:
    QGraphicsScene *mapScene;
    VymModel *model; //!< Vym Map, includding several mapCenters

    bool adjustCanvasRequested; // collect requests until end of user event
    BranchObj *editingBO;       // entering Text into BO

    QCursor HandOpenCursor;  // cursor while moving canvas view
    QCursor PickColorCursor; // cursor while picking color
    QCursor CopyCursor;      // cursor while picking color
    QCursor XLinkCursor;     // cursor while picking color

    // Various states of the MapEditor
  public:
    MapEditor::EditorState state();

  private:
    EditorState editorState;

    void setState(EditorState);
    bool objectMoved; // true if object was not clicked, but moved with mouse

    // Temporary used for linkx
    Link *tmpLink;

    // Temporary used for panning view
    QPoint panning_initialPointerPos;           // initial pos in pointer coordinates
    QPoint panning_initialScrollBarValues;      // inital values of scrollbars

    // Moving containers
    QList <TreeItem*> movingItems;              // selected items which are currently moved
    QPointF movingObj_initialScenePos;          // coord when button was pressed
    QPointF movingObj_initialContainerOffset;   // offset from above coordinates to object
    TmpParentContainer *tmpParentContainer;

    QPointF contextMenuPos; // position where context event was triggered

    bool printFrame;  // Print frame around map
    bool printFooter; // Print footer below map

    QPoint exportOffset; // set before export, used in save

    //////////// Selection related
  signals:
    void selectionChanged(const QItemSelection &, const QItemSelection &);

  public slots:
    void updateData(const QModelIndex &);                 // update data
    void togglePresentationMode();
    void updateSelection(QItemSelection, QItemSelection); // update selection
                                                          //
  public:  
    void updateSelection(); // update selection after selection style changed
};
#endif
