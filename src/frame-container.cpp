#include "frame-container.h"

#include <QColor>
#include <QDebug>
#include <QGraphicsScene>

#include <math.h>

#include "misc.h" //for roof function

/////////////////////////////////////////////////////////////////
// FrameContainer
/////////////////////////////////////////////////////////////////
FrameContainer::FrameContainer()
{
    //qDebug() << "Constr FrameContainer this=" << this;
    init();
}

FrameContainer::~FrameContainer()
{
    //qDebug() << "Destr FrameContainer";
    clear();
}

void FrameContainer::init()
{
    containerType = Frame;
    frameTypeInt = NoFrame;
    clear();
    framePen.setColor(Qt::black);
    framePen.setWidth(1);
    frameBrush.setColor(Qt::white);
    frameBrush.setStyle(Qt::SolidPattern);

    setLayout(Container::Horizontal);
    setHorizontalDirection(Container::LeftToRight);

    setVisible(true);

    usage = Undefined;
}

void FrameContainer::clear()
{
    switch (frameTypeInt) {
        case NoFrame:
            break;
        case Rectangle:
            delete rectFrame;
            break;
        case RoundedRectangle:
            delete pathFrame;
            break;
        case Ellipse:
        case Circle:
            delete ellipseFrame;
            break;
        case Cloud:
            delete pathFrame;
            break;
    }
    frameTypeInt = NoFrame;
    framePaddingInt = 0; // No frame requires also no padding
}

void FrameContainer::repaint()
{
    // Repaint, when e.g. borderWidth has changed or a color
    switch (frameTypeInt) {
        case Rectangle:
            rectFrame->setPen(framePen);
            rectFrame->setBrush(frameBrush);
            break;
        case RoundedRectangle:
            pathFrame->setPen(framePen);
            pathFrame->setBrush(frameBrush);
            break;
        case Ellipse:
        case Circle:
            ellipseFrame->setPen(framePen);
            ellipseFrame->setBrush(frameBrush);
            break;
        case Cloud:
            pathFrame->setPen(framePen);
            pathFrame->setBrush(frameBrush);
            break;
        default:
            qWarning() << "FrameContainer::repaint  unknown frame type " << frameTypeInt;
            break;
    }
}

void FrameContainer::reposition()
{
    // Assumption: FrameContainer only has one child (Ornamentscontainer or Outer/InnerContainer)
    if (childContainers().count() > 1) {
        qWarning() << "FrameContainer has more than one container! Parent=" << parentContainer()->info();
        foreach (Container *c, childContainers())
            qWarning() << " - " << c->info();
        return;
    }

    if (childContainers().isEmpty()) {
        qWarning() << "FrameContainer has no container!";
        return;
    }

    // FrameContainer only has one child (Inner-, Outer-, OrnamentsContainer)
    Container *c = childContainers().first();
    c->reposition();

    updateGeometry(c->rect());
}

FrameContainer::FrameType FrameContainer::frameType() { return frameTypeInt; }

FrameContainer::FrameType FrameContainer::frameTypeFromString(const QString &s)
{
    if (s == "Rectangle")
        return Rectangle;
    else if (s == "RoundedRectangle")
        return RoundedRectangle;
    else if (s == "Ellipse")
        return Ellipse;
    else if (s == "Circle")
        return Circle;
    else if (s == "Cloud")
        return Cloud;
    return NoFrame;
}

QString FrameContainer::frameTypeString()
{
    switch (frameTypeInt) {
    case Rectangle:
        return "Rectangle";
    case RoundedRectangle:
        return "RoundedRectangle";
    case Ellipse:
        return "Ellipse";
    case Circle:
        return "Circle";
    case Cloud:
        return "Cloud";
    case NoFrame:
        return "NoFrame";
    default:
        qWarning() << "FrameContainer::setFrameType  unknown frame type " << frameTypeInt;
        break;
    }
}

void FrameContainer::setFrameType(const FrameType &t)
{
    if (t != frameTypeInt) {
        clear();
        frameTypeInt = t;
        switch (frameTypeInt) {
            case NoFrame:
                break;
            case Rectangle:
                rectFrame = new QGraphicsRectItem;  // FIXME-3 Use my own Container rect!
                rectFrame->setPen(framePen);
                rectFrame->setBrush(frameBrush);
                rectFrame->setFlag(ItemStacksBehindParent, true);
                rectFrame->setParentItem(this);
                rectFrame->show();
                break;
            case Ellipse:
            case Circle:
                ellipseFrame = scene()->addEllipse(QRectF(0, 0, 0, 0),
                                                   framePen, frameBrush);
                ellipseFrame->setPen(framePen);
                ellipseFrame->setBrush(frameBrush);
                ellipseFrame->setFlag(ItemStacksBehindParent, true);
                ellipseFrame->setParentItem(this);
                ellipseFrame->show();
                break;
            case RoundedRectangle: {
                QPainterPath path;
                pathFrame = new QGraphicsPathItem;
                pathFrame->setPen(framePen);
                pathFrame->setBrush(frameBrush);
                pathFrame->setFlag(ItemStacksBehindParent, true);
                pathFrame->setParentItem(this);
                pathFrame->show();
            } break;
            case Cloud: {
                QPainterPath path;
                pathFrame = scene()->addPath(path, framePen, frameBrush);
                pathFrame->setPen(framePen);
                pathFrame->setBrush(frameBrush);
                pathFrame->setFlag(ItemStacksBehindParent, true);
                pathFrame->setParentItem(this);
                pathFrame->show();
            }
            break;
            default:
                qWarning() << "FrameContainer::setframeType  unknown frame type " << frameTypeInt;
                break;
        }
    }
    // reposition() is called in vymmodel for all containers
}

void FrameContainer::setFrameType(const QString &t)
{
    if (t == "Rectangle")
        setFrameType(Rectangle);
    else if (t == "RoundedRectangle")
        setFrameType(RoundedRectangle);
    else if (t == "Ellipse")
        setFrameType(Ellipse);
    else if (t == "Circle")
        setFrameType(Circle);
    else if (t == "Cloud")
        setFrameType(Cloud);
    else
        setFrameType(NoFrame);
}

void FrameContainer::updateGeometry(const QRectF &childRect)
{
    QRectF r;

    switch (frameTypeInt) {
        case NoFrame:
            break;

        case Rectangle:
            rectFrame->setRect(
                childRect.left() - framePaddingInt,
                childRect.top() - framePaddingInt,
                childRect.width() + framePaddingInt * 2,
                childRect.height() + framePaddingInt * 2);

            r.setRect(
                    childRect.left() - framePaddingInt * 2,
                    childRect.top() - framePaddingInt * 2,
                    childRect.width() + framePaddingInt * 4,
                    childRect.height() + framePaddingInt * 4);
            break;

        case RoundedRectangle: {
            qreal radius = 20;
            qreal radius_2 = radius * 2;

            QPointF tl = childRect.topLeft() + QPointF(- radius - framePaddingInt, - radius - framePaddingInt);
            QPointF tr = childRect.topRight() + QPointF(radius + framePaddingInt, - radius - framePaddingInt);
            QPointF bl = childRect.bottomLeft() + QPointF(- radius - framePaddingInt, radius + framePaddingInt);
            QPointF br = childRect.bottomRight() + QPointF(radius + framePaddingInt, + radius + framePaddingInt);
            QPainterPath path;

            path.moveTo(tl.x() + radius, tl.y());

            // Top path
            path.lineTo(tr.x() - radius, tr.y());
            path.arcTo(tr.x() - radius_2, tr.y(), radius_2, radius_2, 90, -90);
            path.lineTo(br.x(), br.y() - radius);
            path.arcTo(br.x() - radius_2, br.y() - radius_2, radius_2, radius_2, 0, -90);

            path.lineTo(bl.x() + radius, br.y());
            path.arcTo(bl.x(), bl.y() - radius_2, radius_2, radius_2, -90, -90);
            path.lineTo(tl.x(), tl.y() + radius);
            path.arcTo(tl.x(), tl.y(), radius_2, radius_2, 180, -90);
            pathFrame->setPath(path);

            r.setRect(
                    childRect.left() - framePaddingInt * 2 - radius,
                    childRect.top() - framePaddingInt * 2 - radius,
                    childRect.width() + framePaddingInt * 4 + radius * 2,
                    childRect.height() + framePaddingInt * 4 + radius * 2);
        } break;

        case Ellipse:   // FIXME-0 adapt to new frames
            ellipseFrame->setRect(
                QRectF(childRect.x(), childRect.y(), childRect.width(), childRect.height()));
            break;

        case Circle: {
            qreal radius = sqrt(childRect.height() * childRect.height() + childRect.width() * childRect.width()) / 2;
            qreal radius_2 = radius * 2;
            ellipseFrame->setRect(
                QRectF(
                    - radius - framePaddingInt, 
                    - radius - framePaddingInt,
                    radius_2 + 2 * framePaddingInt,
                    radius_2 + 2 * framePaddingInt));

            r.setRect(
                    - radius - framePaddingInt * 2,
                    - radius - framePaddingInt * 2,
                    radius * 2  + framePaddingInt * 4,
                    radius * 2 + framePaddingInt * 4);
            }
            break;

        case Cloud: {   // FIXME-0 adapt to new frames
            QPointF tl = childRect.topLeft() + QPointF( - framePaddingInt, - framePaddingInt);
            QPointF tr = childRect.topRight() + QPointF(  framePaddingInt, - framePaddingInt);
            QPointF bl = childRect.bottomLeft();
            QPainterPath path;
            path.moveTo(tl);

            float w = childRect.width();
            float h = childRect.height();
            int n = 10; //w / 40;          // number of intervalls
            float d = w / n; // width of interwall

            float a = 50;    // Parameter with "size" if arcs used for Bezier controlpoints

            // Top path
            for (float i = 0; i < n; i++) {
                path.cubicTo(
                        tl.x() + i * d, tl.y() - 100 * roof((i + 0.5) / n),
                        tl.x() + (i + 1) * d, tl.y() - 100 * roof((i + 0.5) / n),
                        tl.x() + (i + 1) * d + 20 * roof((i + 1) / n), tl.y() - 50 * roof((i + 1) / n));
            }
            // Right path
            n = h / 20;
            d = h / n;
            for (float i = 0; i < n; i++) {
                path.cubicTo(tr.x() + 100 * roof((i + 0.5) / n), tr.y() + i * d,
                             tr.x() + 100 * roof((i + 0.5) / n),
                             tr.y() + (i + 1) * d, tr.x() + 60 * roof((i + 1) / n),
                             tr.y() + (i + 1) * d);
            }
            n = w / 60;
            d = w / n;
            // Bottom path
            for (float i = n; i > 0; i--) {
                path.cubicTo(bl.x() + i * d, bl.y() + 100 * roof((i - 0.5) / n),
                             bl.x() + (i - 1) * d,
                             bl.y() + 100 * roof((i - 0.5) / n),
                             bl.x() + (i - 1) * d + 20 * roof((i - 1) / n),
                             bl.y() + 50 * roof((i - 1) / n));
            }
            // Left path
            n = h / 20;
            d = h / n;
            for (float i = n; i > 0; i--) {
                path.cubicTo(tl.x() - 100 * roof((i - 0.5) / n), tr.y() + i * d,
                             tl.x() - 100 * roof((i - 0.5) / n),
                             tr.y() + (i - 1) * d, tl.x() - 60 * roof((i - 1) / n),
                             tr.y() + (i - 1) * d);
            }
            pathFrame->setPath(path);
            r.setRect(
                    - framePaddingInt * 2,
                    - framePaddingInt * 2,
                    a * 2  + framePaddingInt * 4,
                    a * 2 + framePaddingInt * 4);
            }
            break;
        default:
            qWarning() << "FrameContainer::setFrameRect  unknown frame type " << frameTypeInt;
            break;
        }
    setRect(r);
}

int FrameContainer::framePadding()
{
    if (frameTypeInt == NoFrame)
        return 0;
    else
        return framePaddingInt;
}

void FrameContainer::setFramePadding(const int &i)  // FIXME-0 not fully supported yet
{
    framePaddingInt = i;
    updateGeometry(childContainers().first()->rect());
}

int FrameContainer::framePenWidth() { return framePen.width(); }

void FrameContainer::setFramePenWidth(const int &i)
{
    framePen.setWidth(i);
    repaint();
}

QColor FrameContainer::framePenColor() { return framePen.color(); }

void FrameContainer::setFramePenColor(const QColor &col)
{
    framePen.setColor(col);
    repaint();
}

QColor FrameContainer::frameBrushColor() { return frameBrush.color(); }

void FrameContainer::setFrameBrushColor(const QColor &col)
{
    frameBrush.setColor(col);
    repaint();
}

void FrameContainer::setUsage(FrameUsage u)
{
    usage = u;
}

QString FrameContainer::saveFrame()
{
    QStringList attrList;
    if (usage == InnerFrame)
        attrList << attribute("frameUsage", "innerFrame");
    else if (usage == OuterFrame)
        attrList << attribute("frameUsage", "outerFrame");

    attrList <<  attribute("frameType", frameTypeString());

    if (frameTypeInt != NoFrame) {
        attrList <<  attribute("penColor", framePen.color().name(QColor::HexArgb));
        attrList <<  attribute("brushColor", frameBrush.color().name(QColor::HexArgb));
        attrList <<  attribute("padding", QString::number(framePaddingInt));
        attrList <<  attribute("penWidth", QString::number(framePen.width()));
    }

    return singleElement("frame", attrList.join(" "));
}

