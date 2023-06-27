#include "export-orgmode.h"

#include "mainwindow.h"
#include <QMessageBox>

extern Main *mainWindow;

ExportOrgMode::ExportOrgMode()
{
    exportName = "OrgMode";
    filter = "org-mode (*.org);;All (* *.*)";
}

void ExportOrgMode::doExport()
{
    // Exports a map to an org-mode file.
    // This file needs to be read
    // by EMACS into an org mode buffer
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            0, QObject::tr("Critical Export Error"),
            QObject::tr("Could not export as OrgMode to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }
    QTextStream ts(&file);
    ts.setCodec("UTF-8");

    // Main loop over all branches
    QString s;
    int i;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;
    model->nextBranch(cur, prev);
    while (cur) {
        if (!cur->hasHiddenExportParent()) {
            for (i = 0; i <= cur->depth(); i++)
                ts << ("*");
            ts << (" " + cur->getHeadingPlain() + "\n");
            // If necessary, write note
            if (!cur->isNoteEmpty()) {
                ts << (cur->getNoteASCII(0, 80));
                ts << ("\n");
            }
        }
        model->nextBranch(cur, prev);
    }
    file.close();

    result = ExportBase::Success;

    displayedDestination = filePath;
    completeExport();
}
