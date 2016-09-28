#include "warningdialog.h"
#include "settings.h"

extern Settings settings;

WarningDialog::WarningDialog(QWidget* parent):QDialog (parent)
{
    ui.setupUi(this);
    //TODO proper icon for proceed needed
    ui.okButton->setText(tr("Proceed"));
    // ui.warningSign->setPixmap (QPixmap(":/vym.png"));
    ui.showAgainBox->setText (tr("Show this message again"));
    useShowAgain=false;
    ui.showAgainBox->hide();
}

int WarningDialog::exec()
{
    int result; 
    if (settings.value ("/warningDialog/"+showAgainName+"/showAgain",true).toBool()  )
    {
	// Really show dialog
	result=QDialog::exec();
	if (result==QDialog::Accepted )
	{
	    if (useShowAgain)
	    {
		settings.setValue ("/warningDialog/"+showAgainName+"/value",result);
		settings.setValue ("/warningDialog/"+showAgainName+"/showAgain",ui.showAgainBox->isChecked() );
	    }
	}
    } else
    {
	// Surpress dialog and use result from last shown dialog
	result=settings.value ("/warningDialog/"+showAgainName+"/value",0).toInt();
    }
    return result;
}

void WarningDialog::showCancelButton (bool b)
{
    if (b)
    {
	ui.cancelButton->show();
	ui.cancelButton->setText(tr("Cancel"));
    } else
	ui.cancelButton->hide();
}

void WarningDialog::setShowAgainName (const QString &s) 
{
    showAgainName=s;
    useShowAgain=true;
    ui.showAgainBox->show();
}

void WarningDialog::setText (const QString &s)
{
    ui.warningTE->setText(s);
}

void WarningDialog::setCaption(const QString &s)
{
    QDialog::setWindowTitle("VYM - "+s);
}
