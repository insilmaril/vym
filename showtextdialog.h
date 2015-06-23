#ifndef SHOWTEXTDIALOG_H
#define SHOWTEXTDIALOG_H

#include "ui_showtextdialog.h"

// #include <QLayout>
// #include <QTextBrowser>

class ShowTextDialog:public QDialog
{
    Q_OBJECT
public:
    ShowTextDialog (QWidget *parent=0);
    void append     (const QString &);
    void setHtml    (const QString &);
    void setText    (const QString &);
    void useFixedFont (bool);
    
private:
    Ui::ShowTextDialog ui;
};

#endif // SHOWTEXTDIALOG_H

