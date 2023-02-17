#ifndef DARKTHEME_SETTINGS_DIALOG_H
#define DARKTHEME_SETTINGS_DIALOG_H

#include "ui_darktheme-settings-dialog.h"

class DarkThemeSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    DarkThemeSettingsDialog(QWidget *parent = 0);
    Ui::DarkThemeSettingsDialog ui;
};

#endif // DARKTHEME_SETTINGS_DIALOG_H
