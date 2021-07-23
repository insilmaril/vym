#include "confluence-userdialog.h"

#include <QRegExp>

#include "confluence-agent.h"

ConfluenceUserDialog::ConfluenceUserDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("Find Confluence user", "dialog window title"));

    connect(ui.lineEdit, SIGNAL(textChanged(const QString &)), this,
            SLOT(lineEditChanged()));

    connect(ui.buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(ui.buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    connect(ui.userList, SIGNAL(itemPressed(QListWidgetItem *)), this,
            SLOT(itemSelected(QListWidgetItem *)));

    currentRow = -1;
}

int ConfluenceUserDialog::exec()
{
    int result = QDialog::exec();

    return result;
}

ConfluenceUser ConfluenceUserDialog::getSelectedUser()
{
    if (userList.length() > 0 && currentRow < userList.length() &&
        currentRow > -1)
        return userList.at(currentRow);
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
    currentRow = ui.userList->row(item);
    accept();
}

void ConfluenceUserDialog::updateResultsList(QList <ConfluenceUser> results)
{
    ui.userList->clear();
    userList.clear();
    currentRow = -1;

    foreach (ConfluenceUser u, results) {
        // qDebug() << u.getTitle() << u.getDisplayName() << u.getUserName(); 
        userList << u;
        new QListWidgetItem(u.getDisplayName() + " (" + u.getUserName() + ")", ui.userList);
    }
}
