#include "extrainfodialog.h"


ExtraInfoDialog::ExtraInfoDialog(QWidget *parent):QDialog (parent)
{
    ui.setupUi (this);
}


void ExtraInfoDialog::setMapName(const QString &s)
{
    ui.mapNameLE->setText (s);
}

void ExtraInfoDialog::setFileLocation(const QString &s)
{
    ui.fileLocationLE->setText (s);
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

void ExtraInfoDialog::setStats(const QString &s)
{
    ui.statsTE->setText (s);
}

void ExtraInfoDialog::setReadOnly(bool b)
{
    readOnly = b;
    ui.authorLE->setReadOnly( readOnly );
    ui.commentTE->setReadOnly( readOnly );
    ui.mapTitleLE->setReadOnly( readOnly );
}

bool ExtraInfoDialog::isReadOnly()
{
    return readOnly;
}
