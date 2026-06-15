#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QDialog>

#include "settings.h"
#include "ui_historywindow.h"

/////////////////////////////////////////////////////////////////////////////
class HistoryWindow : public QDialog {
    Q_OBJECT

  public:
    HistoryWindow(QWidget *parent = 0);
    ~HistoryWindow();
    void setFocus();
    void update(uint modelId, SimpleSettings &);
    void setStepsTotal(int);

  protected:
    void closeEvent(QCloseEvent *);

  private slots:
    void closeWindow();
    void undo();
    void redo();
    void select();

  signals:
    void windowClosed();

  private:
    Ui::HistoryWindow ui;

    uint modelIdInt;
    void clearRow(int);
    void updateRow(int, int, SimpleSettings &);
};

#endif
