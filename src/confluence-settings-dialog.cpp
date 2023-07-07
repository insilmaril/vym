#include "confluence-settings-dialog.h"

#include "settings.h"

extern QString confluencePassword;
extern Settings settings;

ConfluenceSettingsDialog::ConfluenceSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("Confluence settings", "Confluence settings dialog title"));

    ui.urlLineEdit->setText(
        settings.value("/atlassian/confluence/url", "Confluence base URL").toString());

    bool b;
    b = settings.value("/atlassian/confluence/authUsingPAT", true).toBool();
    if (b)
        ui.usePATCheckBox->setCheckState(Qt::Checked);
    else
        ui.usePATCheckBox->setCheckState(Qt::Unchecked);
    ui.PATLineEdit->setText(
        settings.value("/atlassian/confluence/PAT", "").toString());

    ui.userLineEdit->setText(
            settings.value("/atlassian/confluence/username", "Confluence username")
                    .toString());

    if (!confluencePassword.isEmpty())
        // password is only in memory, not saved in settings
        ui.passwordLineEdit->setText(confluencePassword);
    else
        ui.passwordLineEdit->setText(
            settings.value("/atlassian/confluence/password", "").toString());

    b = settings.value("/atlassian/confluence/savePassword", false).toBool();
    if (b)
        ui.savePasswordCheckBox->setCheckState(Qt::Checked);
    else
        ui.savePasswordCheckBox->setCheckState(Qt::Unchecked);

    connect(ui.usePATCheckBox, SIGNAL(clicked()), this, SLOT(updateAuthenticationFields()));
    connect(this, &QDialog::accepted, this, &ConfluenceSettingsDialog::updateSettings);

    updateAuthenticationFields();
}

void ConfluenceSettingsDialog::updateAuthenticationFields()
{
    if (ui.usePATCheckBox->isChecked()) {
        ui.PATLineEdit->show();
        ui.PATLabel->show();
        ui.userLabel->hide();
        ui.userLineEdit->hide();
        ui.passwordLabel->hide();
        ui.passwordLineEdit->hide();
        ui.savePasswordCheckBox->hide();
    } else {
        ui.PATLineEdit->hide();
        ui.PATLabel->hide();
        ui.userLabel->show();
        ui.userLineEdit->show();
        ui.passwordLabel->show();
        ui.passwordLineEdit->show();
        ui.savePasswordCheckBox->show();
    }
    adjustSize();
}

void ConfluenceSettingsDialog::updateSettings()
{
    settings.remove("confluence");

    settings.beginGroup("/atlassian/confluence");
    settings.setValue("authUsingPAT", ui.usePATCheckBox->isChecked());
    settings.setValue("username", ui.userLineEdit->text());
    if (ui.usePATCheckBox->isChecked()) {
        // Don't save password if PAT is used
        settings.remove("savePassword");
        settings.remove("password");
        if(!ui.PATLineEdit->text().isEmpty())
            settings.setValue("PAT", ui.PATLineEdit->text());
        else
            settings.remove("PAT");
    } else {
        settings.remove("PAT");
        settings.setValue("savePassword", ui.savePasswordCheckBox->isChecked());

        // Save password only on request persistently in settings
        if (ui.savePasswordCheckBox->isChecked())
            settings.setValue("password", ui.passwordLineEdit->text());
        else
            settings.remove("password");

        // Save password in memory
        confluencePassword = ui.passwordLineEdit->text();
    }
        
    settings.endGroup();
}
