#include "editxlinkdialog.h"

#include <typeinfo>
#include <QColorDialog>

#include "branchitem.h"

EditXLinkDialog::EditXLinkDialog (QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);

    delink=false;
    link=NULL;

    connect ( ui.widthBox, SIGNAL (valueChanged( int)), this, SLOT (widthChanged (int)));
    connect ( ui.colorButton, SIGNAL (clicked( )), this, SLOT (colorButtonPressed()));
    //FIXME-4 connect ( ui.setColorHeadingButton, SIGNAL (clicked( )), this, SLOT (setColorHeadingButtonPressed()));
    ui.setColorHeadingButton->hide();
}


void EditXLinkDialog::widthChanged( int  w)
{
    QPen pen=link->getPen();
    pen.setWidth (w);
    link->setPen (pen);
}

void EditXLinkDialog::setLink( Link * l)
{
    link=l;
    colorChanged (link->getPen().color() );
    ui.widthBox->setValue(link->getPen().width());
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
    }
}

void EditXLinkDialog::colorChanged (QColor c)
{
    
    QPixmap pix( 16, 16 );
    pix.fill( c );
    ui.colorButton->setIcon( pix );
}

void EditXLinkDialog::setColorHeadingButtonPressed()	//FIXME-4 add 2nd button for begin/end and include beginnings of headings
{
    if (link)
    {	
    }
}

bool EditXLinkDialog::useSettingsGlobal ()
{
    return ui.useSettings->isChecked();
}
