#include "showtextdialog.h"

#include <QFont>
#include <QString>
#include "settings.h"

extern Settings settings;

ShowTextDialog::ShowTextDialog (QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
    ui.textBrowser->show();
}

void ShowTextDialog::append  (const QString &s)
{
    ui.textBrowser->append (s);
}

void ShowTextDialog::setText (const QString &s)
{
    ui.textBrowser->setText (s);
}

void ShowTextDialog::useFixedFont (bool useFixedFont)
{
    if (useFixedFont) 
    {
        QFont fixedFont;
        fixedFont.fromString (settings.value(
                    "/satellite/noteeditor/fonts/fixedFont",
                    "Courier,10,-1,5,48,0,0,0,1,0").toString() 
                );
        ui.textBrowser->setOpenExternalLinks( true );
    }
}


