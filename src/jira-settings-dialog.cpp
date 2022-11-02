#include "jira-settings-dialog.h"

#include <QDebug>

#include "settings.h"

extern Settings settings;
extern QString jiraPassword;

JiraSettingsDialog::JiraSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("JiraSettingsDialog dialog", "dialog window title"));

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
    settings.endGroup();
    ui.tableWidget->resizeColumnsToContents();
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);

    ui.userLineEdit->setText(settings.value("/atlassian/jira/username", "JIRA  username")
                    .toString());

    if (    settings.value("/atlassian/jira/savePassword", false).toBool())
        ui.savePasswordCheckBox->setCheckState(Qt::Checked);
    else
        ui.savePasswordCheckBox->setCheckState(Qt::Unchecked);

    if (!jiraPassword.isEmpty())
        ui.passwordLineEdit->setText(jiraPassword);

    connect(ui.addServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::addServer);
    connect(ui.deleteServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::deleteServer);
    connect(this, &QDialog::accepted, this, &JiraSettingsDialog::updateSettings);

}

void JiraSettingsDialog::addServer()
{
    ui.tableWidget->insertRow(0);
}

void JiraSettingsDialog::deleteServer()
{
    ui.tableWidget->removeRow(ui.tableWidget->currentRow());
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

    settings.setValue("username", ui.userLineEdit->text());
    settings.setValue("savePassword", ui.savePasswordCheckBox->isChecked());

    // Save password in memory
    jiraPassword = ui.passwordLineEdit->text();

    // Save password only on request persistently in settings
    if (ui.savePasswordCheckBox->isChecked())
        settings.setValue("password", ui.passwordLineEdit->text());
    else
        settings.remove("password");

    settings.endGroup();
}
