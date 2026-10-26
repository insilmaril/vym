#include "export-markdown.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>

#include "branchitem.h"
#include "mainwindow.h"
#include "misc.h"
#include "task.h"
#include "vymmodel.h"

extern QString vymName;
extern Main *mainWindow;

ExportMarkdown::ExportMarkdown()
{
    exportName = "Markdown";
    filter = "Markdown (*.md);;All (* *.*)";
    caption = vymName + " -" + QObject::tr("Export as Markdown");
}

void ExportMarkdown::doExport()
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            0, QObject::tr("Critical Export Error"),
            QObject::tr("Could not export as Markdown to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }

    QString out;

    // Main loop over all branches
    QString s;
    QString curIndent;
    QString dashIndent;
    int i;
    BranchItem *cur = nullptr;
    BranchItem *prev = nullptr;

    QString curHeading;

    int lastDepth = 0;

    QStringList tasks;

    model->nextBranch(cur, prev);
    while (cur) {
        if (cur->getType() == TreeItem::Branch ||
            cur->getType() == TreeItem::MapCenter) {

            curHeading = cur->headingText();

            // If necessary, write heading as URL
            if (cur->hasUrl())
                curHeading = "[" + curHeading + "](" + cur->url() + ")";

            if (!cur->hasHiddenParent()) {
                if (0) {
                    for (curIndent = "", i = 1; i < cur->depth() - 1;
                         i++, curIndent += indentPerDepth);
                    qDebug() << "ExportMarkdown:: "<<curIndent.toStdString()
                             <<curHeading.toStdString();
                }

                dashIndent = "";
                switch (cur->depth()) {
                case 0:
                    out += "\n";
                    out += ("# " + curHeading);
                    out += "\n";
                    break;
                case 1:
                    out += "\n";
                    out += ("## " + curHeading);
                    out += "\n";
                    break;
                case 2:
                    out += "\n";
                    out += ("### " + curHeading);
                    out += "\n";
                    break;
                case 3:
                    out += "\n";
                    out += ("#### " + curHeading);
                    out += "\n";
                    break;
                case 4:
                    out += "\n";
                    out += ("##### " + curHeading);
                    out += "\n";
                    break;
                case 5:
                    out += "\n";
                    out += ("###### " + curHeading);
                    out += "\n";
                    break;
                default:
                    out += "\n";
                    for (i = 1; i < cur->depth() - 5; i++)
                        dashIndent += "  ";
                    out += (dashIndent + "- " + curHeading);
                    out += "\n";
                    // indent for note/content under this dash
                    dashIndent += "  ";
                    break;
                }

                // If there is a task, save it for potential later display
                if (listTasks && cur->getTask()) {
                    tasks.append(QString("[%1]: %2")
                                     .arg(cur->getTask()->getStatusString())
                                     .arg(curHeading));
                }

                // If necessary, write vymlink
                if (cur->hasVymLink()) {
                    out += "\n";
                    out += (dashIndent + "[vym mindmap](" + cur->vymLink() + ")");
                    out += "\n";
                }

                // If necessary, write note
                if (!cur->isNoteEmpty()) {
                    out += "\n";
                    out += cur->getNoteASCII(dashIndent, 80);
                    out += "\n";
                }
                lastDepth = cur->depth();
            }
        }
        model->nextBranch(cur, prev);
    }

    if (listTasks) {
        out += "\n\nTasks\n-----\n\n";

        foreach (QString t, tasks) {
            out += " - " + t + "\n";
        }
    }
    QTextStream ts(&file);
    ts << out;
    file.close();

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(out);

    QString listTasksString = listTasks ? "true" : "false";

    displayedDestination = filePath;

    result = ExportBase::Success;

    QStringList args;
    args << filePath;
    args << listTasksString;

    completeExport(args);
}

