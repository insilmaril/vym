#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "mapobj.h"
#include "treeitem.h"

Container::Container(QGraphicsItem *parent, TreeItem *ti) : QGraphicsRectItem(parent)
{
    qDebug() << "* Const Container begin this = " << this << "  branchitem = " << ti;
    treeItem = ti;
    init();
}

Container::~Container()
{
    QString h;
    if (treeItem) h = treeItem->getHeadingPlain();
    qDebug() << "* Destr Container" << name << h << this;

    if (treeItem)
    {
        // Unlink containers in my own subtree from related treeItems
        // That results in deleting all containers in subtree first 
        // and later deleting subtree of treeItems
        ((BranchItem*)treeItem)->unlinkBranchContainer();
    }
}

void Container::init()
{
    contentType = Undefined;
    contentObj = nullptr;

    layout = Horizontal;

    // Testing only:
    QPointF p; // = QPointF(qrand() % 100, qrand() % 100);
    qreal w = qrand() % 200 + 20;
    qreal h = qrand() % 200 + 20;
    setRect(QRectF(p.x(), p.y(), w, h));
    show();
}

void Container::copy(Container *other)  // FIXME-0 not implemented
{
}


void Container::setContentType(const ContentType &ctype)
{
    contentType = ctype;
}

Container::ContentType Container::getContentType()
{
    return contentType;
}

void Container::setContent(MapObj *mapObj)
{
    contentObj = mapObj;
    contentType = MapObject;
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

void Container::setTreeItem(TreeItem *ti) { treeItem = ti; }

TreeItem *Container::getTreeItem() const { return treeItem; }

void Container::addContainer(Container *c)
{
    qDebug() << "Adding container " << c->getName() << c << " to " << name << this;
    c->setParentItem(this);

    // adjust dimensions of container depending on children
}

void Container::reposition()
{
    qDebug() << "Container::reposition of container " << name << this;

    QRectF r = rect();

    // Repositioning is done recursively. First the size sizes of subcontainers are calculated
    // Then the subcontainers are positioned.
    //
    // a) calc sizes of subcontainers based on their layouts
    
    if (contentType == Containers  ) {
        qDebug() << " * Content type: Container" << contentType;
        if (childItems().count() == 0) {
            qDebug() << " * Setting r to minimal size";
            r.setWidth(10); // FIXME-0 Minimum size for testing only
            r.setHeight(10);
            setRect(r);
        }
    } else if (contentType == MapObject) {

        qDebug() << " * Content type: MapObj" << contentType;
        qDebug() << " * children: " << childItems().count();
        r.setWidth(contentObj->getBBox().width());
        r.setHeight(contentObj->getBBox().height());
        setRect(r);
        qDebug() << " * Setting r=" << r;
        return;
    } else {
        qWarning() << " * Content type: Unknown";
    }

    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        // FIXME-0 don't call, if c has no children, otherwise sizes become 0
        c = (Container*) child; // FIXME-0    why the cast here????
        qDebug() << " * Repositioning childItem " << child << " of type " << c->contentType;
        if (c->contentType == Containers || c->contentType == MapObject) {
            c->reposition();
        } else
            qDebug() << " * MapObj, skipping Repositioning childItem due to type " << contentType;

    }

    // b) Align my own containers

    // The children Container is empty, if there are no children branches
    // No repositioning of children required then, of course.
    if (childItems().count() == 0) return;

    switch (layout) {
        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;
                qreal h;
                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    h = c->rect().height();
                    h_max = (h_max < h) ? h : h_max;
                    w_total += c->rect().width();
                }

                qreal x = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    c->setPos (x, (h_max - c->rect().height() ) / 2);
                    x += c->rect().width();
                }
                r.setWidth(w_total);
                r.setHeight(h_max);
            }
            setRect(r);
            qDebug() << " * Horizontal layout - Setting rect of " << name << this << " to " << r;
            break;
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;
                // Calc total height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    w = c->rect().width();
                    w_max = (w_max < w) ? w : w_max;
                    h_total += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // align centered: 
                    // c->setPos ( (w_max - c->rect().width() ) / 2, y);

                    // Align to left
                    c->setPos (0, y);
                    y += c->rect().height();
                }
                r.setWidth(w_max);
                r.setHeight(h_total);
            }
            setRect(r);
            qDebug() << " * Vertical layout - Setting rect of " << name << this << " to " << r;
            break;
        default:
            break;
    }
}

void Container::setName(const QString &n) {
    // qDebug() << "Container::setName = " <<n;    // FIXME-1 testing
    name = n;
}

QString Container::getName() {
    // qDebug() << "Container::getName = " << name;// FIXME-1 testing
    return name;
}
