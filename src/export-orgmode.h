#ifndef EXPORT_ORGMODE_H
#define EXPORT_ORGMODE_H

#include "export-base.h"

class ExportOrgMode : public ExportBase {
  public:
    ExportOrgMode();
    virtual void doExport();
};

#endif
