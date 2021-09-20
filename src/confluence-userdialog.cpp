#include "confluence-userdialog.h"

#include <QKeyEvent>
#include <QRegExp>

#include "confluence-agent.h"
#include "confluence-user.h"

ConfluenceUserDialog::ConfluenceUserDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("Find Confluence user", "dialog window title"));

    connect(ui.lineEdit, SIGNAL(textChanged(const QString &)), this,
            SLOT(lineEditChanged()));

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui.userListWidget, SIGNAL(itemPressed(QListWidgetItem *)), this,
            SLOT(itemSelected(QListWidgetItem *)));

    currentRow = -1;
}

void ConfluenceUserDialog::keyPressEvent(QKeyEvent *e)
{
    if (ui.lineEdit->hasFocus() && e->key() == Qt::Key_Down) 
    {
        ui.userListWidget->setCurrentRow(0, QItemSelectionModel::Select);
        ui.userListWidget->setFocus();
    }
    QDialog::keyPressEvent(e);
}

int ConfluenceUserDialog::exec()
{
    int result = QDialog::exec();

    return result;
}

ConfluenceUser ConfluenceUserDialog::getSelectedUser()
{
    if (ui.userListWidget->count() > 0 && ui.userListWidget->currentRow() < ui.userListWidget->count() &&
        ui.userListWidget->currentRow() > -1)
        return userList.at(ui.userListWidget->currentRow());
    else
        return ConfluenceUser();
}

void ConfluenceUserDialog::lineEditChanged()
{
    if (ui.lineEdit->text().length() > 3) {
        ConfluenceAgent *agent = new ConfluenceAgent;
        bool b = connect(agent, &ConfluenceAgent::foundUsers, this, &ConfluenceUserDialog::updateResultsList);

        agent->getUsers(ui.lineEdit->text());
    }
}

void ConfluenceUserDialog::itemSelected(QListWidgetItem *item)
{
    currentRow = ui.userListWidget->row(item);
    accept();
}

void ConfluenceUserDialog::updateResultsList(QList <ConfluenceUser> results)
{
    ui.userListWidget->clear();
    userList.clear();
    currentRow = -1;

    foreach (ConfluenceUser u, results) {
        //qDebug() << u.getTitle() << u.getDisplayName() << u.getUserName(); 
        userList << u;
        new QListWidgetItem(u.getDisplayName() + " (" + u.getUserName() + ")", ui.userListWidget);
    }
}
