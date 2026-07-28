#include "selection-dialog.h"

#include <QColorDialog>
#include <QPainter>

#include "mainwindow.h"
#include "mapdesign.h"
#include "vymmodel.h"

extern Main *mainWindow;

SelectionDialog::SelectionDialog(VymModel *m)
{
    ui.setupUi(this);

    model = m;

    QDialog::setWindowTitle(
        "VYM - " + tr("Set selection box", "Dialog to set selection box"));

    updateControls();

    connect(ui.borderWidthSpinBox, SIGNAL(valueChanged(int)), this,
            SLOT(borderWidthChanged(int)));
    connect(ui.borderColorButton, SIGNAL(pressed()), this,
            SLOT(selectBorderColor()));
    connect(ui.backgroundColorButton, SIGNAL(pressed()), this,
            SLOT(selectBackgroundColor()));
}

void SelectionDialog::borderWidthChanged(int w)
{
    model->setSelectionPenWidth(w);
}

void SelectionDialog::selectBorderColor()
{
    QPen orgPen = model->mapDesign()->selectionPen();

    QColorDialog colorDialog(orgPen.color());
    colorDialog.setOption(QColorDialog::ShowAlphaChannel);
    colorDialog.setWindowTitle(
        tr("Border color of selection box", "Selection box dialog"));

    connect(&colorDialog, SIGNAL(currentColorChanged(QColor)), this,
            SLOT(borderColorChanged(QColor)));

    int result = colorDialog.exec();

    // Undo the preview in any case: Either the original settings are restored,
    // or the selected color is set below, which then also can be undone
    model->previewSelectionPen(orgPen);

    if (result != QDialog::Accepted)
        return;

    model->setSelectionPenColor(colorDialog.selectedColor());
    updateColorButton(ui.borderColorButton, model->getSelectionPenColor());
    mainWindow->updateActions();
}

void SelectionDialog::selectBackgroundColor()
{
    QBrush orgBrush = model->mapDesign()->selectionBrush();

    QColorDialog colorDialog(orgBrush.color());
    colorDialog.setOption(QColorDialog::ShowAlphaChannel);
    colorDialog.setWindowTitle(
        tr("Background color of selection box", "Selection box dialog"));

    connect(&colorDialog, SIGNAL(currentColorChanged(QColor)), this,
            SLOT(backgroundColorChanged(QColor)));

    int result = colorDialog.exec();

    model->previewSelectionBrush(orgBrush);

    if (result != QDialog::Accepted)
        return;

    model->setSelectionBrushColor(colorDialog.selectedColor());
    updateColorButton(ui.backgroundColorButton, model->getSelectionBrushColor());
    mainWindow->updateActions();
}

void SelectionDialog::borderColorChanged(QColor col)
{
    QPen pen = model->mapDesign()->selectionPen();
    pen.setColor(col);
    model->previewSelectionPen(pen);
}

void SelectionDialog::backgroundColorChanged(QColor col)
{
    QBrush brush = model->mapDesign()->selectionBrush();
    brush.setColor(col);
    model->previewSelectionBrush(brush);
}

void SelectionDialog::updateColorButton(QPushButton *button, const QColor &col)
{
    // Draw a checkerboard below the color, so that also the opacity
    // of the color is visible
    QPixmap pix(16, 16);
    pix.fill(Qt::white);

    QPainter painter(&pix);
    painter.fillRect(0, 0, 8, 8, Qt::lightGray);
    painter.fillRect(8, 8, 8, 8, Qt::lightGray);
    painter.fillRect(pix.rect(), col);
    painter.end();

    button->setIcon(pix);
}

void SelectionDialog::updateControls()
{
    ui.borderWidthSpinBox->setValue(model->getSelectionPenWidth());
    updateColorButton(ui.borderColorButton, model->getSelectionPenColor());
    updateColorButton(ui.backgroundColorButton, model->getSelectionBrushColor());
}
