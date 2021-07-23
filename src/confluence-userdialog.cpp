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
        qDebug() << "Connected:  " << b;
        qDebug() << "agent: " << agent;

        agent->getUsers(ui.lineEdit->text());
        /*
        QString results = ca.getResult();

        QStringList list = results.split("\n");
        QRegExp re(
            "name:\\s*\"(.*)\",\\s*login: \"(.*)\"\\s*,\\s*key:\\s*\"(.*)\"");
        re.setMinimal(true);

        ui.userList->clear();
        userNameList.clear();
        userLoginList.clear();
        userKeyList.clear();
        currentRow = -1;

        for (int i = 0; i < list.length(); i++) {
            if (re.indexIn(list.at(i)) >= 0 && !re.cap(1).isEmpty()) {
                userNameList.append(re.cap(1));
                userLoginList.append(re.cap(2));
                userKeyList.append(re.cap(3));
                new QListWidgetItem(userNameList.last() + " (" +
                                        userLoginList.last() + ")",
                                    ui.userList);
            }
        }
        */
    }
}

void ConfluenceUserDialog::itemSelected(QListWidgetItem *item)
{
    currentRow = ui.userList->row(item);
    accept();
}

void ConfluenceUserDialog::updateResultsList(QList <ConfluenceUser> results)
{
    qDebug() << "ConfluenceUserDialog: Results received";
    ui.userList->clear();
    userList.clear();
    currentRow = -1;

    foreach (ConfluenceUser u, results) {
        qDebug() << u.getTitle() << u.getDisplayName() << u.getUserName(); 
        userList << u;
        new QListWidgetItem(u.getDisplayName() + " (" + u.getUserName() + ")", ui.userList);
    }
}
