#ifndef TREEITEM_H
#define TREEITEM_H

#include <QColor>
#include <QList>
#include <QUuid>
#include <QVariant>

#include "flagrow.h"
#include "heading.h"
#include "vymnote.h"
#include "xmlobj.h"

class AttributeItem;
class BranchObj;
class BranchItem;
class FloatImageObj;
class ImageItem;
class QModelIndex;
class VymModel;
class XLinkItem;
class XLinkObj;

class TreeItem : public XMLObj {
  public:
    enum Type { Undefined, MapCenter, Branch, Image, Attribute, XLink };
    enum HideTmpMode { HideNone, HideExport };

    TreeItem(TreeItem *parent = nullptr);
    virtual ~TreeItem();
    void init();
    virtual void clear() = 0;

    // General housekeeping
    virtual void setModel(VymModel *m);
    virtual VymModel *getModel();

    /*! Return number of item, as it would be after it would have been appended.
    This is used to notify view about layout changes before model is modified.
  */
    virtual int getRowNumAppend(TreeItem *child);

    virtual void appendChild(TreeItem *child);
    virtual void removeChild(int row);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    virtual int childNumber() const;
    virtual int columnCount() const;
    virtual int branchCount() const;
    virtual int imageCount() const;
    virtual int xlinkCount() const;
    virtual int attributeCount() const;

    virtual int row() const;
    virtual int depth();
    virtual TreeItem *parent();
    virtual bool isChildOf(TreeItem *ti);

    /*! Return number of item in parent by type,
    e.g. first branch has number 0           */
    virtual int childNum(); //! Return number of item in list of all children
    virtual int num();      //! Return number of item by type
    virtual int num(TreeItem *item); //! Return number of item by type

  protected:
    Type type;

  public:
    virtual void setType(const Type t);
    virtual Type getType();
    virtual bool hasTypeBranch() const;
    virtual bool hasTypeImage() const;
    virtual QString getTypeName();

    // Accessing data
    virtual QVariant data(int column) const;

  protected:
    Heading heading;
    QColor backgroundColor;

  public:
    virtual void setHeading(const VymText &vt);
    virtual void setHeadingPlainText(const QString &s);
    Heading getHeading() const;
    virtual QString getHeadingText();
    virtual std::string
    getHeadingStd() const; //! convenience function used for debugging
    virtual QString
    getHeadingPlain() const; //! Some views or methods can't cope with RichText
    virtual QString getHeadingPlainWithParents(
        uint numberOfParents); //! Show also some of the parents
    virtual QString getHeadingDepth();
    virtual void
    setHeadingColor(QColor color);    //! Set color of heading. In BranchItem
                                      //! overloaded to update QGraphicsView
    virtual QColor getHeadingColor(); //! Returns color of heading
    virtual void setBackgroundColor(
        QColor color); //! Set color of frame brush, if LMO exists for item
    virtual QColor getBackgroundColor(); //! Returns color of frame brush, if
                                         //! LMO exists for item

  protected:
    QString url;

  public:
    void setURL(const QString &url); //! Set URL
    QString getURL();                //! Get URL

  protected:
    QString vymLink;

  public:
    void setVymLink(const QString &url); //! Set URL
    QString getVymLink();                //! Get URL

  protected:
    bool target;

  public:
    void toggleTarget(); //! Toggle target status
    bool isTarget();     //! Returns true if item is is a target

  protected:
    VymNote note;

  public:
    bool isNoteEmpty();
    virtual bool clearNote();
    virtual bool hasEmptyNote();
    virtual bool setNote(const VymText &vt); // FIXME-3 setNote is called for
                                             // every select or so???
    virtual bool setNote(const VymNote &vn);

    virtual VymNote getNote();
    virtual QString getNoteASCII(const QString &indent,
                                 const int &width); // returns note  (ASCII)
    virtual QString getNoteASCII();                 // returns note (ASCII)

  protected:
    FlagRow standardFlags;
    FlagRow systemFlags;
    FlagRow userFlags;

  public:
    virtual void activateStandardFlagByName(const QString &flag);
    virtual void deactivateStandardFlagByName(const QString &flag);
    virtual void deactivateAllStandardFlags();

    Flag *findFlagByUid(const QUuid &uid);

    /*! \brief Toggle a Flag
    If master is not NULL,, only one Flag from FlagRow master may
    be active simultanously, the others get deactivated.
    */
    // virtual void toggleFlag(const QString &name, bool useGroups = true);
    Flag *toggleFlagByUid(const QUuid &uid, bool useGroups = true);
    virtual void toggleSystemFlag(const QString &flag, FlagRow *master = NULL);
    virtual bool hasActiveFlag(const QString &flag);
    virtual bool hasActiveSystemFlag(const QString &flag);
    QList<QUuid> activeFlagUids();

    virtual QList<QUuid> activeSystemFlagUids();

    virtual bool canMoveDown();
    virtual bool canMoveUp();

  protected:
    ulong itemID;
    QUuid uuid;

  public:
    virtual ulong getID();
    virtual void setUuid(const QString &id);
    virtual QUuid getUuid();

    // Navigation and selection
    virtual TreeItem *getChildNum(const int &n);
    virtual BranchItem *getFirstBranch();
    virtual BranchItem *getLastBranch();
    virtual ImageItem *getFirstImage();
    virtual ImageItem *getLastImage();

    /*! Get next branch after current branch. Return NULL if there is no
    next branch */
    virtual BranchItem *getNextBranch(BranchItem *currentBranch);

    virtual BranchItem *getBranchNum(const int &n);

    virtual ImageItem *getImageNum(const int &n);

    virtual AttributeItem *getAttributeNum(const int &n);
    virtual AttributeItem *getAttributeByKey(const QString &k);

    virtual XLinkItem *getXLinkItemNum(const int &n);

  protected:
    bool hideExport; //! Hide this item in export
    bool hidden;     //! Hidden in export if true
  public:
    virtual void setHideTmp(HideTmpMode);
    virtual bool hasHiddenExportParent();
    virtual void setHideInExport(bool); // set export of object (and children)
    virtual bool hideInExport();
    virtual bool isHidden();
    virtual void
    updateVisibility(); //! Sets visibility in LinkableMapObj, if existing

    virtual QString getGeneralAttr();

  protected:
    VymModel *model;

    QList<TreeItem *> childItems;
    QList<QVariant> itemData;   // Heading for TreeEditor in first column
    TreeItem *parentItem;

    /*!  Set rootItem (does not change, needed for some quick checks
     e.g. if some branch is mapCenter and isChildOf  */
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
