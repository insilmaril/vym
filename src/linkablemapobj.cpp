#include <cstdlib>
#include <iostream>
#include <math.h>

#include "linkablemapobj.h"
#include "vymmodel.h"

extern bool debug;

/////////////////////////////////////////////////////////////////
// LinkableMapObj
/////////////////////////////////////////////////////////////////

LinkableMapObj::LinkableMapObj(QGraphicsItem *parent, TreeItem *ti)
    : MapObj(parent, ti)
{
    // qDebug() << "Const LinkableMapObj this=" << this; // << "  ti=" << ti <<"
    // treeItem="<<treeItem;
}

LinkableMapObj::~LinkableMapObj()
{
    // qDebug()<< "Destructor LMO  this="<<this<<" style="<<style<<" l="<<l<<"
    // p="<<p<<"  segment="<<segment.count();
}

