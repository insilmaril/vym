#include "credentials.h"

CredentialsDialog::CredentialsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle(
        "VYM - " + tr("Credentials dialog", "dialog window title"));

    //connect (ui.lineEdit, SIGNAL(textChanged(const QString&)), this, SLOT(lineEditChanged()));

    //connect (ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    //connect (ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void CredentialsDialog::setURL( const QString &u)
{
    ui.urlLineEdit->setText(u);
}

QString CredentialsDialog::getURL()
{
    return ui.urlLineEdit->text();
}

void CredentialsDialog::setUser( const QString &u)
{
    ui.userLineEdit->setText(u);
}

QString CredentialsDialog::getUser()
{
    return ui.userLineEdit->text();
}

void CredentialsDialog::setPassword( const QString &p)
{
    ui.passwordLineEdit->setText(p);
}

QString CredentialsDialog::getPassword()
{
    return ui.passwordLineEdit->text();
}

void CredentialsDialog::setSavePassword(const bool &b)
{
    if (b)
        ui.savePasswordCheckBox->setCheckState(Qt::Checked);
    else
        ui.savePasswordCheckBox->setCheckState(Qt::Unchecked);
}

bool CredentialsDialog::savePassword()
{
    return ui.savePasswordCheckBox->checkState();
}
