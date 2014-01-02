#include "editxlinkdialog.h"

#include <typeinfo>
#include <QColorDialog>
#include <QColorDialog>

#include "branchitem.h"
#include "vymmodel.h"
#include "xlinkobj.h"

extern QString iconPath;

EditXLinkDialog::EditXLinkDialog (QWidget *parent):QDialog (parent) 
{
    ui.setupUi (this);

    delink=false;
    link=NULL;

    ui.lineStyleCombo->addItem (QIcon(iconPath+"/linestyle-solid.png"),"Solid line",0);   
    ui.lineStyleCombo->addItem (QIcon(iconPath+"/linestyle-dash.png"),"Dash line",1);
    ui.lineStyleCombo->addItem (QIcon(iconPath+"/linestyle-dot.png"),"Dot line",2);
    ui.lineStyleCombo->addItem (QIcon(iconPath+"/linestyle-dashdot.png"),"Dash Dot line",3);
    ui.lineStyleCombo->addItem (QIcon(iconPath+"/linestyle-dashdotdot.png"),"Dash Dot Dot line",4);
    connect ( ui.widthBox, SIGNAL (valueChanged( int)), this, SLOT (widthChanged (int)));
    connect ( ui.colorButton, SIGNAL (clicked( )), this, SLOT (colorButtonPressed()));
    connect ( ui.lineStyleCombo, SIGNAL (currentIndexChanged( int )), this, SLOT (lineStyleChanged(int)));
    connect ( ui.checkBoxArrowBegin, SIGNAL (stateChanged( int )), this, SLOT (beginStyleChanged( int )));
    connect ( ui.checkBoxArrowEnd  , SIGNAL (stateChanged( int )), this, SLOT (  endStyleChanged( int )));
    //FIXME-4 connect ( ui.setColorHeadingButton, SIGNAL (clicked( )), this, SLOT (setColorHeadingButtonPressed()));
    ui.setColorHeadingButton->hide();
}


void EditXLinkDialog::widthChanged( int  w)
{
    link->getModel()->setXLinkWidth( w );
}

void EditXLinkDialog::setLink( Link * l)
{
    link=l;
    QPen pen=link->getPen();
    colorChanged (pen.color() );
    ui.widthBox->setValue(pen.width() );
    switch (pen.style() )
    {
	case Qt::DashLine: ui.lineStyleCombo->setCurrentIndex (1); break;
	case Qt::DotLine: ui.lineStyleCombo->setCurrentIndex (2); break;
	case Qt::DashDotLine: ui.lineStyleCombo->setCurrentIndex (3); break;
	case Qt::DashDotDotLine: ui.lineStyleCombo->setCurrentIndex (4); break;
	default: ui.lineStyleCombo->setCurrentIndex (0);
    }
    if ( link->getXLinkObj()->getStyleEnd() )
        ui.checkBoxArrowEnd->setChecked( true );
    else
        ui.checkBoxArrowEnd->setChecked( false );
    if ( link->getXLinkObj()->getStyleBegin() )
        ui.checkBoxArrowBegin->setChecked( true );
    else
        ui.checkBoxArrowBegin->setChecked( false );
}

void EditXLinkDialog::colorButtonPressed()
{
    if (link)
    {	
	QPen pen=link->getPen();
	QColor col = QColorDialog::getColor(pen.color(), this );
	if ( !col.isValid() ) return;
        link->getModel()->setXLinkColor( col.name() );
    }
}

void EditXLinkDialog::colorChanged (QColor c)
{
    
    QPixmap pix( 16, 16 );
    pix.fill( c );
    ui.colorButton->setIcon( pix );
}

void EditXLinkDialog::setColorHeadingButtonPressed()	//FIXME-4 not implemented yet
{
    if (link)
    {	
    }
}

void EditXLinkDialog::lineStyleChanged (int i)
{
    if (link)
    {	
        QString style;
	switch (i)
	{
            case 0: style = "Qt::SolidLine"; break;
	    case 1: style = "Qt::DashLine"; break;
	    case 2: style = "Qt::DotLine"; break;
	    case 3: style = "Qt::DashDotLine"; break;
	    case 4: style = "Qt::DashDotDotLine"; break;
	    default: style = "Qt::NoPen";
	}
        link->getModel()->setXLinkLineStyle( style );
    }
}

void EditXLinkDialog::beginStyleChanged( int state )
{
    if (link)
    {
        if (state)
            link->getModel()->setXLinkStyleBegin( "HeadFull" );
        else
            link->getModel()->setXLinkStyleBegin( "None" );
    }
}

void EditXLinkDialog::endStyleChanged( int state )
{
    if (link)
    {
        if (state)
            link->getModel()->setXLinkStyleEnd( "HeadFull" );
        else
            link->getModel()->setXLinkStyleEnd( "None" );
    }
}

bool EditXLinkDialog::useSettingsGlobal ()
{
    return ui.useSettings->isChecked();
}
