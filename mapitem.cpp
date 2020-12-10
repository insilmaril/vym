#include "mapitem.h"

#include "linkablemapobj.h"
#include "ornamentedobj.h"

#include <QDebug>

extern FlagRow *systemFlagsMaster;

MapItem::MapItem()
{
    init();
}

MapItem::MapItem(const QList<QVariant> &data, TreeItem *parent):TreeItem (data,parent)
{
    init();
}

void MapItem::init()
{
    mo=NULL;
    posMode=Unused;
    hideLinkUnselected=false;
}

void MapItem::appendChild (TreeItem *item)
{
    TreeItem::appendChild (item);

    // FIXME-4 maybe access parent in MapObjs directly via treeItem
    // and remove this here...

    // If lmo exists, also set parObj there
    LinkableMapObj *lmo=getLMO();
    if (lmo)
    {
	LinkableMapObj *itemLMO=((MapItem*)item)->getLMO();
	if (itemLMO)
	    itemLMO->setParObj (lmo);
    }
}

void MapItem::setRelPos (const QPointF &p)
{
    posMode=Relative;
    pos=p;
    LinkableMapObj *lmo=getLMO();
    if (lmo)
    {
	((OrnamentedObj*)lmo)->setUseRelPos (true);
	((OrnamentedObj*)lmo)->move2RelPos(p);
    }
}

void MapItem::setAbsPos (const QPointF &p)
{
    posMode=Absolute;
    pos=p;
    if (mo) mo->move (p);
}

void MapItem::setPositionMode (PositionMode mode)
{
    posMode=mode;
}

MapItem::PositionMode MapItem::getPositionMode ()
{
    return posMode;
}

void MapItem::setHideLinkUnselected (bool b)
{
    hideLinkUnselected=b;
    LinkableMapObj *lmo=getLMO();
    if (lmo) 
    {
	//lmo->setHideLinkUnselected();
	lmo->setVisibility (lmo->isVisibleObj());
	lmo->updateLinkGeometry();
    }	
}

bool MapItem::getHideLinkUnselected()
{
    return hideLinkUnselected;
}   

QString MapItem::getMapAttr ()	
{
    QString s;
    LinkableMapObj *lmo=getLMO();

    if (parentItem==rootItem)
	posMode=Absolute;
    else
    {
	if (type==TreeItem::Image ||depth()==1 || lmo->getUseRelPos() )
	    posMode=Relative;   //FiXME-2 shouldn't this be replaced by relPos?
	else
	    posMode=Unused;
    }
    switch (posMode)
    {
	case Relative:	
	    if (lmo) pos=lmo->getRelPos();
	    s= attribut("relPosX",QString().setNum(pos.x())) +
	       attribut("relPosY",QString().setNum(pos.y())); 
	    break;
	case Absolute:	
	    if (mo) pos=mo->getAbsPos();
	    s=attribut("absPosX",QString().setNum(pos.x())) +
	      attribut("absPosY",QString().setNum(pos.y())); 
	    break;
	default: break;
    }
    if (hideLinkUnselected)
	s+=attribut ("hideLink","true");
    else
	s+=attribut ("hideLink","false");

    // Rotation angle
    MapObj *mo=getMO();
    if (mo)
	angle=mo->getRotation();
    if (angle!=0)	
	s+=attribut("rotation",QString().setNum(angle) );
	
    return s;
}

QRectF MapItem::getBBoxURLFlag ()
{
    QString s = "system-url";
    QStringList list = systemFlags.activeFlagNames().filter (s);
    if (list.count() > 1)
    {
	qWarning()<<"MapItem::getBBoxURLFlag found more than one system-url*";
	return QRectF ();
    }	

    Flag *f = systemFlagsMaster->findFlag(s);
    if (f) 
    {
        QUuid u = f->getUuid();
        LinkableMapObj *lmo = getLMO();
        if (lmo) return ((OrnamentedObj*)lmo)->getBBoxSystemFlagByUid (u);
    }
    return QRectF ();
}

void MapItem::setRotation(const qreal &a)
{
    angle=a;
    MapObj *mo=getMO();
    if (mo) mo->setRotation (a);
}

MapObj* MapItem::getMO()
{
    return mo;
}

LinkableMapObj* MapItem::getLMO()
{
    if (isBranchLikeType() || type==Image)
	return (LinkableMapObj*)mo;
    else
	return NULL;
}

void MapItem::initLMO()
{
    LinkableMapObj *lmo=getLMO();
    if (!lmo) return;
    switch (posMode)
    {
	case Relative:	
	    lmo->setRelPos (pos);
	    break;
	case Absolute:	
	    lmo->move (pos);
	    break;
	default:
	    break;
    }
}

