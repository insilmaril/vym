#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "mapobj.h" // FIXME-2 needed?
#include "misc.h"   // FIXME-2 debugging only

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
    type = Undefined;
    layout = Horizontal;

    orientationMode = Auto;
    orientation = UndefinedOrientation;

    show();
}

Container::ContainerType Container::containerType()
{
    return type;
}

void Container::setType(const Container::ContainerType &t)
{
    type = t;
}

void Container::setName(const QString &n)   // FIXME-4 debugging only
{
    name = n;
}

QString Container::getName()    // FIXME-4 debugging only
{
    QString t;
    switch (type) {
        case Undefined:
            t = "Undefined";
            break;
        case TmpParent:
            t = "TmpParent";
            break;
        case FloatingContent:
            t = "FloatingContent";
            break;
        case InnerContent:
            t = "InnerContent";
            break;
        case Children:
            t = "Children";
            break;
        case Branch:
            t = "Branch";
            break;
        case Heading:
            t = "Heading";
            break;
        default:
            t = "Unknown";
            break;
    }
    return QString("[%1]").arg(t);
}

QString Container::info (const QString &prefix)
{
    return prefix +
        getName() +
        QString(" scenePos: (%1, %2)").arg(scenePos().x()).arg(scenePos().y()) + 
        QString(" pos: (%1, %2)").arg(pos().x()).arg(pos().y()) +
        QString(" Layout: %1").arg(layout);
}

void Container::setOrientationMode(const OrientationMode &om)
{
    orientationMode = om;
}

Container::OrientationMode Container::getOrientationMode()
{
    return orientationMode;
}

void Container::setOrientation(const Orientation &m)
{
    orientation = m;
}

Container::Orientation Container::getOrientation()
{
    if (orientationMode == Auto)
    {
        if (pos().x() >= 0)
            return RightOfParent;
        else
            return LeftOfParent;
    } else 
        return orientation;
}

void Container::setLayoutType(const LayoutType &ltype)
{
    layout = ltype;
}

Container::LayoutType Container::getLayoutType()
{
    return layout;
}

void Container::setHorizontalDirection(const HorizontalDirection &hdir)
{
    horizontalDirection = hdir;
}

Container::HorizontalDirection Container::getHorizontalDirection()
{
    return horizontalDirection;
}

void Container::setVerticalAlignment(const VerticalAlignment &a)
{
    verticalAlignment = a;
}

bool Container::isFloating()
{
    Container *pc = parentContainer();
    if (pc && pc->getLayoutType() == Floating)
        return true;
    else
        return false;
}

void Container::addContainer(Container *c)
{
    c->setParentItem(this);
}

QVariant Container::itemChange(GraphicsItemChange change, const QVariant &value)    // FIXME-2 needed?
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

void Container::setOrgPos()     // FIXME-2 Only used for BranchContainer and ImageContainer
{
    originalPos = pos();
}

QPointF Container::orgPos()     // Only used for BranchContainer and ImageContainer
{
    return originalPos;
}

Container* Container::parentContainer() 
{
    return (Container*)parentItem();
}

void Container::reposition()
{
    // qDebug() << QString("Container::reposition of %1 container").arg(info()) << this;

    QRectF r;

    // Repositioning is done recursively: 
    // First the size sizes of subcontainers are calculated, 
    // Container::reposition is overloaded, so for example HeadingContainer 
    // will return correct size of heading!
    //
    // Then the subcontainers are positioned.
    //
    // a) calc sizes of subcontainers based on their layouts 
    //    (overloaded, not there e.g. for HeadingContainer!)

    if (childItems().count() == 0 && layout != BFloat) {   // FIXME-0 check!  (BFloat unused...)
        // qDebug() << " * Setting r to minimal size. r =" << rect();
        r.setWidth(0);
        r.setHeight(0);
        setRect(r);
        //return;
    }

    // FIXME-2 testing only, rotate children
    /*         move to BranchContainer
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
    ct = QPointF (0, 0);
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
//        if (c->type != Undefined && c->type != Heading) {
            c->reposition();

    /*
            // If child container has floating layout, consider it's translation 
            // vector for my own children
            if (c->getLayoutType() == BFloat)
            {
                qDebug() << " # Adding floating ct= " << c->ct << " of " << c->getName() << " to " << getName();
                ct += c->ct;
            }
    */
//        }
    }

    // b) Align my own containers

    // The children Container is empty, if there are no children branches
    // No repositioning of children required then, of course.
    if (childItems().count() == 0) 
    {
        qDebug() << " * no children, could return?";
        //return;  // FIXME-2 still required to continue, if children are BoundedFloat
    }

    switch (layout) {
        case Floating: 
                qDebug() << " * Layout Floating begin of " << info() << "Children.count= " << childItems().count() << this ;

                {
                    /////////////////////////////////////////////////// fix from here
                    // Calc bbox of all children to prepare calculating rect()

                    QPointF tl; // topLeft in scene coord
                    QPointF br; // bottomRight in scene coord
                    QPointF p;
                    if (childItems().count() > 0) {
                        // Set initial minima and maxima
                        c = (Container*) childItems().first();
                        tl = c->mapToScene(c->rect().topLeft());
                        br = c->mapToScene(c->rect().bottomRight());

                        // Consider other children
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;

                            p = c->mapToScene(c->rect().topLeft());

                            if (p.x() < tl.x()) tl.setX(p.x());
                            if (p.y() < tl.y()) tl.setY(p.y());

                            p = c->mapToScene(c->rect().bottomRight());
                            if (p.x() > br.x()) br.setX(p.x());
                            if (p.y() > br.y()) br.setY(p.y());


                            qDebug() << "   - c:" << c->info() << 
                                "c->r=" << c->rect() << 
                                "tl=" <<
                                qpointFToString(tl) <<
                                "br=" <<
                                qpointFToString(br);
                        }
                    }
                    
                r.setTopLeft(mapFromScene(tl));
                r.setBottomRight(mapFromScene(br));

                setRect(r);
                qDebug() << " * Layout Floating end of " << info() << " r=" << r << " ct=" << ct;
            }
            break;
        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;  // total width of non-floating children
                qreal h;

                bool hasFloatingContent = false;
                ct = QPointF(); // Reset translation vector
                ctr = QRectF();
                QPointF p_float;
                
                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    if (c->layout == Floating) {
                        if (!hasFloatingContent)
                            // Initial assignment
                            ctr = c->rect();
                        hasFloatingContent = true;

                        // Consider bounding boxes of floating children to my own ctr
                        if (c->rect().left() < ctr.left()) ctr.setLeft(c->rect().left());
                        if (c->rect().top()  < ctr.top())  ctr.setTop(c->rect().top());
                        if (c->rect().right()  > ctr.right()) ctr.setRight(c->rect().right());
                        if (c->rect().bottom() > ctr.bottom()) ctr.setBottom(c->rect().bottom());
                    } else {
                        h = c->rect().height();
                        h_max = (h_max < h) ? h : h_max;
                        w_total += c->rect().width();
                        //qDebug() << "    - h_max=" << h_max << "h=" << h << " c:" << c->info();;
                    }
                }

                qreal x;
                qreal x_float;  // x coord of floating content in my coord system
                QRectF r_float; // rectangle of floating content in children coord system

                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout != Floating) {
                        // Order from left to right
                        if (horizontalDirection == LeftToRight)
                        {
                            c->setPos (x, (h_max - c->rect().height() ) / 2);
                            x += c->rect().width();
                        } else
                        {
                            c->setPos (x - c->rect().width(), (h_max - c->rect().height() ) / 2);
                            x -= c->rect().width();
                        }
                    } else {
                        // c->layout == Floating: Save position and rectangle
                        x_float = x;
                        r_float = c->ctr;
                    }
                }
                if (c->layout == Floating) {
                    // width is what???ß FIXME-0
                    r.setWidth(w_total + ct.x());   // FIXME-2 adding ct here: testing only
                    r.setHeight(h_max + ct.y());
                } else {
                    r.setWidth(w_total);
                    r.setHeight(h_max);
                }
            }
            setRect(r);
            qDebug() << " * Horizontal layout - Setting rect of " << info() << " to " << r << "ctr=" << ctr;
            break;
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;

                // child is aligned further down, just go with sum and max
                // Calc total height and max width for stacked children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    // child is aligned further down, just go with sum and max
                    w = c->rect().width();
                    w_max = (w_max < w) ? w : w_max;    // FIXME-2 use max function
                    h_total += c->rect().height();
                }

                qreal y = 0;
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    switch (verticalAlignment) {
                        case Left:
                            c->setPos (0, y);
                            break;
                        case Right:
                            c->setPos (w_max - c->rect().width(), y);
                            break;
                        case Centered:
                            c->setPos ( (w_max - c->rect().width() ) / 2, y);
                            break;
                    }

                    y += c->rect().height();
                }

                r.setWidth(w_max);
                r.setHeight(h_total);
            }
            setRect(r);
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type!";
            break;
    }
}
