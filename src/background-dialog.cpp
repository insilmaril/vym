#include "background-dialog.h"

#include <QColorDialog>
#include <QFileDialog>

#include "mainwindow.h"
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
        model->setMapBackgroundImageName(ui.imageNameLineEdit->text());
    return r;
}

void BackgroundDialog::selectBackgroundColor()
{
    QColor col = QColorDialog::getColor(
        model->getMapEditor()->getScene()->backgroundBrush().color(), nullptr);
    if (!col.isValid())
        return;
    model->setMapBackgroundColor(col);

    // Update local and maybe also global color button
    updateBackgroundColorButton();
    mainWindow->updateActions();
}

void BackgroundDialog::toggleBackgroundImage()
{
    if (!ui.useBackgroundImageCheckbox->isChecked()) {
        model->unsetMapBackgroundImage();
        updateBackgroundImageControls();
    } else
        selectBackgroundImage();
}

void BackgroundDialog::selectBackgroundImage()
{
    QStringList filters;
    filters << tr("Images") +
                   " (*.png *.bmp *.xbm *.jpg *.png *.xpm *.gif *.pnm)";
    QFileDialog fd;
    fd.setFileMode(QFileDialog::ExistingFile);
    fd.setWindowTitle(vymName + " - " + tr("Load background image"));
    fd.setDirectory(lastImageDir);
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    if (fd.exec() == QDialog::Accepted && !fd.selectedFiles().isEmpty()) {
        lastImageDir = QDir(fd.directory().path());
        if (model->setMapBackgroundImage(fd.selectedFiles().first()))
            updateBackgroundImageControls();
    }
}

void BackgroundDialog::updateBackgroundColorButton()
{
    QPixmap pix(16, 16);
    pix.fill(model->getMapBackgroundColor());
    ui.backgroundColorButton->setIcon(pix);
}

void BackgroundDialog::updateBackgroundImageControls()
{
    if (model->hasMapBackgroundImage()) {
        ui.imageNameLineEdit->setText(model->mapBackgroundImageName());
        ui.useBackgroundImageCheckbox->setChecked(true);
    } else {
        ui.imageNameLineEdit->setText("");
        ui.useBackgroundImageCheckbox->setChecked(false);
    }
}
