#ifndef SHOWTEXTDIALOG_H
#define SHOWTEXTDIALOG_H

#include "ui_showtextdialog.h"

class ShowTextDialog:public QDialog
{
    Q_OBJECT
public:
    ShowTextDialog (QWidget *parent=0);
    void append     (const QString &);
    void setText    (const QString &);
    
private:
    Ui::ShowTextDialog ui;
};

#endif // SHOWTEXTDIALOG_H

