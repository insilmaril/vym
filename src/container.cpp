#include <QDebug>

#include "container.h"

#include "treeitem.h"

Container::Container(QGraphicsItem *parent, TreeItem *ti) : QGraphicsRectItem(parent)
{
    qDebug() << "* Const Container this = " << this << "  branchitem = " << ti;
    treeItem = ti;
    init();
}

Container::~Container()
{
    QString h;
    if (treeItem) h = treeItem->getHeadingPlain();
    qDebug() << "* Destr Container " << this << name << h;

    foreach (Container *c, childrenList) {
        // Avoid that QGraphicsScene deletes children
        c->setParentItem(NULL);

        // FIXME-0 check if there still is a link to a TreeItem
        // then this needs to be unlinked first
        //if (c->contType != Containers) 
        //    delete c;   // FIXME-0 children container is not deleted at all, needs tob done in BranchItem?!!!
    }
}

void Container::init()
{
    contType = Undefined;

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
    contType = ctype;
}

Container::ContentType Container::contentType()
{
    return contType;
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

void Container::setTreeItem(TreeItem *ti) { treeItem = ti; }

TreeItem *Container::getTreeItem() const { return treeItem; }

int  Container::subContainerCount()
{
    return childrenList.count();
}

void Container::addContainer(Container *c)
{
    qDebug() << "Adding container " << c->getName() << c << " to " << name << this;
    c->setParentItem(this);
    childrenList.append(c);

    // adjust dimensions of container depending on children
}

void Container::reposition()
{
    qDebug() << "Container::reposition of " << name << this;

    QRectF r = rect();

    // a) calc sizes of subcontainers based on their layouts
    
    if (contType == Containers  ) {
        if (subContainerCount() == 0) {
            r.setWidth(10); // FIXME-0 testing only
            r.setHeight(10);
            setRect(r);
        }
    }

    foreach (Container *c, childrenList) {
        // FIXME-0 don't call, if c has no children, otherwise sizes become 0
        if (c->contType == Containers) {
            c->reposition();
        }
    }

    // b) align subcontainers within their parents

    // c) Align my own containers

    if (childrenList.count() == 0) return;

    switch (layout) {
        case Horizontal: {
                qreal max_h = 0;
                qreal max_w = 0;
                // Calc max height and width
                foreach (Container *c, childrenList) {
                    qreal h = c->rect().height();
                    max_h = (max_h < h) ? h : max_h;
                    max_w += c->rect().width();
                }

                qreal x = 0;
                // Position children
                foreach (Container *c, childrenList) {
                    c->setPos (x, (max_h - c->rect().height() ) / 2);
                    x += c->rect().width();
                }
                r.setWidth(max_w);
                r.setHeight(max_h);
            }
            setRect(r);
            qDebug() << "Horizontal layout - Setting rect of " << name << this << " to " << r;
            break;
        case Vertical: {
                qreal max_h = 0;
                qreal max_w = 0;
                // Calc height and max width
                foreach (Container *c, childrenList) {
                    qreal w = c->rect().width();
                    max_w = (max_w < w) ? w : max_w;
                    max_h += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (Container *c, childrenList) {
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
            qDebug() << "Vertical layout - Setting rect of " << name << this << " to " << r;
            break;
        default:
            break;
    }
}

void Container::setName(const QString &n) { name = n; }

QString Container::getName() { return name; }



