#ifndef JIRA_SETTINGS_DIALOG_H
#define JIRA_SETTINGS_DIALOG_H

#include "ui_jira-settings-dialog.h"

class JiraSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    JiraSettingsDialog(QWidget *parent = 0);
    void setUser(const QString &u);
    QString getUser();
    void setPassword(const QString &p);
    QString getPassword();
    void setSavePassword(const bool &b);
    bool savePassword();

  private:
    Ui::JiraSettingsDialog ui;
};

#endif // JIRA_SETTINGS_DIALOG_H
