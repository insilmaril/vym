#ifndef CONFLUENCE_SETTINGS_DIALOG_H
#define CONFLUENCE_SETTINGS_DIALOG_H

#include "ui_confluence-settings-dialog.h"

class ConfluenceSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    ConfluenceSettingsDialog(QWidget *parent = 0);

  public slots:
    void updateAuthenticationFields();
    void updateSettings();

  private:
    Ui::ConfluenceSettingsDialog ui;
};

#endif // CONFLUENCE_SETTINGS_DIALOG_H
