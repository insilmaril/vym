#include <QDebug>

#include "container.h"

#include "branchitem.h"

Container::Container(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    //qDebug() << "* Const Container begin this = " << this << ";
    init();
}

Container::~Container()
{
    //qDebug() << "* Destr Container" << getName()<< this;
}

void Container::copy(Container *other)
{
    type = other->type;

    originalPos = other->originalPos;
    name = other->name;

    orientation = other->orientation;

    layout = other->layout;
    horizontalDirection = other->horizontalDirection;
    verticalAlignment = other->verticalAlignment;
}

void Container::init()
{
    type = Undefined;
    layout = Horizontal;

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

void Container::setOrientation(const Orientation &m)
{
    orientation = m;
}

Container::Orientation Container::getOrientation()
{
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

bool Container::isFloating()
{
    Container *pc = parentContainer();
    if (pc && pc->hasFloatingLayout())
        return true;
    else
        return false;
}

bool Container::hasFloatingLayout() {
    if (layout == FloatingBounded || layout == FloatingFree)
        return true;
    else
        return false;
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
    /*
    qDebug() << QString("#### Reposition of %1").arg(getName()) 
        << "Layout: " << layout 
        << " direction: " << horizontalDirection
        << "orientation: " << orientation;
        */

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
        setRect(r);
        return;
    }

    switch (layout) {
        case FloatingBounded: 
                {
                    // Calc bbox of all children to prepare calculating rect()
                    QRectF rc;
                    if (childItems().count() > 0) {
                        bool first_iteration = true;

                        // Set initial minima and maxima
                        c = (Container*) childItems().first();
                        rc = mapRectFromItem(c, c->rect());

                        // Consider other children
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;
                            rc = mapRectFromItem(c, c->rect());

                            if (first_iteration) {
                                first_iteration = false;
                                r = rc;
                            } else 
                                r = r.united(rc);
                        }
                    }
                    
                setRect(r);
            }
            break;
        case FloatingFree: 
            setRect(r);
            break;
        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;  // total width of non-floating children
                qreal h;

                bool hasFloatingContent = false;
                
                QRectF c_bbox;  // Current bbox of floating c in my own coord (usually childrenContainer)
                QRectF bbox;    // United bboxes of all floating containers in my own coord

                // Calc max height and total width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    // Special case: containers with floating content always are assumed to 
                    // be at my own origin. Even if we (temporarily) move
                    // them later, so that container is aligned with my left/top border
                    if (c->layout == FloatingBounded) c->setPos(0, 0);

                    c_bbox = mapRectFromItem(c, c->rect());

                    if (c->layout == FloatingBounded) {
                        // Floating does not directly increase max height or sum of widths, 
                        // but build max bbox of floating children
                        if (!hasFloatingContent) {
                            hasFloatingContent = true;
                            bbox = c_bbox;
                        }

                        // Unite bounding boxes of floating children to my own bbox in my own coord
                        bbox = bbox.united(c_bbox);
                    } else {
                        // For width and height we can use the already mapped dimensions
                        h = c_bbox.height();
                        h_max = (h_max < h) ? h : h_max;
                        w_total += c_bbox.width();
                    }
                }

                qreal x;
                qreal w_last;   // last width before adding current container width to bbox later

                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children initially. 
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout != FloatingBounded) {
                        // Non-floating child, consider width and height
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
                    }
                }

                // Set rect to the non-floating containers we have so far
                r.setWidth(w_total);
                r.setHeight(h_max);
                setRect(r);

                if (hasFloatingContent) {
                    // Calculate translation vector t to move *parent* later on
                    // now after regular containers have been positioned
                    // Also enlarge bounding box to maximum of floating and regular content

                    r = r.united(bbox);

                    QPointF t; // Translation vector for all children to move topLeft corner to origin

                    if (r.topLeft().x() < 0) t.setX(-r.topLeft().x());
                    if (r.topLeft().y() < 0) t.setY(-r.topLeft().y());

                    if (t != QPointF()) {
                        // Finally move containers by t
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;
                            c->setPos(c->pos() + t);
                        }

                        r.translate(t);
                    }
                }
            } // Horizontal layout
            setRect(r);
            break;
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;

                bool hasFloatingContent = false;

                QRectF c_bbox;  // Current bbox of floating c in my own coord (usually childrenContainer)
                QRectF bbox;    // United bboxes of all floating containers in my own coord

                // Calc total height and max width
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout == FloatingBounded) c->setPos(0, 0);

                    c_bbox = mapRectFromItem(c, c->rect());

                    if (c->layout == FloatingBounded) {
                        // Floating does not directly increase max height or sum of widths, 
                        // but build max bbox of floating children
                        if (!hasFloatingContent) {
                            hasFloatingContent = true;
                            bbox = c_bbox;
                        }

                        // Unite bounding boxes of floating children to my own bbox in my own coord
                        bbox = bbox.united(c_bbox);
                    } else {
                        // For width and height we can use the already mapped dimensions
                        w = c->rect().width();
                        w_max = (w_max < w) ? w : w_max;
                        h_total += c->rect().height();
                    }
                }

                qreal y = 0;
                qreal y_float;  // y coord of floating content in my coord system to calc bbox later
                qreal h_last;   // last height before adding current container width to bbox later
                // Position children initially
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    if (c->layout != FloatingBounded) {
                        switch (verticalAlignment) {
                            case AlignedLeft:
                                c->setPos (0, y);
                                break;
                            case AlignedRight:
                                c->setPos (w_max - c->rect().width(), y);
                                break;
                            case AlignedCentered:
                                c->setPos ( (w_max - c->rect().width() ) / 2, y);
                                break;
                        }

                        //qDebug() << " - VL Positioning c:" << c->info();
                        y += c->rect().height();
                    } else {
                        // c->layout == FloatingBounded  save position
                        y_float = y;
                    }
                }

                // Set rect to the non-floating containers we have so far
                r.setWidth(w_max);
                r.setHeight(h_total);
                setRect(r);

                if (hasFloatingContent) {
                    // Calculate translation vector t to move *parent* later on
                    // now after regular containers have been positioned
                    // Also enlarge bounding box to maximum of floating and regular content

                    r = r.united(bbox);

                    QPointF t; // Translation vector for all children to move topLeft corner to origin

                    if (r.topLeft().x() < 0) t.setX(-r.topLeft().x());
                    if (r.topLeft().y() < 0) t.setY(-r.topLeft().y());

                    if (t != QPointF()) {
                        // Finally move containers by t
                        foreach (QGraphicsItem *child, childItems()) {
                            c = (Container*) child;
                            c->setPos(c->pos() + t);
                        }

                        r.translate(t);
                    }
                }

            } // Vertical layout
            setRect(r);
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type!";
            break;
    }
}
