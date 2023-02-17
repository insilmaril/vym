#include "darktheme-settings-dialog.h"

#include <QDebug>

#include "settings.h"

extern Settings settings;

DarkThemeSettingsDialog::DarkThemeSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);
    QDialog::setWindowTitle("VYM - " +
                            tr("DarkThemeSettingsDialog dialog", "dialog window title"));

}
