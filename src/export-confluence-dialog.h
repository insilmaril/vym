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
    virtual bool warnings();
    virtual bool hasChanged();

  public slots:
    virtual void readSettings();
    virtual void setURL(const QString &);
    virtual void setPageName(const QString &);
    virtual void pageButtonPressed();
    virtual void URLChanged();
    virtual void pageNameChanged();
    virtual void mapCenterToPageNameCheckBoxPressed(bool b);
    virtual void includeImagesCheckBoxPressed(bool b);
    virtual void TOCCheckBoxPressed(bool b);
    virtual void numberingCheckBoxPressed(bool b);
    virtual void taskFlagsCheckBoxPressed(bool b);
    virtual void userFlagsCheckBoxPressed(bool b);
    virtual void textcolorCheckBoxPressed(bool b);
    virtual void saveSettingsInMapCheckBoxPressed(bool b);
    virtual void warningsCheckBoxPressed(bool b);
    virtual void outputCheckBoxPressed(bool b);
    virtual void saveSettings();
    virtual void setFilePath(const QString &s);
    virtual void setMapName(const QString &s);

  public:
    bool includeMapImage;
    bool includeImages;
    bool useTOC;
    bool useNumbering;
    bool useTaskFlags;
    bool useUserFlags;
    bool useTextColor;
    bool mapCenterToPageName;

  protected:
    QButtonGroup *buttonGroup;
    bool showWarnings;
    bool showOutput;
    QString url;
    QString pageName;
    QString pageNameHint;
    QString filepath;
    bool settingsChanged;
    QString mapname;
    bool saveSettingsInMap;

  private:
    Ui::ExportConfluenceDialog ui;
    void init();
};

#endif // ExportConfluenceDialog_H
