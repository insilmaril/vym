#include "frame-container.h"

#include <QColor>
#include <QDebug>
#include <QGraphicsScene>

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
    // FIXME-2 containerType = Frame;
    frameType = NoFrame;
    clear();
    framePen.setColor(Qt::black);
    framePen.setWidth(1);
    frameBrush.setColor(Qt::white);
    frameBrush.setStyle(Qt::SolidPattern);
    frameIncludeChildren = false;

    setVisible(true);

    // Don't consider for sizes or repositioning
    overlay = true;

    // Rotation
    angle = 0;
}

void FrameContainer::clear()
{
    switch (frameType) {
        case NoFrame:
            break;
        case Rectangle:
            delete rectFrame;
            break;
        case RoundedRectangle:
            delete pathFrame;
            break;
        case Ellipse:
            delete ellipseFrame;
            break;
        case Cloud:
            delete pathFrame;
            break;
    }
    frameType = NoFrame;
    framePadding = 0; // No frame requires also no padding
    frameXSize = 0;
}

void FrameContainer::setFrameRect(const QRectF &frameSize)
{
    //qDebug() << "FC::setFrameRect t=" << frameType << " r=" << qrectFToString(frameSize, 0);
    frameRect = frameSize;
    switch (frameType) {
        case NoFrame:
            break;

        case Rectangle:
            rectFrame->setRect(frameRect);
            //qDebug() << "  FC  rect: " << rectFrame << "vis=" << rectFrame->isVisible();
            break;

        case RoundedRectangle: {
            QPointF tl = frameRect.topLeft();
            QPointF tr = frameRect.topRight();
            QPointF bl = frameRect.bottomLeft();
            QPointF br = frameRect.bottomRight();
            QPainterPath path;

            qreal n = 10;
            path.moveTo(tl.x() + n / 2, tl.y());

            // Top path
            path.lineTo(tr.x() - n, tr.y());
            path.arcTo(tr.x() - n, tr.y(), n, n, 90, -90);
            path.lineTo(br.x(), br.y() - n);
            path.arcTo(br.x() - n, br.y() - n, n, n, 0, -90);
            path.lineTo(bl.x() + n, br.y());
            path.arcTo(bl.x(), bl.y() - n, n, n, -90, -90);
            path.lineTo(tl.x(), tl.y() + n);
            path.arcTo(tl.x(), tl.y(), n, n, 180, -90);
            pathFrame->setPath(path);
        } break;
        case Ellipse:
            ellipseFrame->setRect(
                QRectF(frameRect.x(), frameRect.y(), frameRect.width(), frameRect.height()));
            frameXSize = 20; // max(frameRect.width(), frameRect.height()) / 4;
            break;

        case Cloud: {
            QPointF tl = frameRect.topLeft();
            QPointF tr = frameRect.topRight();
            QPointF bl = frameRect.bottomLeft();
            QPainterPath path;
            path.moveTo(tl);

            float w = frameRect.width();
            float h = frameRect.height();
            int n = w / 40;          // number of intervalls
            float d = w / n;         // width of interwall

            // Top path
            for (float i = 0; i < n; i++) {
                path.cubicTo(tl.x() + i * d, tl.y() - 100 * roof((i + 0.5) / n),
                             tl.x() + (i + 1) * d,
                             tl.y() - 100 * roof((i + 0.5) / n),
                             tl.x() + (i + 1) * d + 20 * roof((i + 1) / n),
                             tl.y() - 50 * roof((i + 1) / n));
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
            frameXSize = 50;
        }
            break;
        default:
            qWarning() << "FrameContainer::setFrameRect  unknown frame type " << frameType;
            break;
    }
}

QRectF FrameContainer::getFrameRect()
{
    return frameRect;
}

void FrameContainer::setFramePadding(const int &i) { framePadding = i; }  // FIXME-2 not supported yet

int FrameContainer::getFramePadding()
{
    if (frameType == NoFrame)
        return 0;
    else
        return framePadding;
}

qreal FrameContainer::getFrameTotalPadding() { return frameXSize + framePadding + framePen.width(); }

qreal FrameContainer::getFrameXPadding() { return frameXSize; }

void FrameContainer::setFramePenWidth(const int &i)
{
    framePen.setWidth(i);
    repaint();
}

int FrameContainer::getFramePenWidth() { return framePen.width(); }

FrameContainer::FrameType FrameContainer::getFrameType() { return frameType; }

FrameContainer::FrameType FrameContainer::getFrameTypeFromString(const QString &s)
{
    if (s == "Rectangle")
        return Rectangle;
    else if (s == "RoundedRectangle")
        return RoundedRectangle;
    else if (s == "Ellipse")
        return Ellipse;
    else if (s == "Cloud")
        return Cloud;
    return NoFrame;
}

QString FrameContainer::getFrameTypeName()
{
    switch (frameType) {
    case Rectangle:
        return "Rectangle";
    case RoundedRectangle:
        return "RoundedRectangle";
    case Ellipse:
        return "Ellipse";
    case Cloud:
        return "Cloud";
    case NoFrame:
        return "NoFrame";
    default:
        qWarning() << "FrameContainer::setFrameType  unknown frame type " << frameType;
        break;
    }
}

void FrameContainer::setFrameType(const FrameType &t)
{
    if (t != frameType) {
        clear();
        frameType = t;
        switch (frameType) {
            case NoFrame:
                break;
            case Rectangle:
                rectFrame = new QGraphicsRectItem;  // FIXME-3 Use my own Container rect!
                rectFrame->setPen(framePen);
                rectFrame->setBrush(frameBrush);
                rectFrame->setFlag(ItemStacksBehindParent, true);
                //rectFrame->setZValue(dZ_FRAME_LOW);
                rectFrame->setParentItem(this);
                rectFrame->show();
                break;
            case Ellipse:
                ellipseFrame = scene()->addEllipse(QRectF(0, 0, 0, 0),
                                                   framePen, frameBrush);
                ellipseFrame->setPen(framePen);
                ellipseFrame->setBrush(frameBrush);
                ellipseFrame->setFlag(ItemStacksBehindParent, true);
                //ellipseFrame->setZValue(dZ_FRAME_LOW);
                ellipseFrame->setParentItem(this);
                ellipseFrame->show();
                break;
            case RoundedRectangle: {
                QPainterPath path;
                pathFrame = new QGraphicsPathItem;
                pathFrame->setPen(framePen);
                pathFrame->setBrush(frameBrush);
                pathFrame->setFlag(ItemStacksBehindParent, true);
                //pathFrame->setZValue(dZ_FRAME_LOW);
                pathFrame->setParentItem(this);
                pathFrame->show();
            } break;
            case Cloud: {
                QPainterPath path;
                pathFrame = scene()->addPath(path, framePen, frameBrush);
                pathFrame->setPen(framePen);
                pathFrame->setBrush(frameBrush);
                pathFrame->setFlag(ItemStacksBehindParent, true);
                //pathFrame->setZValue(dZ_FRAME_LOW);
                pathFrame->setParentItem(this);
                pathFrame->show();
            }
            break;
            default:
                qWarning() << "FrameContainer::setFrameType  unknown frame type " << frameType;
                break;
        }
    }
    setFrameRotation(angle);
    reposition();   // FIXME-2 needed?
}

void FrameContainer::setFrameType(const QString &t)
{
    if (t == "Rectangle")
        FrameContainer::setFrameType(Rectangle);
    else if (t == "RoundedRectangle")
        FrameContainer::setFrameType(RoundedRectangle);
    else if (t == "Ellipse")
        FrameContainer::setFrameType(Ellipse);
    else if (t == "Cloud")
        FrameContainer::setFrameType(Cloud);
    else
        FrameContainer::setFrameType(NoFrame);
}

void FrameContainer::setFramePenColor(QColor col)
{
    framePen.setColor(col);
    repaint();  // FIXME-2 needed?
}

QColor FrameContainer::getFramePenColor() { return framePen.color(); }

void FrameContainer::setFrameBrushColor(QColor col)
{
    frameBrush.setColor(col);
    repaint();  // FIXME-2 needed?
}

QColor FrameContainer::getFrameBrushColor() { return frameBrush.color(); }

void FrameContainer::setFrameIncludeChildren(bool b)
{
    frameIncludeChildren = b;
}

bool FrameContainer::getFrameIncludeChildren() { return frameIncludeChildren; }

void FrameContainer::repaint()
{
    // Repaint, when e.g. borderWidth has changed or a color
    switch (frameType) {
        case Rectangle:
            rectFrame->setPen(framePen);
            rectFrame->setBrush(frameBrush);
            break;
        case RoundedRectangle:
            pathFrame->setPen(framePen);
            pathFrame->setBrush(frameBrush);
            break;
        case Ellipse:
            ellipseFrame->setPen(framePen);
            ellipseFrame->setBrush(frameBrush);
            break;
        case Cloud:
            pathFrame->setPen(framePen);
            pathFrame->setBrush(frameBrush);
            break;
        default:
            qWarning() << "FrameContainer::repaint  unknown frame type " << frameType;
            break;
    }
}

void FrameContainer::setFrameZValue(double z)
{
    switch (frameType) {
        case NoFrame:
            break;
        case Rectangle:
            rectFrame->setZValue(z);
            break;
        case RoundedRectangle:
            pathFrame->setZValue(z);
            break;
        case Ellipse:
            ellipseFrame->setZValue(z);
            break;
        case Cloud:
            pathFrame->setZValue(z);
            break;
        default:
            qWarning() << "FrameContainer::setFrameZValue unknown frame type " << frameType;
            break;
    }
}

void FrameContainer::setFrameRotation(qreal a)
{
    angle = a;
    switch (frameType) {
        case NoFrame:
            break;
        case Rectangle:
            rectFrame->setTransformOriginPoint(0, 0);  // FIXME-2 originpoint needed?
            rectFrame->setRotation(angle);
            break;
        case RoundedRectangle:
            pathFrame->setRotation(angle);
            break;
        case Ellipse:
            ellipseFrame->setRotation(angle);
            break;
        case Cloud:
            pathFrame->setRotation(angle);
            break;
        default:
            qWarning() << "FrameContainer::setFrameRotation unknown frame type " << frameType;
            break;
    }
}

void FrameContainer::setFramePos(const QPointF &p)
{
    switch (frameType) {
        case NoFrame:
            break;
        case Rectangle:
            rectFrame->setPos(p);
            break;
        case RoundedRectangle:
            pathFrame->setPos(p);
            break;
        case Ellipse:
            ellipseFrame->setPos(p);
            break;
        case Cloud:
            pathFrame->setPos(p);
            break;
        default:
            qWarning() << "FrameContainer::setFramePos unknown frame type " << frameType;
            break;
    }
}

QString FrameContainer::saveFrame()
{
    if (frameType == NoFrame)
        return QString();

    QString frameTypeAttr = attribut("frameType", getFrameTypeName());
    QString penColAttr = attribut("penColor", framePen.color().name(QColor::HexArgb));
    QString brushColAttr = attribut("brushColor", frameBrush.color().name(QColor::HexArgb));
    QString paddingAttr = attribut("padding", QString::number(framePadding));
    QString penWidthAttr =
        attribut("penWidth", QString::number(framePen.width()));
    QString incChildren;
    if (frameIncludeChildren)
        incChildren = attribut("includeChildren", "true");
    return singleElement("frame", frameTypeAttr + penColAttr + brushColAttr +
                                      paddingAttr + penWidthAttr +
                                      incChildren);
}

void FrameContainer::reposition()
{
    qDebug() << "FC::reposition()";
}