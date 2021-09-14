#include "export-confluence-dialog.h"

#include <QDateTime>
#include <QFileDialog>
#include <QMessageBox>

#include "file.h"
#include "settings.h"

extern QDir vymBaseDir;
extern Settings settings;
extern bool debug;

ExportConfluenceDialog::ExportConfluenceDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    filepath = "";
    settingsChanged = false;

    // connect(buttonGroup, SIGNAL(buttonPressed(int)), this,
    // SLOT(pageButtonPressed(int)));
    connect(ui.createPageButton, SIGNAL(clicked(bool)), this,
            SLOT(pageButtonPressed()));
    connect(ui.updatePageButton, SIGNAL(clicked(bool)), this,
            SLOT(pageButtonPressed()));

    // signals and slots connections
    connect(ui.mapCenterToPageNameCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(mapCenterToPageNameCheckBoxPressed(bool)));
    connect(ui.textColorCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(textcolorCheckBoxPressed(bool)));
    connect(ui.lineEditURL, SIGNAL(textChanged(const QString &)), this,
            SLOT(URLChanged()));
    connect(ui.lineEditPageName, SIGNAL(textChanged(const QString &)), this,
            SLOT(pageNameChanged()));
    connect(ui.saveSettingsInMapCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(saveSettingsInMapCheckBoxPressed(bool)));

    // Not implemented yet
    ui.includeImagesCheckBox->hide();
    ui.TOCCheckBox->hide();
    ui.taskFlagsCheckBox->hide();
    ui.userFlagsCheckBox->hide();
    /*
    connect(ui.includeImagesCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(includeImagesCheckBoxPressed(bool)));
    connect(ui.TOCCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(TOCCheckBoxPressed(bool)));
    connect(ui.numberingCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(numberingCheckBoxPressed(bool)));
    connect(ui.taskFlagsCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(taskFlagsCheckBoxPressed(bool)));
    connect(ui.userFlagsCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(userFlagsCheckBoxPressed(bool)));
    */
}

void ExportConfluenceDialog::readSettings()
{
    url = settings
                  .localValue(filepath, "/export/confluence/url",
                              "Enter URL of page")
                  .toString();
    ui.lineEditURL->setText(url);

    pageName = settings
                    .localValue(filepath, "/export/confluence/pagename",
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

    /*  FIXME-3 cleanup the copied HTML parameters
    includeMapImage = settings.localValue (filepath,
    "/export/confluence/includeMapImage", "true").toBool();
    ui.imageCheckBox->setChecked(includeMapImage);

    includeImages = settings.localValue (filepath,
    "/export/confluence/includeImages", "true").toBool();
    ui.includeImagesCheckBox->setChecked(includeImages);

    useTOC = settings.localValue (filepath, "/export/confluence/useTOC",
    "true").toBool(); ui.TOCCheckBox->setChecked(useTOC);

    useNumbering = settings.localValue (filepath,
    "/export/confluence/useNumbering", "true").toBool();
    ui.numberingCheckBox->setChecked(useNumbering);

    useTaskFlags = settings.localValue (filepath,
    "/export/confluence/useTaskFlags", "true").toBool();
    ui.taskFlagsCheckBox->setChecked(useTaskFlags);

    useUserFlags = settings.localValue (filepath,
    "/export/confluence/useUserFlags", "true").toBool();
    ui.userFlagsCheckBox->setChecked(useUserFlags);

    */
    useTextColor =
        settings.localValue(filepath, "/export/confluence/useTextColor", false)
            .toBool();
    ui.textColorCheckBox->setChecked(useTextColor);

    saveSettingsInMap =
        settings
            .localValue(filepath, "/export/confluence/saveSettingsInMap", true)
            .toBool();
    ui.saveSettingsInMapCheckBox->setChecked(saveSettingsInMap);


    pageButtonPressed();
}

void ExportConfluenceDialog::setURL(const QString &u) { url = u; }

void ExportConfluenceDialog::setPageName(const QString &s) { pageName = s; }

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

void ExportConfluenceDialog::URLChanged()
{
    settingsChanged = true;
    url = ui.lineEditURL->text();
}

void ExportConfluenceDialog::pageNameChanged()
{
    settingsChanged = true;
    pageName = ui.lineEditPageName->text();
}

void ExportConfluenceDialog::mapCenterToPageNameCheckBoxPressed(bool b)
{
    mapCenterToPageName = b;
    if (mapCenterToPageName)
    {
        ui.lineEditPageName->setText(pageNameHint);
        ui.lineEditPageName->setEnabled(false);
    } else
    {
        ui.lineEditPageName->show();
        ui.lineEditPageName->setEnabled(true);
    }
    settingsChanged = true;
}

void ExportConfluenceDialog::includeImagesCheckBoxPressed(bool b)
{
    includeImages = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::TOCCheckBoxPressed(bool b)
{
    useTOC = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::numberingCheckBoxPressed(bool b)
{
    useNumbering = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::taskFlagsCheckBoxPressed(bool b)
{
    useTaskFlags = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::userFlagsCheckBoxPressed(bool b)
{
    useUserFlags = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::textcolorCheckBoxPressed(bool b)
{
    useTextColor = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::saveSettingsInMapCheckBoxPressed(bool b)
{
    saveSettingsInMap = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::warningsCheckBoxPressed(bool b)
{
    showWarnings = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::outputCheckBoxPressed(bool b)
{
    showOutput = b;
    settingsChanged = true;
}

void ExportConfluenceDialog::saveSettings()
{
    // Save options to settings file
    // (but don't save at destructor, which
    // is called for "cancel", too)
    if (!saveSettingsInMap)
        settings.clearLocal(filepath, "/export/confluence");
    else {
        settings.setLocalValue(filepath, "/export/confluence/saveSettingsInMap",
                               "yes");
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/includeMapImage", includeMapImage);
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/includeImages", includeImages);
        //        settings.setLocalValue (filepath, "/export/confluence/useTOC",
        //        useTOC); settings.setLocalValue (filepath,
        //        "/export/confluence/useNumbering", useNumbering);
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/useTaskFlags", useTaskFlags);
        //        settings.setLocalValue (filepath,
        //        "/export/confluence/useUserFlags", useUserFlags);
        settings.setLocalValue(filepath, "/export/confluence/useTextColor",
                               useTextColor);
        settings.setValue("/export/confluence/showWarnings", showWarnings);
        settings.setValue("/export/confluence/showOutput", showOutput);
        settings.setLocalValue(filepath, "/export/confluence/url", url);
        settings.setLocalValue(filepath, "/export/confluence/pagename", pageName);
        settings.setLocalValue(filepath, "/export/confluence/createNewPage", ui.createPageButton->isChecked());
    }
}

void ExportConfluenceDialog::setFilePath(const QString &s) { filepath = s; }

void ExportConfluenceDialog::setMapName(const QString &s) { mapname = s; }

void ExportConfluenceDialog::setCreateNewPage(bool b) { ui.createPageButton->setChecked(b); }

bool ExportConfluenceDialog::getCreateNewPage() { return ui.createPageButton->isChecked(); }

QString ExportConfluenceDialog::getURL() { return url; }

QString ExportConfluenceDialog::getPageName() { return pageName; }

void ExportConfluenceDialog::setPageNameHint(const QString &s) 
{
    pageNameHint = s;
}

bool ExportConfluenceDialog::warnings() { return showWarnings; }

bool ExportConfluenceDialog::hasChanged() { return settingsChanged; }
