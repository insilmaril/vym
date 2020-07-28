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
    init ();
}

FlagRowObj::FlagRowObj(QGraphicsItem *parent):MapObj(parent) 
{
//    qDebug() << "Const FlagRowObj (p)";
    init ();
}

FlagRowObj::~FlagRowObj()
{
    //qDebug() << "Destr FlagRowObj";
    while (!flagobjs.isEmpty())
	delete (flagobjs.takeFirst() );
}

void FlagRowObj::init ()
{
    showFlags = true;
}

void FlagRowObj::copy (FlagRowObj* other)
{
    MapObj::copy(other);
    flagobjs.clear();
    for (int i = 0; i < flagobjs.size(); ++i)
	addFlag (flagobjs.at(i));
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
    for (int i = 0; i < flagobjs.size(); ++i)
	flagobjs.at(i)->setZValue (z);
}

void FlagRowObj::setVisibility (bool v)
{
    MapObj::setVisibility(v);
    for (int i = 0; i < flagobjs.size(); ++i)
	flagobjs.at(i)->setVisibility (v);
}

FlagObj* FlagRowObj::addFlag (FlagObj *fo)
{
    FlagObj *newfo = new FlagObj (parentItem() );
    newfo->copy (fo);	// create a deep copy of fo
    newfo->move (absPos.x() + bbox.width(), absPos.y() );
    flagobjs.append(newfo);
    calcBBoxSize();
    positionBBox();
    return newfo;
}

void FlagRowObj::updateActiveFlagObjs(FlagRow *masterRow, const QList <QUuid> &activeFlagUids)
{
    qDebug() << "FRO::updateActiveFOs   " << activeFlagUids.size() << "masterRow: " << masterRow;
    
    bool changed = false;

    // Add missing active flags
    for (int i = 0; i<= activeFlagUids.size() - 1; i++)
    {
        if (!isFlagActive(activeFlagUids.at(i) ))  //FIXME-0 cont here - do we really need standardFlagsRow AND userFlagsRow?
        {
            Flag *f = masterRow->findFlag(activeFlagUids.at(i));
            if (f) 
            {
                qDebug() << "activating flag " << f->getName();
                activateFlag (f);
                changed = true;
            }
        }
    }

    // Remove standard flags no longer active in TreeItem  
    foreach (FlagObj* fo, flagobjs)
        if (!activeFlagUids.contains (fo->getUuid() ))
        {
            qDebug() << "  removing: " << fo->getUuid();
            flagobjs.removeAll (fo);
            delete (fo);
            changed = true;
        }

    if (changed) 
    {
        calcBBoxSize();
        positionBBox();
    }
}

QStringList FlagRowObj::activeFlagNames()
{
    QStringList list;
    for (int i = 0; i < flagobjs.size(); ++i)
	list.append (flagobjs.at(i)->getName());
    return list;
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

bool FlagRowObj::isFlagActiveByName (const QString &foname) // FIXME-0 should become obsolete: no FLO by name, only uuid
{
    FlagObj *fo = findFlagObj (foname);
    if (fo) 
	return true;
    else
	return false;
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
	FlagObj *fo = addFlag (new FlagObj (this));
	fo->load (flag->getPixmap() );
	fo->setName (flag->getName() );
        fo->setUuid (flag->getUuid() );
	fo->activate();
	if (showFlags)	// FIXME-1 necessary? only called from FIO::init
	    fo->setVisibility (visible);
	else
	    fo->setVisibility (false);
	calcBBoxSize();
    }
}

void FlagRowObj::setShowFlags (bool b)
{
    showFlags = b;
}

FlagObj* FlagRowObj::findFlagObj(const QString &name)
{
    for (int i = 0; i < flagobjs.size(); ++i)
	if (flagobjs.at(i)->getName() == name) return flagobjs.at(i);
    return NULL;
}

FlagObj* FlagRowObj::findFlagObj(const QUuid &uid)
{
    for (int i = 0; i < flagobjs.size(); ++i)
	if (flagobjs.at(i)->getUuid() == uid) return flagobjs.at(i);
    return NULL;
}

QString FlagRowObj::findFlagNameByPos (const QPointF &p)
{
    if (!isInBox (p, clickPoly.boundingRect() )) return "";
    for (int i = 0; i < flagobjs.size(); ++i)
	if (isInBox (p, flagobjs.at(i)->getClickPoly().boundingRect() )) return flagobjs.at(i)->getName();
    return "";	
}

