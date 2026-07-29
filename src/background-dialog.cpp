#include "background-dialog.h"

#include <QColorDialog>
#include <QFileDialog>
#include <QStyle>

#include "mainwindow.h"
#include "mapeditor.h"
#include "vymmodel.h"

extern QDir lastImageDir;
extern Main *mainWindow;
extern QString vymName;

BackgroundDialog::BackgroundDialog(VymModel *m)
{
    ui.setupUi(this);

    model = m;

    QDialog::setWindowTitle(
        "VYM - " + tr("Set background", "Dialog to set background color or image"));

    updateBackgroundColorButton();
    updateBackgroundImageControls();

    //ui.selectImageButton->setIcon(QIcon::fromTheme("document-new"));
    ui.selectImageButton->setIcon(style()->standardIcon(QStyle::SP_DialogOpenButton));
    connect(ui.selectImageButton, SIGNAL(pressed()), this, SLOT(selectBackgroundImage()));
    connect(ui.backgroundColorButton, SIGNAL(pressed()), this, SLOT(selectBackgroundColor()));

    connect(ui.useBackgroundImageCheckbox, SIGNAL(clicked()), this, SLOT(toggleBackgroundImage()));
}

int BackgroundDialog::exec()
{
    int r = QDialog::exec();
    if (ui.useBackgroundImageCheckbox->isChecked())
        model->setBackgroundImageName(ui.imageNameLineEdit->text());
    return r;
}

void BackgroundDialog::selectBackgroundColor()
{
    QColor orgCol = model->mapDesign()->backgroundColor();

    QColorDialog colorDialog(orgCol);
    colorDialog.setOption(QColorDialog::ShowAlphaChannel);
    colorDialog.setWindowTitle( tr("Map backgroundcolor","Map background dialog"));

    //colorDialog.disconnect();

    connect(&colorDialog, SIGNAL(currentColorChanged(QColor)), this, SLOT(colorChanged(QColor)));

    int result = colorDialog.exec();

    // Undo the preview in any case: Either the original settings are restored,
    // or the selected color is set below, which then also can be undone
    model->previewBackgroundColor(orgCol);

    if (result != QDialog::Accepted)
        return;

    model->setBackgroundColor(colorDialog.selectedColor());

    // Update local and maybe also global color button
    updateBackgroundColorButton();
    updateBackgroundImageControls();
    mainWindow->updateActions();
}

void BackgroundDialog::toggleBackgroundImage()
{
    if (!ui.useBackgroundImageCheckbox->isChecked()) {
        model->unsetBackgroundImage();
        updateBackgroundImageControls();
    } else
        selectBackgroundImage();
}

void BackgroundDialog::selectBackgroundImage()
{
    QStringList images = openImageDialog( tr("Load background image"));
    if (!images.isEmpty() && model->loadBackgroundImage(images.first()))
        updateBackgroundImageControls();
}

void BackgroundDialog::colorChanged(QColor col)
{
    // Live preview only, the accepted color is saved for undo in
    // selectBackgroundColor()
    model->previewBackgroundColor(col);
}

void BackgroundDialog::updateBackgroundColorButton()
{
    QPixmap pix(16, 16);
    pix.fill(model->mapDesign()->backgroundColor());
    ui.backgroundColorButton->setIcon(pix);
}

void BackgroundDialog::updateBackgroundImageControls()
{
    if (model->hasBackgroundImage()) {
        ui.imageNameLineEdit->setText(model->backgroundImageName());
        ui.useBackgroundImageCheckbox->setChecked(true);
    } else {
        ui.imageNameLineEdit->setText("");
        ui.useBackgroundImageCheckbox->setChecked(false);
    }
}
