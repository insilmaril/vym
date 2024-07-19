#ifndef EXPORT_HTML_H
#define EXPORT_HTML_H

#include "export-base.h"
#include "export-html-dialog.h"

class BranchItem;

class ExportHTML : public ExportBase {
  public:
    ExportHTML();
    ExportHTML(VymModel *m);
    virtual void init();
    virtual QString createTOC();
    virtual void doExport(bool useDialog = true);

  private:
    QString getBranchText(BranchItem *);
    QString buildList(BranchItem *);
    QString imageMap;
    QString cssSrc;
    QString cssDst;

    bool frameURLs;

    QPointF offset;

    QSet<QUuid> activeFlags;

    ExportHTMLDialog dia;
};

#endif
