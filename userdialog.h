#ifndef USERDIALOG_H
#define USERDIALOG_H

#include "ui_userdialog.h"

#include <QStringList>

class UserDialog : public QDialog {
    Q_OBJECT

  public:
    UserDialog(QWidget *parent = 0);
    int exec();
    QString selectedUser();
    QString selectedUserKey();


  public slots:
    void lineEditChanged();
    void itemSelected(QListWidgetItem*);

  private:
    void init();
    Ui::UserDialog ui;

    QStringList userNameList;
    QStringList userLoginList;
    QStringList userKeyList;

    int currentRow;
};

#endif // USERDIALOG_H
