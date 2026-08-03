#ifndef SHOWTEXTDIALOG_H
#define SHOWTEXTDIALOG_H

#include "ui_showtextdialog.h"

// #include <QLayout>
// #include <QTextBrowser>

class ShowTextDialog : public QDialog {
    Q_OBJECT
  public:
    ShowTextDialog(QWidget *parent = 0);
    void append(const QString &);
    void setHtml(const QString &);
    void setText(const QString &);
    void useFixedFont(bool);

  private slots:
    void filterText();
    void clearFilter();

  private:
    void showText();

    Ui::ShowTextDialog ui;
    QString originalText;  // Unfiltered text, as it was set by caller
    bool originalIsHtml;
};

#endif // SHOWTEXTDIALOG_H
