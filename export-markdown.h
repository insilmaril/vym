#ifndef EXPORT_MARKDOWN_H
#define EXPORT_MARKDOWN_H

#include "export-base.h"

class ExportMarkdown : public ExportBase {
  public:
    ExportMarkdown();
    virtual void doExport();
    virtual QString underline(const QString &text, const QString &line);
};

#endif
