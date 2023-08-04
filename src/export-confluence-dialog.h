#ifndef EXPORTCONFLUENCEDIALOG_H
#define EXPORTCONFLUENCEDIALOG_H

#include "ui_export-confluence-dialog.h"

#include <QButtonGroup>
#include <QDir>

/*! \brief Dialog to export a map as HTML document

This is an overloaded QDialog with various settings needed to call
convert the vym.xml to a HTML document.
*/

class ExportConfluenceDialog : public QDialog {
    Q_OBJECT
  public:
    ExportConfluenceDialog(QWidget *parent = 0);

    void setCreateNewPage(bool b);
    bool getCreateNewPage();
    virtual QString getURL();
    virtual QString getPageName();
    void setPageNameHint(const QString &s);

  public slots:
    void readSettings();
    void setURL(const QString &);
    void setPageName(const QString &);
    void pageButtonPressed();
    void URLChanged();
    void pageNameChanged();
    void mapCenterToPageNameCheckBoxPressed(bool b);
    void saveSettings();
    void setFilePath(const QString &s);
    void setMapName(const QString &s);
    bool useTextColor();
    bool mapCenterToPageName();
    bool useNumbering();
    bool includeMapImage();
    void doExport();

  protected:
    QString url;
    QString pageName;
    QString pageNameHint;
    QString filepath;
    QString mapname;

  private:
    Ui::ExportConfluenceDialog ui;
    void init();
};

#endif // ExportConfluenceDialog_H
