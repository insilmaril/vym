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
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
        c->reposition();
    }

    // b) Align my own containers

    // The children Container is empty, if there are no children branches
    // No repositioning of children required then, of course.
    if (childItems().count() == 0) 
    {
        qDebug() << " * no children, could return?";
        setRect(r);
        return;  // FIXME-2 still required to continue, if children are Floating
    }

    switch (layout) {
        case Floating: 
                {
                    // Calc bbox of all children to prepare calculating rect()

                    QPointF tl; // topLeft in scene coord
                    QPointF br; // bottomRight in scene coord
                    QPointF p;
                    QRectF rc;
                    if (childItems().count() > 0) {
                        // Set initial minima and maxima
                        c = (Container*) childItems().first();
                        rc = mapRectFromItem(c, c->rect());

                        tl = rc.topLeft();
                        br = rc.bottomRight();
                        // Consider other children // FIXME-2 skipt 1st one, already done above
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;
                            rc = mapRectFromItem(c, c->rect());
                            p = rc.topLeft();

                            if (p.x() < tl.x()) tl.setX(p.x());
                            if (p.y() < tl.y()) tl.setY(p.y());

                            p = rc.bottomRight();
                            if (p.x() > br.x()) br.setX(p.x());
                            if (p.y() > br.y()) br.setY(p.y());
                        }
                    }
                    
                r.setTopLeft(tl);
                r.setBottomRight(br);

                setRect(r);
                //setPos(QPointF(0, 0));    // FIXME-2 needed?
                qDebug() << " * Layout Floating end of " << info() << " r=" << r;
            }
            break;
        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;  // total width of non-floating children
                qreal h;

                bool hasFloatingContent = false;
                QPointF p_float;
                
                QRectF c_bbox;
                QRectF ctr;
                bool first_iteration = true;
                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    c_bbox = mapRectFromItem(c, c->rect());

                    if (first_iteration) {  // FIXME-2 not needed IMHO
                        // Initial assignment
                        first_iteration = false;
                        ctr = c_bbox;
                    }

                    if (c->layout == Floating) {
                        // Floating does not directly increase max height or sum of widths, 
                        // but build max bbox of floating children
                        if (!hasFloatingContent) 
                            hasFloatingContent = true;

                        // Consider bounding boxes of floating children to my own ctr
                        if (c_bbox.left() < ctr.left()) ctr.setLeft(c_bbox.left());
                        if (c_bbox.top()  < ctr.top())  ctr.setTop(c_bbox.top());
                        if (c_bbox.right()  > ctr.right()) ctr.setRight(c_bbox.right());
                        if (c_bbox.bottom() > ctr.bottom()) ctr.setBottom(c_bbox.bottom());
                    } else {
                        h = c_bbox.height();
                        h_max = (h_max < h) ? h : h_max;
                        w_total += c_bbox.width();
                    }
                }

                qreal x;
                qreal x_float;  // x coord of floating content in my coord system to calc bbox later
                qreal w_last;   // last width before adding current container width to bbox later

                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children initially. 
                // Floating children might be left or above of my current origin!
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout != Floating) {
                        w_last = c->rect().width();

                        if (horizontalDirection == LeftToRight)
                        {
                            c->setPos (x, (h_max - c->rect().height() ) / 2 );
                            x += w_last;
                        } else
                        {
                            c->setPos (x - c->rect().width(), (h_max - c->rect().height() ) / 2);
                            x -= w_last;
                        }
                    } else {
                        // c->layout == Floating: Save position and rectangle
                        c->setPos (x, 0);
                        x_float = x;
                    }
                }

                // Set rect to the non-floating containers we have so far
                r.setWidth(w_total);
                r.setHeight(h_max);
                setRect(r);

                if (hasFloatingContent) {
                    // Calculate translation vector ct to move *parent* later on
                    // now after regular containers have been positioned
                    // Also enlarge bounding box to maximum of floating and regular content
                    qDebug() << "   - HL floating content r=" << r << "r.united=" << r.united(ctr);

                    r = r.united(ctr);

                    QPointF t; // Translation vector for all children to move topLeft cornert to origin

                    if (r.topLeft().x() < 0) t.setX(-r.topLeft().x());
                    if (r.topLeft().y() < 0) t.setY(-r.topLeft().y());

                    if (t != QPointF()) {
                        // Finally move containers by ct    // FIXME-0 should we move mapCenter or will this cause flickering?
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;
                            qDebug() << " -HL moving c: " << c->info() << " by t=" << t;
                            c->setPos(c->pos() + t);
                        }

                        r.translate(t);
                    }
                }
            } // Horizontal layout
            setRect(r);
            break;
        case Vertical: {    // FIXME-2 floating content is untested
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;

                bool hasFloatingContent = false;
                QPointF p_float;

                QRectF ctr;
                // Calc total height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout == Floating) {
                        if (!hasFloatingContent) {
                            // Initial assignment
                            ctr = mapRectFromItem(c, c->rect());
                            hasFloatingContent = true;
                        }

                        // Consider bounding boxes of floating children to my own ctr
                        if (c->rect().left() < ctr.left()) ctr.setLeft(c->rect().left());
                        if (c->rect().top()  < ctr.top())  ctr.setTop(c->rect().top());
                        if (c->rect().right()  > ctr.right()) ctr.setRight(c->rect().right());
                        if (c->rect().bottom() > ctr.bottom()) ctr.setBottom(c->rect().bottom());
                    } else {
                        w = c->rect().width();
                        w_max = (w_max < w) ? w : w_max;
                        h_total += c->rect().height();
                        qDebug() << " - LV c: " << c->info() << "  h_total=" << h_total;
                    }
                }

                qreal y = 0;
                qreal y_float;  // y coord of floating content in my coord system to calc bbox later
                qreal h_last;   // last height before adding current container width to bbox later
                // Position children
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout != Floating) {
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

                        qDebug() << " - LV Positioning c:" << c->info();
                        y += c->rect().height();
                    } else {
                        // c->layout == Floating  save position
                        y_float = y;
                    }
                }

                if (hasFloatingContent) {
                    // Calculate translation vector ct to move *parent* later on
                    // now after regular containers have been positioned
                    // Also enlarge bounding box to maximum of floating and regular content
                    //qDebug() << "   - floating content  ctr=" << ctr << "x_float=" << x_float;
                    qDebug() << " # LV ctr=" << ctr << "h_total=" << h_total << "w_max=" <<w_max;
                    if (ctr.left() < 0) {
                        //ct.setX(-ctr.left());
                        //w_max += ct.x();
                        qDebug() << "  ## left < 0";
                    }
                    if (ctr.top() < 0) {
                        //ct.setY(-ctr.top() - y_float);
                        //h_total += ct.y();
                        qDebug() << "  ## top < 0";
                    }
                    if (ctr.right() > w_max) {
                        w_max = ctr.width(); 
                        qDebug() << "  ## right > w_max";
                    }
                    if (ctr.height() > h_total) {
                        h_total += ctr.height() - y_float -h_last;
                    }

                    // Finally move containers by ct    // FIXME-0 should we move mapCenter or will this cause flickering?
                    foreach (QGraphicsItem *child, childItems()) {
                        c = (Container*) child;
                        c->setPos(c->pos());
                    }
                }

                r.setWidth(w_max);
                r.setHeight(h_total);
            } // Vertical layout
            setRect(r);
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type!";
            break;
    }
}
