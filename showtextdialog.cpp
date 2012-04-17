#include "showtextdialog.h"

#include <QFont>
#include <QString>
#include "settings.h"

extern Settings settings;

ShowTextDialog::ShowTextDialog (QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
    QFont fixedFont;
    fixedFont.fromString (settings.value(
	"/satellite/noteeditor/fonts/fixedFont",
	"Courier,10,-1,5,48,0,0,0,1,0").toString() 
    );
    ui.textEdit->setFont (fixedFont);
}

void ShowTextDialog::append  (const QString &s)
{
    ui.textEdit->append (s);
}

void ShowTextDialog::setText (const QString &s)
{
    ui.textEdit->setText (s);
}


