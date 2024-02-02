#include "export-markdown.h"

#include "mainwindow.h"
#include <QMessageBox>

extern QString vymName;
extern Main *mainWindow;

ExportMarkdown::ExportMarkdown()
{
    exportName = "Markdown";
    filter = "TXT (*.txt);;All (* *.*)";
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
            // Insert newline after previous list
            if (cur->depth() < lastDepth)
                out += "\n";

            // Make indentstring
            curIndent = "";
            for (i = 1; i < cur->depth() - 1; i++)
                curIndent += indentPerDepth;

            curHeading = cur->getHeadingText();

            // If necessary, write heading as URL
            if (!cur->getURL().isEmpty())
                curHeading = "[" + curHeading + "](" + cur->getURL() + ")";

            if (!cur->hasHiddenExportParent()) {
                // qDebug() << "ExportMarkdown::
                // "<<curIndent.toStdString()<<cur->curHeading.toStdString();

                dashIndent = "";
                switch (cur->depth()) {
                case 0:
                    out += underline(curHeading, QString("="));
                    out += "\n";
                    break;
                case 1:
                    out += "\n";
                    out += (underline(curHeading, QString("-")));
                    out += "\n";
                    break;
                case 2:
                    out += "\n";
                    out += (curIndent + "### " + curHeading);
                    out += "\n";
                    dashIndent = "  ";
                    break;
                case 3:
                    out += (curIndent + "- " + curHeading);
                    out += "\n";
                    dashIndent = "  ";
                    break;
                default:
                    out += (curIndent + "- " + curHeading);
                    out += "\n";
                    dashIndent = "  ";
                    break;
                }

                // If there is a task, save it for potential later display
                if (listTasks && cur->getTask()) {
                    tasks.append(QString("[%1]: %2")
                                     .arg(cur->getTask()->getStatusString())
                                     .arg(curHeading));
                }

                // If necessary, write vymlink
                if (!cur->getVymLink().isEmpty())
                    out += (curIndent + dashIndent + cur->getVymLink()) +
                           " (vym mindmap)\n";

                // If necessary, write note
                if (!cur->isNoteEmpty()) {
                    // curIndent +="  | ";
                    // Only indent for bullet points
                    if (cur->depth() > 2)
                        curIndent += "  ";
                    out += '\n' + cur->getNoteASCII(curIndent, 80);
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

QString ExportMarkdown::underline(const QString &text, const QString &line)
{
    QString r = text + "\n";
    for (int j = 0; j < text.length(); j++)
        r += line;
    return r;
}
