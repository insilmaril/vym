#include "editxlinkdialog.h"

#include <typeinfo>
#include <QColorDialog>

#include "branchitem.h"

EditXLinkDialog::EditXLinkDialog (QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);

    delink=false;
    link=NULL;
    selbi=NULL;

    connect ( ui.widthBox, SIGNAL (valueChanged( int)), this, SLOT (widthChanged (int)));
    connect ( ui.colorButton, SIGNAL (clicked( )), this, SLOT (colorButtonPressed()));
    connect ( ui.setColorHeadingButton, SIGNAL (clicked( )), this, SLOT (setColorHeadingButtonPressed()));
    connect ( ui.deleteButton, SIGNAL (clicked( )), this, SLOT (deleteButtonPressed()));
}

void EditXLinkDialog::deleteButtonPressed()
{
    delink=true;
    accept();
}

bool EditXLinkDialog::deleteXLink()
{
    return delink;
}   


void EditXLinkDialog::widthChanged( int  w)
{
    link->setWidth(w);
}

void EditXLinkDialog::setLink( Link * l)
{
    link=l;
    //FIXME-0 ui.colorButton->setPaletteBackgroundColor (link->getColor() );
    ui.widthBox->setValue(link->getWidth());
}

void EditXLinkDialog::setSelection(BranchItem *bi)
{
    selbi=bi;
}

void EditXLinkDialog::colorButtonPressed()
{
    if (link)
    {	
	QColor col = QColorDialog::getColor(link->getColor(), this );
	if ( !col.isValid() ) return;
	link->setColor( col );
	//FIXME-0 ui.colorButton->setPaletteBackgroundColor (col);
    }
}

void EditXLinkDialog::setColorHeadingButtonPressed()	
{
    if (link)
    {	
	if (selbi)
	{
	    QColor col=selbi->getHeadingColor();
	    link->setColor(col);
	    //FIXME-0 ui.colorButton->setPaletteBackgroundColor (col);
	}
    }
}

bool EditXLinkDialog::useSettingsGlobal ()
{
    return ui.useSettings->isChecked();
}
