#include "mapeditor.h"

#include <QObject>

#include "branchitem.h"
#include "geometry.h"
#include "mainwindow.h"
#include "misc.h"
#include "warningdialog.h"
#include "xlinkitem.h"


extern Main *mainWindow;
extern QString tmpVymDir;
extern QString clipboardDir;
extern QString clipboardFile;
extern bool clipboardEmpty;
extern bool debug;
extern QPrinter *printer;

extern QMenu* branchContextMenu;
extern QMenu* floatimageContextMenu;
extern QMenu* canvasContextMenu;

extern Settings settings;
extern QString iconPath;

///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
MapEditor::MapEditor( VymModel *vm) 
{
    //qDebug() << "Constructor ME "<<this;
    mapScene= new QGraphicsScene(NULL);
    mapScene->setBackgroundBrush (QBrush(Qt::white, Qt::SolidPattern));

    zoomFactor=zoomFactorTarget=1;

    model=vm;
    model->registerEditor(this);
    model->makeDefault();   // No changes in model so far

    setScene (mapScene);

    // Create bitmap cursors, platform dependant
    HandOpenCursor=QCursor (QPixmap(iconPath+"cursorhandopen.png"),1,1);	
    PickColorCursor=QCursor ( QPixmap(iconPath+"cursorcolorpicker.png"), 5,27 ); 
    CopyCursor=QCursor ( QPixmap(iconPath+"cursorcopy.png"), 1,1 ); 
    XLinkCursor=QCursor ( QPixmap(iconPath+"cursorxlink.png"), 1,7 ); 

    pickingColor=false;
    drawingLink=false;
    copyingObj=false;
    objectMoved=false;

    editingBO=NULL;
    movingObj=NULL;

    printFrame=true;
    printFooter=true;

    setAcceptDrops (true);  

    // Shortcuts and actions
    QAction *a;
    a = new QAction("Select upper branch", this);
    a->setShortcut (Qt::Key_Up );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorUp() ) );

    a = new QAction( "Select lower branch",this);
    a->setShortcut ( Qt::Key_Down );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorDown() ) );

    a = new QAction( "Select left branch", this);
    a->setShortcut (Qt::Key_Left );
//  a->setShortcutContext (Qt::WindowShortcut);
//  a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorLeft() ) );

    a = new QAction( "Select child branch", this);
    a->setShortcut (Qt::Key_Right);
//  a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorRight() ) );

    a = new QAction(  "Select first branch", this);
    a->setShortcut (Qt::Key_Home );
    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorFirst() ) );

    a = new QAction( "Select last branch",this);
    a->setShortcut ( Qt::Key_End );
    a->setShortcutContext (Qt::WidgetWithChildrenShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( cursorLast() ) );

    // Action to embed LineEdit for heading in Scene
    editingHeading=false;
    lineEdit=NULL;

    // Moving branches:
    a = new QAction("Move branch up", this);
    a->setShortcut (Qt::Key_PageUp );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), mainWindow, SLOT( editMoveUp() ) );

    a = new QAction("Move branch down", this);
    a->setShortcut (Qt::Key_PageDown );
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), mainWindow, SLOT( editMoveDown() ) );

    a = new QAction( tr( "Edit heading","MapEditor" ), this);
    a->setShortcut ( Qt::Key_Return );			//Edit heading
    a->setShortcutContext (Qt::WidgetShortcut);
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading() ) );
    a = new QAction( tr( "Edit heading","MapEditor" ), this);
    a->setShortcut ( Qt::Key_Enter);			//Edit heading
    addAction (a);
    connect( a, SIGNAL( triggered() ), this, SLOT( editHeading() ) );

    // Selections
    selectionColor =QColor (255,255,0);
    

    // Attributes   //FIXME-3 testing only...
    QString k;
    AttributeDef *ad;
    attrTable= new AttributeTable();
    k="A - StringList";
    ad=attrTable->addKey (k,StringList);
    if (ad)
    {
	QStringList sl;
	sl <<"val 1"<<"val 2"<< "val 3";
	ad->setValue (QVariant (sl));
    }
    //attrTable->addValue ("Key A","P 1");
    //attrTable->addValue ("Key A","P 2");
    //attrTable->addValue ("Key A","P 3");
    //attrTable->addValue ("Key A","P 4");
    k="B - FreeString";
    ad=attrTable->addKey (k,FreeString);
    if (ad)
    {
	//attrTable->addValue ("Key B","w1");
	//attrTable->addValue ("Key B","w2");
    }
    k="C - UniqueString";
    ad=attrTable->addKey (k,UniqueString);
    if (ad)
    {
    //attrTable->addKey ("Key Prio");
    //attrTable->addValue ("Key Prio","Prio 1");
    //attrTable->addValue ("Key Prio","Prio 2");
    }
}

MapEditor::~MapEditor()
{
    //qDebug ()<<"Destr MapEditor this="<<this;
}

VymModel* MapEditor::getModel()
{
    return model;
}

QGraphicsScene * MapEditor::getScene()
{
    return mapScene;
}

void MapEditor::scrollTo (const QModelIndex &index) 
{
    if (index.isValid())
    {
	LinkableMapObj* lmo=NULL;
	TreeItem *ti= static_cast<TreeItem*>(index.internalPointer());
	if (ti->getType()==TreeItem::Image ||ti->isBranchLikeType() )
	    lmo=((MapItem*)ti)->getLMO();
	if (lmo) 
	{
	    QRectF r=lmo->getBBox();
	    setScrollBarPosTarget (r);
	}   
    }
}

void MapEditor::setScrollBarPosTarget (const QRectF &rect)
{
    // Code copied from Qt sources
    int xmargin=50;
    int ymargin=50;

    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QRectF viewRect = matrix().mapRect(rect);

    qreal left = horizontalScrollBar()->value();
    qreal right = left + width;
    qreal top = verticalScrollBar()->value();
    qreal bottom = top + height;

    scrollBarPosTarget=getScrollBarPos();

    if (viewRect.left() <= left + xmargin) {
        // need to scroll from the left
  //      if (!d->leftIndent)
            scrollBarPosTarget.setX(int(viewRect.left() - xmargin - 0.5));
    }
    if (viewRect.right() >= right - xmargin) {
        // need to scroll from the right
//        if (!d->leftIndent)
            scrollBarPosTarget.setX(int(viewRect.right() - width + xmargin + 0.5));
    }
    if (viewRect.top() <= top + ymargin) {
        // need to scroll from the top
   //     if (!d->topIndent)
            scrollBarPosTarget.setY(int(viewRect.top() - ymargin - 0.5));
    }
    if (viewRect.bottom() >= bottom - ymargin) {
        // need to scroll from the bottom
//        if (!d->topIndent)
            scrollBarPosTarget.setY(int(viewRect.bottom() - height + ymargin + 0.5));
    }

    if (scrollBarPosTarget==getScrollBarPos()) return;

    if (scrollBarPosAnimation.state()==QAbstractAnimation::Running)
	scrollBarPosAnimation.stop();
    
    if (settings.value ("/animation/use/",true).toBool() )
    {
	scrollBarPosAnimation.setTargetObject (this);
	scrollBarPosAnimation.setPropertyName ("scrollBarPos");
	scrollBarPosAnimation.setDuration(1000);
	scrollBarPosAnimation.setEasingCurve ( QEasingCurve::OutQuint);
	scrollBarPosAnimation.setStartValue(
	    QPointF (horizontalScrollBar()->value() ,
		     verticalScrollBar()->value() ) );
	scrollBarPosAnimation.setEndValue(scrollBarPosTarget);
	scrollBarPosAnimation.start();
    } else
	setScrollBarPos (scrollBarPosTarget);
}

QPointF MapEditor::getScrollBarPosTarget()
{
    return scrollBarPosTarget;
}


void MapEditor::setScrollBarPos(const QPointF &p)
{
    scrollBarPos=p;
    horizontalScrollBar()->setValue(int(p.x()));
    verticalScrollBar()->setValue(int(p.y()));
}

QPointF MapEditor::getScrollBarPos()
{
    return QPointF (horizontalScrollBar()->value(),verticalScrollBar()->value());
    //return scrollBarPos;
}

void MapEditor::setZoomFactorTarget (const qreal &zft)
{
    zoomFactorTarget=zft;
    if (zoomAnimation.state()==QAbstractAnimation::Running)
	zoomAnimation.stop();
    if (settings.value ("/animation/use/",true).toBool() )
    {
	zoomAnimation.setTargetObject (this);
	zoomAnimation.setPropertyName ("zoomFactor");
	zoomAnimation.setDuration(1000);
	zoomAnimation.setEasingCurve ( QEasingCurve::OutQuint);
	zoomAnimation.setStartValue(zoomFactor);
	zoomAnimation.setEndValue(zft);
	zoomAnimation.start();
    } else
	setZoomFactor (zft);
}

qreal MapEditor::getZoomFactorTarget()
{
    return zoomFactorTarget;
}


void MapEditor::setZoomFactor(const qreal &zf)
{
    zoomFactor=zf;
    setMatrix (QMatrix(zf, 0, 0, zf, 0, 0),false );
}

qreal MapEditor::getZoomFactor()
{
    return zoomFactor;
}

void MapEditor::print()
{
/*
    if ( !printer ) //FIXME-3 printer always true meanwhile
    {
	qDebug()<<"ME::print creating printer";
	//printer = new QPrinter;   //FIXME-3 use global printer
	printer->setColorMode (QPrinter::Color);
	printer->setPrinterName (settings.value("/mainwindow/printerName",printer->printerName()).toString());
	printer->setOutputFormat((QPrinter::OutputFormat)settings.value("/mainwindow/printerFormat",printer->outputFormat()).toInt());
	printer->setOutputFileName(settings.value("/mainwindow/printerFileName",printer->outputFileName()).toString());
    }
*/
    QRectF totalBBox=getTotalBBox();

    // Try to set orientation automagically
    // Note: Interpretation of generated postscript is amibiguous, if 
    // there are problems with landscape mode, see
    // http://sdb.suse.de/de/sdb/html/jsmeix_print-cups-landscape-81.html

    if (totalBBox.width()>totalBBox.height())
	// recommend landscape
	printer->setOrientation (QPrinter::Landscape);
    else    
	// recommend portrait
	printer->setOrientation (QPrinter::Portrait);

    QPrintDialog dialog (printer, this);
    dialog.setWindowTitle(tr("Print vym map","MapEditor")); 
    if (dialog.exec() == QDialog::Accepted)
    {
	QPainter pp(printer);

	pp.setRenderHint(QPainter::Antialiasing,true);

	// Don't print the visualisation of selection
	model->unselect();

	QRectF mapRect=totalBBox;
	QGraphicsRectItem *frame=NULL;

	if (printFrame) 
	{
	    // Print frame around map
	    mapRect.setRect (totalBBox.x()-10, totalBBox.y()-10, 
		totalBBox.width()+20, totalBBox.height()+20);
	    frame=mapScene->addRect (mapRect, QPen(Qt::black),QBrush(Qt::NoBrush));
	    frame->setZValue(0);
	    frame->show();    
	}	


	double paperAspect = (double)printer->width()   / (double)printer->height();
	double   mapAspect = (double)mapRect.width() / (double)mapRect.height();
	int viewBottom;
	if (mapAspect>=paperAspect)
	{
	    // Fit horizontally to paper width
	    //pp.setViewport(0,0, printer->width(),(int)(printer->width()/mapAspect) );	
	    viewBottom=(int)(printer->width()/mapAspect);   
	}   else
	{
	    // Fit vertically to paper height
	    //pp.setViewport(0,0,(int)(printer->height()*mapAspect),printer->height());	
	    viewBottom=printer->height();   
	}   
	
	if (printFooter) 
	{
	    // Print footer below map
	    QFont font;	    
	    font.setPointSize(10);
	    pp.setFont (font);
	    QRectF footerBox(0,viewBottom,printer->width(),15);
	    pp.drawText ( footerBox,Qt::AlignLeft,"VYM - " +model->getFileName());
	    pp.drawText ( footerBox, Qt::AlignRight, QDate::currentDate().toString(Qt::TextDate));
	}
	mapScene->render (
	    &pp, 
	    QRectF (0,0,printer->width(),printer->height()-15),
	    QRectF(mapRect.x(),mapRect.y(),mapRect.width(),mapRect.height())
	);
	
	// Viewport has paper dimension
	if (frame)  delete (frame);

	// Restore selection
	model->reselect();

	// Save settings in vymrc   //FIXME-3 do this in mainwindow.cpp
	settings.setValue("/mainwindow/printerName",printer->printerName());
	settings.setValue("/mainwindow/printerFormat",printer->outputFormat());
	settings.setValue("/mainwindow/printerFileName",printer->outputFileName());
    }
}

QRectF MapEditor::getTotalBBox()    
{
    QPen pen;
    pen.setWidth (1);
    pen.setCapStyle ( Qt::RoundCap );

    QRectF rt;
    BranchObj *bo;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    model->nextBranch(cur,prev);
    while (cur) 
    {
	if (!cur->hasHiddenExportParent())
	{
	    // Branches
	    bo=(BranchObj*)(cur->getLMO());
	    if (bo && bo->isVisibleObj())
	    {
		bo->calcBBoxSizeWithChildren();
		QRectF r1=bo->getBBox();

		if (rt.isNull()) rt=r1;
		rt=addBBox (r1, rt);
	    }

	    // Images
	    FloatImageObj *fio;
	    for (int i=0; i<cur->imageCount(); i++)
	    {
		fio=cur->getImageObjNum (i);
		if (fio) rt=addBBox (fio->getBBox(),rt);
	    }
	}
	model->nextBranch(cur,prev);
    }
    return rt;	
}


QImage MapEditor::getImage( QPointF &offset)
{
    //mapScene->update();   //FIXME-3 needed?

    QRectF mapRect=getTotalBBox();
    
    offset=QPointF (mapRect.x(), mapRect.y() );
    QImage pix (mapRect.width()+2, mapRect.height()+2,QImage::Format_RGB32);

    QPainter pp (&pix);

    pp.setRenderHints(renderHints());
    mapScene->render (	&pp, 
	QRectF(0,0,mapRect.width()+2,mapRect.height()+2),
	QRectF(mapRect.x(),mapRect.y(),mapRect.width()+2,mapRect.height()+2));
	//mapRect );
    return pix;
}


void MapEditor::setAntiAlias (bool b)
{
    setRenderHint(QPainter::Antialiasing,b);
}

void MapEditor::setSmoothPixmap(bool b)
{
    setRenderHint(QPainter::SmoothPixmapTransform,b);
}

#include <iostream>
void MapEditor::autoLayout()
{
    // Create list with all bounding polygons
    QList <LinkableMapObj*> mapobjects;
    QList <ConvexPolygon> polys; 
    ConvexPolygon p;
    QList <Vector> vectors;
    QList <Vector> orgpos;
    QStringList headings;   //FIXME-3 testing only
    Vector v;
    BranchItem *bi;
    BranchItem *bi2;
    BranchObj *bo;

    // Outer loop: Iterate until we no more changes in orientation 
    bool orientationChanged=true;
    while (orientationChanged)
    {
	BranchItem *ri=model->getRootItem();
	for (int i=0;i<ri->branchCount();++i)
	{
	    bi=ri->getBranchNum (i);
	    bo=(BranchObj*)bi->getLMO();
	    if (bo)
	    {
		mapobjects.append (bo);
		p=bo->getBoundingPolygon();
		p.calcCentroid();
		polys.append(p);
		vectors.append (QPointF(0,0));
		orgpos.append (p.at(0));
		headings.append (bi->getHeading());
	    }
	    for (int j=0;j<bi->branchCount();++j)
	    {
		bi2=bi->getBranchNum (j);
		bo=(BranchObj*)bi2->getLMO();
		if (bo)
		{
		    mapobjects.append (bo);
		    p=bo->getBoundingPolygon();
		    p.calcCentroid();
		    polys.append(p);
		    vectors.append (QPointF(0,0));
		    orgpos.append (p.at(0));
		    headings.append (bi2->getHeading());
		}   
	    }
	}

	// Iterate moving bounding polygons until we have no more collisions
	int collisions=1;
	while (collisions>0)
	{
	    collisions=0;
	    for (int i=0; i<polys.size()-1; ++i)
	    {
		for (int j=i+1; j<polys.size();++j)
		{
		    if (polygonCollision (polys.at(i),polys.at(j), QPointF(0,0)).intersect )
		    {
			collisions++;
			if (debug) qDebug() << "Collision: "<<headings[i]<<" - "<<headings[j];
			v=polys.at(j).centroid()-polys.at(i).centroid();
			// Move also away if centroids are identical
			if (v.isNull()) 
			{
			    v.setX (rand()%200 -100);
			    v.setY (rand()%200 -100);
			}
			// Add random direction
			Vector w (cos (rand()%1000),sin(rand()%1000));
			
			// Scale translation vector by area of polygons
			v.normalize();
			vectors[j]=v*10000/polys.at(j).weight();	
			vectors[i]=v*10000/polys.at(i).weight();	
			vectors[i].invert();
			//FIXME-3 outer loop, "i" get's changed several times...
			// Better not move away from centroid of 2 colliding polys, 
			// but from centroid of _all_
		    }  
		}
	    }
	    for (int i=0;i<vectors.size();i++)
	    {
		//qDebug() << " v="<<vectors[i]<<" "<<headings[i];
		if (!vectors[i].isNull() )
		polys[i].translate (vectors[i]);
	    }
	    if (debug) qDebug()<< "Collisions total: "<<collisions;
	    //collisions=0;
	}   

	// Finally move the real objects and update 
	QList <LinkableMapObj::Orientation> orients;
	for (int i=0;i<polys.size();i++)
	{
	    Vector v=polys[i].at(0)-orgpos[i];
	    orients.append (mapobjects[i]->getOrientation());
	    if (!v.isNull())
	    {
		if (debug) cout<<" Moving "<<polys.at(i).weight()<<" "<<mapobjects[i]->getAbsPos()<<" -> "<<mapobjects[i]->getAbsPos() + v<<"  "<<headings[i].toStdString()<<endl;
		//mapobjects[i]->moveBy(v.x(),v.y() );
		//mapobjects[i]->setRelPos();
		model->startAnimation ((BranchObj*)mapobjects[i], v);
		if (debug) cout<<i<< " Weight: "<<polys.at(i).weight()<<" "<<v<<" "<<headings.at(i).toStdString()<<endl;
	    }
	}   
	/*
	model->reposition();	
	orientationChanged=false;
	for (int i=0;i<polys.size();i++)
	    if (orients[i]!=mapobjects[i]->getOrientation())
	    {
		orientationChanged=true;
		break;
	    }
	*/
    
	break;

	//orientationChanged=false;
    } // loop if orientation has changed

    model->emitSelectionChanged();
}

TreeItem* MapEditor::findMapItem (QPointF p,TreeItem *exclude)
{
    // Start with mapcenter, no images allowed at rootItem
    int i=0;
    BranchItem *bi=model->getRootItem()->getFirstBranch();
    TreeItem *found=NULL;
    while (bi)
    {
	found=bi->findMapItem (p, exclude);
	if (found) return found;
	i++;
	bi=model->getRootItem()->getBranchNum(i);
    }
    return NULL;
}

AttributeTable* MapEditor::attributeTable()
{
    return attrTable;
}

void MapEditor::testFunction1()
{
/*
    qDebug()<< "ME::test1  selected TI="<<model->getSelectedItem();
    model->setExportMode (true);
*/

    /*
    // Code copied from Qt sources
    QRectF rect=model->getSelectedBranchObj()->getBBox();
    int xmargin=50;
    int ymargin=50;

    qreal width = viewport()->width();
    qreal height = viewport()->height();
    QRectF viewRect = matrix().mapRect(rect);

    qreal left = horizontalScrollBar()->value();
    qreal right = left + width;
    qreal top = verticalScrollBar()->value();
    qreal bottom = top + height;

    if (viewRect.left() <= left + xmargin) {
        // need to scroll from the left
  //      if (!d->leftIndent)
            horizontalScrollBar()->setValue(int(viewRect.left() - xmargin - 0.5));
    }
    if (viewRect.right() >= right - xmargin) {
        // need to scroll from the right
//        if (!d->leftIndent)
            horizontalScrollBar()->setValue(int(viewRect.right() - width + xmargin + 0.5));
    }
    if (viewRect.top() <= top + ymargin) {
        // need to scroll from the top
   //     if (!d->topIndent)
            verticalScrollBar()->setValue(int(viewRect.top() - ymargin - 0.5));
    }
    if (viewRect.bottom() >= bottom - ymargin) {
        // need to scroll from the bottom
//        if (!d->topIndent)
            verticalScrollBar()->setValue(int(viewRect.bottom() - height + ymargin + 0.5));
    }
    qDebug() << "test1:  hor="<<horizontalScrollBar()->value();
    qDebug() << "test1:  ver="<<verticalScrollBar()->value();
}

*/
/*
     QtPropertyAnimation *animation=new QtPropertyAnimation(this, "sceneRect");
     animation->setDuration(5000);
     //animation->setEasingCurve ( QtEasingCurve::OutElastic);
     animation->setEasingCurve ( QtEasingCurve::OutQuint);
     animation->setStartValue(sceneRect() );
     animation->setEndValue(QRectF(50, 50, 1000, 1000));

     animation->start();
*/   
/*
    QDialog *dia= new QDialog (this);
    dia->setGeometry (50,50,10,10);

     dia->show();
     dia ->raise();

     QtPropertyAnimation *animation=new QtPropertyAnimation(dia, "geometry");
     animation->setDuration(1000);
     //animation->setEasingCurve ( QtEasingCurve::OutElastic);
     animation->setEasingCurve ( QtEasingCurve::OutQuint);
     animation->setStartValue(QRect(50, 50, 10, 10));
     animation->setEndValue(QRect(250, 250, 100, 100));

     animation->start();
 */

}
    
void MapEditor::testFunction2()
{
    autoLayout();
}

BranchItem* MapEditor::getBranchDirectAbove (BranchItem *bi)
{
    if (bi)
    {
	int i=bi->num();
	if (i>0) return bi->parent()->getBranchNum(i-1);
    }
    return NULL;
}

BranchItem* MapEditor::getBranchAbove (BranchItem *selbi)
{
    if (selbi)
    {
	int dz=selbi->depth();	// original depth
	bool invert=false;
	if (selbi->getLMO()->getOrientation()==LinkableMapObj::LeftOfCenter)
	    invert=true;

	BranchItem *bi;

	// Look for branch with same parent but directly above
	if (dz==1 && invert)
	    bi=getBranchDirectBelow(selbi);
	else
	    bi=getBranchDirectAbove (selbi);

	if (bi) 
	    // direct predecessor
	    return bi;

	// Go towards center and look for predecessor
	while (selbi->depth()>0)
	{
	    selbi=(BranchItem*)(selbi->parent());
	    if (selbi->depth()==1 && invert)
		bi=getBranchDirectBelow (selbi);
	    else
		bi=getBranchDirectAbove (selbi);
	    if (bi)
	    {
		// turn 
		selbi=bi;
		while (selbi->depth()<dz)
		{
		    // try to get back to original depth dz
		    bi=selbi->getLastBranch();
		    if (!bi) 
		    {
			return selbi;
		    }
		    selbi=bi;
		}
		return selbi;
	    }
	}
    }
    return NULL;
}

BranchItem* MapEditor::getBranchDirectBelow(BranchItem *bi)
{
    if (bi)
    {
	int i=bi->num();
	if (i+1<bi->parent()->branchCount()) return bi->parent()->getBranchNum(i+1);
    }
    return NULL;
}

BranchItem* MapEditor::getBranchBelow (BranchItem *selbi)
{
    if (selbi)
    {
	BranchItem *bi;
	int dz=selbi->depth();	// original depth
	bool invert=false;
	if (selbi->getLMO()->getOrientation()==LinkableMapObj::LeftOfCenter)
	    invert=true;


	// Look for branch with same parent but directly below
	if (dz==1 && invert)
	    bi=getBranchDirectAbove (selbi);
	else
	    bi=getBranchDirectBelow (selbi);
	if (bi) 
	    // direct successor
	    return bi;


	// Go towards center and look for neighbour
	while (selbi->depth()>0)
	{
	    selbi=(BranchItem*)(selbi->parent());
	    if (selbi->depth()==1 && invert)
		bi=getBranchDirectAbove (selbi);
	    else
		bi=getBranchDirectBelow (selbi);
	    if (bi)
	    {
		// turn 
		selbi=bi;
		while (selbi->depth()<dz)
		{
		    // try to get back to original depth dz
		    bi=selbi->getFirstBranch();
		    if (!bi) 
		    {
			return selbi;
		    }
		    selbi=bi;
		}
		return selbi;
	    }
	}
    }
    return NULL;
}

BranchItem* MapEditor::getLeftBranch (BranchItem *bi)
{
    if (bi)
    {
	if (bi->depth()==0)
	    // Special case: use alternative selection index
	    return bi->getLastSelectedBranchAlt();  
	if (bi->getBranchObj()->getOrientation()==LinkableMapObj::RightOfCenter)    
	    // right of center
	    return (BranchItem*)(bi->parent());
	else
	    // left of center
	    if (bi->getType()== TreeItem::Branch )
		return bi->getLastSelectedBranch();
    }
    return NULL;
}

BranchItem* MapEditor::getRightBranch(BranchItem *bi)
{
    if (bi)
    {
	if (bi->depth()==0) return bi->getLastSelectedBranch();	
	if (bi->getBranchObj()->getOrientation()==LinkableMapObj::LeftOfCenter)	
	    // left of center
	    return (BranchItem*)(bi->parent());
	else
	    // right of center
	    if (bi->getType()== TreeItem::Branch )
		return (BranchItem*)bi->getLastSelectedBranch();
    }
    return NULL;
}



void MapEditor::cursorUp()
{
    BranchItem *bi=model->getSelectedBranch();
    if (bi) model->select (getBranchAbove(bi));
}

void MapEditor::cursorDown()	

{
    BranchItem *bi=model->getSelectedBranch();
    if (bi) model->select (getBranchBelow(bi));
}

void MapEditor::cursorLeft()
{
    BranchItem *bi=getLeftBranch (model->getSelectedBranch());
    if (bi) model->select (bi);
}

void MapEditor::cursorRight()	
{
    BranchItem *bi=getRightBranch (model->getSelectedBranch());
    if (bi) model->select (bi);
}

void MapEditor::cursorFirst()	
{
    model->selectFirstBranch();
}

void MapEditor::cursorLast()	
{
    model->selectLastBranch();
}


void MapEditor::editHeading()
{
    if (editingHeading)
    {
	editHeadingFinished();
	return;
    }
    BranchObj *bo=model->getSelectedBranchObj();
    BranchItem *bi=model->getSelectedBranch();
    if (bo) 
    {
	model->setSelectionBlocked(true);

	lineEdit=new QLineEdit;
	QGraphicsProxyWidget *pw=mapScene->addWidget (lineEdit);
	pw->setZValue (Z_LINEEDIT);
	lineEdit->setCursor(Qt::IBeamCursor);
	lineEdit->setCursorPosition(1);

	QPoint p = mapTo (this,bo->getAbsPos().toPoint() );
//	qDebug()<<"ME::editHeading a) p="<<p;

	scene()->update();
	p = mapTo (this,bo->getAbsPos().toPoint() );
//	qDebug()<<"ME::editHeading b) p="<<p;

	lineEdit->setGeometry(p.x(),p.y(),230,25);
	lineEdit->setText (bi->getHeading());
	lineEdit->setFocus();
	lineEdit->selectAll();	// Hack to enable cursor in lineEdit
	lineEdit->deselect();	// probably a Qt bug...
	lineEdit->grabKeyboard();   //FIXME-3 reactived for tests...
	editingHeading=true;
    }

}
void MapEditor::editHeadingFinished()
{
    editingHeading=false;
    //lineEdit->releaseKeyboard();
    lineEdit->clearFocus();
    QString s=lineEdit->text();
    s.replace (QRegExp ("\\n")," ");	// Don't paste newline chars
    model->setHeading (s);
    model->setSelectionBlocked(false);
    delete (lineEdit);

    // Maybe reselect previous branch 
    mainWindow->editHeadingFinished (model);

/*
    //FIXME testing
    setFocus();
    qDebug()<<"ME::editHF hasFocus="<<hasFocus();
*/  
}


void MapEditor::contextMenuEvent ( QContextMenuEvent * e )
{
    // Lineedits are already closed by preceding
    // mouseEvent, we don't need to close here.

    QPointF p = mapToScene(e->pos());
    TreeItem *ti=findMapItem (p, NULL);
    LinkableMapObj* lmo=NULL;
    if (ti) lmo=((MapItem*)ti)->getLMO();
    
    if (lmo) 
    {	// MapObj was found
	if (model->getSelectedLMO() != lmo)
	{
	    // select the MapObj
	    model->select(lmo);
	}
	// Context Menu 
	if (model->getSelectedBranchObj() ) 
	{
	    // Context Menu on branch or mapcenter
	    branchContextMenu->popup(e->globalPos() );
	} else
	{
	    if (model->getSelectedImage() )
	    {
		// Context Menu on floatimage
		floatimageContextMenu->popup(e->globalPos() );
	    }	
	}   
    } else 
    { // No MapObj found, we are on the Canvas itself
	// Context Menu on scene
	
	// Open context menu synchronously to position new mapcenter
	model->setContextPos (p);
	canvasContextMenu->exec(e->globalPos() );
	model->unsetContextPos ();
    } 
    e->accept();
}

void MapEditor::keyPressEvent(QKeyEvent* e)
{
    if (e->key()==Qt::Key_PageUp || e->key()==Qt::Key_PageDown)
	// Ignore PageUP/Down to avoid scrolling with keys
	return;

    if (e->modifiers() & Qt::ControlModifier)
    {
	switch (mainWindow->getModMode())
	{
	    case Main::ModModeColor: 
		setCursor (PickColorCursor);
		break;
	    case Main::ModModeCopy: 
		setCursor (CopyCursor);
		break;
	    case Main::ModModeXLink: 
		setCursor (XLinkCursor);
		break;
	    default :
		setCursor (Qt::ArrowCursor);
		break;
	} 
    }	
    QGraphicsView::keyPressEvent(e);
}

void MapEditor::keyReleaseEvent(QKeyEvent* e)
{
    if (!(e->modifiers() & Qt::ControlModifier))
	setCursor (Qt::ArrowCursor);
}

void MapEditor::mousePressEvent(QMouseEvent* e)
{
    // Ignore right clicks or wile editing heading
    if (e->button() == Qt::RightButton or model->isSelectionBlocked() )
    {
	//qDebug() << "  ME::ignoring right mouse event...\n";
	e->ignore();
	QGraphicsView::mousePressEvent(e);
	return;
    }

    QPointF p = mapToScene(e->pos());
    TreeItem *ti=findMapItem (p, NULL);
    LinkableMapObj* lmo=NULL;
    if (ti) lmo=((MapItem*)ti)->getLMO();
    
    
    e->accept();

    //Take care of  system flags _or_ modifier modes
    //
    if (lmo && ti->isBranchLikeType() )
    {
	QString foname=((BranchObj*)lmo)->getSystemFlagName(p);
	if (!foname.isEmpty())
	{
	    // systemFlag clicked
	    model->select (lmo);    
	    if (foname.contains("system-url")) 
	    {
		if (e->modifiers() & Qt::ControlModifier)
		    mainWindow->editOpenURLTab();
		else	
		    mainWindow->editOpenURL();
	    }	
	    else if (foname=="system-vymLink")
	    {
		mainWindow->editOpenVymLink();
		// tabWidget may change, better return now
		// before segfaulting...
	    } else if (foname=="system-note")
		mainWindow->windowToggleNoteEditor();
	    else if (foname=="hideInExport")	    
		model->toggleHideExport();
	    return; 
	} else
	{
	    // Take care of xLink
	    if (ti->xlinkCount()>0 && lmo->getBBox().width()>30)
	    {
		if ((lmo->getOrientation()!=LinkableMapObj::RightOfCenter && p.x() < lmo->getBBox().left()+20)  ||
		    (lmo->getOrientation()!=LinkableMapObj::LeftOfCenter && p.x() > lmo->getBBox().right()-20) ) 
		{
		    QMenu menu;
		    QList <QAction*> alist;
		    QList <BranchItem*> blist;
		    for (int i=0;i<ti->xlinkCount();i++)
		    {
			//qDebug()<<"ME::  ti="<<ti;
			XLinkItem *xli=ti->getXLinkItemNum(i);
			//qDebug()<<"     xli="<<xli;
			BranchItem *bit=xli->getPartnerBranch();
			//qDebug()<<"      bi="<<bit;
			if (bit) 
			{
			    alist.append (new QAction(ti->getXLinkItemNum(i)->getPartnerBranch()->getHeading(),&menu));
			    blist.append (bit);
			}
		    }	
		    menu.addActions (alist);	
		    QAction *ra=menu.exec (e->globalPos() );
		    if (ra)
			model->select (blist.at( alist.indexOf(ra)));
		    while (!alist.isEmpty())
		    {
			QAction *a=alist.takeFirst();
			delete a;
		    }	
		    return;
		}   
	    }
	}
    }	

    // No system flag clicked, take care of modmodes (CTRL-Click)
    if (e->modifiers() & Qt::ControlModifier)
    {
	if (mainWindow->getModMode()==Main::ModModeColor)
	{
		pickingColor=true;
		setCursor (PickColorCursor);
		return;
	} 
	if (mainWindow->getModMode()==Main::ModModeXLink)
	{   
	    BranchItem *bi_begin=model->getSelectedBranch();
	    if (bi_begin)   
	    {
		drawingLink=true;
		tmpLink=new Link (model);
		tmpLink->setBeginBranch (bi_begin);
		tmpLink->setColor(model->getMapDefXLinkColor());
		tmpLink->setWidth(model->getMapDefXLinkWidth());
		tmpLink->setEndPoint   (p);
		tmpLink->createMapObj(mapScene);
		tmpLink->updateLink();
		return;
	    } 
	}
    }	// End of modmodes

    if (lmo) 
    {	
    /*
	qDebug() << "ME::mouse pressed\n";
	qDebug() << "  lmo="<<lmo;
	qDebug() << "   ti="<<ti->getHeadingStd();
    */
	// Select the clicked object

	// Get clicked LMO
	model->select (ti);

	// Left Button	    Move Branches
	if (e->button() == Qt::LeftButton )
	{
	    movingObj_start.setX( p.x() - lmo->x() );	
	    movingObj_start.setY( p.y() - lmo->y() );	
	    movingObj_orgPos.setX (lmo->x() );
	    movingObj_orgPos.setY (lmo->y() );
	    if (ti->depth()>0)
	    {
		lmo->setRelPos();   
		movingObj_orgRelPos=lmo->getRelPos();
	    }

	    // If modMode==copy, then we want to "move" the _new_ object around
	    // then we need the offset from p to the _old_ selection, because of tmp
	    if (mainWindow->getModMode()==Main::ModModeCopy &&
		e->modifiers() & Qt::ControlModifier)
	    {
		BranchItem *bi=model->getSelectedBranch();
		if (bi)
		{
		    copyingObj=true;
		    //model->select(model->createBranch (bi));
		    model->copy();
		    model->paste();
		    model->select (bi->getLastBranch());
		    model->reposition();
		}
	    } 

	    movingObj=model->getSelectedLMO();	
	} else
	    // Middle Button    Toggle Scroll
	    // (On Mac OS X this won't work, but we still have 
	    // a button in the toolbar)
	    if (e->button() == Qt::MidButton )
		model->toggleScroll();
    } else 
    { // No MapObj found, we are on the scene itself
	// Left Button	    move Pos of sceneView
	if (e->button() == Qt::LeftButton )
	{
	    movingObj=NULL; // move Content not Obj
	    movingObj_start=e->globalPos();
	    movingCont_start=QPointF (
		horizontalScrollBar()->value(),
		verticalScrollBar()->value());
	    movingVec=QPointF(0,0);
	    setCursor(HandOpenCursor);
	} 
    } 
    QGraphicsView::mousePressEvent(e);
}

void MapEditor::mouseMoveEvent(QMouseEvent* e)
{

    QPointF p = mapToScene(e->pos());

    if (debug) mainWindow->statusMessage (QString ("Mouse: scene: %1 widget: %2").arg(qpointFToString(p)).arg(qpointFToString(e->pos())));

    TreeItem *seli=model->getSelectedItem();
    LinkableMapObj* lmosel=NULL;    
    if (seli && (seli->isBranchLikeType() ||seli->getType()==TreeItem::Image))
	lmosel=((MapItem*)seli)->getLMO();

    // Move the selected MapObj
    if ( lmosel && movingObj) 
    {	
	objectMoved=true;
	// reset cursor if we are moving and don't copy
	if (mainWindow->getModMode()!=Main::ModModeCopy)
	    setCursor (Qt::ArrowCursor);

	// Now move the selection, but add relative position 
	// (movingObj_start) where selection was chosen with 
	// mousepointer. (This avoids flickering resp. jumping 
	// of selection back to absPos)
	
	// Check if we could link 
	TreeItem *ti=findMapItem (p, seli);
	BranchItem *dsti=NULL;
	LinkableMapObj* dst=NULL;
	if (ti && ti!=seli && ti->isBranchLikeType())
	{
	    dsti=(BranchItem*)ti;
	    dst=dsti->getLMO(); 
	} else
	    dsti=NULL;
	

	if (lmosel && seli->getType()==TreeItem::Image)	
	{
	    FloatObj *fio=(FloatImageObj*)lmosel;
	    fio->move   (p.x() -movingObj_start.x(), p.y()-movingObj_start.y() );	
	    fio->setRelPos();
	    fio->updateLinkGeometry(); //no need for reposition, if we update link here
	    model->emitSelectionChanged();  // position has changed

	    // Relink float to new mapcenter or branch, if shift is pressed 
	    // Only relink, if selection really has a new parent
	    if ( e->modifiers()==Qt::ShiftModifier && dsti &&  dsti != seli->parent()  )
	    {
		// Also save the move which was done so far
		QString pold=qpointFToString(movingObj_orgRelPos);
		QString pnow=qpointFToString(fio->getRelPos());
		model->saveState(
		    seli,
		    "moveRel "+pold,
		    seli,
		    "moveRel "+pnow,
		    QString("Move %1 to relative position %2").arg(model->getObjectName(fio)).arg(pnow));
		fio->getParObj()->requestReposition();
		model->reposition();

		model->relinkImage ((ImageItem*) seli,dsti);
		model->select (seli);

		model->reposition();
	    }
	} else	
	{   // selection != a FloatObj
	    if (seli->depth()==0)	
	    {
		// Move mapcenter
		lmosel->move   (p-movingObj_start);	
		if (e->buttons()== Qt::LeftButton && e->modifiers()==Qt::ShiftModifier) 
		{
		    // Move only mapcenter, leave its children where they are
		    QPointF v;
		    v=lmosel->getAbsPos();
		    for (int i=0; i<seli->branchCount(); ++i)
		    {
			seli->getBranchObjNum(i)->setRelPos();
			seli->getBranchObjNum(i)->setOrientation();
		    }
		}   
		lmosel->move   (p-movingObj_start);	
	    } else
	    {	
		if (seli->depth()==1)
		{
		    // Move mainbranch
		    if (!lmosel->hasParObjTmp())
			lmosel->move(p-movingObj_start);	
		    lmosel->setRelPos();
		} else
		{
		    // Move ordinary branch
		    if (lmosel->getOrientation() == LinkableMapObj::LeftOfCenter)
			// Add width of bbox here, otherwise alignRelTo will cause jumping around
			lmosel->move(p.x() -movingObj_start.x() , //lmosel->getBBox().width(), 
			    p.y()-movingObj_start.y() +lmosel->getTopPad() );	    
		    else    
			lmosel->move(p.x() -movingObj_start.x(), p.y()-movingObj_start.y() -lmosel->getTopPad());
		    lmosel->setRelPos();    
		} 

	    } // depth>0

	    // Maybe we can relink temporary?
	    if (dsti)
	    {
		if (e->modifiers()==Qt::ControlModifier)
		{
		    // Special case: CTRL to link below dst
		    lmosel->setParObjTmp (dst,p,+1);
		} else if (e->modifiers()==Qt::ShiftModifier)
		    lmosel->setParObjTmp (dst,p,-1);
		else
		    lmosel->setParObjTmp (dst,p,0);
	    } else  
	    {
		lmosel->unsetParObjTmp();
	    }	    
	    // reposition subbranch
	    lmosel->reposition();

	    QItemSelection sel=model->getSelectionModel()->selection();
	    updateSelection(sel,sel);	// position has changed

	    //scrollTo (model->index (seli->parent()));	

	} // no FloatImageObj


	if (!lmosel->hasParObjTmp())
	    // Scroll if needed
	    // To avoid jumping of the sceneView, only 
	    // show selection, if not tmp linked
	    {
		model->emitShowSelection();	//FIXME-3 better use scrollArea at borders of MapEditor. Also necessary for drag&drop to TreeEditor
	    }
	
	scene()->update();

	return;
    } // selection && moving_obj
	
    // Draw a link from one branch to another
    if (drawingLink)
    {
	 tmpLink->setEndPoint (p);
	 tmpLink->updateLink();
    }	 
    
    // Move sceneView 
    if (!movingObj && !pickingColor &&!drawingLink && e->buttons() == Qt::LeftButton ) 
    {
	QPointF p=e->globalPos();
	movingVec.setX(-p.x() + movingObj_start.x() );
	movingVec.setY(-p.y() + movingObj_start.y() );
	horizontalScrollBar()->setSliderPosition((int)( movingCont_start.x()+movingVec.x() ));
	verticalScrollBar()->setSliderPosition((int)( movingCont_start.y()+movingVec.y() ) );
    }
    QGraphicsView::mouseMoveEvent (e);	
}


void MapEditor::mouseReleaseEvent(QMouseEvent* e)
{
    QPointF p = mapToScene(e->pos());
    TreeItem *seli=model->getSelectedItem();

    TreeItem *dsti=NULL;
    if (seli) dsti=findMapItem(p, seli);
    LinkableMapObj* dst=NULL;
    if (dsti && dsti->isBranchLikeType ()) 
	dst=((MapItem*)dsti)->getLMO();	
    else
	dsti=NULL;


    // Have we been picking color?
    if (pickingColor)
    {
	pickingColor=false;
	setCursor (Qt::ArrowCursor);
	// Check if we are over another branch
	if (dst) 
	{   
	    if (e->modifiers() & Qt::ShiftModifier)
		model->colorBranch (((BranchObj*)dst)->getColor());
	    else    
		model->colorSubtree (((BranchObj*)dst)->getColor());
	} 
	return;
    }

    // Have we been drawing a link?
    if (drawingLink)	
    {
	drawingLink=false;
	// Check if we are over another branch
	if (dsti)
	{   
	    tmpLink->setEndBranch ( ((BranchItem*)dsti) );
	    tmpLink->updateLink();
	    model->createLink (tmpLink);
	    model->saveState(	
		tmpLink->getBeginLinkItem(),"delete ()",
		seli,QString("addXLink (\"%1\",\"%2\",%3,\"%4\")")
		    .arg(model->getSelectString(tmpLink->getBeginBranch()))
		    .arg(model->getSelectString(tmpLink->getEndBranch()))
		    .arg(tmpLink->getWidth())
		    .arg(tmpLink->getColor().name()),
		QString("Adding Link from %1 to %2").arg(model->getObjectName(seli)).arg(model->getObjectName (dsti))
	    );	
	} else
	{
	    delete (tmpLink);
	    tmpLink=NULL;
	}
	return;
    }
    
    // Have we been moving something?
    if ( seli && movingObj ) 
    {	
	if (seli->getType()==TreeItem::Image)
	{
	    FloatImageObj *fio=(FloatImageObj*)( ((MapItem*)seli)->getLMO());
	    if(fio)
	    {
		// Moved FloatObj. Maybe we need to reposition
		QString pold=qpointFToString(movingObj_orgRelPos);
		QString pnow=qpointFToString(fio->getRelPos());
		model->saveState(
		    seli,
		    "moveRel "+pold,
		    seli,
		    "moveRel "+pnow,
		    QString("Move %1 to relative position %2").arg(model->getObjectName(seli)).arg(pnow));

		fio->getParObj()->requestReposition();
		model->reposition();
	    }	
	}

	BranchItem *bi=model->getSelectedBranch();
	if (bi && bi->depth()==0)
	{   
            if (movingObj_orgPos != bi->getBranchObj()->getAbsPos())	
            {
                QString pold=qpointFToString(movingObj_orgPos);
                QString pnow=qpointFToString(bi->getBranchObj()->getAbsPos());	

                model->saveState(
                    bi,
                    "move "+pold,
                    bi,
                    "move "+pnow,
                    QString("Move mapcenter %1 to position %2").arg(model->getObjectName(bi)).arg(pnow));
            }
	}
    
	if (seli->isBranchLikeType() ) //(seli->getType() == TreeItem::Branch )
	{   // A branch was moved
	    LinkableMapObj* lmosel=NULL;	
	    lmosel=((MapItem*)seli)->getLMO();
		
	    // save the position in case we link to mapcenter
	    QPointF savePos=QPointF (lmosel->getAbsPos()  );

	    // Reset the temporary drawn link to the original one
	    lmosel->unsetParObjTmp();

	    // For Redo we may need to save original selection
	    QString preSelStr=model->getSelectString(seli);

	    copyingObj=false;	
	    if (dsti && objectMoved)
	    {
		// We have a destination, relink to that

		BranchObj* bsel=model->getSelectedBranchObj();

		QString preParStr=model->getSelectString (bsel->getParObj());
		QString preNum=QString::number (seli->num(),10);
		QString preDstParStr;
		bool relinked;

		if (e->modifiers() & Qt::ShiftModifier && dst->getParObj())
		{   // Link above dst
		    preDstParStr=model->getSelectString (dst->getParObj());
		    relinked=model->relinkBranch (
			(BranchItem*)seli,
			(BranchItem*)dsti->parent(),
			((BranchItem*)dsti)->num(),
			true);
		} else 
		if (e->modifiers() & Qt::ControlModifier && dst->getParObj())
		{
		    // Link below dst
		    preDstParStr=model->getSelectString (dst->getParObj());
		    relinked=model->relinkBranch (
			(BranchItem*)seli,
			(BranchItem*)dsti->parent(),
			((BranchItem*)dsti)->num()+1,
			true);
		} else	
		{   // Append to dst
		    preDstParStr=model->getSelectString(dst);
		    relinked=model->relinkBranch (
			(BranchItem*)seli,
			(BranchItem*)dsti,
			-1,
			true);
		    if (dsti->depth()==0) bsel->move (savePos);
		} 
		if (relinked)
		{
		    QString postSelStr=model->getSelectString(lmosel);
		    QString postNum=QString::number (seli->num(),10);

		    QString undoCom="relinkTo (\""+ 
			preParStr+ "\"," + preNum  +"," + 
			QString ("%1,%2").arg(movingObj_orgPos.x()).arg(movingObj_orgPos.y())+ ")";

		    QString redoCom="relinkTo (\""+ 
			preDstParStr + "\"," + postNum + "," +
			QString ("%1,%2").arg(savePos.x()).arg(savePos.y())+ ")";

		    model->saveState (
			postSelStr,undoCom,
			preSelStr, redoCom,
			QString("Relink %1 to %2").arg(model->getObjectName(bsel)).arg(model->getObjectName(dst)) );

		}
	    } else
	    {
		// No destination, undo  temporary move	

		if (seli->depth()==1)
		{
		    // The select string might be different _after_ moving around.
		    // Therefor reposition and then use string of old selection, too
		    model->reposition();

                    QPointF rp(lmosel->getRelPos());
                    if (rp != movingObj_orgRelPos)
                    {
                        QString ps=qpointFToString(rp);
                        model->saveState(
                            model->getSelectString(lmosel), "moveRel "+qpointFToString(movingObj_orgRelPos), 
                            preSelStr, "moveRel "+ps, 
                            QString("Move %1 to relative position %2").arg(model->getObjectName(lmosel)).arg(ps));
                    }
		}

		// Draw the original link, before selection was moved around
		if (settings.value("/animation/use",true).toBool() 
		    && seli->depth()>1
//		    && distance (lmosel->getRelPos(),movingObj_orgRelPos)<3
		) 
		{
		    lmosel->setRelPos();    // calc relPos first for starting point
		    
		    model->startAnimation(
			(BranchObj*)lmosel,
			lmosel->getRelPos(),
			movingObj_orgRelPos
		    );	
		} else	
		    model->reposition();
	    }
	}
	model->emitSelectionChanged();  //FIXME-3 needed? at least not after pos of selection has changed...
	// Finally resize scene, if needed
	scene()->update();
	movingObj=NULL;	    
	objectMoved=false;

    } else 
	// maybe we moved View: set old cursor
	setCursor (Qt::ArrowCursor);

    QGraphicsView::mouseReleaseEvent(e);
}

void MapEditor::mouseDoubleClickEvent(QMouseEvent* e)
{
    if (debug) 
    {
	qDebug()<< "ME p="<<mapToScene (e->pos())<<"  scrollBarPos="<<getScrollBarPos() <<
		   "  min="<<QPointF(horizontalScrollBar()->minimum(),verticalScrollBar()->minimum());
    }


/*
    if (model->isSelectionBlocked() ) 
    {
	e->ignore();
	return;
    }
*/
    if (e->button() == Qt::LeftButton )
    {
	QPointF p = mapToScene(e->pos());
	TreeItem *ti=findMapItem (p, NULL);
	if (ti) {   // MapObj was found
	    // First select the MapObj than edit heading
	    model->select (ti);
	    editHeading();
	}
    }
}

void MapEditor::wheelEvent(QWheelEvent* e)
{
    //qDebug ()<<"ME::wheelEvent";  //FIXME-3 stop all animations in vymModel here
    //model->stopAllAnimation();
    QGraphicsView::wheelEvent (e);
}

void MapEditor::resizeEvent (QResizeEvent* e)
{
    QGraphicsView::resizeEvent( e );
}

void MapEditor::dragEnterEvent(QDragEnterEvent *event)
{
    //for (unsigned int i=0;event->format(i);i++) // Debug mime type
    //	cerr << event->format(i) << endl;

    if (event->mimeData()->hasImage())
	event->acceptProposedAction();
    else    
	if (event->mimeData()->hasUrls())
	    event->acceptProposedAction();
}

void MapEditor::dragMoveEvent(QDragMoveEvent *)
{
}

void MapEditor::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void MapEditor::dropEvent(QDropEvent *event)
{
    BranchItem *selbi=model->getSelectedBranch();
    if (selbi)
    {
	if (debug)
	{
	    foreach (QString format,event->mimeData()->formats()) 
		qDebug()<< "MapEditor: Dropped format: "<<qPrintable (format);
	    foreach (QUrl url,event->mimeData()->urls())
		qDebug()<< "  URL:"<<url.path();
	    //foreach (QString plain,event->mimeData()->text())
	    //	qDebug()<< "   PLAIN:"<<plain;
	    QByteArray ba=event->mimeData()->data("STRING");
	    
	    QString s;
	    s=ba;
	    qDebug() << "  STRING:" <<s;

	    ba=event->mimeData()->data("TEXT");
	    s=ba;
	    qDebug() << "    TEXT:" <<s;

	    ba=event->mimeData()->data("COMPOUND_TEXT");
	    s=ba;
	    qDebug() << "   CTEXT:" <<s;

	    ba=event->mimeData()->data("text/x-moz-url");
	    s=ba;
	    qDebug() << "   x-moz-url:" <<s;
	    foreach (char b,ba)
		if (b!=0) qDebug() << "b="<<b;
	}

	if (event->mimeData()->hasImage()) 
	{
	     QVariant imageData = event->mimeData()->imageData();
	     model->addFloatImage (qvariant_cast<QImage>(imageData));
	} else
	if (event->mimeData()->hasUrls())
	{
	    //model->selectLastBranch();
	    QList <QUrl> uris=event->mimeData()->urls();
	    QStringList files;
	    QString s;
	    QString heading;
	    BranchItem *bi;
	    for (int i=0; i<uris.count();i++)
	    {
		// Workaround to avoid adding empty branches
		if (!uris.at(i).toString().isEmpty())
		{
		    bi=model->addNewBranch();
		    if (bi)
		    {
			model->select(bi);
			QString u=uris.at(i).toString();
			s=uris.at(i).toLocalFile();
			if (!s.isEmpty()) 
			{
			   QString file = QDir::fromNativeSeparators(s);
			   heading = QFileInfo(file).baseName();
			   files.append(file);
			   if (file.endsWith(".vym", Qt::CaseInsensitive))
			       model->setVymLink(file);
			   else
			       model->setURL(u);
		       } else 
		       {
			    model->setURL(u);

/*
			    // Automatically try to fetch data from Bugzilla
			    if (settings.value( "/mainwindow/showTestMenu",false).toBool() 
				&& u.contains ("https://bugzilla.novell.com"))
				model->getBugzillaData();
*/				
		       }

		       if (!heading.isEmpty())
			   model->setHeading(heading);
		       else
			   model->setHeading(u);
			   
			model->select (bi->parent());	   
		    }
		}
	    }
	    //model->reposition();
	}
    }	
    event->acceptProposedAction();
}

void MapEditor::updateSelection(QItemSelection newsel,QItemSelection oldsel)
{
    // Note: Here we are prepared for multiple selections, though this 
    // is not yet implemented elsewhere

    // Here in MapEditor we can only select Branches and Images
    QList <TreeItem*> treeItemsNew;
    QList <TreeItem*> treeItemsOld;

    QModelIndex newIndex;

    bool do_reposition=false;

    QModelIndex ix;
    foreach (ix,newsel.indexes() )
    {
	TreeItem *ti= static_cast<TreeItem*>(ix.internalPointer());
	if (ti->isBranchLikeType() || ti->getType()==TreeItem::Image )
	    if (!treeItemsNew.contains(ti)) treeItemsNew.append (ti);
    }
    foreach (ix,oldsel.indexes() )
    {
	TreeItem *ti= static_cast<TreeItem*>(ix.internalPointer());
	if (ti->isBranchLikeType() || ti->getType()==TreeItem::Image )
	    if (!treeItemsOld.contains(ti)) treeItemsOld.append (ti);
    }

    // Trim list of selection rectangles 
    while (treeItemsNew.count() < selboxList.count() )
	delete selboxList.takeFirst();

    // Take care to tmp scroll/unscroll
    if (!oldsel.isEmpty())
    {
	QModelIndex ix=oldsel.indexes().first(); 
	if (ix.isValid() )
	{
	    TreeItem *ti= static_cast<TreeItem*>(ix.internalPointer());
	    if (ti)
	    {
		if (ti->isBranchLikeType() )
		{
		    // reset tmp scrolled branches
		    BranchItem *bi=(BranchItem*)ti;
		    if (bi->resetTmpUnscroll() )
			do_reposition=true;
		}
		if (ti->isBranchLikeType() || ti->getType()==TreeItem::Image)
		    // Hide link if not needed
		    ((MapItem*)ti)->getLMO()->updateVisibility();
	    }
	}
    }

    if (!treeItemsNew.isEmpty())
    {
	QModelIndex ix=newsel.indexes().first(); 
	if (ix.isValid() )
	{
	    newIndex=ix;

	    // Temporary unscroll if necessary
	    TreeItem *ti= static_cast<TreeItem*>(ix.internalPointer());
	    if (ti->isBranchLikeType() )
	    {
		BranchItem *bi=(BranchItem*)ti;
		if (bi->hasScrolledParent(bi) )
		{
		    if (bi->parentBranch()->tmpUnscroll() )
			do_reposition=true;
		}   
	    }
	    if (ti->isBranchLikeType() || ti->getType()==TreeItem::Image)
		// Show link if needed
		((MapItem*)ti)->getLMO()->updateVisibility();
	}
    }
    if (do_reposition) model->reposition();

    // Reduce rectangles
    while (treeItemsNew.count() < selboxList.count() )
	delete selboxList.takeFirst();

    // Add additonal rectangles
    QGraphicsRectItem *sb;
    while (treeItemsNew.count() > selboxList.count() )
    {
	sb = mapScene->addRect(
	    QRectF(0,0,0,0), 
	    QPen(selectionColor),
	    selectionColor);
	sb->show();
	selboxList.append (sb);
    }

    // Reposition rectangles
    QRectF box;
    QModelIndex index;

    LinkableMapObj *lmo;
    for (int i=0; i<treeItemsNew.count();++i)
    {
	lmo=((MapItem*)treeItemsNew.at(i) )->getLMO();
	// Don't draw huge selBox if children are included in frame
	if (treeItemsNew.at(i)->isBranchLikeType() && 
	    ((BranchItem*)treeItemsNew.at(i))->getFrameIncludeChildren() )
	    box=lmo->getClickBox();
	else
	    box=lmo->getBBox();
	sb=selboxList.at(i);
	sb->setRect (box);
	sb->setPen (selectionColor);	
	sb->setBrush (selectionColor);	
	sb->setZValue (dZ_DEPTH*treeItemsNew.at(i)->depth() + dZ_SELBOX);
	i++;
    }

    scene()->update();  
}

void MapEditor::updateData (const QModelIndex &sel)
{
    TreeItem *ti= static_cast<TreeItem*>(sel.internalPointer());

/* testing
    qDebug() << "ME::updateData";
    if (!ti) 
    {
	qDebug() << "  ti=NULL";
	return;
    }
    qDebug() << "  ti="<<ti;
    qDebug() << "  h="<<ti->getHeading();
*/
    
    if (ti && ti->isBranchLikeType())
    {
	BranchObj *bo=(BranchObj*) ( ((MapItem*)ti)->getLMO());
	bo->updateData();
    }
}

void MapEditor::setSelectionColor (QColor col)
{
    selectionColor=col;
    selectionColor.setAlpha (220);
    QItemSelection sel=model->getSelectionModel()->selection();
    updateSelection(sel,sel);
}


QColor MapEditor::getSelectionColor ()
{
    return selectionColor;
}


