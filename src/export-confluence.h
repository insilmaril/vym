#ifndef EXPORT_CONFLUENCE_H
#define EXPORT_CONFLUENCE_H

#include "export-base.h"
#include "export-confluence-dialog.h"

class ExportConfluence : public ExportBase {
  public:
    ExportConfluence();
    ExportConfluence(VymModel *m);
    virtual void init();
    void setCreateNewPage(bool);
    void setURL(const QString &u);
    void setPageName(const QString &t);
    virtual QString createTOC();
    virtual void doExport(bool useDialog = true);

  private:
    QString getBranchText(BranchItem *);
    QString buildList(BranchItem *);
    QString imageMap;
    QString cssSrc;
    QString cssDst;

    bool createNewPage;
    QString url;
    QString pageName;

    bool frameURLs;

    QPointF offset;

    ExportConfluenceDialog dia;
};

#endif
