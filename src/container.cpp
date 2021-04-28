#include <QDebug>

#include "container.h"

#include "treeitem.h"

Container::Container(QGraphicsItem *parent, TreeItem *ti) : QGraphicsRectItem(parent)
{
    qDebug() << "Const Container (this,ti)=(" << this << "," << ti << ")";
    treeItem = ti;
    init();
}

Container::~Container()
{
    qDebug() << "Destr Container " << this << name;
    foreach (Container *c, childrenList) {
        // Avoid that QGraphicsScene deletes children
        c->setParentItem(NULL);

        // FIXME-0 check if there still is a link to a TreeItem
        // then this needs to be unlinked first
        delete c;
    }

}

void Container::init()
{
    QPointF p; // = QPointF(qrand() % 100, qrand() % 100);
    qreal w = qrand() % 200 + 20;
    qreal h = qrand() % 200 + 20;
    setRect(QRectF(p.x(), p.y(), w, h));
    show();

}

void Container::copy(Container *other)  // FIXME-0 not implemented
{
}

void Container::setTreeItem(TreeItem *ti) { treeItem = ti; }

TreeItem *Container::getTreeItem() const { return treeItem; }

void Container::addContainer(Container *c)
{
    c->setParentItem(this);
    childrenList.append(c);

    // adjust dimensions of container depending on children
}

void Container::reposition()
{
    qDebug() << "Container::reposition";

    // Align LeftToRight, vertical: Center

    qreal y = 0;
    // Calc max height
    foreach (Container *c, childrenList) {
        qreal h = c->rect().height();
        y = (y < h) ? h : y;
    }

    qreal x = 0;
    // Position children
    foreach (Container *c, childrenList) {
        c->setPos (x, (y - c->rect().height() ) / 2);
        x += c->rect().width();

        qreal h = c->rect().height();
        y = (y < h) ? h : y;
    }
    QRectF r = rect();
    r.setWidth(x);
    r.setHeight(y);
    setRect(r);
}

void Container::setName(const QString &n) { name = n; }

QString Container::getName() { return name; }



