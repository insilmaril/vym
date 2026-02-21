#include "jira-settings-dialog.h"

#include <QDebug>
#include <QSignalBlocker>

#include "settings.h"

extern Settings settings;
extern QString jiraPassword;

JiraSettingsDialog::JiraSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("Jira settings", "Jira settings dialog title"));

    ui.tableWidget->setColumnCount(4);

        settings.beginGroup("/atlassian/jira");
        QTableWidgetItem *newItem;

        QStringList headers;
        headers << "Name";
        headers << "URL";
        headers << "Pattern";
        headers << "Method";
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

                QString method = settings.value("method", "userpass").toString();
                if (method == "userpass")
                    newItem = new QTableWidgetItem("Username/Password");
                else if (method == "pat")
                    newItem = new QTableWidgetItem("PAT");
                else
                    newItem = new QTableWidgetItem("Cloud");
                ui.tableWidget->setItem(0, 3, newItem);
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
    connect(ui.apiTokenLineEdit, SIGNAL(editingFinished()), 
            this, SLOT(fieldsChanged()));
    connect(ui.authMethodComboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(updateAuthenticationFields()));
    connect(ui.authMethodComboBox, SIGNAL(currentIndexChanged(int)),
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
    int rowCount = ui.tableWidget->rowCount();
    int row = -1;
    if (ui.tableWidget->selectionModel()) {
        QModelIndexList sel = ui.tableWidget->selectionModel()->selectedIndexes();
        if (!sel.isEmpty()) row = sel.first().row();
    }

    if (row < 0) {
        ui.selectedServerLineEdit->setText("");
        ui.authMethodComboBox->setEnabled(false);
        ui.authMethodComboBox->hide();
        ui.authMethodLabel->hide();
        ui.PATLineEdit->setEnabled(false);
        ui.PATLineEdit->hide();
        ui.PATLabel->hide();
        ui.apiTokenLineEdit->setEnabled(false);
        ui.apiTokenLineEdit->hide();
        ui.apiTokenLabel->hide();
        ui.userLineEdit->setEnabled(false);
        ui.userLineEdit->hide();
        ui.userLabel->hide();
        ui.passwordLineEdit->setEnabled(false);
        ui.passwordLineEdit->hide();
        ui.passwordLabel->hide();

        ui.userLineEdit->setText("");
        ui.passwordLineEdit->setText("");
        ui.PATLineEdit->setText("");
        ui.apiTokenLineEdit->setText("");

    } else {
        // Index of selected server in settings
        int n_server = ui.tableWidget->rowCount() - row;
        QString selectedServer = QString("/atlassian/jira/servers/%1/").arg(n_server);

        // Enable fields
        if (ui.tableWidget->item(row, 0))
            ui.selectedServerLineEdit->setText( ui.tableWidget->item(row, 0)->text());
        else
            ui.selectedServerLineEdit->setText("");
        // Block signal emissions while updating widgets to avoid re-entrancy
        QSignalBlocker blockCombo(ui.authMethodComboBox);
        QSignalBlocker blockUser(ui.userLineEdit);
        QSignalBlocker blockPass(ui.passwordLineEdit);
        QSignalBlocker blockPat(ui.PATLineEdit);

        ui.authMethodComboBox->setEnabled(true);
        ui.authMethodComboBox->show();
        ui.authMethodLabel->show();
        ui.PATLineEdit->setEnabled(true);
        ui.PATLabel->setEnabled(true);
        ui.apiTokenLineEdit->setEnabled(true);
        ui.apiTokenLabel->setEnabled(true);
        ui.userLabel->setEnabled(true);
        ui.userLineEdit->setEnabled(true);
        ui.passwordLabel->setEnabled(true);
        ui.passwordLineEdit->setEnabled(true);

        int methodIdx = ui.authMethodComboBox->currentIndex();
        if (methodIdx == 2) { // cloud
            ui.apiTokenLineEdit->show();
            ui.apiTokenLineEdit->setText(
                settings.value(selectedServer + "apiToken","").toString());
            ui.apiTokenLabel->show();
            ui.PATLineEdit->hide();
            ui.PATLabel->hide();
            ui.userLabel->show();
            ui.userLabel->setText(tr("Email:"));
            ui.userLineEdit->show();
            ui.userLineEdit->setText(
                settings.value(QString("/atlassian/jira/servers/%1/email").arg(n_server), "").toString());
            ui.passwordLabel->hide();
            ui.passwordLineEdit->hide();
        } else if (methodIdx == 1) { // PAT
            ui.PATLineEdit->show();
            ui.PATLineEdit->setText(
                settings.value(selectedServer + "PAT","").toString());
            ui.PATLabel->show();
            ui.apiTokenLineEdit->hide();
            ui.apiTokenLabel->hide();
            ui.userLabel->hide();
            ui.userLineEdit->hide();
            ui.passwordLabel->hide();
            ui.passwordLineEdit->hide();
        } else { // user/pass
            ui.PATLineEdit->hide();
            ui.PATLabel->hide();
            ui.apiTokenLineEdit->hide();
            ui.apiTokenLabel->hide();
            ui.userLabel->show();
            ui.userLabel->setText(tr("Username:"));
            ui.userLineEdit->show();
            ui.userLineEdit->setText(
                settings.value(QString("/atlassian/jira/servers/%1/username").arg(n_server), "").toString());
            ui.passwordLabel->show();
            ui.passwordLineEdit->show();
            ui.passwordLineEdit->setText(
                settings.value(QString("/atlassian/jira/servers/%1/password").arg(n_server), "").toString());
        }
    }
}


void JiraSettingsDialog::fieldsChanged()
{
    int rowCount = ui.tableWidget->rowCount();
    if (rowCount < 1) return;
    int row = ui.tableWidget->currentRow();
    if (row < 0 || row >= rowCount) return;
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

    int methodIdx = ui.authMethodComboBox->currentIndex();
    QString method = (methodIdx == 0) ? "userpass" : (methodIdx == 1) ? "pat" : "cloud";
    settings.setValue("method", method);
    if (method == "userpass") {
        settings.setValue("username", ui.userLineEdit->text());
        settings.setValue("password", ui.passwordLineEdit->text());
    } else if (method == "pat") {
        settings.setValue("PAT", ui.PATLineEdit->text());
    } else { // cloud
        settings.setValue("email", ui.userLineEdit->text());
        settings.setValue("apiToken", ui.apiTokenLineEdit->text());
    }
    settings.setValue("servers/size", rowCount);

    settings.endArray();
    settings.endGroup();

    // Reflect method in table overview without re-triggering cellChanged
    {
        QSignalBlocker blockTable(ui.tableWidget);
        QString methodText = (method == "userpass") ? "Username/Password" : (method == "pat") ? "PAT" : "Cloud";
        QTableWidgetItem *methodItem = new QTableWidgetItem(methodText);
        ui.tableWidget->setItem(row, 3, methodItem);
    }
}

void JiraSettingsDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &)
{
    Q_UNUSED(selected);

    // If nothing selected, just refresh to the hidden state
    if (!ui.tableWidget->selectionModel() ||
        ui.tableWidget->selectionModel()->selectedIndexes().isEmpty()) {
        updateAuthenticationFields();
        return;
    }

    int row = ui.tableWidget->selectionModel()->selectedIndexes().first().row();
    int rowCount = ui.tableWidget->rowCount();
    if (row < 0 || row >= rowCount) {
        updateAuthenticationFields();
        return;
    }

    int n_server = rowCount - row; // absolute numbering used in settings path
    QString selectedServer = QString("/atlassian/jira/servers/%1/").arg(n_server);

    // Load and set method for selected server without triggering save yet
    QString method = settings.value(selectedServer + "method", "userpass").toString();
    int methodIndex = 0; // 0=userpass,1=pat,2=cloud
    if (method == "pat") methodIndex = 1;
    else if (method == "cloud") methodIndex = 2;
    {
        QSignalBlocker blockCombo(ui.authMethodComboBox);
        ui.authMethodComboBox->setCurrentIndex(methodIndex);
    }

    updateAuthenticationFields();
}
