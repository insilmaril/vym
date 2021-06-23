#ifndef WARNINGDIALOG_H
#define WARNINGDIALOG_H

#include "ui_warningdialog.h"

class WarningDialog : public QDialog {
    Q_OBJECT

  public:
    WarningDialog(QWidget *parent = 0);
    int exec();

  public slots:
    virtual void showCancelButton(bool b);
    virtual void setShowAgainName(const QString &s);
    virtual void setText(const QString &s);
    virtual void setCaption(const QString &s);

  private:
    void init();
    bool useShowAgain;
    QString showAgainName;
    Ui::WarningDialog ui;
};

#endif // WARNINGDIALOG_H
