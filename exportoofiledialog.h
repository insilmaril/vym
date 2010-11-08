#ifndef EXPORTOOFILEDIALOG
#define EXPORTOOFILEDIALOG

#include <QFileDialog>
#include <QStringList>

#include "settings.h"

/*! \brief Dialog to select output file and format for Open Office documents

This is an overloaded QFileDialog, which allows to select templates by setting a type.
*/

class ExportOOFileDialog:public QFileDialog
{
    Q_OBJECT
public:
    ExportOOFileDialog();

    ExportOOFileDialog (QWidget * parent , const  QString &caption=QString());
    bool foundConfig();
    QString selectedConfig();
    QString selectedFile();
    void show();
     
private slots:
    void  newConfigPath (const QString&f);

private:
    void init();
    void addFilter(const QString &);
    void scanExportConfigs(QDir );
    QStringList configPaths;
    QStringList filters;
    QString lastFilter;
    
};
#endif
