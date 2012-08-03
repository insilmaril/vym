#include "branchpropeditor.h"

#include <QColorDialog>

#include "branchitem.h"
#include "frameobj.h"
#include "settings.h"

extern Settings settings;
extern QString vymName;


BranchPropertyEditor::BranchPropertyEditor (QWidget *parent): QDialog (parent)	// FIXME-4 not updating when data is set elsewhere (connect to dataCHanged)

{
    ui.setupUi (this);

    setWindowTitle (vymName +" - " +tr ("Property Editor","Window caption"));

    branchObj=NULL;
    branchItem=NULL;
    model=NULL;

    ui.tabWidget->setEnabled(false);

    penColor=QColor (Qt::black);
    brushColor=QColor (Qt::black);
    QPixmap pix( 16,16);
    pix.fill (penColor);
    ui.framePenColorButton->setIcon (pix);
    ui.frameBrushColorButton->setIcon (pix);

    if (!settings.value( "/mainwindow/showTestMenu",false).toBool() )
	ui.tabWidget->widget (3)->hide();

	
    /* Create Model and View to hold attributes	    
    attributeModel = new QStandardItemModel (1,3,this);
    attributeModel->setHeaderData(0, Qt::Horizontal, tr("Name","Branchprop window: Attribute name"));
    attributeModel->setHeaderData(1, Qt::Horizontal, tr("Value","Branchprop window: Attribute value"));
    attributeModel->setHeaderData(2, Qt::Horizontal, tr("Type","Branchprop window: Attribute type"));
    ui.attributeTableView->setModel (attributeModel);
    */

    // Load Settings
    resize (settings.value ( "/satellite/propertywindow/geometry/size", QSize(450,600)).toSize());
    move   (settings.value ( "/satellite/propertywindow/geometry/pos", QPoint (250,50)).toPoint());
    
    if (settings.value ( "/satellite/propertywindow/showWithMain",true).toBool())
	show();
    else    
	hide();
}

BranchPropertyEditor::~BranchPropertyEditor ()
{
    settings.setValue( "/satellite/propertywindow/geometry/size", size() );
    settings.setValue( "/satellite/propertywindow/geometry/pos", pos() );
    settings.setValue( "/satellite/propertywindow/showWithMain",isVisible() );
}

void BranchPropertyEditor::setItem (TreeItem *ti)   
{
    disconnectSignals();
    if (!ti)
	ui.tabWidget->setEnabled (false);
    else if (ti->isBranchLikeType() )
    {
	branchItem=(BranchItem*)ti;

	branchObj=(BranchObj*)(branchItem->getLMO());
	if (branchObj)	    // FIXME-4 replace by branchItem later, when Frame is ported...
	{
	    ui.tabWidget->setEnabled (true);
	    for (int i=0; i<3;++i)
		ui.tabWidget->setTabEnabled (i,true);
	    ui.tabWidget->setTabEnabled (3,false);

	    // Frame
	    FrameObj::FrameType t=branchObj->getFrameType();
	    if (t==FrameObj::NoFrame)
	    {
		ui.frameTypeCombo->setCurrentIndex (0);
		penColor=Qt::white;
		brushColor=Qt::white;
		ui.colorGroupBox->setEnabled (false);
		ui.framePaddingSpinBox->setEnabled (false);
		ui.frameWidthSpinBox->setEnabled (false);
		ui.framePaddingLabel->setEnabled (false);
		ui.frameBorderLabel->setEnabled (false);
		ui.includeChildrenCheckBox->setEnabled (false);
	    } else  
	    {
		penColor=branchObj->getFramePenColor();
		brushColor=branchObj->getFrameBrushColor();
		QPixmap pix( 16,16);
		pix.fill (penColor);
		ui.framePenColorButton->setIcon (pix);
		pix.fill (brushColor);
		ui.frameBrushColorButton->setIcon (pix);
		ui.colorGroupBox->setEnabled (true);
		ui.framePaddingSpinBox->setEnabled (true);
		ui.framePaddingSpinBox->setValue (branchObj->getFramePadding());
		ui.frameWidthSpinBox->setEnabled (true);
		ui.frameWidthSpinBox->setValue (branchObj->getFrameBorderWidth());
		ui.framePaddingLabel->setEnabled (true);
		ui.frameBorderLabel->setEnabled (true);
		ui.includeChildrenCheckBox->setEnabled (true);

		switch (t)
		{
		    case FrameObj::Rectangle: 
			ui.frameTypeCombo->setCurrentIndex (1);
			break;
		    case FrameObj::RoundedRectangle: 
			ui.frameTypeCombo->setCurrentIndex (2);
			break;
		    case FrameObj::Ellipse: 
			ui.frameTypeCombo->setCurrentIndex (3);
			break;
		    case FrameObj::Cloud: 
			ui.frameTypeCombo->setCurrentIndex (4);
			break;
		    default: 
			break;
		}
		if (branchItem->getFrameIncludeChildren())
		    ui.includeChildrenCheckBox->setCheckState (Qt::Checked);
		else    
		    ui.includeChildrenCheckBox->setCheckState (Qt::Unchecked);
	    }	
	    
	    // Link
	    if (branchItem->getHideLinkUnselected())
		ui.hideLinkIfUnselected->setCheckState (Qt::Checked);
	    else    
		ui.hideLinkIfUnselected->setCheckState (Qt::Unchecked);

	    // Layout
	    if (branchItem->getIncludeImagesVer())
		ui.incImgVer->setCheckState (Qt::Checked);
	    else    
		ui.incImgVer->setCheckState (Qt::Unchecked);
	    if (branchItem->getIncludeImagesHor())
		ui.incImgHor->setCheckState (Qt::Checked);
	    else    
		ui.incImgHor->setCheckState (Qt::Unchecked);

    /*
	    // Attributes
	    attributeModel->removeRows(0, attributeModel->rowCount(), QModelIndex());

	    // FIXME-5 some samples for attribute testing
	    QStringList attrTypes=mapEditor->attributeTable()->getTypes();
	    for (int i=0; i<attrTypes.count()-1;i++)
	    {
		attributeModel->insertRow (i,QModelIndex ());
		attributeModel->setData(attributeModel->index(i, 0, QModelIndex()), QString ("Name %1").arg(i));
		attributeModel->setData(attributeModel->index(i, 1, QModelIndex()), i);
		attributeModel->setData(attributeModel->index(i, 2, QModelIndex()), attrTypes.at(i));
	    }


	    ui.attributeTableView->resizeColumnsToContents();

	    // Initialize Delegate
	    delegate.setAttributeTable (mapEditor->attributeTable());
	    ui.attributeTableView->setItemDelegate (&delegate);
    */

	    // Finally activate signals
	    connectSignals();
	} // BranchItem
    } else if (ti->getType()==TreeItem::Attribute)
    {
	ui.tabWidget->setEnabled (true);
	for (int i=0; i<3;++i)
	    ui.tabWidget->setTabEnabled (i,false);
	ui.tabWidget->setTabEnabled (3,true);
    } else
    {
	ui.tabWidget->setEnabled (false);
    }
}

void BranchPropertyEditor::setModel (VymModel *m)
{
    model=m;
    if (model) 
	setItem (model->getSelectedItem() );
    else
	ui.tabWidget->setEnabled (false);
	
}

void BranchPropertyEditor::frameTypeChanged (int i)
{
    if (model)
    {
	switch (i)
	{
	    case 0: model->setFrameType (FrameObj::NoFrame); break;
	    case 1: 
		model->setFrameType (FrameObj::Rectangle); 
		break;
	    case 2: 
		model->setFrameType (FrameObj::RoundedRectangle); 
		break;
	    case 3: 
		model->setFrameType (FrameObj::Ellipse); 
		break;
	    case 4: 
		model->setFrameType (FrameObj::Cloud); 
		break;
	}
	setItem (branchItem);
    }	
}

void BranchPropertyEditor::framePenColorClicked()
{
    if (model) 
    {	
	QColor col = QColorDialog::getColor( penColor, this );
	if ( col.isValid() ) 
	{
	    penColor=col;
	    model->setFramePenColor (penColor);
	}   
    }
}

void BranchPropertyEditor::frameBrushColorClicked()
{
    if (model) 
    {
	QColor col = QColorDialog::getColor( brushColor, this );
	if ( col.isValid() ) 
	{
	    brushColor=col;
	    model->setFrameBrushColor (brushColor);
	}   
    }	
}

void BranchPropertyEditor::framePaddingChanged(int i)
{
    if (model) model->setFramePadding (i);
}

void BranchPropertyEditor::frameBorderWidthChanged(int i)
{
    if (model) model->setFrameBorderWidth(i);
}

void BranchPropertyEditor::frameIncludeChildrenChanged(int i)
{
    if (model) model->setFrameIncludeChildren(i);
}

void BranchPropertyEditor::linkHideUnselectedChanged (int i)
{
    if (!branchItem) return;
    model->setHideLinkUnselected(i);
}

void BranchPropertyEditor::incImgVerChanged (int  i)
{
    if (model) model->setIncludeImagesVer (i);
}

void BranchPropertyEditor::incImgHorChanged (int  i)
{
    if (model) model->setIncludeImagesHor (i);
}

void BranchPropertyEditor::closeEvent( QCloseEvent* ce )
{
    ce->accept();   // can be reopened with show()
    hide();
    emit (windowClosed() );
    return;
}

/*
void BranchPropertyEditor::addAttributeClicked()
{
    // Add empty line for adding attributes
    attributeModel->insertRow (attributeModel->rowCount (),QModelIndex ());
    attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 0, QModelIndex()),  "Add new");
    attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 2, QModelIndex()),  "Undefined");

    // Select attribute from list
    ui.attributeTableView->edit (attributeModel->index(attributeModel->rowCount()-1,0, QModelIndex() ));
    ui.attributeTableView->resizeColumnsToContents();

//  QString attname=attributeModel->in
//  attributeModel->setData(attributeModel->index(attributeModel->rowCount()-1, 2, QModelIndex()),  );



    ui.attributeTableView->edit (attributeModel->index(attributeModel->rowCount()-1,1, QModelIndex() ));

}

void BranchPropertyEditor::deleteAttributeClicked()
{
    //qDebug() << "BPW::delete";
}
*/

void BranchPropertyEditor::connectSignals()
{
    // Frame
    connect ( 
	ui.framePenColorButton, SIGNAL (clicked()), 
	this, SLOT (framePenColorClicked()));
    connect ( 
	ui.framePaddingSpinBox, SIGNAL (valueChanged( int)), 
	this, SLOT (framePaddingChanged (int)));
    connect ( 
	ui.frameWidthSpinBox, SIGNAL (valueChanged( int)), 
	this, SLOT (frameBorderWidthChanged (int)));
    connect ( 
	ui.frameBrushColorButton, SIGNAL (clicked()), 
	this, SLOT (frameBrushColorClicked()));
    connect ( 
	ui.frameTypeCombo, SIGNAL (currentIndexChanged( int)), 
	this, SLOT (frameTypeChanged (int)));
    connect ( 
	ui.includeChildrenCheckBox, SIGNAL (stateChanged( int)), 
	this, SLOT (frameIncludeChildrenChanged (int)));

    // Link 
    connect ( 
	ui.hideLinkIfUnselected, SIGNAL (stateChanged( int)), 
	this, SLOT (linkHideUnselectedChanged (int)));

    // Layout	
    connect ( 
	ui.incImgVer, SIGNAL (stateChanged( int)), 
	this, SLOT (incImgVerChanged (int)));
    connect ( 
	ui.incImgHor, SIGNAL (stateChanged( int)), 
	this, SLOT (incImgHorChanged (int)));

/*
    // Attributes   
    connect ( 
	ui.addAttributeButton, SIGNAL (clicked()), 
	this, SLOT (addAttributeClicked()));
    connect ( 
	ui.deleteAttributeButton, SIGNAL (clicked()), 
	this, SLOT (deleteAttributeClicked()));
*/	
}


void BranchPropertyEditor::disconnectSignals()
{
    // Frame 
    disconnect ( ui.frameTypeCombo, 0,0,0);
    disconnect ( ui.framePenColorButton, 0,0,0);
    disconnect ( ui.framePaddingSpinBox, 0,0,0);
    disconnect ( ui.frameWidthSpinBox, 0,0,0);
    disconnect ( ui.frameBrushColorButton, 0,0,0);
    disconnect ( ui.includeChildrenCheckBox, 0,0,0);

    // Link 
    disconnect ( ui.hideLinkIfUnselected, 0,0,0);

    // Layout	
    disconnect ( ui.incImgVer, 0,0,0);
    disconnect ( ui.incImgHor, 0,0,0);

    // Attributes
    /*
    disconnect ( ui.addAttributeButton, 0,0,0);
    disconnect ( ui.deleteAttributeButton, 0,0,0);
    */
}


