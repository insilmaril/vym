#ifndef BRANCHITEM_H
#define BRANCHITEM_H

#include "branch-container.h"
#include "image-container.h"
#include "mapdesign.h"
#include "mapitem.h"
#include "task.h"

#include <QList>

class QString;
class QGraphicsScene;

class BranchWrapper;
class HeadingContainer;
class Link;
class XLinkItem;

// Get heading in a safe form, works also for nullptr
QString headingText(BranchItem *bi);

class BranchItem : public MapItem {
  public:
    BranchItem(TreeItem *parent = nullptr);
    virtual ~BranchItem();
    virtual void copy(BranchItem *item);
    virtual BranchItem *parentBranch();

    BranchWrapper *branchWrapper();

    virtual void insertBranch(int pos, BranchItem *branch);
    virtual void insertImage (int pos, ImageItem *image);

    virtual QString saveToDir(const QString &tmpdir, const QString &prefix,
                              const QPointF &offset, QList<Link *> &tmpLinks,
                              const bool &exportBoundingBoxes);

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
    BranchWrapper *branchWrapperInt;

  public:
    virtual void scroll();
    virtual void unScroll();
    virtual bool toggleScroll(); // scroll or unscroll
    virtual bool isScrolled();   // returns scroll state
    virtual bool hasScrolledParent(
        BranchItem *start = nullptr); // true, if any of the parents is scrolled
    virtual bool tmpUnscroll(
        BranchItem *start = nullptr);   // unscroll scrolled parents temporary e.g.
                                     // during "find" process
    virtual bool resetTmpUnscroll(); // scroll all tmp scrolled parents again
                                     // e.g. when unselecting

  public:
    void setBranchesLayout(const QString &);
    void setImagesLayout(const QString &);
    QColor getBackgroundColor(BranchItem *start, bool checkInnerFrame = true);

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
        QList <TreeItem*> excludedItems); //! search map for branches or images. Ignore
                              //! excludeItems, where search is started or which are selected

    void updateVisuals();

    BranchContainer *createBranchContainer(
        QGraphicsScene *scene); //! Create classic object in GraphicsView

    BranchContainer* getBranchContainer();
    void unlinkBranchContainer();
    Container* getBranchesContainer();
    Container* getImagesContainer();

  private:
    BranchContainer *branchContainer;

  public:
    void updateContainerStackingOrder();
    void addToBranchesContainer(BranchContainer*);
    void addToImagesContainer(ImageContainer*);
    void repositionContainers();
};

#endif
