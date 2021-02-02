#ifndef LINEEDITDIALOG_H
#define LINEEDITDIALOG_H

#include "ui_lineeditdialog.h"

class LineEditDialog : public QDialog {
    Q_OBJECT

  public:
    LineEditDialog(QWidget *parent = 0);

  public slots:
    virtual void showCancelButton(bool b);
    virtual void setCaption(const QString &s);
    virtual void setLabel(const QString &s);
    virtual void setText(const QString &s);
    virtual QString getText();

  private:
    void init();
    Ui::LineEditDialog ui;
};

#endif // LINEEDITDIALOG_H
