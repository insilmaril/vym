#ifndef BRANCHITEM_H
#define BRANCHITEM_H

#include "mapitem.h"
#include "task.h"

#include <QList>

class QString;
class QGraphicsScene;
class BranchObj;
class Link;
class XLinkItem;

class BranchItem : public MapItem {
  public:
    enum LayoutHint { AutoPositioning, FreePositioning };

    BranchItem(TreeItem *parent = nullptr);
    virtual ~BranchItem();
    virtual void clear();
    virtual void copy(BranchItem *item);
    virtual BranchItem *parentBranch();

    virtual void insertBranch(int pos, BranchItem *branch);

    virtual QString saveToDir(const QString &tmpdir, const QString &prefix,
                              const QPointF &offset, QList<Link *> &tmpLinks);

    virtual void updateVisibility();

    virtual void setHeadingColor(
        QColor color); //! Overloaded from TreeItem to update QGraphicsView

  protected:
    bool scrolled;      // true if all children are scrolled and thus invisible
    bool tmpUnscrolled; // can only be true (temporary) for a scrolled subtree

  public:
    void updateTaskFlag();
    void setTask(Task *t);
    Task *getTask();

  private:
    Task *task;

  public:
    virtual void scroll();
    virtual void unScroll();
    virtual bool toggleScroll(); // scroll or unscroll
    virtual bool isScrolled();   // returns scroll state
    virtual bool hasScrolledParent(
        BranchItem *start = NULL); // true, if any of the parents is scrolled
    virtual bool tmpUnscroll(
        BranchItem *start = NULL);   // unscroll scrolled parents temporary e.g.
                                     // during "find" process
    virtual bool resetTmpUnscroll(); // scroll all tmp scrolled parents again
                                     // e.g. when unselecting
    virtual void sortChildren(bool inverse = false); //! Sort children
    virtual void setChildrenLayout(BranchItem::LayoutHint layoutHint);
    virtual BranchItem::LayoutHint getChildrenLayout();

  protected:
    bool includeImagesVer;     //! include floatimages in bbox vertically
    bool includeImagesHor;     //! include floatimages in bbox horizontally
    bool includeChildren;      //! include children in frame
    LayoutHint childrenLayout; //! should children be positioned freely?

  public:
    void setIncludeImagesVer(bool);
    bool getIncludeImagesVer();
    void setIncludeImagesHor(bool);
    bool getIncludeImagesHor();
    QString getIncludeImageAttr();
    BranchItem *getFramedParentBranch(BranchItem *start);
    void setFrameIncludeChildren(bool);
    bool getFrameIncludeChildren();

  protected:
    int lastSelectedBranchNum;
    int lastSelectedBranchNumAlt;

  public:
    virtual void
    setLastSelectedBranch(); //! Set myself as last selected in parent
    virtual void
    setLastSelectedBranch(int i); //! Set last selected branch directly
    virtual BranchItem *
    getLastSelectedBranch(); //! Returns last selected branch usually
    virtual BranchItem *
    getLastSelectedBranchAlt(); //! Used to return last selected branch left of
                                //! a mapcenter

  public:
    TreeItem *findMapItem(
        QPointF p,
        TreeItem *excludeTI); //! search map for branches or images. Ignore
                              //! excludeTI, where search is started

    virtual void
    updateStyles(const bool &keepFrame =
                     false); //! update related fonts, parObjects, links, ...
    virtual BranchObj *getBranchObj();
    virtual BranchObj *createMapObj(
        QGraphicsScene *scene); //! Create classic object in GraphicsView
};

#endif
