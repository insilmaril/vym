#include "export-base.h"

#include <cstdlib>

#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "branchitem.h"
#include "file.h"
#include "linkablemapobj.h"
#include "misc.h"
#include "mainwindow.h"
#include "warningdialog.h"
#include "xsltproc.h"
#include "vymprocess.h"


extern Main *mainWindow;
//extern QDir vymBaseDir;
//extern QString flagsPath;
//extern QString vymName;
//extern QString vymVersion;
//extern QString vymHome;
extern Settings settings;
extern QDir lastExportDir;

ExportBase::ExportBase()
{
    init();
}

ExportBase::ExportBase(VymModel *m)
{
    model = m;
    init();
}

ExportBase::~ExportBase()
{
    // Cleanup tmpdir: No longer required, part of general tmp dir of vym instance now

    // Remember current directory
    lastExportDir = QDir(dirPath);
}

void ExportBase::init()
{
    indentPerDepth = "  ";
    exportName     = "unnamed";
    lastCommand    = "";
    cancelFlag = false;
    defaultDirPath = lastExportDir.absolutePath();
    dirPath = defaultDirPath;
}

void ExportBase::setupTmpDir()
{
    bool ok;
    tmpDir.setPath (
        makeTmpDir(ok, 
        model->tmpDirPath(), 
        QString("export-%2").arg(exportName)));
    if (!tmpDir.exists() || !ok)
        QMessageBox::critical( 
            0, 
            QObject::tr( "Error" ),
            QObject::tr("Couldn't access temporary directory\n"));
}

void ExportBase::setDirPath (const QString &s)
{
    if (!s.isEmpty())
        dirPath = s;
    // Otherwise lastExportDir is used, which defaults to current dir
}

QString ExportBase::getDirPath()
{
    return dirPath;
}

void ExportBase::setFilePath (const QString &s)
{
    if(!s.isEmpty())
    {
        filePath = s;
        if (!filePath.startsWith("/"))
            // Absolute path
            filePath = lastExportDir.absolutePath() + "/" + filePath;
    }
}

QString ExportBase::getFilePath ()
{
    if (!filePath.isEmpty())
        return filePath;
    else
        return dirPath + "/" + model->getMapName() + extension;
}

QString ExportBase::getMapName ()
{
    QString fn = basename(filePath);
    return fn.left(fn.lastIndexOf("."));
}

void ExportBase::setModel(VymModel *m)
{
    model = m;
}

void ExportBase::setWindowTitle (const QString &s)
{
    caption = s;
}

void ExportBase::setName (const QString &s)
{
    exportName = s;
}

QString ExportBase::getName ()
{
    return exportName;
}

void ExportBase::addFilter(const QString &s)
{
    filter = s;
}

void ExportBase::setListTasks(bool b)
{
    listTasks = b;
}

bool ExportBase::execDialog()
{
    QString fn=QFileDialog::getSaveFileName(
                NULL,
                caption,
                dirPath,
                filter,
                NULL,
                QFileDialog::DontConfirmOverwrite);

    if (!fn.isEmpty() )
    {
        if (QFile (fn).exists() )
        {
            WarningDialog dia;
            dia.showCancelButton (true);
            dia.setCaption(QObject::tr("Warning: Overwriting file"));
            dia.setText(QObject::tr("Exporting to %1 will overwrite the existing file:\n%2").arg(exportName).arg(fn));
            dia.setShowAgainName("/exports/overwrite/" + exportName);
            if (!(dia.exec() == QDialog::Accepted))
            {
                cancelFlag = true;
                return false;
            }
        }
        dirPath  = fn.left(fn.lastIndexOf ("/"));
        filePath = fn;
        return true;
    }
    return false;
}

bool ExportBase::canceled()
{
    return cancelFlag;
}

void ExportBase::setLastCommand( const QString &s)
{
    lastCommand = s;
}

void ExportBase::completeExport(QString args) 
{
    QString command;
    if (args.isEmpty()) 
        // Add at least filepath as argument. exportName is added anyway
        command = QString("vym.currentMap().exportMap(\"%1\",\"%2\")").arg(exportName).arg(filePath);
    else
        command = QString("vym.currentMap().exportMap(\"%1\",%2)").arg(exportName).arg(args);

    settings.setLocalValue ( model->getFilePath(), "/export/last/exportPath", filePath);
    settings.setLocalValue ( model->getFilePath(), "/export/last/command", command);
    settings.setLocalValue ( model->getFilePath(), "/export/last/description", exportName);

    // Trigger saving of export command if it has changed
    if (model && (lastCommand != command) ) model->setChanged();

    mainWindow->statusMessage(QString("Exported as %1: %2").arg(exportName).arg(filePath));
}

QString ExportBase::getSectionString(TreeItem *start)
{
    // Make prefix like "2.5.3" for "bo:2,bo:5,bo:3"
    QString r;
    TreeItem *ti = start;
    int depth=ti->depth();
    while (depth>0)
    {
        r  = QString("%1").arg(1 + ti->num(),0,10) + "." + r;
        ti = ti->parent();
        depth = ti->depth();
    }
    if (r.isEmpty())
        return r;
    else
        return r + " ";
}

QString ExportBase::indent (const int &n, bool useBullet)
{
    QString s;
    for (int i=0; i<n; i++) s += indentPerDepth;
    if (useBullet && s.length() >= 2 && bulletPoints.count() > n)
        s.replace( s.length() - 2, 1, bulletPoints.at(n) );
    return s;
}

