#include "showtextdialog.h"

#include "settings.h"
#include <QFont>
#include <QString>

extern Settings settings;

ShowTextDialog::ShowTextDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    ui.textBrowser->show();
    ui.textBrowser->setOpenExternalLinks(true);
}

void ShowTextDialog::append(const QString &s) { ui.textBrowser->append(s); }

void ShowTextDialog::setText(const QString &s) { ui.textBrowser->setText(s); }

void ShowTextDialog::setHtml(const QString &s) { ui.textBrowser->setHtml(s); }

void ShowTextDialog::useFixedFont(bool useFixedFont)
{
    QFont font;
    if (useFixedFont)
        font.fromString(settings
                            .value("/satellite/noteeditor/fonts/fixedFont",
                                   "Courier,10,-1,5,48,0,0,0,1,0")
                            .toString());
    else
        font.fromString(settings
                            .value("/satellite/noteeditor/fonts/varFont",
                                   "DejaVu Sans Mono,12,-1,0,50,0,0,0,0,0")
                            .toString());
    ui.textBrowser->setFont(font);
}
