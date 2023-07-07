#include "default-map-settings-dialog.h"

#include <QFileDialog>

#include "mainwindow.h"

extern Settings settings;
extern Main *mainWindow;
extern QString vymName;

DefaultMapSettingsDialog::DefaultMapSettingsDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    initInputs();

    connect(ui.autoCheckBox, SIGNAL(clicked()), this, SLOT(autoToggled()));
    connect(ui.setPathPushButton, SIGNAL(clicked()), this, SLOT(setPathClicked()));
    connect(this, &QDialog::accepted, this, &DefaultMapSettingsDialog::updateSettings);
}

void DefaultMapSettingsDialog::initInputs()
{
    if (settings.value("/system/defaultMap/auto", true).toBool()) {
        ui.autoCheckBox->setCheckState(Qt::Checked);
        ui.pathLineEdit->setText(mainWindow->defaultMapPath());
        ui.pathLineEdit->setEnabled(false);
        ui.setPathPushButton->setEnabled(false);
    } else {
        ui.autoCheckBox->setCheckState(Qt::Unchecked);
        ui.pathLineEdit->setText(
            settings.value("/system/defaultMap/path", mainWindow->newMapPath()).toString());
        ui.pathLineEdit->setEnabled(true);
        ui.setPathPushButton->setEnabled(true);
    }
}

void DefaultMapSettingsDialog::autoToggled()
{
    if (ui.autoCheckBox->isChecked())
        settings.setValue("/system/defaultMap/auto", true);
    else
        settings.setValue("/system/defaultMap/auto", false);

    initInputs();
}

void DefaultMapSettingsDialog::setPathClicked()
{
    QStringList filters;
    filters << "VYM defaults map (*.vym)";
    QFileDialog fd;
    fd.setDirectory(dirname(mainWindow->defaultMapPath()));
    fd.selectFile(basename(mainWindow->defaultMapPath()));
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " +
                      tr("Set vym default map to be loaded on startup"));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    QString fn;
    if (fd.exec() == QDialog::Accepted) {
        settings.setValue("/system/defaultMap/path", fd.selectedFiles().first());
        initInputs();
    }
}

void DefaultMapSettingsDialog::updateSettings()
{
    settings.beginGroup("/system/defaultMap");
    settings.setValue("auto", ui.autoCheckBox->isChecked());
    settings.setValue("path", ui.pathLineEdit->text());
    settings.endGroup();
}
