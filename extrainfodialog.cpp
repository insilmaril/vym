#include "extrainfodialog.h"


ExtraInfoDialog::ExtraInfoDialog(QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
}


void ExtraInfoDialog::setMapName(const QString &s)
{
    ui.mapNameLE->setText (s);
}

void ExtraInfoDialog::setComment (const QString &s)
{
    ui.commentTE->setText (s);
}

QString ExtraInfoDialog::getComment()
{
    return ui.commentTE->text();
}   


void ExtraInfoDialog::setAuthor(const QString &s)
{
    ui.authorLE->setText (s);
}

QString ExtraInfoDialog::getAuthor()
{
    return ui.authorLE->text();
}

void ExtraInfoDialog::setStats(const QString &s)
{
    ui.statsTE->setText (s);
}
