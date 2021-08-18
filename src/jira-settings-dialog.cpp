#include "jira-settings-dialog.h"

#include "settings.h"

extern Settings settings;

JiraSettingsDialog::JiraSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("JiraSettingsDialog dialog", "dialog window title"));

    ui.tableWidget->setRowCount(2);
    ui.tableWidget->setColumnCount(2);
    // connect (ui.lineEdit, SIGNAL(textChanged(const QString&)), this,
    // SLOT(lineEditChanged()));

    // connect (ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    // connect (ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
}

void JiraSettingsDialog::setUser(const QString &u)
{
    ui.userLineEdit->setText(u);
}

QString JiraSettingsDialog::getUser() { return ui.userLineEdit->text(); }

void JiraSettingsDialog::setPassword(const QString &p)
{
    ui.passwordLineEdit->setText(p);
}

QString JiraSettingsDialog::getPassword() { return ui.passwordLineEdit->text(); }

void JiraSettingsDialog::setSavePassword(const bool &b)
{
    if (b)
        ui.savePasswordCheckBox->setCheckState(Qt::Checked);
    else
        ui.savePasswordCheckBox->setCheckState(Qt::Unchecked);
}

bool JiraSettingsDialog::savePassword()
{
    return ui.savePasswordCheckBox->checkState();
}
