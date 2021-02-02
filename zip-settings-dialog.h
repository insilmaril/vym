#ifndef ZIPSETTINGSDIALOG_H
#define ZIPSETTINGSDIALOG_H

#include "ui_zip-settings-dialog.h"

class ZipSettingsDialog : public QDialog {
    Q_OBJECT

  public:
    ZipSettingsDialog(QWidget *parent = 0);

  public slots:
    void zipToolPathChanged();
    void unzipToolPathChanged();
    void zipToolButtonPressed();
    void unzipToolButtonPressed();

  private:
    void init();
    Ui::ZipSettingsDialog ui;

    void updateCheckResults();
};

#endif
