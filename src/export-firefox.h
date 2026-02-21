#ifndef EXPORT_FIREFOX_H
#define EXPORT_FIREFOX_H

#include "export-base.h"

class BranchItem;

class ExportFirefox : public ExportBase {
  public:
    ExportFirefox();
    QJsonObject buildList(BranchItem *bi);
    void doExport();
};

#endif
