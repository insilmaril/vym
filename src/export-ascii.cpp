#include "export-ascii.h"

#include "mainwindow.h"
#include <QMessageBox>

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
    BranchItem *cur = NULL;
    BranchItem *prev = NULL;

    int lastDepth = 0;

    QStringList tasks;

    model->nextBranch(cur, prev);
    while (cur) {
        if (cur->getType() == TreeItem::Branch ||
            cur->getType() == TreeItem::MapCenter) {
            if (!cur->hasHiddenExportParent()) {
                // qDebug() << "ExportASCII::
                // "<<curIndent.toStdString()<<cur->getHeadingPlain().toStdString();

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
                        out = addEmptyLine(out);
                    out += underline(cur->getHeadingPlain(), QString("="));

                    // Empty line below "====" of MapCenters
                    addEmptyLine(out);
                    dashIndent = "";    // No indention for notes in MapCenter
                    break;
                case 1:
                    startNewLine(out);
                    out += (underline(getSectionString(cur) +
                                          cur->getHeadingPlain(),
                                      QString("-")));
                    // Empty line below "----" of MainBranches
                    addEmptyLine(out);
                    dashIndent = "";    // No indention for notes in MainBranch
                    break;
                case 2:
                    startNewLine(out);
                    out += (curIndent + "* " + cur->getHeadingPlain());
                    dashIndent = "  ";
                    break;
                default:
                    startNewLine(out);
                    out += (curIndent + "- " + cur->getHeadingPlain());
                    dashIndent = "  ";
                    break;
                }

                // If there is a task, save it for potential later display
                if (listTasks && cur->getTask()) {
                    tasks.append(QString("[%1]: %2")
                                     .arg(cur->getTask()->getStatusString())
                                     .arg(cur->getHeadingPlain()));
                }

                // If necessary, write URL
                if (!cur->getURL().isEmpty())
                    out += (curIndent + dashIndent + cur->getURL()) + "\n"; // FIXME-0 check \n

                // If necessary, write vymlink
                if (!cur->getVymLink().isEmpty())
                    out += (curIndent + dashIndent + cur->getVymLink()) +
                           " (vym mindmap)\n";

                // If necessary, write note
                if (!cur->isNoteEmpty()) {
                    // Add at least one empty line before note
                    addEmptyLine(out);

                    // Add note and empty line after note
                    out += cur->getNoteASCII(curIndent + dashIndent, 80);

                    addEmptyLine(out);
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
    ts.setCodec("UTF-8");
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

QString ExportASCII::addEmptyLine(QString &text)
{
    // Add at least one empty line
    if (!text.endsWith("\n\n")) {
        if (!text.endsWith("\n"))
            text += "\n\n";
        else
            text += "\n";
    }
    return text;
}

QString ExportASCII::startNewLine(QString &text)
{
    // Add one line break, if not already there yet e.g. from empty line
    if (!text.endsWith("\n"))
        text += "\n";
    return text;
}
