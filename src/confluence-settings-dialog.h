#ifndef CREDENTIALS_H
#define CREDENTIALS_H

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

#endif // CREDENTIALS_H
