#include <QDebug>

#include "container.h"

#include "branchitem.h"
#include "misc.h"

#define qdbg() qDebug().nospace().noquote()

Container::Container(QGraphicsItem *parent) : QGraphicsRectItem(parent)
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
    type = other->type;

    originalPos = other->originalPos;
    name = other->name;

    layout = other->layout;
    horizontalDirection = other->horizontalDirection;
    horizontalAlignment = other->horizontalAlignment;
}

void Container::init()
{
    type = UndefinedType;
    layout = Horizontal;

    // subcontainers usually may influence position
    // Only mapCenters will stay where they are

    minimumWidth = 0;

    horizontalDirection = LeftToRight;

    centralContainer = false;

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

void Container::setName(const QString &n)   // FIXME-2 debugging only
{
    name = n;
}

QString Container::getName()    // FIXME-2 debugging only
{
    QString t;
    switch (type) {
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
        case FloatingContent:
            t = "FloatingContent";
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
        case InnerContent:
            t = "InnerContent";
            break;
        case Link:
            t = "Link";
            break;
        case OrnamentsContainer:
            t = "OrnamentsContainer";
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
    return prefix +
        getName() +
        QString(" Layout: %1").arg(getLayoutString()) +
        QString(" scenePos: %1").arg(qpointFToString(scenePos(), 0)) +
        QString(" pos: %1").arg(qpointFToString(pos(), 0)) +
        QString(" rect: %1").arg(qrectFToString(rect(), 0));
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
    if (s == "BoundingFloats") return BoundingFloats;
    if (s == "FloatingBounded") return FloatingBounded;
    if (s == "FloatingFree") return FloatingFree;
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
        case FloatingBounded:
            r = "FloatingBounded";
            break;
        case FloatingFree:
            r = "FloatingFree";
            break;
        case UndefinedLayout:
            r = "UndefinedLayout";
            break;
        default:
            r = QString("Unknown: %1").arg(l);
            qWarning () << QString("Container::getLayoutString unknown layout: %1").arg(l);
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
    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
	if (c == cc)
	    c->centralContainer = true;
	else
	    c->centralContainer = false;
    }
}

bool Container::isCentralContainer()
{
    return centralContainer;
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
    qDebug() << "C:setVis v=" << v << info ();
    visible = v;
    setVisible(visible);
    /* FIXME-0 setVis not required recursively, but is inherited to children
    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
        qDebug() << "  c=" << c->info();
        c->setVisibility(v);
    }
    */
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

void Container::reposition()
{
    qdbg() << ind() << QString("### Reposition of %1").arg(info());

    // Repositioning is done recursively:
    // First the size sizes of subcontainers are calculated,
    // Container::reposition is overloaded, so for example HeadingContainer
    // will return correct size of heading!
    //
    // Then the subcontainers are positioned.
    //
    // a) Do we have any chilrden after all?
    if (!isVisible() || childItems().count() == 0)
    {
        setRect(QRect());
        return;
    }

    // b) calc sizes and reposition subcontainers first based on their layouts
    //    (overloaded: Leaf containers like HeadingContainer will not recurse)

    Container *c;
    foreach (QGraphicsItem *child, childItems()) {
        c = (Container*) child;
        c->reposition();
    }

    // c) Align my own containers

    QRectF r;

    switch (layout) {
        case BoundingFloats:
            {
                qdbg() << ind() << " - BF a) info=" << info();
                // BoundingFloats is special case:
                // Only used for innerContainer or outerContainer
                // First child container is ornamentsContainer (or innerContainer),
                // next children are imagesContainer and/or branchesContainer

                if (childItems().count() > 4 ) {
                    qWarning() << "Container::reposition " << info();
                    qWarning() << "Wrong number of children containers: " << childItems().count();
                    foreach (QGraphicsItem *child, childItems()) {
                        Container *c = (Container*) child;
                        qdbg() << "  " << c->info();
                    }

                    return;
                }

                // Calc space required
                QRectF c_bbox;  // bbox of container in my own coord
                QRectF bbox;    // United bboxes

                foreach (QGraphicsItem *child, childItems()) {
                    Container *c = (Container*) child;
                    c_bbox = mapRectFromItem(c, c->rect());
                    qdbg() << ind() << " - BF c=" << c->info();
                    bbox = bbox.united(c_bbox);
                }

                // Translate, so that total bbox and contents move, so that
                // first container (ornaments container) is centered in origin
                Container *oc = (Container*)(childItems().first());
                QPointF t = oc->rect().center();
                //QPointF t = - bbox.center() / 2;
                qdbg() << ind() << " - BF bbox=" << qrectFToString(bbox, 0) << "  t= " << qpointFToString(t) << " " << getName();
                bbox.translate(t);
                foreach (QGraphicsItem *child, childItems()) {
                    Container *c = (Container*) child;
                    c->setPos(c->pos() + t);
                }

                setRect(bbox);

                qdbg() << ind() << " - BF b) info=" << info();
                //setPos(0, 0);  // Center, not really required
                //qdbg() << " - BF c) info=" << info();
            } // BoundingFloats layout
            break;

        case FloatingBounded:
            {
                // Calc bbox of all children to prepare calculating rect()
                if (childItems().count() > 0) {
                    bool first_iteration = true;

                    // Consider other children
                    foreach (QGraphicsItem *child, childItems()) {
                        c = (Container*) child;
                        QRectF c_bbox = mapRectFromItem(c, c->rect());

                        if (first_iteration) {
                            first_iteration = false;
                            r = c_bbox;
                        } else
                            r = r.united(c_bbox);
                    }
                }

                setRect(r);
                qdbg() << ind() << " + FloatingBounded r=" << qrectFToString(r) << "  pos=" << pos() << getName();
            }
            break;

        case FloatingFree:
            setRect(r); // Empty rectangle
            break;

        case Horizontal: {
                Container *centralContainer = nullptr;

                // Calc space required
                qreal h_max = 0;
                qreal w_total = 0;

                qdbg() << ind() << " * Starting to reposition " << info();
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    QRectF c_bbox = mapRectFromItem(c, c->rect());

                    if (c->isCentralContainer())
                        centralContainer = c;

                    /*
                    if (c->rotation() != 0) {// FIXME-0 review rotation
                        // Move rotated container, so that upperLeft corner is in (0,0)
                        // Required to get correct pos bbox of rotated containers.
                        c->setPos(0, 0);
                        c_bbox.moveTopLeft(QPointF(0,0));
                    }
                    */

                    w_total += c_bbox.width();
                    qreal h = c_bbox.height();
                    h_max = (h_max < h) ? h : h_max;
                }
                qdbg() << ind() << " * Required space: w=" << w_total << " h=" << h_max << " for " << getName() << " dir=" << horizontalDirection;
                if (centralContainer)

                    qdbg() << ind() << " * Found central container: " << centralContainer->info();

                qreal w_last;   // last width before adding current container width to bbox later
                qreal x = - w_total / 2;

                if (horizontalDirection == RightToLeft)
                    x = -x;

                // Position children initially. (So far only centered vertically)
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    QRectF c_bbox = mapRectFromItem(c, c->rect());
		    w_last = c_bbox.width();

                    // Center vertically
		    qreal y = 0;

		    // To align to bottom: qreal y = (h_max - c_bbox.height() ) / 2;

		    if (horizontalDirection == LeftToRight)
		    {
			if (c->rotation() == 0)
			    //c->setPos (x + c->rect().width() / 2, y);
			    c->setPos (x + w_last / 2, y);
			else {   // FIXME-0 review rotation
			    // move rotated container to my origin
			    // original: c->setPos (- mapRectFromItem(c, c->rect()).topLeft());
			    c->setPos (x + w_last / 2, y);
			}
                        qdbg() << ind() << " *            after: c=" << c->info();
			x += w_last;
		    } else
		    {
			//if (c->rotation() == 0)   // FIXME-0 review rotation
			    c->setPos (x - c_bbox.width() / 2, y);
			//else {
			    // move rotated container to my origin
			//    c->setPos (- mapRectFromItem(c, c->rect()).topLeft());
			//}
			x -= w_last;
		    }
                }

                // Move everything, so that center of central container will be in origin
                QPointF v_central;
                if (centralContainer) {
                    v_central = centralContainer->pos();    // FIXME-0 really required? Usuall (0,0)

                    qdbg() << ind() << " * central container, moving everything by " << qpointFToString(v_central, 0);
                /*
                foreach (QGraphicsItem *child, childItems()) {
                    child->setPos(child->pos() - v_central);
                    qdbg() << "   * After repositioning: c=" << ((Container*)child)->info();
                }
                */
                r = QRectF(- w_total / 2 - v_central.x(),  - h_max / 2 - v_central.x(), w_total, h_max);
                } else {
                    // FIXME-0 No central container, center whole set of horizontal containers
                    // E.g. flagrow
                    //    v_central = QPointF(0, 0); //QPointF(- w_total / 2, - h_max / 2);
                    qdbg() << "   * No central container, using unite of horizontal containers";
                    bool first_iteration = true;
                    foreach (QGraphicsItem *child, childItems()) {
                        Container *c = (Container*) child;
                        QRectF c_bbox = mapRectFromItem(c, c->rect());
                        if (first_iteration) {
                            first_iteration = false;
                            r = c_bbox;
                        } else
                            r = r.united(c_bbox);
                    }
                }

                // FIXME-00 review: Finally set rect
                setRect(r);
                qdbg() << ind() << " * Finished: " << info();

            } // Horizontal layout
            break;

        case Vertical: {
                qreal h_total = 0;
                qreal w_max = 0;
                qreal w;

                // Calc space required
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;

                    QRectF c_bbox = mapRectFromItem(c, c->rect());

                    // For width and height we can use the already mapped dimensions
                    w = c_bbox.width();
                    w_max = (w_max < w) ? w : w_max;
                    h_total += c_bbox.height();
                }

                qreal y = 0;

                qdbg() << ind() << " ~ Starting to reposition: " << info();
                QPointF rp; // Reference point
                switch (horizontalAlignment) {
                    case AlignedLeft:
                        rp = QPointF(-w_max / 2, -h_total / 2);
                        qdbg() << ind() << " ~ AlignedLeft rp=" << qpointFToString(rp, 0);
                        break;
                    case AlignedRight:
                        rp = QPointF( w_max / 2, -h_total / 2);
                        qdbg() << ind() << " ~ AlignedRight rp=" << qpointFToString(rp, 0);
                        break;
                    case AlignedCentered:
                        rp = QPointF( 0, -h_total / 2);
                        qdbg() << ind() << " ~ AlignedCentered rp=" << qpointFToString(rp, 0);
                        break;
                    default:
                        qWarning() << ind() << " ~ Alignment undefined";
                }

                // Position children initially
                foreach (QGraphicsItem *child, childItems()) {
                    c = (Container*) child;
                    QRectF c_bbox = mapRectFromItem(c, c->rect());
                    qdbg() << ind() << " ~ Initial positioning: "
                        << c->info()
                        << " y=" << y
                        << " cbbox=" << qrectFToString(c_bbox,0)
                        << " rp=" << qpointFToString(rp,0)
                        << " left=" << c_bbox.left();

		    switch (horizontalAlignment) {
			case AlignedLeft:
			    c->setPos (c_bbox.width() / 2 + rp.x(), rp.y() + c_bbox.height() / 2);
                            qdbg() << ind() << " ~               after: " << c->info();
			    break;
			case AlignedRight:
			    c->setPos (rp.x() - c_bbox.width() / 2, rp.y() + c_bbox.height() / 2);
                            qdbg() << ind() << " ~               after: " << c->info();
			    break;
			case AlignedCentered:
			    c->setPos (0, rp.y() + c_bbox.height() / 2);
                            qdbg() << ind() << " ~               after: " << c->info();
			    break;
		    }

		    rp.setY(rp.y() + c_bbox.height());
                }

                // Set rect to the non-floating containers we have so far
                setRect(-w_max / 2, -h_total / 2, w_max, h_total);
                qdbg() << ind() << " ~ rect=" << qrectFToString(rect(),0) << info();
            } // Vertical layout
            break;
        default:
            qWarning() << "Container::reposition  unknown layout type for container: " << info();
            break;
    }
}
