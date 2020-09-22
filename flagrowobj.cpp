#include <QDebug>
#include <QToolBar>

#include "flag.h"
#include "flagrowobj.h"

#include "geometry.h"

/////////////////////////////////////////////////////////////////
// FlagRowObj
/////////////////////////////////////////////////////////////////
FlagRowObj::FlagRowObj()
{
//    qDebug() << "Const FlagRowObj ()";
}

FlagRowObj::FlagRowObj(QGraphicsItem *parent):MapObj(parent) 
{
//    qDebug() << "Const FlagRowObj (p)";
}

FlagRowObj::~FlagRowObj()
{
    //qDebug() << "Destr FlagRowObj";
    while (!flagobjs.isEmpty())
	delete (flagobjs.takeFirst() );
}

void FlagRowObj::move(double x, double y)
{
    MapObj::move(x,y);
    qreal dx = 0;
    for (int i = 0; i < flagobjs.size(); ++i)
    {
	flagobjs.at(i)->move(x+dx,y);
	dx += QSizeF(flagobjs.at(i)->getSize() ).width();
    }
}

void FlagRowObj::moveBy(double x, double y)
{
    move (x + absPos.x(), y + absPos.y() );
}

void FlagRowObj::setZValue (double z)
{
    QGraphicsItem::setZValue(z);
    for (int i = 0; i < flagobjs.size(); ++i)
	flagobjs.at(i)->setZValue (z);
}

void FlagRowObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
    for (int i = 0; i < flagobjs.size(); ++i)
	flagobjs.at(i)->setVisibility (v);
}

void FlagRowObj::updateActiveFlagObjs (const QList <QUuid> activeFlagUids, FlagRow *masterRowMain,  FlagRow *masterRowOptional )
{
    bool changed = false;

    // Add missing active flags
    for (int i = 0; i<= activeFlagUids.size() - 1; i++)
    {
        if (!isFlagActive(activeFlagUids.at(i) ))  
        {
            Flag *f = masterRowMain->findFlag(activeFlagUids.at(i));
            if (f) 
            {
                qDebug() << "FRO::activating main" << i << f<< f->getName();
                activateFlag (f);
                changed = true;
            }
            if (masterRowOptional)
            {
                f = masterRowOptional->findFlag(activeFlagUids.at(i));
                if (f) 
                {
                    qDebug() << "FRO::activating opt" << i << f;
                    activateFlag (f);
                    changed = true;
                }
            }
        }
    }

    // Remove flags no longer active in TreeItem  
    foreach (FlagObj* fo, flagobjs)
        if (!activeFlagUids.contains (fo->getUuid() ))
        {
            qDebug() << "FRO::removing " << fo << "count: " << flagobjs.removeAll (fo);
            delete (fo);
            changed = true;
        }

    if (changed) 
    {
        calcBBoxSize();
        positionBBox();
    }
}

void FlagRowObj::positionBBox()
{
    bbox.moveTopLeft(absPos );
    clickPoly = QPolygonF (bbox);
}

void FlagRowObj::calcBBoxSize()
{
    QSizeF size(0,0);
    QSizeF boxsize(0,0);
    for (int i = 0; i < flagobjs.size(); ++i)
    {
	size = flagobjs.at(i)->getSize();
	// add widths
	boxsize.setWidth(boxsize.width() + size.width() );
	// maximize height
	if (size.height() > boxsize.height() ) 
	    boxsize.setHeight( size.height() );
    }
    bbox.setSize (boxsize);
    clickPoly = QPolygonF (bbox);
}

bool FlagRowObj::isFlagActive(const QUuid &uid)
{
    FlagObj *fo = findFlagObj (uid);
    if (fo) 
	return true;
    else
	return false;
}

void FlagRowObj::activateFlag (Flag *flag)	
{
    if (flag) 
    {
	FlagObj *fo = new FlagObj (this);
	fo->loadImage (flag->getImageObj() );
        fo->setUuid (flag->getUuid() );
        fo->setZValue(QGraphicsItem::zValue());
        fo->move (absPos.x() + bbox.width(), absPos.y() );
        flagobjs.append(fo);
        calcBBoxSize();
        positionBBox();
    }
}

FlagObj* FlagRowObj::findFlagObj(const QString &name)   // FIXME-1 
{
    /*
    for (int i = 0; i < flagobjs.size(); ++i)
	if (flagobjs.at(i)->getName() == name) return flagobjs.at(i);
        */
    return NULL;
}

FlagObj* FlagRowObj::findFlagObj(const QUuid &uid)
{
    for (int i = 0; i < flagobjs.size(); ++i)
	if (flagobjs.at(i)->getUuid() == uid) return flagobjs.at(i);
    return NULL;
}

QUuid FlagRowObj::findFlagUidByPos (const QPointF &p)
{
    if (!isInBox (p, clickPoly.boundingRect() )) return QUuid();
    for (int i = 0; i < flagobjs.size(); ++i)
	if (isInBox (p, flagobjs.at(i)->getClickPoly().boundingRect() )) return flagobjs.at(i)->getUuid();
    return QUuid();	
}

