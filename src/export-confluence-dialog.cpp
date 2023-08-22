#include "export-confluence-dialog.h"

#include <QDateTime>
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

#include "file.h"
#include "mainwindow.h"
#include "settings.h"

extern QDir vymBaseDir;
extern Settings settings;
extern bool debug;
extern Main *mainWindow;

ExportConfluenceDialog::ExportConfluenceDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    filepath = "";

    connect(ui.createPageButton, SIGNAL(clicked(bool)), this,
            SLOT(pageButtonPressed()));
    connect(ui.updatePageButton, SIGNAL(clicked(bool)), this,
            SLOT(pageButtonPressed()));

    // signals and slots connections
    connect(ui.mapCenterToPageNameCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(mapCenterToPageNameCheckBoxPressed(bool)));
    connect(ui.lineEditURL, SIGNAL(textChanged(const QString &)), this,
            SLOT(URLChanged()));
    connect(ui.lineEditPageName, SIGNAL(textChanged(const QString &)), this,
            SLOT(pageNameChanged()));

    connect(ui.exportButton, &QPushButton::clicked, this, &ExportConfluenceDialog::doExport);

    connect(ui.openPageButton, &QPushButton::clicked, this, &ExportConfluenceDialog::openUrl);
}

void ExportConfluenceDialog::setCreateNewPage(bool b) { ui.createPageButton->setChecked(b); }

bool ExportConfluenceDialog::getCreateNewPage() { return ui.createPageButton->isChecked(); }

void ExportConfluenceDialog::openUrl()
{
    mainWindow->openURL(ui.lineEditURL->text());
}

QString ExportConfluenceDialog::getUrl() { return url; }

QString ExportConfluenceDialog::getPageName() { return pageName; }

void ExportConfluenceDialog::setPageNameHint(const QString &s) 
{
    pageNameHint = s;
}
void ExportConfluenceDialog::readSettings()
{
    url = settings
                  .localValue(filepath, "/export/confluence/url",
                              "Enter URL of page")
                  .toString();
    ui.lineEditURL->setText(url);

    pageName = settings
                    .localValue(filepath, "/export/confluence/pageName",
                                "New page created on " +
                                    QDateTime::currentDateTime().toString())
                    .toString();
    ui.lineEditPageName->setText(pageName);

    ui.createPageButton->setChecked(
        settings
            .localValue(filepath, "/export/confluence/createNewPage", true).toBool());

    ui.updatePageButton->setChecked(!
        settings
            .localValue(filepath, "/export/confluence/createNewPage", false).toBool());

    ui.imageCheckBox->setChecked(
        settings.localValue (filepath,
        "/export/confluence/includeMapImage", "true").toBool());

    /*  FIXME-3 cleanup the copied HTML parameters
    includeImages = settings.localValue (filepath,
    "/export/confluence/includeImages", "true").toBool();
    ui.includeImagesCheckBox->setChecked(includeImages);

    useTOC = settings.localValue (filepath, "/export/confluence/useTOC",
    "true").toBool(); ui.TOCCheckBox->setChecked(useTOC);

    useTaskFlags = settings.localValue (filepath,
    "/export/confluence/useTaskFlags", "true").toBool();
    ui.taskFlagsCheckBox->setChecked(useTaskFlags);

    useUserFlags = settings.localValue (filepath,
    "/export/confluence/useUserFlags", "true").toBool();
    ui.userFlagsCheckBox->setChecked(useUserFlags);

    */

    ui.useNumberingCheckBox->setChecked(
        settings.localValue (filepath,
        "/export/confluence/useNumbering", "true").toBool());
    ui.mapCenterToPageNameCheckBox->setChecked(
        settings.localValue(filepath, "/export/confluence/mapCenterToPageName", true)
            .toBool());
    ui.textColorCheckBox->setChecked(
        settings.localValue(filepath, "/export/confluence/useTextColor", false)
            .toBool());

    ui.saveSettingsInMapCheckBox->setChecked(
        settings
            .localValue(filepath, "/export/confluence/saveSettingsInMap", true)
            .toBool());


    pageButtonPressed();
}

void ExportConfluenceDialog::saveSettings()
{
    // Save options to settings file
    // (but don't save at destructor, which
    // is called for "cancel", too)
    if (!ui.saveSettingsInMapCheckBox->isChecked())
        settings.clearLocal(filepath, "/export/confluence");
    else {
        settings.setLocalValue(
                filepath, "/export/confluence/saveSettingsInMap",
                "yes");
        settings.setLocalValue (
                filepath, "/export/confluence/includeMapImage",
                ui.imageCheckBox->isChecked());
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/includeImages", includeImages);
        //        settings.setLocalValue (filepath, "/export/confluence/useTOC",
        //        useTOC); 
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/useNumbering", useNumbering);
        settings.setLocalValue(filepath,
                "/export/confluence/mapCenterToPageName",
                ui.mapCenterToPageNameCheckBox->isChecked());
        settings.setLocalValue(filepath,
                "/export/confluence/useTextColor",
                ui.textColorCheckBox->isChecked());
        settings.setLocalValue(filepath,
                "/export/confluence/useNumbering",
                ui.useNumberingCheckBox->isChecked());
        settings.setLocalValue(filepath, "/export/confluence/url", url);
        settings.setLocalValue(filepath, "/export/confluence/pageName", pageName);
        settings.setLocalValue(filepath, "/export/confluence/createNewPage", ui.createPageButton->isChecked());
    }
}

void ExportConfluenceDialog::setURL(const QString &u) { url = u; }

void ExportConfluenceDialog::setPageName(const QString &s) { pageName = s; }

void ExportConfluenceDialog::setFilePath(const QString &s) { filepath = s; }

void ExportConfluenceDialog::setMapName(const QString &s) { mapname = s; }

bool ExportConfluenceDialog::useTextColor()
{
    return ui.textColorCheckBox->isChecked();
}

bool ExportConfluenceDialog::mapCenterToPageName()
{
    return ui.mapCenterToPageNameCheckBox->isChecked();
}

bool ExportConfluenceDialog::useNumbering()
{
    return ui.useNumberingCheckBox->isChecked();
}

bool ExportConfluenceDialog::includeMapImage()
{
    return ui.imageCheckBox->isChecked();
}

void ExportConfluenceDialog::doExport()
{
    qDebug() << "ECD: Starting export now";
    qDebug() << "  exportImage=" << ui.imageCheckBox->isChecked();
    accept();
}

void ExportConfluenceDialog::pageButtonPressed()
{
    if (ui.createPageButton->isChecked()) {
        ui.URLLabel->setText("URL of parent page");
        ui.pageNameLabel->setText("Page title (required)");
    }
    else {
        ui.URLLabel->setText("URL of existing page");
        ui.pageNameLabel->setText("Page title (optional)");
    }
}

void ExportConfluenceDialog::URLChanged()   // FIXME-2 remove this and similar methods
{
    url = ui.lineEditURL->text();
}

void ExportConfluenceDialog::pageNameChanged()
{
    pageName = ui.lineEditPageName->text();
}

void ExportConfluenceDialog::mapCenterToPageNameCheckBoxPressed(bool b)
{
    if (ui.mapCenterToPageNameCheckBox->isChecked())
    {
        ui.lineEditPageName->setText(pageNameHint);
        ui.lineEditPageName->setEnabled(false); // FIXME-2 better set readonly
    } else
    {
        ui.lineEditPageName->show();
        ui.lineEditPageName->setEnabled(true);
    }
}

