#include "export-ascii.h"

#include <QMessageBox>
#include "mainwindow.h"

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
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical (0, QObject::tr("Critical Export Error"), QObject::tr("Could not export as ASCII to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }
    QTextStream ts( &file );
    ts.setCodec("UTF-8");

    // Main loop over all branches
    QString s;
    QString curIndent;
    QString dashIndent;
    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;

    int lastDepth=0;

    QStringList tasks;

    model->nextBranch (cur,prev);
    while (cur)
    {
        if (cur->getType()==TreeItem::Branch || cur->getType()==TreeItem::MapCenter)
        {
            // Insert newline after previous list
            if ( cur->depth() < lastDepth ) ts << "\n";

            // Make indentstring
            curIndent="";
            for (i=1;i<cur->depth()-1;i++) curIndent+= indentPerDepth;

            if (!cur->hasHiddenExportParent() )
            {
                //qDebug() << "ExportASCII::  "<<curIndent.toStdString()<<cur->getHeadingPlain().toStdString();

                dashIndent="";
                switch (cur->depth())
                {
                case 0:
                    ts << underline (cur->getHeadingPlain(),QString("="));
                    ts << "\n";
                    break;
                case 1:
                    ts << "\n";
                    ts << (underline (getSectionString(cur) + cur->getHeadingPlain(), QString("-") ) );
                    ts << "\n";
                    break;
                case 2:
                    ts << "\n";
                    ts << (curIndent + "* " + cur->getHeadingPlain());
                    ts << "\n";
                    dashIndent="  ";
                    break;
                case 3:
                    ts << (curIndent + "- " + cur->getHeadingPlain());
                    ts << "\n";
                    dashIndent="  ";
                    break;
                default:
                    ts << (curIndent + "- " + cur->getHeadingPlain());
                    ts << "\n";
                    dashIndent="  ";
                    break;
                }

                // If there is a task, save it for potential later display
                if (listTasks && cur->getTask() )
                {
                    tasks.append( QString("[%1]: %2").arg(cur->getTask()->getStatusString()).arg(cur->getHeadingPlain() ) );
                }

                // If necessary, write URL
                if (!cur->getURL().isEmpty())
                    ts << (curIndent + dashIndent + cur->getURL()) +"\n";

                // If necessary, write vymlink
                if (!cur->getVymLink().isEmpty())
                    ts << (curIndent + dashIndent + cur->getVymLink()) +" (vym mindmap)\n";

                // If necessary, write note
                if (!cur->isNoteEmpty())
                {
                    // curIndent +="  | ";
                    // Only indent for bullet points
                    if (cur->depth() > 2) curIndent +="  ";
                    ts << '\n' +  cur->getNoteASCII(curIndent, 80) ;
                }
                lastDepth = cur->depth();
            }
        }
        model->nextBranch(cur,prev);
    }

    if (listTasks)
    {
        ts << "\n\nTasks\n-----\n\n";


        foreach (QString t, tasks)
        {
            ts << " - " << t << "\n";
        }
    }
    file.close();

    QString listTasksString = listTasks ? "true" : "false";
    completeExport( QString("\"%1\",%2").arg(filePath).arg(listTasksString) );
}

QString ExportASCII::underline (const QString &text, const QString &line)
{
    QString r=text + "\n";
    for (int j=0;j<text.length();j++) r+=line;
    return r;
}


