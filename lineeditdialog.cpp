#include "lineeditdialog.h"
#include "settings.h"

extern Settings settings;

LineEditDialog::LineEditDialog(QWidget* parent):QDialog (parent)
{
    ui.setupUi(this);
    ui.okButton->setText(tr("Ok"));
}

void LineEditDialog::showCancelButton (bool b)
{
    if (b)
    {
	ui.cancelButton->show();
	ui.cancelButton->setText(tr("Cancel"));
    } else
	ui.cancelButton->hide();
}

void LineEditDialog::setLabel (const QString &s)
{
    ui.label->setText(s);
}

void LineEditDialog::setCaption(const QString &s)
{
    QDialog::setWindowTitle("VYM - "+s);
}

void LineEditDialog::setText (const QString &s)
{
    ui.lineEdit->setText(s);
    ui.lineEdit->selectAll();
}

QString LineEditDialog::getText ()
{
    return ui.lineEdit->text();
}

