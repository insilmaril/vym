#ifndef ACTIONLOG_DIALOG_H
#define ACTIONLOG_DIALOG_H

#include "ui_actionlog-dialog.h"

class VymModel;

class ActionLogDialog : public QDialog {
    Q_OBJECT

  public:
    ActionLogDialog();
    int  exec();

  public slots:
    void toggleUseLogFile();
    void pathChanged(const QString &);
    void selectLogPathDialog();

  private:
    void updateControls();
    Ui::ActionLogDialog ui;
};

#endif
