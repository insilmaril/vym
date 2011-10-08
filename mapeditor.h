#ifndef MAPEDITOR_H
#define MAPEDITOR_H

#include <QGraphicsView>
#include <QItemSelectionModel>
#include <QLineEdit>	
#include <QPropertyAnimation>	

#include "attribute.h"
#include "xlink.h"
#include "ornamentedobj.h"
#include "settings.h"
#include "vymmodel.h"


class XLinkItem;

/*! \brief Main widget in vym to display and edit a map */


class MapEditor : public QGraphicsView {    
    Q_OBJECT

public:
    MapEditor(VymModel *vm);
    ~MapEditor();
    VymModel* getModel();
    QGraphicsScene * getScene();

Q_PROPERTY(QPointF scrollBarPos READ getScrollBarPos WRITE setScrollBarPos)

protected:
    QPointF scrollBarPos;
    QPointF scrollBarPosTarget;
    QPropertyAnimation scrollBarPosAnimation;
    QTimer *panningTimer;
    QPointF vPan;		        //! Direction of panning during moving of object
    QPoint pointerPos;			//! Pointer position in widget coordinates
    Qt::KeyboardModifiers pointerMod;	//! modifiers of move event

private slots:
    void panView();

public:
    void scrollTo (const QModelIndex &index);
    void setScrollBarPosTarget (QRectF rect);	//!  ensureVisible of rect
    QPointF getScrollBarPosTarget ();
    void setScrollBarPos (const QPointF &p);
    QPointF getScrollBarPos();
    void animateScrollBars();

Q_PROPERTY(qreal zoomFactor READ getZoomFactor WRITE setZoomFactor)

protected:
    qreal zoomFactor;
    qreal zoomFactorTarget;
    QPropertyAnimation zoomAnimation;

public:
    void setZoomFactorTarget (const qreal &zf);
    qreal getZoomFactorTarget();
    void setZoomFactor (const qreal &zf);
    qreal getZoomFactor();

Q_PROPERTY(qreal angle READ getAngle WRITE setAngle)

protected:
    qreal angle;
    qreal angleTarget;
    QPropertyAnimation rotationAnimation;

public:
    void setAngleTarget (const qreal &a);
    qreal getAngleTarget();
    void setAngle (const qreal &a);
    qreal getAngle();

    void updateMatrix();
public:
    void print();		    //!< Print the map
    QRectF getTotalBBox();	    //!< Bounding box of all items in map
    QImage getImage (QPointF &offset);	//!< Get a pixmap of the map
    void setAntiAlias (bool);	    //!< Set or unset antialiasing
    void setSmoothPixmap(bool);	    //!< Set or unset smoothing of pixmaps
public slots:	
    void autoLayout();		    //!< Auto layout of map by using collision detection
public:
    TreeItem *findMapItem (QPointF p,TreeItem *exclude);    //! find item in map at position p. Ignore item exclude 

    AttributeTable* attributeTable();	// FIXME-3
    void testFunction1();		// just testing new stuff
    void testFunction2();		// just testing new stuff

    BranchItem* getBranchDirectAbove(BranchItem *bi);	//!  get branch direct above bi (in TreeView) 
    BranchItem* getBranchAbove(BranchItem *bi);		//! get branch above bi (in TreeView) 
    BranchItem* getBranchDirectBelow(BranchItem *bi);	//!  bet branch direct below bi (in TreeView)
    BranchItem* getBranchBelow(BranchItem *bi);		//! bet branch below bi (in TreeView)
    BranchItem* getLeftBranch(BranchItem *bi);		//! bet branch left of bi (in TreeView)
    BranchItem* getRightBranch(BranchItem *bi);		//! bet branch right of bi (in TreeView)

public slots:
    void cursorUp();
    void cursorDown();
    void cursorLeft();
    void cursorRight();
    void cursorFirst();
    void cursorLast();
    void editHeading();
    void editHeadingFinished();
private:
    bool editingHeading;
    QLineEdit *lineEdit;

protected:
    virtual void contextMenuEvent ( QContextMenuEvent *e );
    virtual void keyPressEvent(QKeyEvent*);
    virtual void keyReleaseEvent(QKeyEvent*);
    virtual void mousePressEvent(QMouseEvent*);
    virtual void mouseMoveEvent(QMouseEvent*);
    void moveObject ();
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void mouseDoubleClickEvent(QMouseEvent*);
    virtual void wheelEvent(QWheelEvent*);
    virtual void focusOutEvent (QFocusEvent*);
    virtual void resizeEvent( QResizeEvent * );

    void dragEnterEvent (QDragEnterEvent *);
    void dragMoveEvent (QDragMoveEvent *);
    void dragLeaveEvent (QDragLeaveEvent *);
    void dropEvent (QDropEvent *);


private:
    QGraphicsScene *mapScene;
    VymModel *model;		//!< Vym Map, includding several mapCenters

    bool adjustCanvasRequested;	// collect requests until end of user event
    BranchObj *editingBO;	// entering Text into BO

    QCursor HandOpenCursor;	// cursor while moving canvas view
    QCursor PickColorCursor;	// cursor while picking color 
    QCursor CopyCursor;		// cursor while picking color 
    QCursor XLinkCursor;	// cursor while picking color 
    bool pickingColor;
    bool drawingLink;		// true while creating a link
    bool copyingObj;		// true while modModeCopy
    bool objectMoved;		// true if object was not clicked, but moved with mouse
    Link* tmpLink;

    MapObj* movingObj;		    // moving a MapObj
    QPointF movingObj_orgPos;	    // org. pos of mouse before move
    QPointF movingObj_orgRelPos;    // org. relative pos of mouse before move
    QPointF movingObj_offset;	    // offset of mousepointer to object
    QPointF movingCont_start;	    // inital pos of moving Content or
    QPointF movingVec;		    // how far has Content moved

    QPointF contextMenuPos;	    // position where context event was triggered

    AttributeTable *attrTable;

    bool printFrame;		// Print frame around map
    bool printFooter;		// Print footer below map

    QPoint exportOffset;	// set before export, used in save

//////////// Selection related
signals:
    void selectionChanged(const QItemSelection &, const QItemSelection &);

private:    
    QList <QGraphicsPolygonItem*> selPolyList;
    QColor selectionColor;

public slots:
    void updateSelection(QItemSelection ,QItemSelection); // update selection
    void updateData (const QModelIndex&); // update data
public:
    void setSelectionColor (QColor c);
    QColor getSelectionColor ();
};
#endif

