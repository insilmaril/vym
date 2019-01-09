#include <QMessageBox>

#include "export-ao.h"
#include "mainwindow.h"

extern QString vymName;
extern Main *mainWindow;
extern Settings settings;

ExportAO::ExportAO()
{
    exportName = "AO";
    filter     = "TXT (*.txt);;All (* *.*)";
    caption    = vymName + " -" +QObject::tr("Export as ASCII")+" "+QObject::tr("(still experimental)");
    indentPerDepth="   ";
    bulletPoints.clear();
    for (int i=0; i<10; i++)
        bulletPoints << "-";
}

void ExportAO::doExport()   
{
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical (0, QObject::tr("Critical Export Error"), QObject::tr("Could not export as AO to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }

    settings.setLocalValue ( model->getFilePath(), "/export/last/command","exportAO");
    settings.setLocalValue ( model->getFilePath(), "/export/last/description","A&O report");

    QTextStream ts( &file );
    ts.setCodec("UTF-8");

    // Main loop over all branches
    QString s;
    QString curIndent;
    QString dashIndent;

    int i;
    BranchItem *cur  = NULL;
    BranchItem *prev = NULL;

    model->nextBranch (cur,prev);
    while (cur)
    {
        QString line;
        QString colString = "";
        QString noColString;
        QString statusString = "";
        QColor col;

        if (cur->getType() == TreeItem::Branch || cur->getType() == TreeItem::MapCenter)
        {
            // Make indentstring
            curIndent = indent(cur->depth()-4, true);

            if (!cur->hasHiddenExportParent() )
            {
                col = cur->getHeadingColor();
                if (col == QColor (255,0,0))
                    colString = "[R] ";
                else if (col == QColor (217,81,0))
                    colString = "[O] ";
                else if (col == QColor (0,85,0))
                    colString = "[G] ";
                else if (cur->depth() == 4)
                    colString = " *  ";
                else
                    colString = "    ";

                noColString = QString(" ").repeated(colString.length() );

                dashIndent = "";
                switch (cur->depth())
                {
                case 0: break;  // Mapcenter (Ignored)
                case 1: break;  // Mainbranch "Archive" (Ignored)
                case 2: // Title: "Current week number..."
                    ts << "\n";
                    ts << underline ( cur->getHeadingPlain(), QString("=") );
                    ts << "\n";
                    break;
                case 3: // Headings: "Achievement", "Bonus", "Objective", ...
                    ts << "\n";
                    ts << underline ( cur->getHeadingPlain(), "-");
                    ts << "\n";
                    break;
                default:    // depth 4 and higher are the items we need to know
                    Task *task=cur->getTask();
                    if (task)
                    {
                        // Task status overrides other flags
                        switch ( task->getStatus() )
                        {
                        case Task::NotStarted:
                            statusString="[NOT STARTED]";
                            break;
                        case Task::WIP:
                            statusString="[WIP]";
                            break;
                        case Task::Finished:
                            statusString="[DONE]";
                            break;
                        }
                    } else
                    {
                        if (cur->hasActiveStandardFlag ("hook-green") )
                            statusString="[DONE]";
                        else if (cur->hasActiveStandardFlag ("wip"))
                            statusString="[WIP]";
                        else if (cur->hasActiveStandardFlag ("cross-red"))
                            statusString="[NOT STARTED]";
                    }

                    line += colString;
                    line += curIndent;
                    if (cur->depth() >3)
                        line += cur->getHeadingPlain();

                    // Pad line width before status
                    i = 80 - line.length() - statusString.length() -1;
                    for (int j=0; j<i; j++) line += " ";
                    line += " "  + statusString + "\n";

                    ts << line;

                    // If necessary, write URL
                    if (!cur->getURL().isEmpty())
                        ts << noColString << indent(cur->depth()-4, false) + cur->getURL() + "\n";

                    // If necessary, write note
                    if (!cur->isNoteEmpty())
                    {
                        curIndent = noColString + indent(cur->depth()-4,false) + "| ";
                        s=cur->getNoteASCII( curIndent, 80);
                        ts << s + "\n";
                    }
                    break;
                }
            }
        }
        model->nextBranch(cur,prev);
    }
    file.close();
    completeExport();
}

QString ExportAO::underline (const QString &text, const QString &line)
{
    QString r=text + "\n";
    for (int j=0;j<text.length();j++) r+=line;
    return r;
}


