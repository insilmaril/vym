#include <QDebug>

#include "floatobj.h"
#include "mapitem.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// FloatObj
/////////////////////////////////////////////////////////////////

FloatObj::FloatObj(QGraphicsItem *parent, TreeItem *ti)
    : OrnamentedObj(parent, ti)
{
    // qDebug() << "Const FloatObj s="<<s<<"  ti="<<ti<<"  treeItem="<<treeItem;
    init();
}

FloatObj::~FloatObj()
{
    //   qDebug() << "Destr FloatObj";
}

void FloatObj::init()
{
    setLinkStyle(LinkableMapObj::Parabel);
    ((MapItem *)treeItem)->setHideLinkUnselected(true);
}

void FloatObj::copy(FloatObj *other)
{
    LinkableMapObj::copy(other);
    setVisibility(other->visible);
}

void FloatObj::move(double x, double y) { MapObj::move(x, y); }

void FloatObj::move(QPointF p) { FloatObj::move(p.x(), p.y()); }

void FloatObj::moveCenter(double x, double y)
{
    absPos = QPointF(x, y);
    bbox.moveTo(x - bbox.width() / 2, y - bbox.height() / 2);
    clickPoly = QPolygonF(bbox);
}

void FloatObj::moveCenter2RelPos(double x, double y)
{
    setRelPos(QPointF(x, y));
    if (parObj) {
        QPointF p = parObj->getFloatRefPos();
        moveCenter(p.x() + x, p.y() + y);
    }
}

void FloatObj::move2RelPos(
    double x, double y) // overloaded to use floatRefPos instead of childRefPos
{
    setRelPos(QPointF(x, y));
    if (parObj) {
        QPointF p = parObj->getFloatRefPos();
        move(p.x() + x, p.y() + y);
    }
}

void FloatObj::move2RelPos(
    QPointF p) // overloaded to use floatRefPos instead of childRefPos
{
    move2RelPos(p.x(), p.y());
}

void FloatObj::setRelPos()
{
    if (parObj)
        setRelPos(absPos - parObj->getFloatRefPos());
    else
        qWarning() << "FO::setRelPos parObj==0   this=" << this;
}

void FloatObj::setRelPos(const QPointF &p)
{
    if (parObj) {
        relPos = p;
        useRelPos = true;
    }
    else
        qWarning() << "LMO::setRelPos (p)  parObj==0   this=" << this;
}

void FloatObj::setDockPos() { parPos = absPos; }

void FloatObj::reposition()
{
    moveCenter2RelPos(relPos.x(), relPos.y());
    updateLinkGeometry();
}

QRectF FloatObj::getBBoxSizeWithChildren() { return bboxTotal; }
