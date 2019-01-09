#ifndef EXPORT_AO_H
#define EXPORT_AO_H

#include "export-base.h"

class ExportAO:public ExportBase
{
public:
    ExportAO();
    virtual void doExport();
    virtual QString underline (const QString &text, const QString &line);
};

#endif
