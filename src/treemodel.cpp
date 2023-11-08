#include <QtGui>

#include "attributeitem.h"
#include "branchitem.h"
#include "imageitem.h"
#include "treeitem.h"
#include "treemodel.h"
#include "xlinkitem.h"

TreeModel::TreeModel(QObject *parent) : QAbstractItemModel(parent)
{
    // qDebug() << "Constr TreeModel  this=" << this;
    QList<QVariant> rootData;
    rootData << "Heading";
    // rootData << "Type";
    rootItem = new BranchItem();
    rootItem->setHeadingPlainText("rootItem");
}

TreeModel::~TreeModel()
{
    //qDebug()<<"Destr TreeModel  this="<<this;
    delete rootItem;
}

QVariant TreeModel::data(const QModelIndex &index, int role) const  // FIXME-2 no foreground color for imageItem and attr. item (use color of parentBranch)
{
    if (!index.isValid())
        return QVariant();

    TreeItem *item = getItem(index);
    BranchItem *bi = nullptr;
    if (item->hasTypeBranch())
        bi = (BranchItem*)item;

    if (role == Qt::EditRole || role == Qt::DisplayRole)
        return item->data(index.column());

    if (role == Qt::ForegroundRole) {
        if (bi)
            return bi->getHeadingColor();
        else
            return qApp->palette().color(QPalette::Text);
    }

    if (role == Qt::BackgroundRole) {
        if (bi)
            return bi->getBackgroundColor(bi);
        else
            // Selected XLink does not have a branchItem
            return qApp->palette().color(QPalette::Window); // FIXME-2 Better return map background, just like in BranchItem
    }

    return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex TreeModel::index(TreeItem *ti)
{
    if (!ti->parent())
        return QModelIndex();
    else
        return createIndex(ti->row(), 0, ti);
}

QModelIndex TreeModel::index(int row, int column,
                             const QModelIndex &parent) const
{
    // Make sure to return invalid index for invalid values (see modeltest)
    if (row < 0 || column < 0)
        return QModelIndex();
    if (column != 0)
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItem(parent);

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *ti = getItem(index);
    TreeItem *parentItem = ti->parent();
    if (parentItem == rootItem)
        return QModelIndex();
    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = getItem(parent);

    return parentItem->childCount();
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
    int c;
    if (parent.isValid())
        c = getItem(parent)->columnCount();
    else
        c = rootItem->columnCount();
    return c;
}

void TreeModel::nextBranch(BranchItem *&current, BranchItem *&previous,
                           bool deepLevelsFirst, BranchItem *start)
{
    if (deepLevelsFirst) {
        // Walk through map beginning at current with previous==0
        // Start at root, if current==nullptr
        if (!current) {
            if (start) {
                current = start;
                previous = current->parentBranch();
            }
            else {
                previous = (BranchItem *)rootItem;
                current = previous->getFirstBranch();
                return;
            }
        }

        // Walk the tree by always turning "left"
        // and returning an element when going up
        if (current == previous) {
            // Had leaf before, go up again.
            if (start && start == current) {
                current = nullptr;
                return;
            }
            current = current->parentBranch();
            if (!current)
                return;
            return nextBranch(current, previous, deepLevelsFirst, start);
        }

        if (current->depth() > previous->depth()) {
            // Coming from above, try to go deeper
            if (current->branchCount() > 0) {
                // Turn "left" and go deeper
                previous = current;
                current = current->getFirstBranch();
                return nextBranch(current, previous, deepLevelsFirst);
            }
            else {
                // turn around and go up again
                previous = current;
                return;
            }
        }
        else {
            // Coming from below, try to go down again to siblings

            BranchItem *sibling = current->getBranchNum(previous->num() + 1);
            if (sibling) {
                // Found sibling of previous, go there
                previous = current;
                current = sibling;
                return nextBranch(current, previous, deepLevelsFirst, start);
            }
            else {
                // and go further up
                if (current == rootItem)
                    current = nullptr;
                previous = current;
                return;
            }
        }
    }
    else {
        // Walk through map beginning at current with previous==0
        // Start at root, if current==nullptr
        if (!current) {
            if (start) {
                current = start;
                previous = (BranchItem *)(start->parent());
                return;
            }
            else {
                previous = (BranchItem *)rootItem;
                current = previous->getFirstBranch();
                return;
            }
        }

        if (current->depth() > previous->depth()) {
            // Going deeper
            if (current->branchCount() > 0) {
                // Turn "left" and go deeper
                previous = current;
                current = current->getFirstBranch();
                return;
            }
            else {
                // turn around and go up again
                previous = current;
                nextBranch(current, previous, deepLevelsFirst, start);
                return;
            }
        }
        else {
            if (start && previous == start) {
                current = nullptr;
                return;
            }

            BranchItem *sibling = current->getBranchNum(previous->num() + 1);
            if (sibling) {
                // Found sibling of previous, go there
                previous = current;
                current = sibling;
                return;
            }
            else {
                // no sibling, go further up left
                previous = current;
                current = current->parentBranch();
                if (!current) {
                    current = nullptr;
                    return;
                }
                else {
                    nextBranch(current, previous, deepLevelsFirst, start);
                }
            }
            return;
        }
    }
}

bool TreeModel::removeRows(int row, int count, const QModelIndex &parent)
{
    int last = row + count - 1;
    TreeItem *pi;
    if (parent.isValid())
        pi = getItem(parent);
    else
        pi = rootItem;
    TreeItem *ti;

    for (int i = row; i <= last; i++) {
        ti = pi->getChildNum(row);
        pi->removeChild(row); // does not delete object!
        delete ti;
    }
    return true;
}

TreeItem *TreeModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        TreeItem *item = static_cast<TreeItem *>(index.internalPointer());
        if (item)
            return item;
    }
    return nullptr;
}

BranchItem *TreeModel::getRootItem() { return rootItem; }

int TreeModel::xlinkCount() { return xlinks.count(); }

Link *TreeModel::getXLinkNum(const int &n)
{
    if (n >= 0 && n < xlinks.count())
        return xlinks.at(n);
    else
        return nullptr;
}
