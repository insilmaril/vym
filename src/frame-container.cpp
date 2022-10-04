#include "frame-container.h"

#include <QColor>
#include <QDebug>
#include <QGraphicsScene>

#include "misc.h" //for roof function

/////////////////////////////////////////////////////////////////
// FrameContainer
/////////////////////////////////////////////////////////////////
FrameContainer::FrameContainer(Container *parent) : Container(parent)
{
    qDebug() << "Constr FrameContainer";
    init();
}

FrameContainer::~FrameContainer()
{
    qDebug() << "Destr FrameContainer";
    clear();
}

void FrameContainer::init()
{
    type = NoFrame;
    clear();
    pen.setColor(Qt::black);
    pen.setWidth(1);
    brush.setColor(Qt::white);
    brush.setStyle(Qt::SolidPattern);
    includeChildren = false;

    setVisible(true);
}

void FrameContainer::clear()
{
    switch (type) {
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
    type = NoFrame;
    padding = 0; // No frame requires also no padding
    xsize = 0;
}

void FrameContainer::setRect(const QRectF &r)
{
    qDebug() << "FC::setRect t=" << type << " r=" << qrectFToString(r, 0);
    frameSize = r;
    switch (type) {
        case NoFrame:
            break;

        case Rectangle:
            rectFrame->setRect(frameSize);
            qDebug() << "  FC  rect: " << rectFrame << "vis=" << rectFrame->isVisible();
            break;

        case RoundedRectangle: {
            QPointF tl = frameSize.topLeft();
            QPointF tr = frameSize.topRight();
            QPointF bl = frameSize.bottomLeft();
            QPointF br = frameSize.bottomRight();
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
                QRectF(frameSize.x(), frameSize.y(), frameSize.width(), frameSize.height()));
            xsize = 20; // max(frameSize.width(), frameSize.height()) / 4;
            break;

        case Cloud:
            QPointF tl = frameSize.topLeft();
            QPointF tr = frameSize.topRight();
            QPointF bl = frameSize.bottomLeft();
            QPainterPath path;
            path.moveTo(tl);

            float w = frameSize.width();
            float h = frameSize.height();
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
            xsize = 50;
            break;
    }
}

void FrameContainer::setPadding(const int &i) { padding = i; }

int FrameContainer::getPadding()
{
    if (type == NoFrame)
        return 0;
    else
        return padding;
}

qreal FrameContainer::getTotalPadding() { return xsize + padding + pen.width(); }

qreal FrameContainer::getXPadding() { return xsize; }

void FrameContainer::setBorderWidth(const int &i)
{
    pen.setWidth(i);
    repaint();
}

int FrameContainer::getBorderWidth() { return pen.width(); }

FrameContainer::FrameType FrameContainer::getFrameType() { return type; }

FrameContainer::FrameType FrameContainer::getFrameTypeFromString(const QString &s)  // FIXME-2 not used, but should in script and undo!
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
    switch (type) {
    case Rectangle:
        return "Rectangle";
        break;
    case RoundedRectangle:
        return "RoundedRectangle";
        break;
    case Ellipse:
        return "Ellipse";
        break;
    case Cloud:
        return "Cloud";
        break;
    default:
        return "NoFrame";
    }
}

void FrameContainer::setFrameType(const FrameType &t)
{
    if (t != type) {
        clear();
        type = t;
        switch (type) {
            case NoFrame:
                break;
            case Rectangle:
                rectFrame = new QGraphicsRectItem;
                rectFrame->setPen(pen);
                rectFrame->setBrush(brush);
                rectFrame->setZValue(dZ_FRAME_LOW);
                rectFrame->setParentItem(this);
                rectFrame->show();
                break;
            case Ellipse:
                ellipseFrame = scene()->addEllipse(QRectF(0, 0, 0, 0),
                                                   pen, brush);
                ellipseFrame->setPen(pen);
                ellipseFrame->setBrush(brush);
                ellipseFrame->setZValue(dZ_FRAME_LOW);
                ellipseFrame->setParentItem(this);
                ellipseFrame->show();
                break;
            case RoundedRectangle: {
                QPainterPath path;
                pathFrame = new QGraphicsPathItem;
                pathFrame->setPen(pen);
                pathFrame->setBrush(brush);
                pathFrame->setZValue(dZ_FRAME_LOW);
                pathFrame->setParentItem(this);
                pathFrame->show();
            } break;
            case Cloud: {
                QPainterPath path;
                pathFrame = scene()->addPath(path, pen, brush); ///// FIXME-2 see below
                pathFrame->setPen(pen);
                pathFrame->setBrush(brush);
                pathFrame->setZValue(dZ_FRAME_LOW);
                pathFrame->setParentItem(this);
                pathFrame->show();
            }
            break;
        }
    }
    setVisibility(visible);
    reposition();
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

void FrameContainer::setPenColor(QColor col)
{
    pen.setColor(col);
    repaint();  // FIXME-2 needed?
}

QColor FrameContainer::getPenColor() { return pen.color(); }

void FrameContainer::setBrushColor(QColor col)
{
    brush.setColor(col);
    repaint();  // FIXME-2 needed?
}

QColor FrameContainer::getBrushColor() { return brush.color(); }

void FrameContainer::setIncludeChildren(bool b) { includeChildren = b; }

bool FrameContainer::getIncludeChildren() { return includeChildren; }

void FrameContainer::repaint()
{
    // Repaint, when e.g. borderWidth has changed or a color
    qDebug() << "FC::repaint  bc=" << parentContainer()->getName();
    switch (type) {
        case Rectangle:
            rectFrame->setPen(pen);
            rectFrame->setBrush(brush);
            break;
        case RoundedRectangle:
            pathFrame->setPen(pen);
            pathFrame->setBrush(brush);
            break;
        case Ellipse:
            ellipseFrame->setPen(pen);
            ellipseFrame->setBrush(brush);
            break;
        case Cloud:
            pathFrame->setPen(pen);
            pathFrame->setBrush(brush);
            break;
        default:
            break;
    }
}

void FrameContainer::setZValue(double z)
{
    switch (type) {
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
    }
}

void FrameContainer::setVisibility(bool v)
{
    // FIXME-0 MapObj::setVisibility(v);
    qDebug() << "FC::setVis " << v;
    return; // FIXME-0 all children will be handled automatically
    switch (type) {
    case NoFrame:
        break;
    case Rectangle:
        if (visible)
            rectFrame->show();
        else
            rectFrame->hide();
        break;
    case RoundedRectangle:
        if (visible)
            pathFrame->show();
        else
            pathFrame->hide();
        break;
    case Ellipse:
        if (visible)
            ellipseFrame->show();
        else
            ellipseFrame->hide();
        break;
    case Cloud:
        if (visible)
            pathFrame->show();
        else
            pathFrame->hide();
        break;
    }
}

QString FrameContainer::saveToDir()
{
    QString frameTypeAttr = attribut("frameType", getFrameTypeName());
    if (type == NoFrame)
        return singleElement("frame", frameTypeAttr);

    QString penColAttr = attribut("penColor", pen.color().name());
    QString brushColAttr = attribut("brushColor", brush.color().name());
    QString paddingAttr = attribut("padding", QString::number(padding));
    QString borderWidthAttr =
        attribut("borderWidth", QString::number(pen.width()));
    QString incChildren;
    if (includeChildren)
        incChildren = attribut("includeChildren", "true");
    return singleElement("frame", frameTypeAttr + penColAttr + brushColAttr +
                                      paddingAttr + borderWidthAttr +
                                      incChildren);
}

void FrameContainer::reposition()
{
    setVisible(true);     // FIXME-0 testing
    setRect(parentContainer()->rect());
    qDebug() << "FC::reposition " + info() + qrectFToString(parentContainer()->rect(),0) << "vis=" << isVisible();
}
