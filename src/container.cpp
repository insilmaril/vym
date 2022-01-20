#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "mapobj.h"

Container::Container(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    //qDebug() << "* Const Container begin this = " << this << ";
    init();
}

Container::~Container()
{
    //qDebug() << "* Destr Container" << name << this;
}

void Container::init()
{
    type = Collection;

    layout = Horizontal;
    boundsType = Bounded;

    show();
}

Container::ContainerType Container::containerType()
{
    return type;
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

void Container::setHorizontalDirection(const HorizontalDirection &hdir)
{
    horizontalDirection = hdir;
}

void Container::addContainer(Container *c)
{
    c->setParentItem(this);
}

QVariant Container::itemChange(GraphicsItemChange change, const QVariant &value)
{
    //qDebug() << "Container::itemChange of " << this << ": " << change << value;
    /*
    if (change == ItemPositionChange && scene()) {
        // value is the new position.
        QPointF newPos = value.toPointF();
        QRectF rect = scene()->sceneRect();
        if (!rect.contains(newPos)) {
            // Keep the item inside the scene rect.
            newPos.setX(qMin(rect.right(), qMax(newPos.x(), rect.left())));
            newPos.setY(qMin(rect.bottom(), qMax(newPos.y(), rect.top())));
            return newPos;
        }
    }
    */
    return  QGraphicsItem::itemChange(change, value);
}

void Container::reposition()
{
    //qDebug() << QString("Container::reposition of %1 container  (Container: %2)  Layout: %3").arg(name).arg(type).arg(layout) << this;

    QRectF r = rect();

    // Repositioning is done recursively: 
    // First the size sizes of subcontainers are calculated, 
    // Container::reposition is overloaded, so for example HeadingContainer 
    // will return correct size
    //
    // Then the subcontainers are positioned.
    //
    // a) calc sizes of subcontainers based on their layouts 
    //    (overloaded, not there e.g. for HeadingContainer!)

    if (childItems().count() == 0) {
        //qDebug() << " * Setting r to minimal size rect=" << rect();
        r.setWidth(0);
        r.setHeight(0);
        setRect(r);
        return;
    }

    // FIXME-1 testing only, rotate children
    /*
    Container *pc = (Container*)parentItem();
    if (pc) {
        TreeItem *ti = pc->getTreeItem();
        if (ti && ti->getHeadingPlain() =="rot")
        {
            qDebug() << " * Setting rot";
            setRotation(20);
        }
    }
    */

    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
//        if (c->type != Undefined && c->type != Heading) {
            c->reposition();
//        }
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

                qreal x;
                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    // Align from left to right
                    if (horizontalDirection == LeftToRight)
                    {
                        c->setPos (x, (h_max - c->rect().height() ) / 2);
                        x += c->rect().width();
                    } else
                    {
                        c->setPos (x - c->rect().width(), (h_max - c->rect().height() ) / 2);
                        x -= c->rect().width();
                    }
                }
                r.setWidth(w_total);
                r.setHeight(h_max);
            }
            setRect(r);
            //qDebug() << " * Horizontal layout - Setting rect of " << name << this << " to " << r;
            break;
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;
                // Calc total height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // Only consider size, if boundsType is Bounded or BoundedFloat:
                        if (c->boundsType != FreeFloat) {
                        w = c->rect().width();
                        w_max = (w_max < w) ? w : w_max;
                        h_total += c->rect().height();
                    }
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // Only position container here, if boundsType is no *Float:
                    if (c->boundsType == Bounded) {
                        if (horizontalDirection == RightToLeft)
                            // Align to left
                            c->setPos (0, y);
                        else
                            // Align to right
                            c->setPos (w_max - c->rect().width(), y);

                            // Align centered (unused): 
                            // c->setPos ( (w_max - c->rect().width() ) / 2, y);

                        y += c->rect().height();
                    } else {
                        //qDebug() << "C:reposition   pos=" << c->pos() << " c=" << c;
                        //c->setPos (-50,-50); // FIXME-2 testing only
                    }    
                    

                }
                r.setWidth(w_max);
                r.setHeight(h_total);
            }
            setRect(r);
            break;
        default:
            break;
    }
}
