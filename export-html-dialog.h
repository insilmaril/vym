#ifndef EXPORTHTMLDIALOG_H
#define EXPORTHTMLDIALOG_H

#include "ui_exporthtmldialog.h"

#include <QDir>

/*! \brief Dialog to export a map as HTML document

This is an overloaded QDialog with various settings needed to call
convert the vym.xml to a HTML document. 
*/

class ExportHTMLDialog:public QDialog
{
    Q_OBJECT
public:
    ExportHTMLDialog(QWidget* parent = 0);

    virtual QDir getDir();
    virtual bool warnings();
    virtual bool hasChanged();

public slots:
    virtual void readSettings();
    virtual void setDirectory (const QString&);
    virtual void dirChanged();
    virtual void browseDirectoryPressed();
    virtual void imageCheckBoxPressed( bool b );
    virtual void includeImagesCheckBoxPressed( bool b );
    virtual void TOCCheckBoxPressed( bool b );
    virtual void numberingCheckBoxPressed( bool b );
    virtual void taskFlagsCheckBoxPressed( bool b );
    virtual void userFlagsCheckBoxPressed( bool b );
    virtual void textcolorCheckBoxPressed( bool b );
    virtual void saveSettingsInMapCheckBoxPressed( bool b );
    virtual void warningsCheckBoxPressed( bool b );
    virtual void outputCheckBoxPressed( bool b );
    virtual void cssSrcChanged();
    virtual void cssDstChanged();
    virtual QString getCssSrc();
    virtual QString getCssDst();
    virtual void copyCssPressed();
    virtual void browseCssSrcPressed();
    virtual void browseCssDstPressed();
    virtual void postscriptChanged();
    virtual void browsePostExportButtonPressed();
    virtual void saveSettings ();
    virtual void setFilePath( const QString & s );
    virtual void setMapName( const QString & s );

public:
    bool includeMapImage;
    bool includeImages;
    bool useTOC;
    bool useNumbering;
    bool useTaskFlags;
    bool useUserFlags;
    bool useTextColor;
    QString postscript;
    bool css_copy;
protected:
    QString css_src;
    QString css_dst;
    bool showWarnings;
    bool showOutput;
    QDir dir;
    QString filepath;
    bool settingsChanged;
    QString mapname;
    bool saveSettingsInMap;

private:
    Ui::ExportHTMLDialog ui;
    void init();

};

#endif // EXPORTHTMLDIALOG_H
