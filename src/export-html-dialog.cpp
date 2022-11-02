#include "export-html-dialog.h"

#include <QFileDialog>
#include <QMessageBox>

#include "file.h"
#include "settings.h"

extern QDir vymBaseDir;
extern Settings settings;
extern bool debug;

ExportHTMLDialog::ExportHTMLDialog(QWidget *parent) : QDialog(parent)
{
    ui.setupUi(this);

    filepath = "";
    settingsChanged = false;

    // signals and slots connections
    connect(ui.browseExportDirButton, SIGNAL(pressed()), this,
            SLOT(browseDirectoryPressed()));
    connect(ui.browseCssSrcButton, SIGNAL(pressed()), this,
            SLOT(browseCssSrcPressed()));
    connect(ui.browseCssDstButton, SIGNAL(pressed()), this,
            SLOT(browseCssDstPressed()));
    connect(ui.imageCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(imageCheckBoxPressed(bool)));
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
    connect(ui.textColorCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(textcolorCheckBoxPressed(bool)));
    connect(ui.lineEditDir, SIGNAL(textChanged(const QString &)), this,
            SLOT(dirChanged()));
    connect(ui.copyCssCheckBox, SIGNAL(pressed()), this,
            SLOT(copyCssPressed()));
    connect(ui.lineEditCssSrc, SIGNAL(textChanged(const QString &)), this,
            SLOT(cssSrcChanged()));
    connect(ui.lineEditCssDst, SIGNAL(textChanged(const QString &)), this,
            SLOT(cssDstChanged()));
    connect(ui.saveSettingsInMapCheckBox, SIGNAL(toggled(bool)), this,
            SLOT(saveSettingsInMapCheckBoxPressed(bool)));
    connect(ui.lineEditPostScript, SIGNAL(textChanged(const QString &)), this,
            SLOT(postscriptChanged()));
    connect(ui.browsePostExportButton, SIGNAL(pressed()), this,
            SLOT(browsePostExportButtonPressed()));
}

void ExportHTMLDialog::readSettings()
{
    dir.setPath(settings
              .localValue(filepath, "/export/html/exportDir",
                          vymBaseDir.currentPath())
              .toString()); // FIXME-3 exportDir only needed for dialog
    ui.lineEditDir->setText(dir.absolutePath());

    includeMapImage =
        settings.localValue(filepath, "/export/html/includeMapImage", "true")
            .toBool();
    ui.imageCheckBox->setChecked(includeMapImage);

    includeImages =
        settings.localValue(filepath, "/export/html/includeImages", "true")
            .toBool();
    ui.includeImagesCheckBox->setChecked(includeImages);

    useTOC =
        settings.localValue(filepath, "/export/html/useTOC", "true").toBool();
    ui.TOCCheckBox->setChecked(useTOC);

    useNumbering =
        settings.localValue(filepath, "/export/html/useNumbering", "true")
            .toBool();
    ui.numberingCheckBox->setChecked(useNumbering);

    useTaskFlags =
        settings.localValue(filepath, "/export/html/useTaskFlags", "true")
            .toBool();
    ui.taskFlagsCheckBox->setChecked(useTaskFlags);

    useUserFlags =
        settings.localValue(filepath, "/export/html/useUserFlags", "true")
            .toBool();
    ui.userFlagsCheckBox->setChecked(useUserFlags);

    useTextColor =
        settings.localValue(filepath, "/export/html/useTextColor", "no")
            .toBool();
    ui.textColorCheckBox->setChecked(useTextColor);

    /* FIXME-3 this was used in old html export, is not yet in new stylesheet
        useHeading=settings.readValue
       ("/export/html/useHeading","false").toBool();
        checkBox4_2->setChecked(useHeading);
    */

    saveSettingsInMap =
        settings.localValue(filepath, "/export/html/saveSettingsInMap", "no")
            .toBool();
    ui.saveSettingsInMapCheckBox->setChecked(saveSettingsInMap);

    // CSS settings
    css_copy =
        settings.localValue(filepath, "/export/html/copy_css", true).toBool();
    ui.copyCssCheckBox->setChecked(css_copy);

    QString css_org = vymBaseDir.path() + "/styles/vym.css";
    css_src = settings.localValue(filepath, "/export/html/css_src", css_org)
                  .toString();
    css_dst =
        settings.localValue(filepath, "/export/html/css_dst", basename(css_org))
            .toString();

    ui.lineEditCssSrc->setText(css_src);
    ui.lineEditCssDst->setText(css_dst);

    postscript =
        settings.localValue(filepath, "/export/html/postscript", "").toString();
    ui.lineEditPostScript->setText(postscript);

    if (!postscript.isEmpty()) {
        QMessageBox::warning(0, tr("Warning"),
                             tr("The settings saved in the map "
                                "would like to run script:\n\n"
                                "%1\n\n"
                                "Please check, if you really\n"
                                "want to allow this in your system!")
                                 .arg(postscript));
    }
}

void ExportHTMLDialog::setDirectory(const QString &d) { dir.setPath(d); }

void ExportHTMLDialog::dirChanged()
{
    setDirectory(ui.lineEditDir->text());
    settingsChanged = true;
}

void ExportHTMLDialog::browseDirectoryPressed()
{
    QFileDialog fd(this);
    fd.setFileMode(QFileDialog::DirectoryOnly);
    fd.setWindowTitle(tr("VYM - Export HTML to directory"));
    fd.setModal(true);
    fd.setDirectory(QDir::current());
    fd.show();

    if (fd.exec() == QDialog::Accepted) {
        QDir dir = fd.directory();
        ui.lineEditDir->setText(dir.path());
        settingsChanged = true;
    }
}

void ExportHTMLDialog::imageCheckBoxPressed(bool b)
{
    includeMapImage = b;
    settingsChanged = true;
}

void ExportHTMLDialog::includeImagesCheckBoxPressed(bool b)
{
    includeImages = b;
    settingsChanged = true;
}

void ExportHTMLDialog::TOCCheckBoxPressed(bool b)
{
    useTOC = b;
    settingsChanged = true;
}

void ExportHTMLDialog::numberingCheckBoxPressed(bool b)
{
    useNumbering = b;
    settingsChanged = true;
}

void ExportHTMLDialog::taskFlagsCheckBoxPressed(bool b)
{
    useTaskFlags = b;
    settingsChanged = true;
}

void ExportHTMLDialog::userFlagsCheckBoxPressed(bool b)
{
    useUserFlags = b;
    settingsChanged = true;
}

void ExportHTMLDialog::textcolorCheckBoxPressed(bool b)
{
    useTextColor = b;
    settingsChanged = true;
}

void ExportHTMLDialog::saveSettingsInMapCheckBoxPressed(bool b)
{
    saveSettingsInMap = b;
    settingsChanged = true;
}

void ExportHTMLDialog::warningsCheckBoxPressed(bool b)
{
    showWarnings = b;
    settingsChanged = true;
}

void ExportHTMLDialog::outputCheckBoxPressed(bool b)
{
    showOutput = b;
    settingsChanged = true;
}

void ExportHTMLDialog::cssSrcChanged()
{
    css_src = ui.lineEditCssSrc->text();
    settingsChanged = true;
}

void ExportHTMLDialog::cssDstChanged()
{
    css_dst = ui.lineEditCssDst->text();
    settingsChanged = true;
}

QString ExportHTMLDialog::getCssSrc()
{
    if (css_copy)
        return css_src;
    else
        return QString();
}

QString ExportHTMLDialog::getCssDst() { return css_dst; }

void ExportHTMLDialog::copyCssPressed()
{
    css_copy = ui.imageCheckBox->isChecked();
    settingsChanged = true;
}

void ExportHTMLDialog::browseCssSrcPressed()
{
    QFileDialog fd(this);
    fd.setModal(true);
    fd.setNameFilter("Cascading Stylesheet (*.css)");
    fd.setDirectory(QDir::current());
    fd.show();

    if (fd.exec() == QDialog::Accepted) {
        if (!fd.selectedFiles().isEmpty()) {
            css_src = fd.selectedFiles().first();
            ui.lineEditCssSrc->setText(css_src);
            settingsChanged = true;
        }
    }
}

void ExportHTMLDialog::browseCssDstPressed()
{
    QFileDialog fd(this);
    fd.setModal(true);
    fd.setNameFilter("Cascading Stylesheet (*.css)");
    fd.setDirectory(QDir::current());
    fd.show();

    if (fd.exec() == QDialog::Accepted) {
        if (!fd.selectedFiles().isEmpty()) {
            css_dst = fd.selectedFiles().first();
            ui.lineEditCssDst->setText(css_dst);
            settingsChanged = true;
        }
    }
}

void ExportHTMLDialog::postscriptChanged()
{
    postscript = ui.lineEditPostScript->text();
    settingsChanged = true;
}

void ExportHTMLDialog::browsePostExportButtonPressed()
{
    QFileDialog fd(this);
    fd.setModal(true);
    fd.setNameFilter("Scripts (*.sh *.pl *.py *.php)");
    fd.setDirectory(QDir::current());
    fd.show();

    if (fd.exec() == QDialog::Accepted) {
        if (!fd.selectedFiles().isEmpty()) {
            postscript = fd.selectedFiles().first();
            ui.lineEditPostScript->setText(postscript);
            settingsChanged = true;
        }
    }
}

void ExportHTMLDialog::saveSettings()
{
    // Save options to settings file
    // (but don't save at destructor, which
    // is called for "cancel", too)
    if (!saveSettingsInMap)
        settings.clearLocal(filepath, "/export/html");
    else {
        settings.setLocalValue(
            filepath, "/export/html/exportDir",
            dir.absolutePath()); // FIXME-3 exportDir only needed for dialog
        settings.setLocalValue(filepath, "/export/html/saveSettingsInMap",
                               "yes");
        settings.setLocalValue(filepath, "/export/html/postscript", postscript);
        settings.setLocalValue(filepath, "/export/html/includeMapImage",
                               includeMapImage);
        settings.setLocalValue(filepath, "/export/html/includeImages",
                               includeImages);
        settings.setLocalValue(filepath, "/export/html/useTOC", useTOC);
        settings.setLocalValue(filepath, "/export/html/useNumbering",
                               useNumbering);
        settings.setLocalValue(filepath, "/export/html/useTaskFlags",
                               useTaskFlags);
        settings.setLocalValue(filepath, "/export/html/useUserFlags",
                               useUserFlags);
        settings.setLocalValue(filepath, "/export/html/useTextColor",
                               useTextColor);
        settings.setLocalValue(filepath, "/export/html/css_copy", css_copy);
        settings.setLocalValue(filepath, "/export/html/css_src", css_src);
        settings.setLocalValue(filepath, "/export/html/css_dst", css_dst);
        settings.setValue("/export/html/showWarnings", showWarnings);
        settings.setValue("/export/html/showOutput", showOutput);
    }
}

void ExportHTMLDialog::setFilePath(const QString &s) { filepath = s; }

void ExportHTMLDialog::setMapName(const QString &s) { mapname = s; }

QDir ExportHTMLDialog::getDir() { return dir; }

bool ExportHTMLDialog::warnings() { return showWarnings; }

bool ExportHTMLDialog::hasChanged() { return settingsChanged; }
