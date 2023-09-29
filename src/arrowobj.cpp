#include "arrowobj.h"
#include "geometry.h"

#include <QDebug>
#include <QGraphicsScene>

/////////////////////////////////////////////////////////////////
// ArrowObj
/////////////////////////////////////////////////////////////////

ArrowObj::ArrowObj(MapObj *parent) : MapObj(parent) { init(); }

ArrowObj::~ArrowObj()
{
    delete arrowBegin;
    delete arrowEnd;
    delete line;
}

void ArrowObj::init()
{
    QPen pen;

    pen.setStyle(Qt::SolidLine);
    arrowBegin = scene()->addPolygon(QPolygonF(), pen);
    arrowBegin->setParentItem(this);
    arrowEnd = scene()->addPolygon(QPolygonF(), pen);
    arrowEnd->setParentItem(this);

    line = scene()->addLine(QLineF(), pen);

    arrowSize = 4;
    useFixedLength = false;
    setStyleBegin(None);
    setStyleEnd(HeadFull);
}

void ArrowObj::setPen(QPen p)
{
    pen = p;
    line->setPen(pen);

    // end shall have same style as xlink
    QPen pen_solid = pen;
    pen_solid.setStyle(Qt::SolidLine);
    arrowBegin->setPen(pen_solid);
    arrowEnd->setPen(pen_solid);

    setStyleBegin(styleBegin);
    setStyleEnd(styleEnd);
}

QPen ArrowObj::getPen() { return pen; }

void ArrowObj::setArrowSize(qreal r) { arrowSize = r; }

qreal ArrowObj::getArrowSize() { return arrowSize; }

void ArrowObj::setUseFixedLength(bool b) { useFixedLength = b; }

bool ArrowObj::getUseFixedLength() { return useFixedLength; }

void ArrowObj::setFixedLength(int i) { fixedLength = i; }

int ArrowObj::getFixedLength() { return fixedLength; }

void ArrowObj::show() { setVisibility(true); }  // FIXME-0 needed?

void ArrowObj::hide() { setVisibility(false); } // FIXME-0 needed?

void ArrowObj::setVisibility(bool b)    // FIXME-0 needed?
{
    qDebug() << "AO::setVis b=" << b;
    if (b) {
        if (styleEnd != None)
            arrowEnd->show();
        else
            arrowEnd->hide();
        if (useFixedLength && fixedLength == 0)
            line->hide();
        else
            line->show();
    }
    else {
        arrowEnd->hide();
        line->hide();
    }
}

void ArrowObj::setEndPoint(QPointF p)
{
    endPoint = p;

    line->setLine(pos().x(), pos().y(), p.x(), p.y());

    qreal a = getAngle(pos() - endPoint);
    arrowEnd->setRotation(-a / 6.28 * 360);
}

QPointF ArrowObj::getEndPoint() { return endPoint; }

void ArrowObj::setStyleBegin(const QString &s)
{
    if (s == "HeadFull")
        setStyleBegin(ArrowObj::HeadFull);
    else
        setStyleBegin(ArrowObj::None);
}

void ArrowObj::setStyleBegin(OrnamentStyle os)
{
    styleBegin = os;
    switch (styleBegin) {
    case HeadFull:
        arrowEnd->setPolygon(getArrowHead());
        arrowBegin->setBrush(pen.color());
        break;
    case Foot:
        break;
    case None:
        arrowBegin->setPolygon(QPolygonF());
        break;
    }
}

ArrowObj::OrnamentStyle ArrowObj::getStyleBegin() { return styleBegin; }

void ArrowObj::setStyleEnd(const QString &s)
{
    if (s == "HeadFull")
        setStyleEnd(ArrowObj::HeadFull);
    else
        setStyleEnd(ArrowObj::None);
}

void ArrowObj::setStyleEnd(OrnamentStyle os)
{
    styleEnd = os;
    switch (styleEnd) {
    case HeadFull:
        arrowEnd->setPolygon(getArrowHead());
        arrowEnd->setBrush(pen.color());
        break;
    case Foot:
        break;
    case None:
        arrowEnd->setPolygon(QPolygonF());
        break;
    }
}

QPolygonF ArrowObj::getArrowHead()
{
    QPointF a, b, c;
    QPolygonF pa;
    b = a + QPointF(-arrowSize * 2, -arrowSize);
    c = a + QPointF(-arrowSize * 2, +arrowSize);
    pa << a << b << c;
    return pa;
}

ArrowObj::OrnamentStyle ArrowObj::getStyleEnd() { return styleEnd; }

QString ArrowObj::styleToString(const OrnamentStyle &os)
{
    switch (os) {
    case HeadFull:
        return "HeadFull";
        break;
    case None:
        return "None";
        break;
    default:
        qWarning() << "ArrowObj::styleToString unknown style " << os;
    }
    return "Unknown";
}
