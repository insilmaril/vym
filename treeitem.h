#ifndef TREEITEM_H
#define TREEITEM_H

#include <QColor>
#include <QList>
#include <QVariant>

#include "flagrow.h"
#include "noteobj.h"
#include "xmlobj.h"

class AttributeItem;
class BranchObj;
class BranchItem;
class FloatImageObj;
class ImageItem;
class LinkableMapObj;
class QModelIndex;
class VymModel;
class XLinkItem;
class XLinkObj;

class TreeItem:public XMLObj
{
public:
    enum Type {Undefined,MapCenter,Branch,Image,Attribute,XLink};
    enum HideTmpMode {HideNone, HideExport};

    TreeItem();
    TreeItem(const QList<QVariant> &data, TreeItem *parent = 0);
    virtual ~TreeItem();
    void init();

    // General housekeeping
    virtual void setModel (VymModel *m);
    virtual VymModel* getModel();


    /*! Return number of item, as it would be after it would have been appended.
	This is used to notify view about layout changes before model is modified. */
    virtual int getRowNumAppend (TreeItem *child);

    virtual void appendChild (TreeItem *child);
    virtual void removeChild (int row);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    virtual int childNumber() const;
    virtual int columnCount() const;
    virtual int branchCount() const;
    virtual int imageCount() const;
    virtual int xlinkCount() const;
    virtual int attributeCount() const;

    virtual int row() const;
    virtual int depth() ;
    virtual TreeItem *parent();

    /*! Return number of item in parent by type, 
	e.g. first branch has number 0           */
    virtual int childNum();		//! Return number of item in list of all children
    virtual int num();			//! Return number of item by type
    virtual int num (TreeItem *item);	//! Return number of item by type

protected:
    Type type;
public:	
    virtual void setType (const Type t);
    virtual Type getType ();
    virtual bool isBranchLikeType() const;
    virtual QString getTypeName ();

// Accessing data
    virtual QVariant data (int column) const;


protected:
    QColor headingColor;
    QColor backgroundColor;
public:	
    virtual void setHeading (const QString s);
    virtual QString getHeading() const;
    virtual std::string getHeadingStd() const;	//! convenience function used for debugging
    virtual QString getHeadingPlain() const;	//! Some views or methods can't cope with RichText
    virtual void setHeadingColor(QColor color);	//! Set color of heading. In BranchItem overloaded to update QGraphicsView
    virtual QColor getHeadingColor();		//! Returns color of heading
    virtual void setBackgroundColor(QColor color);//! Set color of frame brush, if LMO exists for item
    virtual QColor getBackgroundColor();	//! Returns color of frame brush, if LMO exists for item


protected:
    QString url;
public:
    void setURL (const QString &url);		//! Set URL
    QString getURL ();				//! Get URL


protected:
    QString vymLink;
public:
    void setVymLink (const QString &url);	    //! Set URL
    QString getVymLink ();			    //! Get URL

protected:
    bool target;
public:    
    void toggleTarget();			//! Toggle target status
    bool isTarget();				//! Returns true if item is is a target

protected:
    NoteObj note;
public:	
    virtual void setNote(const QString &s);
    virtual void clearNote();
    virtual QString getNote();
    virtual bool hasEmptyNote();
    virtual void setNoteObj(const NoteObj &); //FIXME-3 setNoteObj is called for every select or so???

    virtual NoteObj getNoteObj();	    
    virtual QString getNoteASCII(const QString &indent, const int &width); // returns note  (ASCII)
    virtual QString getNoteASCII();	    // returns note (ASCII)
    virtual QString getNoteOpenDoc();	    // returns note (OpenDoc)


protected:
    FlagRow standardFlags;
    FlagRow systemFlags;
public:	
    virtual void activateStandardFlag(const QString &flag);
    virtual void deactivateStandardFlag(const QString &flag);
    virtual void deactivateAllStandardFlags();

    /*! \brief Toggle a Flag 
	If master is not NULL,, only one Flag from FlagRow master may 
	be active simultanously, the others get deactivated.
    */	
    virtual void toggleStandardFlag(const QString &flag, FlagRow *master=NULL);
    virtual void toggleSystemFlag  (const QString &flag, FlagRow *master=NULL);
    virtual bool hasActiveStandardFlag (const QString &flag);
    virtual bool hasActiveSystemFlag   (const QString &flag);
    virtual QStringList activeStandardFlagNames();
    virtual FlagRow* getStandardFlagRow ();

    virtual QStringList activeSystemFlagNames();

    virtual bool canMoveDown();
    virtual bool canMoveUp();

protected:
    uint id;

public:
    virtual uint getID ();

    // Navigation and selection
    virtual TreeItem* getChildNum(const int &n);
    virtual BranchItem* getFirstBranch();
    virtual BranchItem* getLastBranch();
    virtual ImageItem* getFirstImage();
    virtual ImageItem* getLastImage();

    /*! Get next branch after current branch. Return NULL if there is no
	next branch */
    virtual BranchItem* getNextBranch(BranchItem* currentBranch);

    virtual BranchItem* getBranchNum(const int &n);
    virtual BranchObj* getBranchObjNum(const int &n);

    virtual ImageItem* getImageNum(const int &n);
    virtual FloatImageObj* getImageObjNum(const int &n);

    virtual AttributeItem* getAttributeNum(const int &n);

    virtual XLinkItem* getXLinkItemNum(const int &n);
    virtual XLinkObj* getXLinkObjNum(const int &n);

protected:
    bool hideExport;			    //! Hide this item in export
    bool hidden;			    //! Hidden in export if true
public:
    virtual void setHideTmp (HideTmpMode);
    virtual bool hasHiddenExportParent ();
    virtual void setHideInExport(bool);	    // set export of object (and children)
    virtual bool hideInExport();
    virtual bool isHidden ();	    
    virtual void updateVisibility();	    //! Sets visibility in LinkableMapObj, if existing

    virtual QString getGeneralAttr();
    
protected:
    VymModel *model;

    QList<TreeItem*> childItems;
    QList<QVariant> itemData;
    TreeItem *parentItem;

    /*!  Set rootItem (does not change, needed for quick check 
	 if some branch is mapCenter */
    TreeItem *rootItem;
 
    int branchOffset;
    int branchCounter;
    int imageOffset;
    int imageCounter;

    int attributeOffset;
    int attributeCounter;

    int xlinkOffset;
    int xlinkCounter;
};

#endif
