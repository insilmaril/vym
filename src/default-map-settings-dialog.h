#ifndef DEFAULTMAPSETTINGS_H
#define DEFAULTMAPSETTINGS_H

#include "ui_default-map-settings-dialog.h"

class DefaultMapSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    DefaultMapSettingsDialog(QWidget *parent = 0);

  private:  
    void initInputs();

  public slots:
    void autoToggled();
    void setPathClicked();
    void updateSettings();

  private:
    Ui::DefaultMapSettingsDialog ui;
};

#endif // DEFAULTMAPSETTINGS
