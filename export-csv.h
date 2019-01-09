#ifndef EXPORT_CSV_H
#define EXPORT_CSV_H

#include "export-base.h"

class ExportCSV:public ExportBase
{
public:
    ExportCSV();
    void doExport();
};

#endif
