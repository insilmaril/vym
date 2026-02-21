#ifndef URLDIALOG_H
#define URLDIALOG_H

#include "ui_url-dialog.h"

/* \brief Dialog to display and edit Urls */

class UrlDialog : public QDialog {
    Q_OBJECT

  public:
    UrlDialog(QWidget *parent = 0);

    virtual QString url();
    virtual QString text();
    bool isReadOnly();

  public slots:
    virtual void setUrl(const QString &s);
    virtual void setText(const QString &s);
    void textEdited();
    void updateText(const QString &s);

  private:
    Ui::UrlDialog ui;
    bool updateTextWithUrl;
};

#endif
