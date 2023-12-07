#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "misc.h"

#define qdbg() qDebug().nospace().noquote()

int Container::curIndent = 0; // make instance of curIndent

Container::Container()
{
    //qdbg() << "* Const Container " << this;
    init();
}

Container::~Container()
{
    //qdbg() << "Destr Container" << info() << this;
}

void Container::copy(Container *other)
{
    containerType = other->containerType;

    originalPos = other->originalPos;
    name = other->name;

    layout = other->layout;
    horizontalDirection = other->horizontalDirection;
    horizontalAlignment = other->horizontalAlignment;
}

void Container::init()
{
    containerType = UndefinedType;
    layout = Horizontal;

    // subcontainers usually may influence position
    // Only mapCenters will stay where they are

    minimumWidth = 0;   // FIXME-2 unused

    horizontalDirection = LeftToRight;

    centralContainer = nullptr;

    // Overlay is used for frames: Don't consider for sizes or repositioning
    overlay = false;

    // Initial z-value
    zPos = 0;

    // Not visible usually
    setBrush(Qt::NoBrush);
    setPen(Qt::NoPen);

    // But children are shown
    show();
}

Container::ContainerType Container::getContainerType()
{
    return containerType;
}

int Container::type() const
{
    return Type;
}

void Container::setContainerType(const Container::ContainerType &t)
{
    containerType = t;
}

void Container::setName(const QString &n)   // FIXME-3 debugging only
{
    name = n;
}

QString Container::getName()    // FIXME-3 debugging only
{
    QString t;
    switch (containerType) {
        case Branch:
            t = "Branch";
            break;
        case BranchesContainer:
            t = "BranchesContainer";
            break;
        case FlagCont:
            t = "FlagCont";
            break;
        case FlagRowCont:
            t = "FlagRowCont";
            break;
        case Frame:
            t = "Frame";
            break;
        case Heading:
            t = "Heading";
            break;
        case Image:
            t = "Image";
            break;
        case ImagesContainer:
            t = "ImagesContainer";
            break;
        case InnerContainer:
            t = "InnerContainer";
            break;
        case Link:
            t = "Link";
            break;
        case LinkSpace:
            t = "LinkSpace";
            break;
        case ListContainer:
            t = "ListContainer";
            break;
        case OrnamentsContainer:
            t = "OrnamentsContainer";
            break;
        case OuterContainer:
            t = "OuterContainer";
            break;
        case Selection:
            t = "Selection";
            break;
        case TmpParent:
            t = "TmpParent";
            break;
        case UndefinedType:
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
    return prefix
        + getName()
        // + QString(" zPos: %1").arg(zPos)
        //+ QString(" horDirection: %1").arg(horizontalDirection)
        //+ QString(" z: %1").arg(zPos)
        //+ QString(" a: %1").arg(qRound(rotation()))
        //+ QString(" scenePos: %1").arg(toS(scenePos(), 0))
        //+ QString(" pos: %1").arg(toS(pos(), 0))
        + QString(" rect: %1").arg(toS(rect(), 0))
        + QString(" sceneRect: %1").arg(toS(mapRectToScene(rect()), 0))
        //+ QString(" vis: %1").arg(isVisible());
        + QString(" Layout: %1").arg(getLayoutString())
        + QString(" horDir: %1").arg(horizontalDirection)
        ;
}

void Container::printStructure()
{
   QString indent;
    for (int i = 0; i < curIndent; i++)
        indent += "  ";

    qdbg() << indent << "-" << info();

    if (childContainers().count() > 0) {
        curIndent++;
        foreach (Container* c, childContainers())
            c->printStructure();
        curIndent--;
    }
}

int Container::containerDepth()
{
    int i = 0;
    Container *pc = parentContainer();
    while (pc) {
        i++;
        pc = pc->parentContainer();
    }
    return i;
}

QString Container::ind()
{
    int i = 0;
    QString s;
    while (i++ < containerDepth()) {
        s += "  ";
        i++;
    }
    return s;
}

QPointF Container::pointByName(PointName pn)
{
    switch (pn) {
        case TopLeft:
            return topLeft();
        case TopCenter:
            return topCenter();
        case TopRight:
            return topRight();
        case LeftCenter:
            return leftCenter();
        case Center:
            return center();
        case RightCenter:
            return rightCenter();
        case BottomLeft:
            return bottomLeft();
        case BottomCenter:
            return bottomCenter();
        case BottomRight:
            return bottomRight();
        default:
            qWarning() << "Container::pointByName undefined PointName" << pn;
    }
    return QPointF();
}

QPointF Container::topLeft() {return rect().topLeft();}
QPointF Container::topCenter() {return QPointF((rect().right() + rect().left() ) /2, rect().top());}
QPointF Container::topRight() {return rect().topRight();}
QPointF Container::leftCenter() {return QPointF(rect().left(), (rect().bottom() + rect().top() ) /2);}
QPointF Container::center() {return rect().center();}
QPointF Container::rightCenter() {return QPointF(rect().right(), (rect().bottom() + rect().top() ) /2);}
QPointF Container::bottomLeft() {return rect().bottomLeft();}
QPointF Container::bottomCenter() {return QPointF((rect().right() + rect().left() ) /2, rect().bottom());}
QPointF Container::bottomRight() {return rect().bottomRight();}

void Container::setLayout(const Layout &l)
{
    layout = l;
}

Container::Layout Container::getLayout()
{
    return layout;
}

Container::Layout Container::getLayoutFromString(const QString &s)
{
    if (s == "Horizontal") return Horizontal;
    if (s == "Vertical") return Vertical;
    if (s == "FloatingReservedSpace") return FloatingReservedSpace;
    if (s == "BoundingFloats") return BoundingFloats;
    if (s == "FloatingBounded") return FloatingBounded;
    if (s == "FloatingFree") return FloatingFree;
    if (s == "List") return List;
    return UndefinedLayout;
}

QString Container::getLayoutString(const Layout &l)
{
    QString r;
    switch (l) {
        case Horizontal:
            r = "Horizontal";
            break;
        case Vertical:
            r = "Vertical";
            break;
        case BoundingFloats:
            r = "BoundingFloats";
            break;
        case FloatingReservedSpace:
            r = "FloatingReservedSpace";
            break;
        case FloatingBounded:
            r = "FloatingBounded";
            break;
        case FloatingFree:
            r = "FloatingFree";
            break;
        case List:
            r = "List";
            break;
        case UndefinedLayout:
            r = "UndefinedLayout";
            break;
        default:
            r = QString("Container::getLayoutString unknown layout: %1").arg(l);
            qWarning () << r;
    }
    return r;
}

QString Container::getLayoutString()
{
    return getLayoutString(layout);
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

void Container::setCentralContainer(Container *cc)
{
    centralContainer = cc;
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

QPointF Container::alignTo(PointName ownPointName, Container* targetContainer, PointName targetPointName)
{
    return mapFromItem(targetContainer, targetContainer->pointByName(targetPointName)) - pointByName(ownPointName);
}

void Container::addContainer(Container *c, int z)
{
    if (childContainers().contains(c)) return;

    c->setParentItem(this);
    if (z > 0)
        // Update z of container
        c->zPos = z;

    if (c->zPos > 0) {
        // Order containers, first find sibling with lowest z
        foreach (Container *child, childContainers()) {
            if (c->zPos < child->zPos && c != child) {
                c->stackBefore(child);
                break;
            }
        }
    }
}

QList <Container*> Container::childContainers()
{
    //Return list of children, but ignore QGraphicsItems, which are not Containers
    QList <Container*> list;
    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        if (child->type() > UserType)
            list << (Container*) child;
    }
    return list;
}

void Container::setAnimation(const AnimPoint &ap)
{
    animatedPos = ap;
    animate();
}

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

void Container::setOriginalScenePos()// FIXME-3 Only used for BranchContainer
{
    originalPos = scenePos();
}

QPointF Container::getOriginalPos()
{
    return originalPos;
}

void Container::reposition()    // FIXME-3 Remove comment code used for debugging
{
    qdbg() << ind() << QString("### Reposition of %1").arg(info()) << " childCount=" << childContainers().count();

    // Repositioning is done recursively:
    // First the size sizes of subcontainers are calculated,
    // Container::reposition is overloaded, so for example HeadingContainer
    // will return correct size of heading!
    //
    // Then the subcontainers are positioned.
    //
    // a) Do we have any children after all?
    if (!isVisible() || childContainers().count() == 0)
    {
        if (!overlay)
            setRect(QRectF());
            // The overlay case is handled later in parent container
        return;
    }

    // b) calc sizes and reposition subcontainers first based on their layouts
    //    (overloaded: Leaf containers like HeadingContainer will not recurse)

    foreach (Container *c, childContainers())
        c->reposition();

    // c) Align my own containers

    switch (layout) {
        case BoundingFloats:
            {
                // qdbg() << ind() << " - BF starting for " << info();

                // BoundingFloats is special case:
                // Only used for innerContainer or outerContainer
                // First child container is ornamentsContainer (or innerContainer),
                // next children are imagesContainer and/or branchesContainer

                if (childContainers().count() > 4 ) {
                    qWarning() << "Container::reposition " << info();
                    qWarning() << "Wrong number of children containers: " << childItems().count();
                    foreach (Container *c, childContainers())
                        qdbg() << "  " << c->info();

                    return;
                }

                // Calc space required
                QRectF c_bbox;  // bbox of container in my own coord
                QRectF bbox;    // United bboxes

                foreach (Container *c, childContainers()) {
                    c_bbox = mapRectFromItem(c, c->rect());
                    bbox = bbox.united(c_bbox);
                }

                // Translate everything, so that center of rectangle is in origin
                QPointF t = bbox.center();
                foreach (Container *c, childContainers())
                    c->setPos(c->pos() - t);
                bbox.translate(-t);

                setRect(bbox);

                // qdbg() << ind() << " - BF finished for " << info();
            } // BoundingFloats layout
            break;

        case FloatingReservedSpace:
            {
                // Used only for tmpParentContainer

                /* FIXME-000 do nothing
                */
                qreal w = 0;
                qreal h = 0;
                BranchContainer* bc_first = nullptr;

                QRectF r;
                QList <BranchContainer*> branches = ((BranchContainer*)this)->childBranches();
                if (branches.count() > 0) {
                    // Consider other children
                    foreach (BranchContainer *bc, branches) {
                        QRectF bc_bbox = mapRectFromItem(bc, bc->rect());  // FIXME-000 really map? rect is too big...

                        if (!bc_first) bc_first = bc;

                        qdbg() << ind() << " + FloatingReservedSpace c=" << bc->info() << "  c_bbox=" << toS(bc_bbox, 0);
                        bc->printStructure();
                        w = max(w, bc_bbox.width());
                        h += bc_bbox.height();
                    }
                    r = QRectF(bc_first->rect().left(), bc_first->rect().top(), w, h);
                    // setRect(QRectF(c_first->rect().left(), c_first->rect().top(), w, h));
                }

                qdbg() << ind() << " + FloatingReservedSpace r=" << toS(r) << "  pos=" << pos() << " " << getName() << " w=" << w << " h=" << h << " rect=" << toS(rect(),0);
            }
            break;

        case FloatingBounded:
            {
                // Creates rect from unite of all children
                // Will not move any children, but keep their
                // (relative) positions

                // Calc bbox of all children to prepare calculating rect()
                QRectF r;
                if (childContainers().count() > 0) {
                    bool first_iteration = true;

                    // Consider other children
                    foreach (Container *c, childContainers()) {
                        QRectF c_bbox = mapRectFromItem(c, c->rect());

                        if (first_iteration) {
                            first_iteration = false;
                            r = c_bbox;
                        } else
                            r = r.united(c_bbox);
                    }
                }

                setRect(r);
                //qdbg() << ind() << " + FloatingBounded r=" << toS(r) << "  pos=" << pos() << getName();
            }
            break;

        case FloatingFree:
            setRect(QRectF()); // Empty rectangle
            break;

        case Horizontal: {
                // Calc space required
                qreal h_max = 0;
                qreal w_total = 0;

                // qdbg() << ind() << " * HL starting for " << info();
                foreach (Container *c, childContainers()) {
                    if (!c->overlay) {
                        QRectF c_bbox = mapRectFromItem(c, c->rect());

                        w_total += c_bbox.width();
                        qreal h = c_bbox.height();
                        if (h_max < h) h_max = h;
                    }
                }

                // Left (or right) line, where next children will be aligned to
                qreal x = - w_total / 2;
                if (horizontalDirection == RightToLeft)
                    x = -x;

                // Position children initially. (So far only centered vertically)
                foreach (Container *c, childContainers()) {
                    if (!c->overlay) {
                        QRectF c_bbox = mapRectFromItem(c, c->rect());
                        QPointF origin_mapped = mapFromItem(c, QPointF());
                        qreal offset;

                        // Pre alignment
                        if (horizontalDirection == LeftToRight)
                            offset = - (c_bbox.left() - origin_mapped.x());
                        else
                            offset = - (c_bbox.right() - origin_mapped.x());

                        // qdbg() << ind() << "    HL x=" << x << " offset=" << offset << " c: " << c->info();

                        // Align vertically centered, consider mapped(!) dimensions
                        c->setPos (x + offset, 0);

                        // Align vertically to top
                        // c->setPos (x + offset, - (h_max - c_bbox.height()) / 2);

                        // Align vertically to bottom
                        // c->setPos (x + offset, + (h_max - c_bbox.height()) / 2);

                        // Post alignment
                        if (horizontalDirection == LeftToRight) {
                            x += c_bbox.width();
                        } else
                            x -= c_bbox.width();

                        // qdbg() << ind() << "    HL Done positioning: " << c->info();
                    }   // No overlay container
                }   // Position children 

                // Move everything, so that center of central container will be in origin
                QPointF v_central;

                if (centralContainer) {
		    // Now we might want to adjust positions of children, so
		    // that centralContainer (==headingContainer) keeps its position
		    // This may happen, if
		    // - I am in a floating layout or
		    // - I am a MapCenter myself
                    if ((parentContainer() && parentContainer()->hasFloatingLayout()) || !parentContainer() ) {
			    v_central = mapFromItem(centralContainer, centralContainer->rect().center());
                            if (!v_central.isNull())
                                foreach (Container *c, childContainers())
                                    if (!c->overlay)
                                        c->setPos(c->pos() - v_central);
		    }
                }

                setRect(QRectF(- w_total / 2 - v_central.x(),  - h_max / 2 - v_central.y(), w_total, h_max));

                // qdbg() << ind() << " * HL Finished for " << info();
            } // Horizontal layout
            break;

        case List:
        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;

                // Calc space required
                foreach (Container *c, childContainers()) {
                    QRectF c_bbox = mapRectFromItem(c, c->rect());

                    // For width and height we can use the already mapped dimensions
                    qreal w = c_bbox.width();
                    if (w_max < w) w_max = w;
                    h_total += c_bbox.height();
                }

                // Top line, where next children will be aligned to
                qreal y = - h_total / 2;

                // Position children initially
                foreach (Container *c, childContainers()) {
                    QRectF c_bbox = mapRectFromItem(c, c->rect());
                    y += - c->rect().top();
		    switch (horizontalAlignment) {
			case AlignedLeft:
                            c->setPos((- w_max + c_bbox.width()) / 2, y);
			    break;
			case AlignedRight:
                            c->setPos((w_max - c_bbox.width()) / 2, y);
			    break;
			case AlignedCentered:
			    c->setPos (0, y);
			    break;
                        default:
                            qWarning() << "Container::reposition vertically - undefined alignment:" << horizontalAlignment << " in " << info();
                            if (containerType == BranchesContainer)
                                qWarning() << "  orient=" << ((BranchContainer*)this)->getOrientation();
		    }

                    y += c->rect().bottom();
                }

                // Set rect
                setRect(-w_max / 2, -h_total / 2, w_max, h_total);

            } // Vertical layout
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type for container: " << info();
            break;
    }

    // Now we have calculated our own size, adjust depending overlay containers (selections!)
    foreach (Container *c, childContainers()) {
        if (c->overlay)
            c->setRect(rect());
    }
}
