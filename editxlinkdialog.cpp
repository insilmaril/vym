#include "editxlinkdialog.h"

#include <typeinfo>
#include <QColorDialog>
#include <QColorDialog>

#include "branchitem.h"

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
    //FIXME-4 connect ( ui.setColorHeadingButton, SIGNAL (clicked( )), this, SLOT (setColorHeadingButtonPressed()));
    ui.setColorHeadingButton->hide();
}


void EditXLinkDialog::widthChanged( int  w)
{
    QPen pen=link->getPen();
    pen.setWidth (w);
    link->setPen (pen);
    link->updateLink();
}

void EditXLinkDialog::setLink( Link * l)
{
    link=l;
    QPen pen=link->getPen();
    colorChanged (pen.color() );
    ui.widthBox->setValue(pen.width() );
    switch (pen.style() )
    {
//	case Qt::SolidLine: ui.lineStyleCombo->setCurrentIndex (0); break;
	case Qt::DashLine: ui.lineStyleCombo->setCurrentIndex (1); break;
	case Qt::DotLine: ui.lineStyleCombo->setCurrentIndex (2); break;
	case Qt::DashDotLine: ui.lineStyleCombo->setCurrentIndex (3); break;
	case Qt::DashDotDotLine: ui.lineStyleCombo->setCurrentIndex (4); break;
	default: ui.lineStyleCombo->setCurrentIndex (0);
    }
}

void EditXLinkDialog::colorButtonPressed()
{
    if (link)
    {	
	QPen pen=link->getPen();
	QColor col = QColorDialog::getColor(pen.color(), this );
	if ( !col.isValid() ) return;
	pen.setColor (col);
	link->setPen (pen);
	colorChanged (col);
	link->updateLink();
    }
}

void EditXLinkDialog::colorChanged (QColor c)
{
    
    QPixmap pix( 16, 16 );
    pix.fill( c );
    ui.colorButton->setIcon( pix );
}

void EditXLinkDialog::setColorHeadingButtonPressed()	//FIXME-2 not implemented yet
{
    if (link)
    {	
    }
}

void EditXLinkDialog::lineStyleChanged (int i)
{
    if (link)
    {	
	QPen pen=link->getPen();
	Qt::PenStyle s;
	switch (i)
	{
	    case 0: s=Qt::SolidLine; break;
	    case 1: s=Qt::DashLine; break;
	    case 2: s=Qt::DotLine; break;
	    case 3: s=Qt::DashDotLine; break;
	    case 4: s=Qt::DashDotDotLine; break;
	    default: s=Qt::NoPen;
	}
	pen.setStyle (s);
	link->setPen (pen);
	link->updateLink();
    }
}

bool EditXLinkDialog::useSettingsGlobal ()
{
    return ui.useSettings->isChecked();
}
