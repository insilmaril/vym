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

    ui.tableWidget->setColumnCount(5);

        settings.beginGroup("/atlassian/jira");
        QTableWidgetItem *newItem;

        QStringList headers;
        headers << "Name";
        headers << "URL";
        headers << "Pattern";
        headers << "Method";
        headers << "User";
        ui.tableWidget->setHorizontalHeaderLabels(headers);

        int size = settings.beginReadArray("servers");
        for (int i = 0; i < size; ++i) {
            settings.setArrayIndex(i);
            ui.tableWidget->insertRow(0);
            foreach (QString p, settings.value("pattern").toString().split(",")) {

                newItem = new QTableWidgetItem(settings.value("name").toString());
                ui.tableWidget->setItem(0, 0, newItem);

                newItem = new QTableWidgetItem(settings.value("baseUrl").toString());
                ui.tableWidget->setItem(0, 1, newItem);

                newItem = new QTableWidgetItem(settings.value("pattern").toString());
                ui.tableWidget->setItem(0, 2, newItem);

                if (settings.value("authUsingPAT").toString() == "true")
                    newItem = new QTableWidgetItem("PAT");
                else
                    newItem = new QTableWidgetItem("Username/Password");
                ui.tableWidget->setItem(0, 3, newItem);

                newItem = new QTableWidgetItem(settings.value("username","-").toString());
                ui.tableWidget->setItem(0, 4, newItem);
            }
        }
        settings.endArray();
    ui.tableWidget->resizeColumnsToContents();
    ui.tableWidget->horizontalHeader()->setStretchLastSection(true);

    settings.endGroup();

    connect(ui.addServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::addServer);
    connect(ui.deleteServerButton, &QPushButton::clicked, this, &JiraSettingsDialog::deleteServer);

    connect(ui.tableWidget->selectionModel(),
            SIGNAL(selectionChanged(QItemSelection, QItemSelection)),
            this,
            SLOT(selectionChanged(QItemSelection, QItemSelection)));

    connect(ui.userLineEdit, SIGNAL(editingFinished()), 
            this, SLOT(fieldsChanged()));
    connect(ui.passwordLineEdit, SIGNAL(editingFinished()), 
            this, SLOT(fieldsChanged()));
    connect(ui.PATLineEdit, SIGNAL(editingFinished()), 
            this, SLOT(fieldsChanged()));
    connect(ui.usePATCheckBox, SIGNAL(clicked()), 
            this, SLOT(fieldsChanged()));
    connect(ui.tableWidget, SIGNAL(cellChanged(int, int)),
            this, SLOT(fieldsChanged()));

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
    QModelIndexList ixl = ui.tableWidget->selectionModel()->selectedIndexes();

    int row;
    if (ixl.isEmpty() || ixl.count() > 1)
        row = -1;
    else
        row = ixl.first().row();

    if (row < 0) {
        // No server selected, disable fields
        ui.selectedServerLineEdit->setText("");
        ui.usePATCheckBox->setEnabled(false);
        ui.PATLineEdit->setEnabled(false);
        ui.PATLabel->setEnabled(false);
        ui.userLabel->setEnabled(false);
        ui.userLineEdit->setEnabled(false);
        ui.passwordLabel->setEnabled(false);
        ui.passwordLineEdit->setEnabled(false);

        // Empty unused fields
        ui.userLineEdit->setText("");
        ui.passwordLineEdit->setText("");
        ui.PATLineEdit->setText("");

    } else {
        // Index of selected server in settings
        int n_server = ui.tableWidget->rowCount() - row;
        QString selectedServer = QString("/atlassian/jira/servers/%1/").arg(n_server);

        // Enable fields
        if (ui.tableWidget->item(row, 0))
            ui.selectedServerLineEdit->setText( ui.tableWidget->item(row, 0)->text());
        else
            ui.selectedServerLineEdit->setText("");
        ui.usePATCheckBox->setEnabled(true);
        ui.usePATCheckBox->setChecked(
            settings.value(selectedServer + "authUsingPAT", true).toBool());
        ui.PATLineEdit->setEnabled(true);
        ui.PATLabel->setEnabled(true);
        ui.userLabel->setEnabled(true);
        ui.userLineEdit->setEnabled(true);
        ui.passwordLabel->setEnabled(true);
        ui.passwordLineEdit->setEnabled(true);

        // Show and prefill fields depending on usage of PAT
        if (ui.usePATCheckBox->isChecked()) {
            ui.PATLineEdit->show();
            ui.PATLineEdit->setText(
                settings.value(selectedServer + "PAT","").toString());
                settings.value(selectedServer + "PAT","").toString();
            ui.PATLabel->show();
            ui.userLabel->hide();
            ui.userLineEdit->hide();
            ui.passwordLabel->hide();
            ui.passwordLineEdit->hide();
        } else {
            ui.PATLineEdit->hide();
            ui.PATLabel->hide();
            ui.userLabel->show();
            ui.userLineEdit->show();
            ui.userLineEdit->setText(
                settings.value(QString("/atlassian/jira/servers/%1/username").arg(n_server), "-").toString());
            ui.passwordLabel->show();
            ui.passwordLineEdit->show();
            ui.passwordLineEdit->setText(
                settings.value(QString("/atlassian/jira/servers/%1/password").arg(n_server), "").toString());
        }
    }

    // Update layout
    adjustSize();
}


void JiraSettingsDialog::fieldsChanged()
{
    int rowCount = ui.tableWidget->rowCount();

    if (rowCount < 1) return;

    QModelIndexList ixl = ui.tableWidget->selectionModel()->selectedIndexes();

    if (ixl.isEmpty() || ixl.count() > 1) return;

    int row = ixl.first().row();
    int n_server = rowCount - 1 - row;

    if (n_server < 0) return;

    settings.beginGroup("/atlassian/jira");
    settings.beginWriteArray("servers", rowCount);
    settings.setArrayIndex(n_server);

    if (ui.tableWidget->item(row, 0))
        settings.setValue("name", ui.tableWidget->item(row, 0)->text());
    else
        settings.setValue("name", "");
    if (ui.tableWidget->item(row, 1))
        settings.setValue("baseUrl", ui.tableWidget->item(row, 1)->text());
    else
        settings.setValue("baseUrl", "");
    if (ui.tableWidget->item(row, 2))
        settings.setValue("pattern", ui.tableWidget->item(row, 2)->text());
    else
        settings.setValue("pattern", "");
    settings.setValue("authUsingPAT", ui.usePATCheckBox->isChecked());
    if (ui.usePATCheckBox->isChecked()) {
        // Don't save password if PAT is used
        settings.remove("password");
        settings.setValue("PAT", ui.PATLineEdit->text());
    } else {
        settings.setValue("username", ui.userLineEdit->text());
        settings.setValue("password", ui.passwordLineEdit->text());
        settings.remove("PAT");
    }
    settings.setValue("servers/size", rowCount);

    settings.endArray();
    settings.endGroup();
}

void JiraSettingsDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &)
{
    updateAuthenticationFields();
}

