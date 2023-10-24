#ifndef EXPORT_ASCII_H
#define EXPORT_ASCII_H

#include "export-base.h"

class ExportASCII : public ExportBase {
  public:
    ExportASCII();
    virtual void doExport();
    QString underline(const QString &text, const QString &line);
    QString ensureEmptyLines(QString &text, int n);
    QString ensureNewLine(QString &text);
};

#endif
