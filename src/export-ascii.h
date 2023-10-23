#ifndef EXPORT_ASCII_H
#define EXPORT_ASCII_H

#include "export-base.h"

class ExportASCII : public ExportBase {
  public:
    ExportASCII();
    virtual void doExport();
    QString underline(const QString &text, const QString &line);
    QString addEmptyLine(QString &text);
    QString startNewLine(QString &text);
};

#endif
