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
        c = (Container*) child;
        if (c->contentType == Containers || c->contentType == MapObject) {
            qDebug() << " * Repositioning childItem " << child << " of type " << c->contentType;
            c->reposition();
        } else
            qDebug() << " * MapObj, skipping Repositioning childItem due to type " << contentType;

    }

    // b) align subcontainers within their parents

    // c) Align my own containers

    if (childItems().count() == 0) return;  // FIXME-0   never will be 0...

    switch (layout) {
        case Horizontal: {
                qreal max_h = 0;
                qreal max_w = 0;
                // Calc max height and width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    qreal h = c->rect().height();
                    max_h = (max_h < h) ? h : max_h;
                    max_w += c->rect().width();
                }

                qreal x = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    c->setPos (x, (max_h - c->rect().height() ) / 2);
                    x += c->rect().width();
                }
                r.setWidth(max_w);
                r.setHeight(max_h);
            }
            setRect(r);
            qDebug() << " * Horizontal layout - Setting rect of " << name << this << " to " << r;
            break;
        case Vertical: {
                qreal max_h = 0;
                qreal max_w = 0;
                // Calc height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    qreal w = c->rect().width();
                    max_w = (max_w < w) ? w : max_w;
                    max_h += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // align centered: 
                    // c->setPos ( (max_w - c->rect().width() ) / 2, y);

                    // Align to left
                    c->setPos (0, y);
                    y += c->rect().height();
                }
                r.setWidth(max_w);
                r.setHeight(max_h);
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
