#include "export-latex.h"

#include <QMessageBox>
#include "mainwindow.h"

extern Main *mainWindow;
extern Settings settings;

ExportLaTeX::ExportLaTeX()
{
    exportName="LaTeX";
    filter="LaTeX files (*.tex);;All (* *.*)";

    // Note: key in hash on left side is the regular expression, which
    // will be replaced by string on right side
    // E.g. a literal $ will be replaced by \$
    esc["\\$"]="\\$";
    esc["\\^"]="\\^";
    esc["%"]="\\%";
    esc["&"]="\\&";
    esc["~"]="\\~";
    esc["_"]="\\_";
    esc["\\\\"]="\\";
    esc["\\{"]="\\{";
    esc["\\}"]="\\}";
}

QString ExportLaTeX::escapeLaTeX(const QString &s)
{
    QString r=s;

    QRegExp rx;
    rx.setMinimal(true);

    foreach (QString p,esc.keys() )
    {
        rx.setPattern (p);
        r.replace (rx, esc[p] );
    }
    return r;
}

void ExportLaTeX::doExport()
{
    // Exports a map to a LaTex file.
    // This file needs to be included
    // or inported into a LaTex document
    // it will not add a preamble, or anything
    // that makes a full LaTex document.
    QFile file (filePath);
    if ( !file.open( QIODevice::WriteOnly ) ) {
        QMessageBox::critical (
                    0,
                    QObject::tr("Critical Export Error"),
                    QObject::tr("Could not export as LaTeX to %1").arg(filePath));
        mainWindow->statusMessage(QString(QObject::tr("Export failed.")));
        return;
    }

    // Read default section names
    QStringList sectionNames;
    sectionNames << ""
                 << "chapter"
                 << "section"
                 << "subsection"
                 << "subsubsection"
                 << "paragraph";

    for (int i=0; i<6; i++)
        sectionNames.replace(i,settings.value(
                                 QString("/export/latex/sectionName-%1").arg(i),sectionNames.at(i)).toString() );

    QString out;

    // Main loop over all branches
    QString s;
    BranchItem *cur=NULL;
    BranchItem *prev=NULL;
    model->nextBranch(cur,prev);
    while (cur)
    {
        if (!cur->hasHiddenExportParent() )
        {
            int d=cur->depth();
            s=escapeLaTeX (cur->getHeadingPlain() );
            if ( sectionNames.at(d).isEmpty() || d>=sectionNames.count() )
                out += s + "\n";
            else
                out += "\n";
                out += "\\" + sectionNames.at(d) + "{" + s + "}";
                out += "\n";

            // If necessary, write note
            if (!cur->isNoteEmpty()) {
                out += (cur->getNoteASCII());
                out += "\n";
            }
        }
        model->nextBranch(cur,prev);
    }
    
    QTextStream ts( &file );
    ts.setCodec("UTF-8");
    ts << out;
    file.close();

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(out);
    
    completeExport();
}

