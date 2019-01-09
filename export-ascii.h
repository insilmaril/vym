#ifndef EXPORT_ASCII_H
#define EXPORT_ASCII_H

#include "export-base.h"

class ExportASCII:public ExportBase
{
public:
    ExportASCII();
    virtual void doExport();
    virtual QString underline (const QString &text, const QString &line);
};

#endif
