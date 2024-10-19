#include "export-taskjuggler.h"

#include "file.h"
#include "vymmodel.h"
#include "xsltproc.h"

extern QDir vymBaseDir;

void ExportTaskJuggler::doExport()
{
    exportName = "TaskJuggler";

    bool ok;
    QString xmlPath = model->getMapName() + ".xml";
    QString dpath = makeTmpDir(ok, model->tmpDirPath(), "export");
    xmlPath = dpath + "/" + xmlPath;
    model->exportXML(xmlPath, false);

    XSLTProc p;
    if (!p.setInputFile(xmlPath))
        return;
    p.setOutputFile(filePath);
    if (!p.setXSLFile(vymBaseDir.path() + "/styles/vym2taskjuggler.xsl"))
        return;
    if (p.process())
        result = ExportBase::Success;
    else
        result = ExportBase::Failed;

    displayedDestination = filePath;
    completeExport();
}
