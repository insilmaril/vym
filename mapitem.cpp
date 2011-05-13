#include "mapitem.h"

#include "linkablemapobj.h"
#include "ornamentedobj.h"

#include <QDebug>

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
    lmo=NULL;
    posMode=Unused;
    hideLinkUnselected=false;
}

void MapItem::appendChild (TreeItem *item)
{
    TreeItem::appendChild (item);

    // FIXME-4 maybe access parent in MapObjs directly via treeItem
    // and remove this here...

    // If lmo exists, also set parObj there
    if (lmo && (item->isBranchLikeType() || item->getType()==TreeItem::Image) )
    {
	LinkableMapObj *itemLMO=((MapItem*)item)->lmo;
	if (itemLMO)
	    itemLMO->setParObj (lmo);
    }
}

void MapItem::setRelPos (const QPointF &p)
{
    posMode=Relative;
    pos=p;
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
    if (lmo) lmo->move (p);
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

    if (parentItem==rootItem)
	posMode=Absolute;
    else
    {
	if (type==TreeItem::Image ||depth()==1)
	    posMode=Relative;
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
	    if (lmo) pos=lmo->getAbsPos();
	    s=attribut("absPosX",QString().setNum(pos.x())) +
	      attribut("absPosY",QString().setNum(pos.y())); 
	    break;
	default: break;
    }
    if (hideLinkUnselected)
	s+=attribut ("hideLink","true");
    else
	s+=attribut ("hideLink","false");
    return s;
}

QRectF MapItem::getBBoxURLFlag ()
{
    QStringList list=systemFlags.activeFlagNames().filter ("system-url");
    if (list.count()>1)
    {
	qWarning()<<"MapItem::getBBoxURLFlag found more than one system-url*";
	return QRectF ();
    }	
    return getBBoxFlag (list.first());
}

QRectF MapItem::getBBoxFlag (const QString &fname)
{
    if (lmo)
	return ((OrnamentedObj*)lmo)->getBBoxFlag (fname);
    else    
	return QRectF ();
}

MapObj* MapItem::getMO()
{
    return (MapObj*)lmo;
}

LinkableMapObj* MapItem::getLMO()
{
    return lmo;
}

void MapItem::setLMO(LinkableMapObj *l)
{
    lmo=l;
}

void MapItem::initLMO()
{
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

