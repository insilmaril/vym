#include <QMessageBox>
#include "mainwindow.h"

#include "export-csv.h"

extern QString vymName;
extern Main *mainWindow;

ExportCSV::ExportCSV() 
{
    exportName="CSV";
    filter="CSV (*.csv);;All (* *.*)";
    caption=vymName+ " -" +QObject::tr("Export as CSV");
}

void ExportCSV::doExport()
{
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) )
    {
        QMessageBox::critical (0, QObject::tr("Critical Export Error"), QObject::tr("Could not export as CSV to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }
    QTextStream ts( &file );
    ts.setCodec("UTF-8");

    // Write header
    ts << "\"Note\""  <<endl;

    // Main loop over all branches
    QString s;
    QString curIndent("");
    int i;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    model->nextBranch (cur,prev);
    while (cur)
    {
        if (!cur->hasHiddenExportParent() )
        {
            // If necessary, write note
            if (!cur->isNoteEmpty())
            {
                s =cur->getNoteASCII();
                s=s.replace ("\n","\n"+curIndent);
                ts << ("\""+s+"\",");
            } else
                ts <<"\"\",";

            // Make indentstring
            for (i=0;i<cur->depth();i++) curIndent+= "\"\",";

            // Write heading
            ts << curIndent << "\"" << cur->getHeadingPlain()<<"\""<<endl;
        }

        model->nextBranch(cur,prev);
        curIndent="";
    }
    file.close();
    completeExport();
}
