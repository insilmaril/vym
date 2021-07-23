#ifndef USERDIALOG_H
#define USERDIALOG_H

#include "ui_userdialog.h"

#include <QStringList>

class ConfluenceUser;
class ConfluenceAgent;

class UserDialog : public QDialog {
    Q_OBJECT

  public:
    UserDialog(QWidget *parent = 0);
    int exec();
    ConfluenceUser getSelectedUser();

  public slots:
    void lineEditChanged();
    void itemSelected(QListWidgetItem *);
    void updateResultsList(QList <ConfluenceUser>);

  private:
    void init();
    Ui::UserDialog ui;

    QList <ConfluenceUser> userList;
    int currentRow;
};

#endif // USERDIALOG_H
