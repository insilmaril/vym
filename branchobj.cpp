#include <QDebug>

#include "branchobj.h"

#include "attributeitem.h"
#include "branchitem.h"
#include "geometry.h"
#include "mapeditor.h"
#include "mainwindow.h"   
#include "misc.h"

extern FlagRow *standardFlagsMaster;
extern FlagRow *systemFlagsMaster;
extern bool debug;

/////////////////////////////////////////////////////////////////
// BranchObj
/////////////////////////////////////////////////////////////////

BranchObj::BranchObj (QGraphicsScene* s,TreeItem *ti):OrnamentedObj (s,ti)
{
    //qDebug ()<< "Const BranchObj (s,ti) ti="<<ti;
    scene=s;
    treeItem=ti;
    BranchItem *pi=(BranchItem*)(ti->parent());
    if (pi && pi!=ti->getModel()->getRootItem() )
	parObj=pi->getLMO();
    else
	parObj=NULL;
    init();
}

BranchObj::~BranchObj ()
{
    //qDebug()<< "Destr BranchObj of "<<this<<" ("<<treeItem->getHeading()<<")";

    // If I'm animated, I need to un-animate myself first
    if (anim.isAnimated() )
    {
	anim.setAnimated (false);
	VymModel *model=treeItem->getModel();
	model->stopAnimation (this);
    }

    clear();
}

void BranchObj::init () 
{
    if (parObj)
    {
	absPos=getRandPos();
	absPos+=parObj->getChildPos();
    }
}

void BranchObj::copy (BranchObj* other)
{
    OrnamentedObj::copy(other);

    setVisibility (other->visible);

    positionBBox();
}

void BranchObj::clear() 
{
    //setVisibility (true); //FIXME-4 needed?
}

void BranchObj::setParObjTmp(LinkableMapObj* dst, QPointF m, int off)	
{
    // Temporary link to dst
    // m is position of mouse pointer 
    // offset 0: default 1: below dst   -1 above dst  (if possible)

    BranchItem *dsti=(BranchItem*)(dst->getTreeItem());

    BranchItem *pi=(BranchItem*)(dsti->parent());
    int pi_depth=pi->depth();
    BranchObj* bodst=(BranchObj*)dst;

    if (!tmpParent) 
    {
	tmpParent=true;
	parObjTmpBuf=parObj;
    }

    if (pi_depth<2) off=0;
    if (off==0)
	link2ParPos=false;
    else
	link2ParPos=true;
    parObj=bodst;

    setLinkStyle (dst->getDefLinkStyle (dsti));
 
    // Move temporary to new position at destination
    // Usually the positioning would be done by reposition(),
    // but then also the destination branch would "Jump" around...
    // Better just do it approximately
    if (dsti->depth()==0)   
    {	// new parent is a mapcenter
	Vector v= ( m - bodst->getChildPos());
	v.normalize();
	v.scale (150);
	move2RelPos (v.toQPointF());
    } else
    {	
	qreal y;
	if (off==0)
	{
		// Below is needed e.g. in a freshly loaded map, 
		// bboxTotal seems not to be correct yet
		// relinking positions too far below then
		calcBBoxSizeWithChildren(); 

	    // new parent is just a branch, link to it
	    bodst->calcBBoxSizeWithChildren();
	    QRectF t=bodst->getTotalBBox();
	    if (dsti->getLastBranch())
		y=t.y() + t.height() ;
	    else
		y=t.y();

	    y=t.bottom();

	} else
	{
	    if (off<0)
		// we want to link above dst
		y=bodst->y() - height() + 5;
	    else    
		// we want to link below dst
		// Bottom of sel should be 5 pixels above
		// the bottom of the branch _below_ the target:
		// Don't try to find that branch, guess 12 pixels
		y=bodst->getChildPos().y()  -height() + 12; 
	}   
	if (bodst->getOrientation()==LinkableMapObj::LeftOfCenter)
	    move ( bodst->getChildPos().x() - linkwidth, y );
	else	
	    move (bodst->getChildPos().x() + linkwidth, y );
    }	

    // updateLinkGeometry is called implicitly in move
    requestReposition();    
}

void BranchObj::unsetParObjTmp()
{
    if (tmpParent) 
    {
	tmpParent=false;
	link2ParPos=false;
	parObj=parObjTmpBuf;
	parObjTmpBuf=NULL;
	setLinkStyle (getDefLinkStyle(treeItem->parent() ) );
	updateLinkGeometry();
    }	    
}

void BranchObj::setVisibility(bool v, int toDepth)
{
    BranchItem *bi=(BranchItem*)treeItem;
    if (bi->depth() <= toDepth)
    {
	frame->setVisibility(v);
	heading->setVisibility(v);
	systemFlags->setVisibility(v);
	standardFlags->setVisibility(v);
	LinkableMapObj::setVisibility (v);
	int i;
	for (i=0; i<treeItem->imageCount(); ++i)
	    treeItem->getImageObjNum(i)->setVisibility (v);
	for (i=0; i<treeItem->xlinkCount(); ++i)    
	    treeItem->getXLinkObjNum(i)->setVisibility ();  

	// Only change children, if I am not scrolled
	if (! bi->isScrolled() && (bi->depth() < toDepth))
	{
	    // Now go recursivly through all children //FIXME-3 are there multiple calls for lower level items???
	    for (i=0; i<treeItem->branchCount(); ++i)
		treeItem->getBranchObjNum(i)->setVisibility (v,toDepth);    
	}
    } // depth <= toDepth   
    requestReposition();
}   

void BranchObj::setVisibility(bool v)
{
    setVisibility (v,MAX_DEPTH);
}


void BranchObj::setLinkColor ()
{
    // Overloaded from LinkableMapObj
    // BranchObj can use color of heading

    VymModel *model=treeItem->getModel();
    if (model)
    {
	if (model->getMapLinkColorHint()==HeadingColor)
	    LinkableMapObj::setLinkColor (heading->getColor() );
	else	
	    LinkableMapObj::setLinkColor ();
    }	    
}

void BranchObj::updateContentSize()
{
    calcBBoxSize();
    positionBBox();
    requestReposition();
}

void BranchObj::positionContents()
{
    for (int i=0; i<treeItem->imageCount(); ++i)
	treeItem->getImageObjNum(i)->reposition();
    OrnamentedObj::positionContents();
}

void BranchObj::move (double x, double y)
{
    OrnamentedObj::move (x,y);
    positionBBox();
}

void BranchObj::move (QPointF p)
{
    move (p.x(), p.y());
}

void BranchObj::moveBy (double x, double y)
{
    OrnamentedObj::moveBy (x,y);
    for (int i=0; i<treeItem->branchCount(); ++i)
	treeItem->getBranchObjNum(i)->moveBy (x,y);
    positionBBox();
}
    
void BranchObj::moveBy (QPointF p)
{
    moveBy (p.x(), p.y());
}


void BranchObj::positionBBox()
{
    QPointF ap=getAbsPos();
    //ap.setX(ap.x()+frame->getPadding());
    //ap.setY(ap.y()+frame->getPadding());
    bbox.moveTopLeft (ap);
    positionContents();

    //Update links to other branches	
    XLinkObj *xlo;
    for (int i=0; i<treeItem->xlinkCount(); ++i)    
    {
	xlo=treeItem->getXLinkObjNum(i);
	if (xlo) xlo->updateXLink();
    }	
}

void BranchObj::calcBBoxSize()
{
    QSizeF heading_r=heading->getSize();
    qreal heading_w=(qreal) heading_r.width() ;
    qreal heading_h=(qreal) heading_r.height() ;
    QSizeF sysflags_r=systemFlags->getSize();
    qreal sysflags_h=sysflags_r.height();
    qreal sysflags_w=sysflags_r.width();
    QSizeF stanflags_r=standardFlags->getSize();
    qreal stanflags_h=stanflags_r.height();
    qreal stanflags_w=stanflags_r.width();
    qreal w;
    qreal h;

    // set width to sum of all widths
    w=heading_w + sysflags_w + stanflags_w;

    // set height to maximum needed height
    h=max (sysflags_h,stanflags_h);
    h=max (h,heading_h);

    // Save the dimension of flags and heading
    ornamentsBBox.setSize ( QSizeF(w,h));

    // clickBox includes Flags and Heading
    clickBox.setSize (ornamentsBBox.size() );

    // Floatimages 
    QPointF rp;

    topPad=botPad=leftPad=rightPad=0;
    bool incV=((BranchItem*)treeItem)->getIncludeImagesVer();
    bool incH=((BranchItem*)treeItem)->getIncludeImagesHor();
    if (incH || incV)
    {
	FloatImageObj *fio;
	for (int i=0; i<treeItem->imageCount(); ++i )	
	{
	    fio=treeItem->getImageObjNum(i);
	    rp=fio->getRelPos();
	    if (incV)
	    {
		if (rp.y() < 0) 
		    topPad=max (topPad,-rp.y()-h);
		if (rp.y()+fio->height() > 0)
		    botPad=max (botPad,rp.y()+fio->height());
	    }	    
	    if (incH)
	    {
		if (orientation==LinkableMapObj::RightOfCenter)
		{
		    if (-rp.x()-w > 0) 
			leftPad=max (leftPad,-rp.x()-w);
		    if (rp.x()+fio->width() > 0)
			rightPad=max (rightPad,rp.x()+fio->width());
		} else
		{
		    if (rp.x()< 0) 
			leftPad=max (leftPad,-rp.x());
		    if (rp.x()+fio->width() > w)
			rightPad=max (rightPad,rp.x()+fio->width()-w);
		}
	    }	    
	}   
	h+=topPad+botPad;
	w+=leftPad+rightPad;
    }

    // Frame thickness
    w+=frame->getPadding()*2;
    h+=frame->getPadding()*2;
    
    // Finally set size
    bbox.setSize (QSizeF (w,h));
    if (debug) qDebug()<<"BO: calcBBox "<<treeItem->getHeading()<<" bbox="<<bbox;
}

void BranchObj::setDockPos()
{
    if (treeItem->getType()==TreeItem::MapCenter)
    {
	// set childPos to middle of MapCenterObj
	childPos.setX( clickBox.topLeft().x() + clickBox.width()/2 );
	childPos.setY( clickBox.topLeft().y() + clickBox.height()/2 );
	parPos=childPos;	
	for (int i=0; i<treeItem->branchCount(); ++i)
	    treeItem->getBranchObjNum(i)->updateLinkGeometry();

    } else
    {
	if (orientation==LinkableMapObj::LeftOfCenter )
	{
	    if ( ((BranchItem*)treeItem)->getFrameIncludeChildren() )
	    {
		childPos=QPointF (ornamentsBBox.bottomLeft().x(),  bottomlineY);
		parPos=QPointF   (bboxTotal.bottomRight().x()-frame->getPadding()/2, bottomlineY);
	    } else	
	    {
		childPos=QPointF (ornamentsBBox.bottomLeft().x()-frame->getPadding(),  bottomlineY);
		parPos=QPointF   (ornamentsBBox.bottomRight().x(), bottomlineY);
	    }
	} else
	{
	    if ( ((BranchItem*)treeItem)->getFrameIncludeChildren() )
	    {
		childPos=QPointF(ornamentsBBox.bottomRight().x(), bottomlineY);
		parPos=QPointF ( bboxTotal.bottomLeft().x()+frame->getPadding()/2,  bottomlineY);
	    } else	
	    {
		childPos=QPointF(ornamentsBBox.bottomRight().x()+ frame->getPadding(), bottomlineY);
		parPos=QPointF ( ornamentsBBox.bottomLeft().x(),  bottomlineY);
	    }
	}
    }
}

void BranchObj::updateData()
{
    bool changed=false;
    if (!treeItem)
    {
	qWarning ("BranchObj::udpateHeading treeItem==NULL");
	return;
    }
    QString s=treeItem->getHeading();
    if (s!=heading->text())
    {
	heading->setText (s);
	changed=true;
    }
    QStringList TIactiveFlags=treeItem->activeStandardFlagNames();

    // Add missing standard flags active in TreeItem
    for (int i=0;i<=TIactiveFlags.size()-1;i++)
    {	
	if (!standardFlags->isActive (TIactiveFlags.at(i) ))
	{
	    Flag *f=standardFlagsMaster->getFlag(TIactiveFlags.at(i));
	    if (f) standardFlags->activate (f);
	    changed=true;
	}
    }
    // Remove standard flags no longer active in TreeItem
    QStringList BOactiveFlags=standardFlags->activeFlagNames();
    for (int i=0;i<BOactiveFlags.size();++i)
	if (!TIactiveFlags.contains (BOactiveFlags.at(i)))
	{
	    standardFlags->deactivate (BOactiveFlags.at(i));
	    changed=true;
	}   

    // Add missing system flags active in TreeItem
    TIactiveFlags=treeItem->activeSystemFlagNames();
    for (int i=0;i<TIactiveFlags.size();++i)
    {	
	if (!systemFlags->isActive (TIactiveFlags.at(i) ))
	{
	    Flag *f=systemFlagsMaster->getFlag(TIactiveFlags.at(i));
	    if (f) systemFlags->activate (f);
	    changed=true;
	}
    }
    // Remove system flags no longer active in TreeItem
    BOactiveFlags=systemFlags->activeFlagNames();
    for (int i=0;i<BOactiveFlags.size();++i)
    {
	if (!TIactiveFlags.contains (BOactiveFlags.at(i)))
	{
	    systemFlags->deactivate (BOactiveFlags.at(i));
	    changed=true;
	}   
    }
    if (changed) updateContentSize(); 
}

void BranchObj::setDefAttr (BranchModification mod, bool keepFrame)
{
    int fontsize;
    switch (treeItem->depth())
    {
	case 0: 
	    fontsize=16; 
	    break;
	case 1: 
	    fontsize=14; 
	    break;
	case 2: 
	    fontsize=12; 
	    break;
	default: 
	    fontsize=10; 
	    break;
    }	
    setLinkStyle(getDefLinkStyle(treeItem->parent() ));
    setLinkColor ();
    QFont font("Sans Serif,8,-1,5,50,0,0,0,0,0");
    font.setPointSize(fontsize);
    heading->setFont(font );

    if (mod==NewBranch && !keepFrame)
    {
	if (treeItem->depth()==0)
	    setFrameType (FrameObj::Rectangle);
	else	
	    setFrameType (FrameObj::NoFrame);
    }
    if (mod==NewBranch)
	setColor (treeItem->getHeadingColor() );
    else
    {
	// Also set styles for children
	for (int i=0; i<treeItem->branchCount(); ++i)
	    treeItem->getBranchObjNum(i)->setDefAttr(MovedBranch);
    }	    
    calcBBoxSize();
}

void BranchObj::alignRelativeTo (QPointF ref,bool alignSelf)
{
    qreal th = bboxTotal.height();  
    int depth=0;
    if (parObj)	depth=1 + parObj->getTreeItem()->depth();
// TODO testing
/*
*/
if (debug)
{
    QString h=QString (depth+1,' ');
    h+=treeItem->getHeading();
    h+=QString (15,' ');
    h.truncate (15);
    QPointF pp; 
    if (parObj) pp=parObj->getChildPos();
    qDebug() << "BO::alignRelTo "<<h;
    qDebug() << "    d="<<depth;
    qDebug() <<"  parO="<<parObj;
    qDebug() <<"   ref="<<ref;
    //qDebug() <<   "  bbox.tL="<<bboxTotal.topLeft();
    qDebug() <<"absPos="<<absPos<<
	"  relPos="<<relPos<<
//	"  parPos="<<pp<<
	"  bbox="<<bbox;
//	"  orient="<<orientation<<
//	"  alignSelf="<<alignSelf<<
//	"  scrolled="<<((BranchItem*)treeItem)->isScrolled()<<
//	"  pad="<<topPad<<","<<botPad<<","<<leftPad<<","<<rightPad<<
//	"  hidden="<<hidden<<
//	"  th="<<th<<
}

    setOrientation();
    //updateLinkGeometry();

    if (depth==1)
    {
	move2RelPos (getRelPos() );
    }
    if (depth>1)
    {
	// Align myself depending on orientation and parent, but
	// only if I am not a mainbranch or mapcenter itself

	if (anim.isAnimated())
	{
	    move2RelPos(anim);
	} else
	{
	    LinkableMapObj::Orientation o;
	    o=parObj->getOrientation();
	    if (alignSelf)
		switch (orientation) 
		{
		    case LinkableMapObj::LeftOfCenter:
			move (ref.x() - bbox.width(), ref.y() + (th-bbox.height())/2 );
		    break;
		    case LinkableMapObj::RightOfCenter:	
			move (ref.x() , ref.y() + (th-bbox.height())/2  );
		    break;
		    default:
			qWarning ("LMO::alignRelativeTo: oops, no orientation given...");
		    break;
	    }
	}
    }	    

    if ( ((BranchItem*)treeItem)->isScrolled() ) return;

    // Set reference point for alignment of children
    QPointF ref2;
    if (orientation==LinkableMapObj::LeftOfCenter)
	ref2.setX(childPos.x() - linkwidth);
    else    
	ref2.setX(childPos.x() + linkwidth);

    if (depth==1)
	ref2.setY(absPos.y()-(bboxTotal.height()-bbox.height())/2 + frame->getPadding() );
    else    
	ref2.setY (ref.y() + frame->getPadding());  

    // Align the attribute children depending on reference point 
    // on top like in TreeEditor
    for (int i=0; i<treeItem->attributeCount(); ++i)
    {	
	if (!treeItem->getAttributeNum(i)->isHidden())
	{
	    BranchObj *bo=(BranchObj*)(treeItem->getAttributeNum(i)->getBranchObj());
	    if (!bo) break;
	    bo->alignRelativeTo (ref2,true);

	    // append next branch below current one
	    ref2.setY(ref2.y() + bo->getTotalBBox().height() );
	}
    }
    // Align the branch children depending on reference point 
    for (int i=0; i<treeItem->branchCount(); ++i)
    {	
	if (!treeItem->getBranchNum(i)->isHidden())
	{
	    treeItem->getBranchObjNum(i)->alignRelativeTo (ref2,true);

	    // append next branch below current one
	    ref2.setY(ref2.y() + treeItem->getBranchObjNum(i)->getTotalBBox().height() );
	}
    }
}


void BranchObj::reposition()
{   
/* TODO testing only
    if (!treeItem->getHeading().isEmpty())
	qDebug()<< "BO::reposition  "<<treeItem->depth()<<" "<<treeItem->getHeading()<<endl;
    else    
	qDebug()<< "BO::reposition  ???";
*/	

    if (treeItem->depth()==0)
    {
	// only calculate the sizes once. If the deepest LMO 
	// changes its height,
	// all upper LMOs have to change, too.
	calcBBoxSizeWithChildren();
	updateLinkGeometry();	// This update is needed if the scene is resized 
			// due to excessive moving of a FIO

	alignRelativeTo ( QPointF (absPos.x(),
	    absPos.y()-(bboxTotal.height()-bbox.height())/2) );	
	    //absPos.y() ) );
	positionBBox();	// Reposition bbox and contents
    } else
    {
	// This is only important for moving branches:
	// For editing a branch it isn't called...
	alignRelativeTo ( QPointF (absPos.x(),
			    absPos.y()-(bboxTotal.height()-bbox.height())/2) );	
    }
}

void BranchObj::unsetAllRepositionRequests()
{
    repositionRequest=false;
    for (int i=0; i<treeItem->branchCount(); ++i)
	treeItem->getBranchObjNum(i)->unsetAllRepositionRequests();
}

QRectF BranchObj::getTotalBBox()
{
    return bboxTotal;
}

ConvexPolygon BranchObj::getBoundingPolygon()	
{
/*
    if (!pi)	//FIXME-3 Testing only
    {
	pi=scene->addPolygon(MapObj::getBoundingPolygon() );
	pi->setPen(Qt::NoPen);
	pi->setBrush( QColor(qrand()%32*8,qrand()%32*8,qrand()%32*8) );
	pi->setZValue(Z_BBOX);
    }
    */

    if (treeItem->branchCount()==0 || treeItem->depth()==0)
    {
	if (pi) pi->setPolygon (MapObj::getBoundingPolygon() );
	return MapObj::getBoundingPolygon();
    }

//    calcBBoxSizeWithChildren();	//FIXME-3 really needed?
    QPolygonF p;
    p<<bboxTotal.topLeft();
    p<<bboxTotal.topRight();
    p<<bboxTotal.bottomRight();
    p<<bboxTotal.bottomLeft();
    //cout << "BO::getBP (total)  "<<treeItem->getHeadingStd()<<"  tL="<<bboxTotal.topLeft()<<"  bR="<<bboxTotal.bottomRight()<<endl;
    //cout << "                   "<<"  tL="<<bbox.topLeft()<<"  bR="<<bbox.bottomRight()<<endl;
    if (pi) pi->setPolygon (p );
    return p;
}

void BranchObj::calcBBoxSizeWithChildren()  //FIXME-3 cleanup testcode
{   
    // This is initially called only from reposition and
    // and only for mapcenter. So it won't be
    // called more than once for a single user 
    // action

    if (debug) qDebug()<<"BO: calcBBoxSizwWithChildren a) for "<<treeItem->getHeading();

    // Calculate size of LMO including all children (to align them later)
    //bboxTotal.setX(bbox.x() );
    //bboxTotal.setY(bbox.y() );

    // if branch is scrolled, ignore children, but still consider floatimages
    BranchItem *bi=(BranchItem*)treeItem;
    if ( bi->isScrolled() ) 
    {
	bboxTotal.setWidth (bbox.width());
	bboxTotal.setHeight(bbox.height());
	if (debug) qDebug()<<"BO: calcBBoxSizwWithChildren abort scrolled";
	return;
    }
    
    if (bi->isHidden())
    {
	bboxTotal.setWidth (0);
	bboxTotal.setHeight(0);
	/*
	if (parObj)
	{
	    bboxTotal.setX (parObj->x());
	    bboxTotal.setY (parObj->y());
	} else
	{
	    bboxTotal.setX (bbox.x());
	    bboxTotal.setY (bbox.y());
	}
	*/
	if (debug) qDebug()<<"BO: calcBBoxSizeWithChildren abort hidden";
	return;
    }
    
    QRectF r(0,0,0,0);
    QRectF br;
    // Now calculate recursivly
    // sum of heights 
    // maximum of widths 
    // minimum of y
    for (int i=0; i<treeItem->branchCount(); i++)
    {
	if (!bi->getBranchNum(i)->isHidden())
	{
	    BranchObj *bo=bi->getBranchObjNum(i);
	    bo->calcBBoxSizeWithChildren();
	    br=bo->getTotalBBox();
	    r.setWidth( max (br.width(), r.width() ));
	    r.setHeight(br.height() + r.height() );
	    if (debug)
	    {
		qDebug()<<"  adding: "<<bo->getTreeItem()->getHeading() <<" to "<<bi->getHeading();
		qDebug()<<"      bo: "<<br;
		qDebug()<<"       r: "<<r;
	    }
	    //if (br.y()<bboxTotal.y()) bboxTotal.setY(br.y());
	    //if (br.x()<bboxTotal.x()) bboxTotal.setX(br.x());
	}
    }
    /*
    for (int i=0; i<treeItem->attributeCount(); i++)
    {
	if (!bi->getAttributeNum(i)->isHidden())
	{
	    BranchObj *bo=bi->getAttributeNum(i)->getBranchObj();
	    bo->calcBBoxSizeWithChildren();
	    br=bo->getTotalBBox();
	    r.setWidth( max (br.width(), r.width() ));
	    r.setHeight(br.height() + r.height() );
	    if (br.y()<bboxTotal.y()) bboxTotal.setY(br.y());
	    if (br.x()<bboxTotal.x()) bboxTotal.setX(br.x());
	}
    }
    */
    // Add myself and also
    // add width of link to sum if necessary
    if (debug) qDebug()<<"BO: calcBBoxSizeWithChildren c) for "<<treeItem->getHeading()<<" bbox="<<bbox<<" r="<<r;

if (bi->branchCount()<1)
	bboxTotal.setWidth (bbox.width() + r.width()  );
    else    
	bboxTotal.setWidth (bbox.width() + r.width() + linkwidth );
//if (bi->branchCount()<1)	//FIXME-3
//	bboxTotal.setWidth (bbox.width() + r.width() + frame->getPadding()*2);
//    else    
//	bboxTotal.setWidth (bbox.width() + r.width() + linkwidth + frame->getPadding()*2);
    
    // bbox already contains frame->padding()*2	    
    bboxTotal.setHeight(max (r.height() + frame->getPadding()*2,  bbox.height()) );
    if (debug) qDebug()<<"BO: calcBBoxSizeWithChildren d) for "<<treeItem->getHeading()<< "bboxTotal="<<bboxTotal;

}

void BranchObj::setAnimation(const AnimPoint &ap)
{
    anim=ap;
}

void BranchObj::stopAnimation()
{
    anim.stop();
    if (useRelPos)
	setRelPos (anim);
    else
	move (anim);
}

bool BranchObj::animate()
{
    anim.animate ();
    if ( anim.isAnimated() )
    {
	if (useRelPos)
	    setRelPos (anim);
	else
	    move (anim);
	return true;
    }
    /*FIXME-3 reposition in BO:animate nearly never reached? needed?	
    if (((MapItem*)treeItem)->getPositionMode()==MapItem::Relative)
	parObj->reposition();	// we might have been relinked meanwhile
    */	
    return false;
}

