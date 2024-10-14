#include "export-taskjuggler.h"

#include "vymmodel.h"
#include "xsltproc.h"

extern QDir vymBaseDir;

void ExportTaskjuggler::doExport()  // FIXME-1 Needs to export to XML first, check paths
{
    model->exportXML(filePath + ".xml", tmpDir.path(), false);
    qDebug() << "exported xml: " << filePath + ".xml" << tmpDir.path();

    XSLTProc p;
    p.setInputFile(tmpDir.path() + "/" + model->getMapName() + ".xml");
    p.setOutputFile(filePath);
    p.setXSLFile(vymBaseDir.path() + "/styles/vym2taskjuggler.xsl");
    p.process();

    result = ExportBase::Success;

    displayedDestination = filePath;
    completeExport();
}
