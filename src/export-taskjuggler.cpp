#include "export-taskjuggler.h"

#include "xsltproc.h"

extern QDir vymBaseDir;

void ExportTaskjuggler::doExport()
{
    model->exportXML("", tmpDir.path(), false);

    XSLTProc p;
    p.setInputFile(tmpDir.path() + "/" + model->getMapName() + ".xml");
    p.setOutputFile(filePath);
    p.setXSLFile(vymBaseDir.path() + "/styles/vym2taskjuggler.xsl");
    p.process();

    success = true;

    destination = filePath;
    completeExport();
}
