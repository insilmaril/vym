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
    framePaddingInt = 0;

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
    c->setPos(0, 0);    // For cloud this might change again in updateGeometry()

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

QString FrameContainer::frameTypeString(int ftype)
{
    switch (ftype) {
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
            qWarning() << "FrameContainer::setFrameType  unknown frame type " << ftype;
    }
    return QString();
}

QString FrameContainer::frameTypeString()
{
    return frameTypeString(frameTypeInt);
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
                rectFrame = new QGraphicsRectItem;  // FIXME-3 Use my own Container QGraphicsRectItem!
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
    QRectF r;   // Final rectanble of FrameContainer

    qreal pad = framePaddingInt + (framePen.width() - 1) / 2; // "Inner" padding and pen width

    switch (frameTypeInt) {
        case NoFrame:
            break;

        case Rectangle:
            rectFrame->setRect(
                childRect.left() - pad,
                childRect.top() - pad,
                childRect.width() + pad * 2,
                childRect.height() + pad * 2);

            r.setRect(
                    childRect.left() - pad * 2,
                    childRect.top() - pad * 2,
                    childRect.width() + pad * 4,
                    childRect.height() + pad * 4);
            break;

        case RoundedRectangle: {
            qreal radius = 10;
            qreal radius_2 = radius * 2;
            qreal radius_h = radius / 2;

            QPointF tl = childRect.topLeft() + QPointF(- radius_h - pad, - radius_h - pad);
            QPointF tr = childRect.topRight() + QPointF(radius_h + pad, - radius_h - pad);
            QPointF bl = childRect.bottomLeft() + QPointF(- radius_h - pad, radius_h + pad);
            QPointF br = childRect.bottomRight() + QPointF(radius_h + pad, + radius_h + pad);
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
                    childRect.left() - pad * 2 - radius_h,
                    childRect.top() - pad * 2 - radius_h,
                    childRect.width() + pad * 4 + radius_h * 2,
                    childRect.height() + pad * 4 + radius_h * 2);
        } break;

        case Ellipse: {
            // This approach assumes, that proportions in childRect are
            // the same as in ellips. See also calculation in
            // https://stackoverflow.com/questions/433371/ellipse-bounding-a-rectangle
            qreal w = childRect.width() + pad * 2;
            qreal h = childRect.height() + pad * 2;
            qreal a = w / sqrt(2);
            qreal b = h / sqrt(2);

            ellipseFrame->setRect(- a, - b, a * 2, b * 2);

            r.setRect(
                    - a - pad,
                    - b - pad,
                    (a + pad) * 2,
                    (b + pad) * 2);
            }
            break;

        case Circle: {
            qreal radius = sqrt(childRect.height() * childRect.height() + childRect.width() * childRect.width()) / 2;
            qreal radius_2 = radius * 2;
            ellipseFrame->setRect(
                QRectF(
                    - radius - pad,
                    - radius - pad,
                    radius_2 + 2 * pad,
                    radius_2 + 2 * pad));

            r.setRect(
                    - radius - pad * 2,
                    - radius - pad * 2,
                    radius * 2  + pad * 4,
                    radius * 2 + pad * 4);
            }
            break;

        case Cloud: {
            QPointF tl = childRect.topLeft() + QPointF( - pad, - pad);
            QPointF tr = childRect.topRight() + QPointF(  pad, - pad);
            QPointF bl = childRect.bottomLeft() + QPointF( - pad, + pad);
            QPainterPath path;
            path.moveTo(tl);

            float w = tr.x() - tl.x();
            float h = bl.y() - tl.y();

            // Cloud factor: Distance of control points from curve point
            qreal cfx  = w;
            qreal cfx2 = cfx / 2;
            qreal cfy  = h;
            qreal cfy2 = cfy / 2;

            // Top path
            int n = max (3, w / 40);     // number of intervalls
            if (n % 2 == 0 && n > 1) n -= 1;
            float d = w / n;    // width of interwall
            for (float i = 0; i < n; i++) {
                path.cubicTo(
                        tl.x() + i * d,                                tl.y() - cfx2 * roof((i + 0.5) / n),
                        tl.x() + (i + 1) * d,                          tl.y() - cfx2 * roof((i + 0.5) / n),
                        tl.x() + (i + 1) * d - 0 * (cfx2 / 5) * roof((i + 1) / n), tl.y() - cfx2 * roof((i + 1) / n));
            }

            // Right path
            n = max(1, h / 40);
            if (n % 2 == 0 && n > 1) n -= 1;
            d = h / n;
            for (float i = 0; i < n; i++) {
                path.cubicTo(tr.x() + cfy2 * roof((i + 0.5) / n), tr.y() + i * d,
                             tr.x() + cfy2 * roof((i + 0.5) / n), tr.y() + (i + 1) * d,
                             tr.x() + cfy2 * roof((i + 1) / n), tr.y() + (i + 1) * d);
            }

            // Bottom path
            n = max (2, w / 60);     // number of intervalls
            if (n % 2 == 0 && n > 1) n -= 1;
            d = w / n;
            for (float i = n; i > 0; i--) {
                path.cubicTo(bl.x() + i * d,                                bl.y() + cfy2 * roof((i - 0.5) / n),
                             bl.x() + (i - 1) * d,                          bl.y() + cfy2 * roof((i - 0.5) / n),
                             bl.x() + (i - 1) * d - 0 * (cfy / 5 ) * roof((i - 1) / n), bl.y() + cfy2 * roof((i - 1) / n));
            }
            // Left path
            n = max(1, h / 40);
            if (n % 2 == 0 && n > 1) n -= 1;

            d = h / n;
            for (float i = n; i > 0; i--) {
                path.cubicTo(tl.x() - cfy2 * roof((i - 0.5) / n), tr.y() + i * d,
                             tl.x() - cfy2 * roof((i - 0.5) / n), tr.y() + (i - 1) * d,
                             tl.x() - cfy2 * roof((i - 1) / n), tr.y() + (i - 1) * d);
            }
            pathFrame->setPath(path);
            QRectF br = path.boundingRect();

            // center of pathFrame might be outside of origin, due to cloud not completely symmetrical
            // Correct position of pathFrame and child
            QPointF p(pad / 2, 0);
            pathFrame->setPos(- br.center() + p);
            childContainers().first()->setPos(- br.center() + p);

            r.setRect(
                    - (br.width() + pad) / 2,
                    - (br.height() + pad) / 2,  // Vertically centered anyway later...
                    br.width() + 2 * pad,
                    br.height() + 2 * pad);
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

void FrameContainer::setFramePadding(const int &i)
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

