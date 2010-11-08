#include "showtextdialog.h"


ShowTextDialog::ShowTextDialog (QWidget *parent):QDialog (parent)
{
	ui.setupUi (this);
}

void ShowTextDialog::append  (const QString &s)
{
	ui.textEdit->append (s);
}

void ShowTextDialog::setText (const QString &s)
{
	ui.textEdit->setText (s);
}


