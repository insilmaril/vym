#ifndef BACKGROUND_DIALOG_H
#define BACKGROUND_DIALOG_H

#include "ui_background-dialog.h"

class VymModel;

class BackgroundDialog : public QDialog {
    Q_OBJECT

  public:
    BackgroundDialog(VymModel *m);
    int exec();

  public slots:
    void selectBackgroundColor();
    void toggleBackgroundImage();
    void selectBackgroundImage();

  private:
    void updateBackgroundColorButton();
    void updateBackgroundImageControls();
    VymModel *model;
    Ui::BackgroundDialog ui;
};

#endif // BACKGROUND_DIALOG_H
