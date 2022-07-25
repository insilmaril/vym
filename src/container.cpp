#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "misc.h"

Container::Container(QGraphicsItem *parent) : QGraphicsRectItem(parent)
{
    //qDebug() << "* Const Container " << this;
    init();
}

Container::~Container()
{
    //qDebug() << "Destr Container" << info() << this;
}

void Container::copy(Container *other)
{
    type = other->type;

    originalPos = other->originalPos;
    name = other->name;

    layout = other->layout;
    horizontalDirection = other->horizontalDirection;
    horizontalAlignment = other->horizontalAlignment;
}

void Container::init()
{
    type = Undefined;
    layout = Horizontal;

    // subcontainers usually may influence position
    // Only mapCenters will stay where they are
    movableByFloats = true; 

    minimumWidth = 0;

    horizontalDirection = LeftToRight;

    // Not visible usually
    setBrush(Qt::NoBrush);
    setPen(Qt::NoPen);

    // But children are shown
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
        case Branch:
            t = "Branch";
            break;
        case BranchCollection:
            t = "BranchCollection";
            break;
        case FloatingContent:
            t = "FloatingContent";
            break;
        case Heading:
            t = "Heading";
            break;
        case Image:
            t = "Image";
            break;
        case ImageCollection:
            t = "ImagesCollection";
            break;
        case InnerContent:
            t = "InnerContent";
            break;
        case Link:
            t = "Link";
            break;
        case Ornaments:
            t = "Ornaments";
            break;
        case TmpParent:
            t = "TmpParent";
            break;
        case Undefined:
            t = "Undefined";
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
        QString(" Layout: %1").arg(getLayoutString()) +
        QString(" scenePos: %1").arg(qpointFToString(scenePos())) +
        QString(" pos: %1").arg(qpointFToString(pos())) +
        QString(" rect: (%1, %2  %3x%4)").arg(rect().x()).arg(rect().y()).arg(rect().width()).arg(rect().height());
}

void Container::setLayout(const Layout &l)
{
    layout = l;
}

Container::Layout Container::getLayout()
{
    return layout;
}

QString Container::getLayoutString()
{
    QString r;
    switch (layout) {
        case Horizontal:
            r = "Horizontal";
            break;
        case Vertical:
            r = "Vertical";
            break;
        case BoundingFloats:
            r = "BoundingFloats";
            break;
        case FloatingBounded:
            r = "FloatingBounded";
            break;
        case FloatingFree:
            r = "FloatingFree";
            break;
        default:
            r = QString("Unknown: %1").arg(layout);
            qWarning () << "Container::getLayoutString unknown layout";
    }
    return r;
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

void Container::setMovableByFloats(bool movable)
{
    movableByFloats = movable;
}

void Container::setHorizontalDirection(const HorizontalDirection &hdir)
{
    horizontalDirection = hdir;
}

Container::HorizontalDirection Container::getHorizontalDirection()
{
    return horizontalDirection;
}

void Container::setHorizontalAlignment(const HorizontalAlignment &a)
{
    horizontalAlignment = a;
}

bool Container::isVisibleContainer()
{
    return visible;
}

void Container::setVisibility(bool v)
{
    visible = v;
}

void Container::addContainer(Container *c)
{
    c->setParentItem(this);
}

void Container::setAnimation(const AnimPoint &ap) { animatedPos = ap; }

void Container::stopAnimation()
{
    animatedPos.stop();
}

bool Container::animate()
{
    animatedPos.animate();
    if (animatedPos.isAnimated()) {
        setPos(animatedPos);
        return true;
    }
    return false;
}

bool Container::isAnimated()
{
    return animatedPos.isAnimated();
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

Container* Container::parentContainer() 
{
    return (Container*)parentItem();
}

void Container::setPos(QPointF p)
{
    if (animatedPos.isAnimated()) 
        QGraphicsItem::setPos(animatedPos);
    else
        QGraphicsItem::setPos(p);
}

void Container::setPos(qreal x, qreal y)
{
    setPos(QPointF(x, y));
}

void Container::setOriginalPos()    // FIXME-3 Only used for BranchContainer and ImageContainer -> maybe move to LinkableContainer?
{
    originalPos = pos();
}

QPointF Container::getOriginalPos()
{
    return originalPos;
}

void Container::reposition()    // FIXME-0 rotated mapcenters: rect of branchContainer needs to be adapted (translated) to contain the innerContainer. (Works for non-mapcenter branches)
{
    // qDebug() << QString("#### Reposition of %1").arg(getName()) << "Layout: " << getLayoutString() << horizontalDirection;

    QRectF r;

    // Repositioning is done recursively: 
    // First the size sizes of subcontainers are calculated, 
    // Container::reposition is overloaded, so for example HeadingContainer 
    // will return correct size of heading!
    //
    // Then the subcontainers are positioned.
    //
    // a) Do we have any chilrden after all?
    if (childItems().count() == 0)
    {
        setRect(r);
        return;
    }

    // b) calc sizes of subcontainers based on their layouts
    //    (overloaded, not there e.g. for HeadingContainer!)

    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
        c->reposition();
    }

    // c) Align my own containers

    switch (layout) {
        case BoundingFloats: 
            {
                // BoundingFloats is special case: 
                // Only used for innerContainer
                // First child container is ornamentsContainer,
                // next children are imagesContainer and branchesContainer

                if (childItems().count() > 3 ) {
                    qWarning() << "Container::reposition " << info();
                    qWarning() << "Wrong number of children containers: "  << childItems().count();
                    foreach (QGraphicsItem *child, childItems()) {
                        Container *c = (Container*) child;
                        qDebug() << "  " << c->info();
                    }

                    return;
                }

                // Calc space required
                QRectF c_bbox;  // bbox of container in my own coord 
                QRectF bbox;    // United bboxes 

                foreach (QGraphicsItem *child, childItems()) {
                    Container *c = (Container*) child;
                    c_bbox = mapRectFromItem(c, c->rect());
                    bbox = bbox.united(c_bbox);
                }

                // Translate, so that total bbox and moves 
                // to origin, along with contents
                QPointF t = - QPointF(bbox.topLeft().x(), bbox.topLeft().y());
                bbox.translate(t);
                foreach (QGraphicsItem *child, childItems()) {
                    Container *c = (Container*) child;
                    c->setPos(c->pos() + t);
                }

                setRect(bbox);

            } // BoundingFloats layout
            break;

        case FloatingBounded: 
            {
                // Calc bbox of all children to prepare calculating rect()
                QRectF c_bbox;
                if (childItems().count() > 0) {
                    bool first_iteration = true;

                    // Consider other children
                    foreach (QGraphicsItem *child, childItems()) {
                        c = (Container*) child;
                        c_bbox = mapRectFromItem(c, c->rect());

                        if (first_iteration) {
                            first_iteration = false;
                            r = c_bbox;
                        } else 
                            r = r.united(c_bbox);
                    }
                }
                    
                setRect(r);
            }
            break;

        case FloatingFree: 
            setRect(r); // Empty rectangle
            break;

        case Horizontal: {
                qreal h_max = 0;
                qreal w_total = 0;  // total width of non-floating children
                qreal h;

                QRectF c_bbox;  // bbox of subcontainer c in my own coord 
                QRectF bbox;    // United bboxes of all all floating and sum of 
                                // regular containers in my own coord

                // Calc space required
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    // For floatingBounded containers and calculation of 
                    // heights and widths we need move subcontainers 
                    // to origin first, because the translation is calculated by
                    // their bounding boxes, not position
                    c->setPos(0, 0);    // FIXME-0 needed?

                    c_bbox = mapRectFromItem(c, c->rect());

                    if (c->rotation() != 0) {
                        // Move rotated container, so that upperLeft corner is in (0,0)
                        // Required to get correct pos bbox of rotated containers.
                        c_bbox.moveTopLeft(QPointF(0,0));
                    }

                    // For width and height we can use the already mapped dimensions
                    h = c_bbox.height();
                    h_max = (h_max < h) ? h : h_max;
                    w_total += c_bbox.width();
                }

                // bbox so far only considers floating subcontainers. Extend by 
                // regular ones, where rectangle has w_total h_max
                bbox = QRectF(0, 0, w_total, h_max);
                
                qreal x;
                qreal w_last;   // last width before adding current container width to bbox later

                horizontalDirection == LeftToRight ? x = 0 : x = w_total;

                // Position children initially.
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    //if (c->layout != FloatingFree) {
                    if (true) {     // FIXME-0 
                        // Non-floating child, consider width and height and 
                        // align horizontally
                        w_last = c->rect().width();
                        qreal y = 0;
    
                        if (movableByFloats) {
                            // Usally c may be moved, if it has a bbox above/left 
                            // of origin due to some floating containers
                            // Exception are mapCenters, which need to "stick" to their 
                            // scene position and must not be moved by
                            // position (or bboxes) of main branches
                            y = (h_max - c->rect().height() ) / 2;  

                            if (horizontalDirection == LeftToRight)
                            {
                                if (c->rotation() == 0)
                                    c->setPos (x, y);
                                else {
                                    // move rotated container to my origin
                                    c->setPos (- mapRectFromItem(c, c->rect()).topLeft());
                                }
                                x += w_last;
                            } else
                            {
                                if (c->rotation() == 0)
                                    c->setPos (x - c->rect().width(), y);
                                else {
                                    // move rotated container to my origin
                                    c->setPos (- mapRectFromItem(c, c->rect()).topLeft());
                                }
                                x -= w_last;
                            }
                        } // movableByFloats
                    } // Non-floating children
                } 
                r = bbox;

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
                        switch (horizontalAlignment) {
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

                    // Translation vector for all children to move topLeft corner to origin
                    QPointF t;

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
