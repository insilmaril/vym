#ifndef EXPORT_LATEX_H
#define EXPORT_LATEX_H

#include "export-base.h"

class ExportLaTeX:public ExportBase
{
public:
    ExportLaTeX();
    QString escapeLaTeX (const QString &s);
    virtual void doExport();
private:
    QHash <QString,QString> esc;
};  

#endif
