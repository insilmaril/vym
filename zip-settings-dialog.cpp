#include "zip-settings-dialog.h"

#include <QDebug>
#include <QFileDialog>
#include <QString>

#include "file.h"

extern QString vymName;
extern QString zipToolPath;
extern QString unzipToolPath;
extern bool zipToolAvailable;
extern bool unzipToolAvailable;


ZipSettingsDialog::ZipSettingsDialog(QWidget *parent) : QDialog(parent)
{

    ui.setupUi(this);
    init();

    QDialog::setWindowTitle(
        "VYM - " + tr("zip settings", "dialog window title"));

    connect (ui.zipToolPathLE, SIGNAL(textChanged(const QString&)), this, SLOT(zipToolPathChanged()));
    connect (ui.unzipToolPathLE, SIGNAL(textChanged(const QString&)), this, SLOT(unzipToolPathChanged()));

    connect (ui.zipToolButton, SIGNAL(clicked()), this, SLOT(zipToolButtonPressed()));
    connect (ui.unzipToolButton, SIGNAL(clicked()), this, SLOT(unzipToolButtonPressed()));
    connect (ui.closeButton, SIGNAL(clicked()), this, SLOT(accept()));

}

void ZipSettingsDialog::zipToolPathChanged()
{
    zipToolPath = ui.zipToolPathLE->text();
    updateCheckResults();
}

void ZipSettingsDialog::unzipToolPathChanged()
{
    unzipToolPath = ui.unzipToolPathLE->text();
    updateCheckResults();
}

void ZipSettingsDialog::zipToolButtonPressed()
{
    QString filter;
    QString text;

#if defined(Q_OS_WIN32)
    filter = "Windows executable (*.exe);;";
    text = QString(tr("Set path to 7z to zip/unzip files"));
#else
    filter = "All (*);;";
    text = QString(tr("Set path to zip files"));
#endif

    QString fn = QFileDialog::getOpenFileName(
        this,
        vymName + " - " + text + ":",
        zipToolPath,
        filter);

    if (!fn.isEmpty() ) 
    {
        zipToolPath = fn;
        ui.zipToolPathLE->setText(fn);
        updateCheckResults();
    }
}

void ZipSettingsDialog::unzipToolButtonPressed()
{
    QString filter;
    QString text;

#if defined(Q_OS_WIN32)
    // On windows we just use 7z for both zip/unzip
    return;
#else
    filter = "All (*);;";
    text = QString(tr("Set path to unzip files"));
#endif

    QString fn = QFileDialog::getOpenFileName(
        this,
        vymName + " - " + text + ":",
        zipToolPath,
        filter);

    if (!fn.isEmpty() ) 
    {
        unzipToolPath = fn;
        ui.unzipToolPathLE->setText(fn);
        updateCheckResults();
    }
}

void ZipSettingsDialog::init()
{
    ui.zipToolPathLE->setText(zipToolPath);
    ui.unzipToolPathLE->setText(unzipToolPath);
    updateCheckResults();
}

void ZipSettingsDialog::updateCheckResults()
{
    checkZipTool();
    checkUnzipTool();
    QString zipStatus;
    if (zipToolAvailable)
        zipStatus = QString(tr("Status: %1").arg("ok"));
    else
        zipStatus = QString(tr("Status: %1").arg("not ok"));
    ui.zipToolStatusLabel->setText( zipStatus );

    QString unzipStatus;
    if (unzipToolAvailable)
        unzipStatus = QString(tr("Status: %1").arg("ok"));
    else
        unzipStatus = QString(tr("Status: %1").arg("not ok"));
    ui.unzipToolStatusLabel->setText( unzipStatus );
}
