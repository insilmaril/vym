#include "actionlog-dialog.h"

#include <QFileDialog>

#include "file.h"
#include "mainwindow.h"

extern Main *mainWindow;
extern QString vymName;

extern QString iconTheme;

extern bool useActionLog;
extern QString actionLogPath;

ActionLogDialog::ActionLogDialog()
{
    ui.setupUi(this);

    QDialog::setWindowTitle( vymName 
            + tr("Logfile settings", "Dialog to set if and where logfile is used"));

    updateControls();

    ui.setPathButton->setIcon(QPixmap(QString(":/document-open-%1.svg").arg(iconTheme)));
    connect(ui.setPathButton, SIGNAL(pressed()), this, SLOT(selectLogPathDialog()));

    connect(ui.useLogFileCheckbox, SIGNAL(clicked()), this, SLOT(toggleUseLogFile()));
    connect(ui.logFilePathLineEdit, SIGNAL(textEdited(const QString &)), 
            this, SLOT(pathChanged(const QString &)));
}

int ActionLogDialog::exec()
{
    int r = QDialog::exec();
    /*
    if (ui.useBackgroundImageCheckbox->isChecked())
        model->setBackgroundImageName(ui.imageNameLineEdit->text());
    */
    return r;
}

void ActionLogDialog::toggleUseLogFile()
{
    useActionLog = ui.useLogFileCheckbox->isChecked();
    updateControls();
}

void ActionLogDialog::pathChanged(const QString &s)
{
    actionLogPath = ui.logFilePathLineEdit->text();
}

void ActionLogDialog::selectLogPathDialog()
{
    QStringList filters;
    filters << tr("Logfiles") + " (*.log)";
    QFileDialog fd;
    fd.setFileMode(QFileDialog::AnyFile);
    fd.setNameFilters(filters);
    fd.setWindowTitle(vymName + " - " + tr("Set path to logfile"));
    fd.setDirectory(dirname(actionLogPath));
    fd.setAcceptMode(QFileDialog::AcceptOpen);

    if (fd.exec() == QDialog::Accepted && !fd.selectedFiles().isEmpty()) {
        actionLogPath = fd.selectedFiles().first();
        updateControls();
    }
}

void ActionLogDialog::updateControls()
{
    ui.logFilePathLineEdit->setText(actionLogPath);
    ui.useLogFileCheckbox->setChecked(useActionLog);
    ui.logFilePathLineEdit->setEnabled(useActionLog);
    ui.setPathButton->setEnabled(useActionLog);
}
