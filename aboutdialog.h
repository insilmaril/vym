#ifndef ABOUTDIALOG_H
#define ABOUTDIALOG_H

#include <QTabWidget>
#include <QTextBrowser>
#include <QLayout>
#include <QDialog>
#include <QPushButton>
#include <QString>
#include <QMessageBox>

class AboutTextBrowser;

/*! \mainpage View Your Mind - Development documentation
 
  \section intro_sec Introduction
 
VYM  (View Your Mind) is a tool to generate and manipulate maps which
show your thoughts. Such maps can help you to improve your creativity
and effectivity. You can use them for time management, to organize
tasks, to get an overview over complex contexts, to sort your ideas
etc. Some people even think it is fun to work with such maps...

  
  \section install_sec Installation

  Please look at the INSTALL.TXT file 
 
  \section doc_sec Documentation

  The complete documentation is available as a PDF file. Please look
  into the doc subdirectory.

*/



/*! \brief Dialog showing authors, version and license
*/


class AboutDialog :public QDialog
{
    Q_OBJECT

public:
    AboutDialog(QWidget* parent = 0);

private:
    QTabWidget *tabs;
    AboutTextBrowser *credits;
    AboutTextBrowser *license;

    QLayout *mainLayout;
    QLayout *topLayout;
    QLayout *bottomLayout;
    QPushButton *okbutton;
    
};

/*! \brief Overloaded QTextBrowser which is used in AboutDialog
*/
class AboutTextBrowser : public QTextBrowser
{
  Q_OBJECT;
 public:
  AboutTextBrowser(QWidget *parent);
 public slots:
    virtual void setSource(const QUrl &url);
};

#endif
