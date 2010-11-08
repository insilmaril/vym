#ifndef HISTORYWINDOW_H
#define HISTORYWINDOW_H

#include <QDialog>

#include "settings.h"
#include "ui_historywindow.h"


/////////////////////////////////////////////////////////////////////////////
class HistoryWindow:public QDialog
{
    Q_OBJECT

public:
    HistoryWindow(QWidget* parent = 0);
    ~HistoryWindow();
    void update (SimpleSettings &);
    void setStepsTotal (int);

protected:
    void closeEvent( QCloseEvent* );

private slots:	
    void undo();
    void redo();
    void select();

signals:
    void windowClosed();

private:
    void clearRow (int);
    void updateRow (int, int, SimpleSettings &);
    Ui::HistoryWindow ui;
};


#endif
