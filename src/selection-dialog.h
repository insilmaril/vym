#ifndef SELECTION_DIALOG_H
#define SELECTION_DIALOG_H

#include "ui_selection-dialog.h"

class VymModel;

/*! \brief Dialog to set the parameters of the selection box

The selection box is drawn around the currently selected branches. Its
border width, border color and background color can be set here. Both
colors may be semi transparent.
*/

class SelectionDialog : public QDialog {
    Q_OBJECT

  public:
    SelectionDialog(VymModel *m);

  private slots:
    void borderWidthChanged(int);
    void selectBorderColor();
    void selectBackgroundColor();

    // Live preview while a color is selected in the subdialogs
    void borderColorChanged(QColor);
    void backgroundColorChanged(QColor);

  private:
    void updateColorButton(QPushButton *, const QColor &);
    void updateControls();

    VymModel *model;
    Ui::SelectionDialog ui;
};

#endif // SELECTION_DIALOG_H
