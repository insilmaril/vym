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
class Flag;
class FloatImageObj;
class ImageItem;
class MapDesign;
class QModelIndex;
class VymModel;
class XLinkItem;
class XLinkObj;

class TreeItem : public XMLObj {
  public:
    enum Type { Undefined, MapCenter, Branch, Image, Attribute, XLinkItemType };    // FIXME-3 Remove type MapCenter, not needed. // FIXME-3 Rename all to {$NAME}ItemType
    enum HideTmpMode { HideNone, HideExport };

    TreeItem(TreeItem *parent = nullptr);
    virtual ~TreeItem();
    void init();

    // General housekeeping
    virtual void setModel(VymModel *m);
    virtual VymModel *getModel();
    virtual MapDesign *mapDesign();

    /*! Return number of item, as it would be after it would have been appended.
    This is used to notify view about layout changes before model is modified.
  */
    virtual int getRowNumAppend(TreeItem *child);

    virtual void appendChild(TreeItem *child);
    virtual void removeChild(int row);

    virtual TreeItem *child(int row);
    virtual int childCount() const;
    virtual int columnCount() const;
    virtual int branchCount() const;
    virtual int branchOffset() const;
    virtual int imageCount() const;
    virtual int xlinkCount() const;
    virtual int attributeCount() const;

    virtual int row() const;
    virtual int depth();
    virtual TreeItem *parent();
    virtual bool isChildOf(TreeItem *ti);

    /*! Return number of item in parent by type,
    e.g. first branch has number 0           */
    virtual int num();      //! Return number of item by type
    virtual int num(TreeItem *item); //! Return number of item by type

  protected:
    Type type;

  public:
    virtual void setType(const Type t);
    virtual Type getType();
    virtual bool hasTypeAttribute() const;
    virtual bool hasTypeBranch() const;
    virtual bool hasTypeImage() const;
    virtual bool hasTypeBranchOrImage() const;
    virtual bool hasTypeXLink() const;
    virtual QString getTypeName();

    // Accessing data
    virtual QVariant data(int column) const;

  protected:
    Heading headingInt;

  public:
    virtual void setHeading(const VymText &vt);
    virtual void setHeadingPlainText(const QString &s);
    Heading heading() const;
    virtual QString headingText(bool indented = false);
    virtual std::string
    headingStd() const; //! convenience function used for debugging
    virtual QString headingPlain() const; //! Some views or methods can't cope with RichText
    virtual QString headingPlainWithParents(
        uint numberOfParents); //! Show also some of the parents
    virtual void
    setHeadingColor(QColor color);    //! Set color of heading. In BranchItem
                                      //! overloaded to update QGraphicsView
    virtual QColor headingColor(); //! Returns color of heading

  public:
    enum UrlType{NoUrl, GeneralUrl, JiraUrl};

  protected:
    QString urlInt;
    UrlType urlTypeInt;

  public:
    void setUrl(const QString &url); //! Set Url
    QString url();                   //! Get Url
    bool hasUrl();
    void setUrlType(UrlType);
    UrlType urlType();
    Flag *urlFlag();

  protected:
    QString vymLinkInt;

  public:
    void setVymLink(const QString &s);  //! Set vymLink
    QString vymLink();                  //! Get vymLink
    bool hasVymLink();

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
    If master is not nullptr,, only one Flag from FlagRow master may
    be active simultanously, the others get deactivated.
    */
    // virtual void toggleFlag(const QString &name, bool useGroups = true);
    Flag *toggleFlagByUid(const QUuid &uid, bool useGroups = true);
    virtual void toggleSystemFlag(const QString &flag, FlagRow *master = nullptr);
    virtual bool hasActiveFlag(const QString &flag);
    virtual bool hasActiveSystemFlag(const QString &flag);
    virtual void activateSystemFlagByName(const QString &);
    virtual void deactivateSystemFlagByName(const QString &);
    QList<QUuid> activeFlagUids();

    virtual QList<QUuid> activeSystemFlagUids();

  protected:
    ulong itemID;
    QUuid uuid;

  public:
    virtual ulong getID();
    virtual void setUuid(const QString &id);
    virtual QUuid getUuid();

    // Navigation and selection
    virtual TreeItem *childItemByRow(const int &n);
    virtual BranchItem *getFirstBranch();
    virtual BranchItem *getLastBranch();
    virtual ImageItem *getFirstImage();
    virtual ImageItem *getLastImage();
    virtual TreeItem *getFirstItem();
    virtual TreeItem *getLastItem();

    /*! Get next branch after current branch. Return nullptr if there is no
    next branch */
    virtual BranchItem* getNextBranch(BranchItem *currentBranch);

    virtual BranchItem* getBranchNum(const int &n);

    /*! Convenience list of branches */
    virtual QList <BranchItem*> getBranches();

    virtual ImageItem* getImageNum(const int &n);

    virtual AttributeItem* getAttributeNum(const int &n);
    virtual AttributeItem* getAttributeByKey(const QString &k);
    virtual QVariant attributeValue(const QString &k);

    virtual XLinkItem* getXLinkItemNum(const int &n);
    virtual XLinkObj* getXLinkObjNum(const int &n);

  protected:
    bool hideTemporaryInt;  //! Hide this item e.g. during export if set
    bool hidden;            //! Currently hidden if true
  public:
    virtual void setHideMode(HideTmpMode);
    virtual bool hasHiddenParent();
    virtual void setHideTemporary(bool); // set export of object (and children)
    virtual bool hideTemporary();
    virtual bool isHidden();

    virtual QString getGeneralAttr();

  protected:
    VymModel *model;

    QList<TreeItem *> childItems;
    QList<QVariant> itemData;   // Heading for TreeEditor in first column
    TreeItem *parentItem;

    /*!  Set rootItem (does not change, needed for some quick checks
     e.g. if some branch is mapCenter and isChildOf  */
    TreeItem *rootItem;

    int attributeOffsetInt;    // Always 0
    int attributeCounter;

    int imageOffsetInt;
    int imageCounter;

    int branchOffsetInt;
    int branchCounter;

    int xlinkOffsetInt;
    int xlinkCounter;
};

#endif
