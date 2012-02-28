#ifndef EXPORTHTMLDIALOG_H
#define EXPORTHTMLDIALOG_H

#include "ui_exporthtmldialog.h"

/*! \brief Dialog to export a map as HTML document

This is an overloaded QDialog with various settings needed to call
convert the vym.xml to a HTML document. 
*/

class ExportHTMLDialog:public QDialog
{
    Q_OBJECT
public:
    ExportHTMLDialog(QWidget* parent = 0);

    virtual QString getDir();
    virtual bool warnings();
    virtual bool hasChanged();

public slots:
    virtual void readSettings();
    virtual void setDirectory (const QString&);
    virtual void dirChanged();
    virtual void browseDirectoryPressed();
    virtual void imageButtonPressed( bool b );
    virtual void textcolorButtonPressed( bool b );
    virtual void saveSettingsInMapButtonPressed( bool b );
    virtual void warningsButtonPressed( bool b );
    virtual void outputButtonPressed( bool b );
    virtual void cssChanged();
    virtual QString getCSSPath();
    virtual void browseCSSPressed();
    virtual void postscriptChanged();
    virtual void browsePostExportButtonPressed();
    virtual void saveSettings ();
    virtual void setFilePath( const QString & s );
    virtual void setMapName( const QString & s );

public:
    bool useImage;
    bool useTextColor;
    QString postscript;

protected:
    bool showWarnings;
    QString css;
    bool showOutput;
    QString dir;
    QString filepath;
    bool settingsChanged;
    QString mapname;
    bool saveSettingsInMap;

private:
    Ui::ExportHTMLDialog ui;
    void init();

};

#endif // EXPORTHTMLDIALOG_H
