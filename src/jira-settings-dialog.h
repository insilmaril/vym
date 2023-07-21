#ifndef JIRA_SETTINGS_DIALOG_H
#define JIRA_SETTINGS_DIALOG_H

#include "ui_jira-settings-dialog.h"

class JiraSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    JiraSettingsDialog(QWidget *parent = 0);

  public slots:
    void addServer();
    void deleteServer();
    void updateAuthenticationFields();
    void fieldsChanged();
    void selectionChanged(const QItemSelection &selected, const QItemSelection &);

  private:
    Ui::JiraSettingsDialog ui;
};

#endif // JIRA_SETTINGS_DIALOG_H
