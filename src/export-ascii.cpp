#include "export-ascii.h"

#include <QClipboard>
#include <QGuiApplication>
#include <QMessageBox>

#include "branchitem.h"
#include "mainwindow.h"
#include "task.h"
#include "vymmodel.h"

extern QString vymName;
extern Main *mainWindow;

ExportASCII::ExportASCII()
{
    exportName = "ASCII";
    filter = "TXT (*.txt);;All (* *.*)";
    caption = vymName + " -" + QObject::tr("Export as ASCII");
}

void ExportASCII::doExport()
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(
            0, QObject::tr("Critical Export Error"),
            QObject::tr("Could not export as ASCII to %1").arg(filePath));
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

    int lastDepth = 0;

    QStringList tasks;

    model->nextBranch(cur, prev);
    while (cur) {
        if (cur->getType() == TreeItem::Branch ||
            cur->getType() == TreeItem::MapCenter) {
            if (!cur->hasHiddenExportParent()) {
                // qDebug() << "ExportASCII::
                // "<<curIndent.toStdString()<<cur->headingPlain().toStdString();

                // Insert newline after previous list
                //if (cur->depth() < lastDepth)
                //    out += "\n";

                // Make indentstring
                curIndent = "";
                for (i = 1; i < cur->depth() - 1; i++)
                    curIndent += indentPerDepth;

                dashIndent = "";
                switch (cur->depth()) {
                case 0:
                    if (!out.isEmpty())
                        // Add extra line breaks for 2nd, 3rd, ... MapCenter
                        ensureEmptyLines(out, 2);
                    out += underline(cur->headingPlain(), QString("="));

                    // Empty line below "====" of MapCenters
                    ensureEmptyLines(out, 1);
                    dashIndent = "";    // No indention for notes in MapCenter
                    break;
                case 1:
                    ensureNewLine(out);
                    out += (underline(getSectionString(cur) +
                        cur->headingPlain(),
                        QString("-")));
                    // Empty line below "----" of MainBranches
                    ensureEmptyLines(out, 1);
                    dashIndent = "";    // No indention for notes in MainBranch
                    break;
                case 2:
                    ensureNewLine(out);
                    out += (curIndent + "* " + cur->headingPlain());
                    dashIndent = "  ";
                    break;
                default:
                    ensureNewLine(out);
                    out += (curIndent + "- " + cur->headingPlain());
                    dashIndent = "  ";
                    break;
                }

                // If there is a task, save it for potential later display
                if (listTasks && cur->getTask()) {
                    tasks.append(QString("[%1]: %2")
                                     .arg(cur->getTask()->getStatusString())
                                     .arg(cur->headingPlain()));
                }

                // If necessary, write URL
                if (cur->hasUrl()) {
                    ensureNewLine(out);
                    out += (curIndent + dashIndent + cur->url()) + "\n";
                }

                // If necessary, write vymlink
                if (!cur->vymLink().isEmpty()) {
                    ensureNewLine(out);
                    out += (curIndent + dashIndent + cur->vymLink()) +
                           " (vym mindmap)\n";
                }

                // If necessary, write note
                if (!cur->isNoteEmpty()) {
                    // Add at least one empty line before note
                    ensureEmptyLines(out, 1);

                    // Add note and empty line after note
                    out += cur->getNoteASCII(curIndent + dashIndent, 80);

                    ensureEmptyLines(out, 1);
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

QString ExportASCII::underline(const QString &text, const QString &line)
{
    QString r = text + "\n";
    for (int j = 0; j < text.length(); j++)
        r += line;
    return r;
}

QString ExportASCII::ensureEmptyLines(QString &text, int n)
{
    // Ensure at least n empty lines at the end of text

    // First count trailing line breaks
    int j = 0;
    int i = text.length() - 1;
    while (i > -1 && text.at(i) == QChar::LineFeed)  {
        i--;
        j++;
    }

    while (j < n + 1) {
        text = text + QChar::LineFeed;
        j++;
    }

    return text;
}

QString ExportASCII::ensureNewLine(QString &text)
{
    // Add one line break, if not already there yet e.g. from empty line
    if (!text.endsWith("\n"))
        text += "\n";
    return text;
}
