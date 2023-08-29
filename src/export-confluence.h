#ifndef EXPORT_CONFLUENCE_H
#define EXPORT_CONFLUENCE_H

#include "export-base.h"
#include "export-confluence-dialog.h"

#include "confluence-agent.h"

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

    bool createNewPage;
    QString url;
    QString pageName;

    QPointF offset;

    ExportConfluenceDialog dia;

    ConfluenceAgent *agent;
};

#endif
