#include "exportoofiledialog.h"

ExportOOFileDialog::ExportOOFileDialog():QFileDialog()
{
    init();
}

ExportOOFileDialog::ExportOOFileDialog (QWidget * parent, const QString &caption ):QFileDialog(parent, caption)
{
    init();
}

bool ExportOOFileDialog::foundConfig()
{
    return !filters.isEmpty();
}


QString ExportOOFileDialog::selectedConfig()
{
    QStringList::Iterator itpath=configPaths.begin();
    QStringList::Iterator itf=filters.begin();
    while (itf != filters.end()) 
    {
	if (*itf==selectedNameFilter()) return *itpath;
	itpath++;   
	itf++;
    }
    qWarning ("ExportOOFileDialog::selectedConfig  No filter found!");
    return "";
}

void ExportOOFileDialog::newConfigPath(const QString &s)
{
    lastFilter=s;
}

void ExportOOFileDialog::show()
{
    setFilters (filters);
    QFileDialog::show();
}

void ExportOOFileDialog::init()
{
    setFileMode( QFileDialog::AnyFile );
    QDir d;
    d.setPath ("/usr/share/vym/exports");
    scanExportConfigs(d);
    d.setPath (d.homePath()+"/.vym/exports");
    scanExportConfigs(d);
    d.setPath (d.currentPath()+"/exports");
    scanExportConfigs(d);

    setNameFilters (filters);
    connect (
	this,SIGNAL (filterSelected(const QString&)),
	this, SLOT( newConfigPath(const QString &)));
}

void ExportOOFileDialog::addFilter(const QString &f)
{
    lastFilter=f;
    filters.append (f);
}

void ExportOOFileDialog::scanExportConfigs(QDir dir)
{
    // Scan existing export configurations
    SimpleSettings set;
    QFile f;
    if (dir.exists())
    {
	// Traverse files
        dir.setFilter(QDir::Files | QDir::Hidden | QDir::NoSymLinks);

        QFileInfoList list = dir.entryInfoList();
        for (int i = 0; i < list.size(); ++i) {
            QFileInfo fi = list.at(i);

	    if (fi.fileName().endsWith(".conf") )
	    {
		configPaths.append (fi.absoluteFilePath());
		set.clear();
		set.readSettings (fi.absoluteFilePath());
		addFilter (set.value (QString("Name")) + " (*.odp)");
	    }	    
        }
    }	    
}
