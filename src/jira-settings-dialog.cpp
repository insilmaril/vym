#include "jira-settings-dialog.h"

#include <QDebug>

#include "settings.h"

extern Settings settings;
extern QString jiraPassword;

JiraSettingsDialog::JiraSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("Jira settings", "Jira settings dialog title"));

    ui.tableWidget->setColumnCount(3);

        settings.beginGroup("/atlassian/jira");
        QTableWidgetItem *newItem;

        QStringList headers;
        headers << "Name";
        headers << "URL";
        headers << "Pattern";
        ui.tableWidget->setHorizontalHeaderLabels(headers);

        int size = settings.beginReadArray("servers");
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            ui.tableWidget->insertRow(0);
            foreach (QString p, settings.value("pattern").toString().split(",")) {

                newItem = new QTableWidgetItem(settings.value("name").toString());
                ui.tableWidget->setItem(0, 0, newItem);

                newItem = new QTableWidgetItem(settings.value("baseURL").toString());
                ui.tableWidget->setItem(0, 1, newItem);

                newItem = new QTableWidgetItem(settings.value("pattern").toString());
                ui.tableWidget->setItem(0, 2, newItem);

            }
        }
        settings.endArray();
    ui.tableWidget->resizeColumnsToContents();
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);

    ui.userLineEdit->setText(settings.value("username", "JIRA  username")
                    .toString());

    if (settings.value("savePassword", false).toBool())
        ui.savePasswordCheckBox->setCheckState(Qt::Checked);
    else
        ui.savePasswordCheckBox->setCheckState(Qt::Unchecked);

    if (!jiraPassword.isEmpty())
        // password is only in memory, not saved in settings
        ui.passwordLineEdit->setText(jiraPassword);
    else
        ui.passwordLineEdit->setText(
            settings.value("password", "").toString());
    settings.endGroup();

    connect(ui.addServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::addServer);
    connect(ui.deleteServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::deleteServer);
    connect(ui.usePATCheckBox, SIGNAL(clicked()), this, SLOT(updateAuthenticationFields()));
    connect(this, &QDialog::accepted, this, &JiraSettingsDialog::updateSettings);

    updateAuthenticationFields();
}

void JiraSettingsDialog::addServer()
{
    ui.tableWidget->insertRow(0);
}

void JiraSettingsDialog::deleteServer()
{
    ui.tableWidget->removeRow(ui.tableWidget->currentRow());
}

void JiraSettingsDialog::updateAuthenticationFields()
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


void JiraSettingsDialog::updateSettings()
{
    settings.remove("jira");

    settings.beginGroup("/atlassian/jira");
    settings.beginWriteArray("servers");
    for (int i = 0; i < ui.tableWidget->rowCount(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", ui.tableWidget->item(i, 0)->text());
        settings.setValue("baseURL", ui.tableWidget->item(i, 1)->text());
        settings.setValue("pattern", ui.tableWidget->item(i, 2)->text());
    }
    settings.endArray();

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
        jiraPassword = ui.passwordLineEdit->text();
    }

    settings.endGroup();
}
