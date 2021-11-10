#ifndef CONFLUENCEUSERDIALOG_H
#define CONFLUENCEUSERDIALOG_H

#include "ui_confluence-userdialog.h"

#include <QStringList>

class ConfluenceUser;
class ConfluenceAgent;
class QKeyEvent;

class ConfluenceUserDialog : public QDialog {
    Q_OBJECT

  public:
    ConfluenceUserDialog(QWidget *parent = 0);
    void keyPressEvent(QKeyEvent *);
    int exec();
    ConfluenceUser getSelectedUser();

  public slots:
    void lineEditChanged();
    void itemSelected(QListWidgetItem *);
    void updateResultsList(QList <ConfluenceUser>);

  private:
    void init();
    Ui::ConfluenceUserDialog ui;

    QList <ConfluenceUser> userList;
    int currentRow;
};

#endif // USERDIALOG_H
