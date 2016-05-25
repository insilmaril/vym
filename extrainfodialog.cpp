#include "extrainfodialog.h"


ExtraInfoDialog::ExtraInfoDialog(QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
}


void ExtraInfoDialog::setMapName(const QString &s)
{
    ui.mapNameLE->setText (s);
}

void ExtraInfoDialog::setMapTitle(const QString &s)
{
    ui.mapTitleLE->setText (s);
}

QString ExtraInfoDialog::getMapTitle()
{
    return ui.mapTitleLE->text();
}

void ExtraInfoDialog::setComment (const QString &s)
{
    ui.commentTE->setText (s);
}

QString ExtraInfoDialog::getComment()
{
    return ui.commentTE->toHtml();
}   


void ExtraInfoDialog::setAuthor(const QString &s)
{
    ui.authorLE->setText (s);
}

QString ExtraInfoDialog::getAuthor()
{
    return ui.authorLE->text();
}

bool ExtraInfoDialog::lockfileUsed()
{
    return ui.lockfileCheckBox->isChecked();
}

void ExtraInfoDialog::setLockfile( bool b)
{
    ui.lockfileCheckBox->setChecked( b );
}

void ExtraInfoDialog::setStats(const QString &s)
{
    ui.statsTE->setText (s);
}
